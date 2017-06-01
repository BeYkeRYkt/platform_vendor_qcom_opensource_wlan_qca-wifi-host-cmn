/*
 * Copyright (c) 2017 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include "dp_types.h"
#include "dp_rx.h"
#include "dp_peer.h"
#include "hal_rx.h"
#include "hal_api.h"
#include "qdf_trace.h"
#include "qdf_nbuf.h"
#include "hal_api_mon.h"
#include "ieee80211.h"
#include "dp_rx_mon.h"


/**
* dp_rx_mon_status_process_tlv() - Process status TLV in status
*	buffer on Rx status Queue posted by status SRNG processing.
* @soc: core txrx main context
* @mac_id: mac_id which is one of 3 mac_ids _ring
*
* Return: none
*/
static inline void
dp_rx_mon_status_process_tlv(struct dp_soc *soc, uint32_t mac_id) {
	struct dp_pdev *pdev = soc->pdev_list[mac_id];
	struct hal_rx_ppdu_info *ppdu_info;
	qdf_nbuf_t status_nbuf;
	uint8_t *rx_tlv;
	uint8_t *rx_tlv_start;
	uint32_t tlv_status;

#ifdef DP_INTR_POLL_BASED
	if (!pdev)
		return;
#endif

	ppdu_info = &pdev->ppdu_info;

	if (pdev->mon_ppdu_status != DP_PPDU_STATUS_START)
		return;

	while (!qdf_nbuf_is_queue_empty(&pdev->rx_status_q)) {

		status_nbuf = qdf_nbuf_queue_remove(&pdev->rx_status_q);
		rx_tlv = qdf_nbuf_data(status_nbuf);
		rx_tlv_start = rx_tlv;

		do {
			tlv_status = hal_rx_status_get_tlv_info(rx_tlv,
				ppdu_info);
			rx_tlv = hal_rx_status_get_next_tlv(rx_tlv);

			if ((rx_tlv - rx_tlv_start) >= RX_BUFFER_SIZE)
				break;

		} while (tlv_status == HAL_TLV_STATUS_PPDU_NOT_DONE);

		qdf_nbuf_free(status_nbuf);

		if (tlv_status == HAL_TLV_STATUS_PPDU_DONE) {

			pdev->mon_ppdu_status = DP_PPDU_STATUS_DONE;
			/* Temperary */
			pdev->mon_ppdu_status =
				DP_PPDU_STATUS_START;
			break;
		}
	}
	return;
}

/*
 * dp_rx_mon_status_srng_process() - Process monitor status ring
 *	post the status ring buffer to Rx status Queue for later
 *	processing when status ring is filled with status TLV.
 *	Allocate a new buffer to status ring if the filled buffer
 *	is posted.
 *
 * @soc: core txrx main context
 * @mac_id: mac_id which is one of 3 mac_ids
 * @quota: No. of ring entry that can be serviced in one shot.

 * Return: uint32_t: No. of ring entry that is processed.
 */
static inline uint32_t
dp_rx_mon_status_srng_process(struct dp_soc *soc, uint32_t mac_id,
	uint32_t quota)
{
	struct dp_pdev *pdev = soc->pdev_list[mac_id];
	void *hal_soc;
	void *mon_status_srng;
	void *rxdma_mon_status_ring_entry;
	QDF_STATUS status;
	uint32_t work_done = 0;

#ifdef DP_INTR_POLL_BASED
	if (!pdev)
		return work_done;
#endif

	mon_status_srng = pdev->rxdma_mon_status_ring.hal_srng;

	qdf_assert(mon_status_srng);

	hal_soc = soc->hal_soc;

	qdf_assert(hal_soc);

	if (qdf_unlikely(hal_srng_access_start(hal_soc, mon_status_srng)))
		goto done;

	/* mon_status_ring_desc => WBM_BUFFER_RING STRUCT =>
	 * BUFFER_ADDR_INFO STRUCT
	 */
	while (qdf_likely((rxdma_mon_status_ring_entry =
		hal_srng_src_peek(hal_soc, mon_status_srng))
			&& quota--)) {
		uint32_t rx_buf_cookie;
		qdf_nbuf_t status_nbuf;
		struct dp_rx_desc *rx_desc;
		uint8_t *status_buf;
		qdf_dma_addr_t paddr;
		uint64_t buf_addr;

		buf_addr =
			(HAL_RX_BUFFER_ADDR_31_0_GET(
				rxdma_mon_status_ring_entry) |
			((uint64_t)(HAL_RX_BUFFER_ADDR_39_32_GET(
				rxdma_mon_status_ring_entry)) << 32));

		if (qdf_likely(buf_addr)) {

			rx_buf_cookie =
				HAL_RX_BUF_COOKIE_GET(
					rxdma_mon_status_ring_entry);
			rx_desc = dp_rx_cookie_2_va_mon_status(soc,
				rx_buf_cookie);

			qdf_assert(rx_desc);

			status_nbuf = rx_desc->nbuf;

			qdf_nbuf_sync_for_cpu(soc->osdev, status_nbuf,
				QDF_DMA_FROM_DEVICE);

			status_buf = qdf_nbuf_data(status_nbuf);

			status = hal_get_rx_status_done(status_buf);

			if (status != QDF_STATUS_SUCCESS) {
				QDF_TRACE(QDF_MODULE_ID_DP,
				QDF_TRACE_LEVEL_WARN,
				"[%s][%d] status not done",
				__func__, __LINE__);
				break;
			}
			qdf_nbuf_set_pktlen(status_nbuf, RX_BUFFER_SIZE);

			qdf_nbuf_unmap_single(soc->osdev, status_nbuf,
				QDF_DMA_FROM_DEVICE);

			/* Put the status_nbuf to queue */
			qdf_nbuf_queue_add(&pdev->rx_status_q, status_nbuf);

		} else {
			union dp_rx_desc_list_elem_t *desc_list = NULL;
			union dp_rx_desc_list_elem_t *tail = NULL;
			struct rx_desc_pool *rx_desc_pool;
			uint32_t num_alloc_desc;

			rx_desc_pool = &soc->rx_desc_status[mac_id];

			num_alloc_desc = dp_rx_get_free_desc_list(soc, mac_id,
							rx_desc_pool,
							1,
							&desc_list,
							&tail);

			rx_desc = &desc_list->rx_desc;
		}

		/* Allocate a new skb */
		status_nbuf = qdf_nbuf_alloc(pdev->osif_pdev, RX_BUFFER_SIZE,
			RX_BUFFER_RESERVATION, RX_BUFFER_ALIGNMENT, FALSE);

		status_buf = qdf_nbuf_data(status_nbuf);

		hal_clear_rx_status_done(status_buf);

		qdf_nbuf_map_single(soc->osdev, status_nbuf,
			QDF_DMA_BIDIRECTIONAL);
		paddr = qdf_nbuf_get_frag_paddr(status_nbuf, 0);

		rx_desc->nbuf = status_nbuf;

		hal_rxdma_buff_addr_info_set(rxdma_mon_status_ring_entry,
			paddr, rx_desc->cookie, HAL_RX_BUF_RBM_SW3_BM);

		rxdma_mon_status_ring_entry =
			hal_srng_src_get_next(hal_soc, mon_status_srng);
		work_done++;
	}
done:

	hal_srng_access_end(hal_soc, mon_status_srng);

	return work_done;

}
/*
 * dp_rx_mon_status_process() - Process monitor status ring and
 *	TLV in status ring.
 *
 * @soc: core txrx main context
 * @mac_id: mac_id which is one of 3 mac_ids
 * @quota: No. of ring entry that can be serviced in one shot.

 * Return: uint32_t: No. of ring entry that is processed.
 */
static inline uint32_t
dp_rx_mon_status_process(struct dp_soc *soc, uint32_t mac_id, uint32_t quota) {
	uint32_t work_done;

	work_done = dp_rx_mon_status_srng_process(soc, mac_id, quota);

	dp_rx_mon_status_process_tlv(soc, mac_id);

	return work_done;
}
/**
 * dp_mon_process() - Main monitor mode processing roution.
 *	This call monitor status ring process then monitor
 *	destination ring process.
 *	Called from the bottom half (tasklet/NET_RX_SOFTIRQ)
 * @soc: core txrx main context
 * @mac_id: mac_id which is one of 3 mac_ids
 * @quota: No. of status ring entry that can be serviced in one shot.

 * Return: uint32_t: No. of ring entry that is processed.
 */
uint32_t
dp_mon_process(struct dp_soc *soc, uint32_t mac_id, uint32_t quota) {
	uint32_t work_done;

	work_done = dp_rx_mon_status_process(soc, mac_id, quota);

	dp_rx_mon_dest_process(soc, mac_id, quota);

	return work_done;
}
/**
 * dp_rx_pdev_mon_detach() - detach dp rx for status ring
 * @pdev: core txrx pdev context
 *
 * This function will detach DP RX status ring from
 * main device context. will free DP Rx resources for
 * status ring
 *
 * Return: QDF_STATUS_SUCCESS: success
 *         QDF_STATUS_E_RESOURCES: Error return
 */
QDF_STATUS
dp_rx_pdev_mon_status_detach(struct dp_pdev *pdev)
{
	uint8_t pdev_id = pdev->pdev_id;
	struct dp_soc *soc = pdev->soc;
	struct rx_desc_pool *rx_desc_pool;

	rx_desc_pool = &soc->rx_desc_status[pdev_id];
	dp_rx_desc_pool_free(soc, pdev_id, rx_desc_pool);

	return QDF_STATUS_SUCCESS;
}

/*
 * dp_rx_buffers_replenish() -  replenish monitor status ring with
 *				rx nbufs called during dp rx
 *				monitor status ring initialization
 *
 * @soc: core txrx main context
 * @mac_id: mac_id which is one of 3 mac_ids
 * @dp_rxdma_srng: dp monitor status circular ring
 * @rx_desc_pool; Pointer to Rx descriptor pool
 * @num_req_buffers: number of buffer to be replenished
 * @desc_list:	list of descs if called from dp rx monitor status
 *		process or NULL during dp rx initialization or
 *		out of buffer interrupt
 * @tail: tail of descs list
 * @owner: who owns the nbuf (host, NSS etc...)
 * Return: return success or failure
 */
static inline
QDF_STATUS dp_rx_mon_status_buffers_replenish(struct dp_soc *dp_soc,
	uint32_t mac_id,
	struct dp_srng *dp_rxdma_srng,
	struct rx_desc_pool *rx_desc_pool,
	uint32_t num_req_buffers,
	union dp_rx_desc_list_elem_t **desc_list,
	union dp_rx_desc_list_elem_t **tail,
	uint8_t owner)
{
	uint32_t num_alloc_desc;
	uint16_t num_desc_to_free = 0;
	uint32_t num_entries_avail;
	uint32_t count;
	int sync_hw_ptr = 1;
	qdf_dma_addr_t paddr;
	qdf_nbuf_t rx_netbuf;
	void *rxdma_ring_entry;
	union dp_rx_desc_list_elem_t *next;
	void *rxdma_srng;
	uint8_t *status_buf;

	rxdma_srng = dp_rxdma_srng->hal_srng;

	qdf_assert(rxdma_srng);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
		"[%s][%d] requested %d buffers for replenish\n",
		__func__, __LINE__, num_req_buffers);

	/*
	 * if desc_list is NULL, allocate the descs from freelist
	 */
	if (!(*desc_list)) {

		num_alloc_desc = dp_rx_get_free_desc_list(dp_soc, mac_id,
							  rx_desc_pool,
							  num_req_buffers,
							  desc_list,
							  tail);

		if (!num_alloc_desc) {
			QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
			"[%s][%d] no free rx_descs in freelist\n",
			__func__, __LINE__);
			return QDF_STATUS_E_NOMEM;
		}

		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
			"[%s][%d] %d rx desc allocated\n", __func__, __LINE__,
			num_alloc_desc);
		num_req_buffers = num_alloc_desc;
	}

	hal_srng_access_start(dp_soc->hal_soc, rxdma_srng);
	num_entries_avail = hal_srng_src_num_avail(dp_soc->hal_soc,
				rxdma_srng, sync_hw_ptr);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
		"[%s][%d] no of availble entries in rxdma ring: %d\n",
		  __func__, __LINE__, num_entries_avail);

	if (num_entries_avail < num_req_buffers) {
		num_desc_to_free = num_req_buffers - num_entries_avail;
		num_req_buffers = num_entries_avail;
	}

	for (count = 0; count < num_req_buffers; count++) {
		rxdma_ring_entry = hal_srng_src_get_next(dp_soc->hal_soc,
							 rxdma_srng);

		rx_netbuf = qdf_nbuf_alloc(dp_soc->osdev,
					RX_BUFFER_SIZE,
					RX_BUFFER_RESERVATION,
					RX_BUFFER_ALIGNMENT,
					FALSE);

		status_buf = qdf_nbuf_data(rx_netbuf);
		hal_clear_rx_status_done(status_buf);

		memset(status_buf, 0, RX_BUFFER_SIZE);

		qdf_nbuf_map_single(dp_soc->osdev, rx_netbuf,
				    QDF_DMA_BIDIRECTIONAL);

		paddr = qdf_nbuf_get_frag_paddr(rx_netbuf, 0);

		next = (*desc_list)->next;

		(*desc_list)->rx_desc.nbuf = rx_netbuf;
		hal_rxdma_buff_addr_info_set(rxdma_ring_entry, paddr,
			(*desc_list)->rx_desc.cookie, owner);

		QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
			"[%s][%d] rx_desc=%p, cookie=%d, nbuf=%p, \
			status_buf=%p paddr=%p\n",
			__func__, __LINE__, &(*desc_list)->rx_desc,
			(*desc_list)->rx_desc.cookie, rx_netbuf,
			status_buf, (void *)paddr);

		*desc_list = next;
	}

	hal_srng_access_end(dp_soc->hal_soc, rxdma_srng);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
		"successfully replenished %d buffers\n", num_req_buffers);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
		"%d rx desc added back to free list\n", num_desc_to_free);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_INFO,
		"[%s][%d] desc_list=%p, tail=%p rx_desc=%p, cookie=%d\n",
		__func__, __LINE__, desc_list, tail, &(*desc_list)->rx_desc,
		(*desc_list)->rx_desc.cookie);

	/*
	 * add any available free desc back to the free list
	 */
	if (*desc_list) {
		dp_rx_add_desc_list_to_free_list(dp_soc, desc_list, tail,
			mac_id, rx_desc_pool);
	}

	return QDF_STATUS_SUCCESS;
}
/**
 * dp_rx_pdev_mon_status_attach() - attach DP RX monitor status ring
 * @pdev: core txrx pdev context
 *
 * This function will attach a DP RX monitor status ring into pDEV
 * and replenish monitor status ring with buffer.
 *
 * Return: QDF_STATUS_SUCCESS: success
 *         QDF_STATUS_E_RESOURCES: Error return
 */
QDF_STATUS
dp_rx_pdev_mon_status_attach(struct dp_pdev *pdev) {
	uint8_t pdev_id = pdev->pdev_id;
	struct dp_soc *soc = pdev->soc;
	union dp_rx_desc_list_elem_t *desc_list = NULL;
	union dp_rx_desc_list_elem_t *tail = NULL;
	struct dp_srng *rxdma_srng;
	uint32_t rxdma_entries;
	struct rx_desc_pool *rx_desc_pool;

	rxdma_srng = &pdev->rxdma_mon_status_ring;

	rxdma_entries = rxdma_srng->alloc_size/hal_srng_get_entrysize(
		soc->hal_soc, RXDMA_MONITOR_STATUS);

	rx_desc_pool = &soc->rx_desc_status[pdev_id];

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_WARN,
			"%s: Mon RX Status Pool[%d] allocation size=%d\n",
			__func__, pdev_id, rxdma_entries);

	dp_rx_desc_pool_alloc(soc, pdev_id, rxdma_entries+1, rx_desc_pool);

	QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_WARN,
			"%s: Mon RX Status Buffers Replenish pdev_id=%d\n",
			__func__, pdev_id);

	dp_rx_mon_status_buffers_replenish(soc, pdev_id, rxdma_srng,
		rx_desc_pool, rxdma_entries, &desc_list, &tail,
		HAL_RX_BUF_RBM_SW3_BM);

	qdf_nbuf_queue_init(&pdev->rx_status_q);

	pdev->mon_ppdu_status = DP_PPDU_STATUS_START;

	return QDF_STATUS_SUCCESS;
}