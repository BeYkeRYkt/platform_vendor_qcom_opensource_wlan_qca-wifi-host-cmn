/*
 * Copyright (c) 2016-2017 The Linux Foundation. All rights reserved.
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
#ifndef _DP_PEER_H_
#define _DP_PEER_H_

#include <qdf_types.h>
#include <qdf_lock.h>
#include "dp_types.h"

#define DP_INVALID_PEER_ID 0xffff

/**
 * dp_peer_find_by_id() - Returns peer object given the peer id
 *
 * @soc		: core DP soc context
 * @peer_id	: peer id from peer object can be retrieved
 *
 * Return: struct dp_peer*: Pointer to DP peer object
 */
static inline struct dp_peer *
dp_peer_find_by_id(struct dp_soc *soc,
		   uint16_t peer_id)
{
	struct dp_peer *peer;

	/* TODO: Hold lock */
	peer = (peer_id >= soc->max_peers) ? NULL :
				soc->peer_id_to_obj_map[peer_id];

	return peer;
}

void dp_rx_peer_map_handler(void *soc_handle, uint16_t peer_id,
	uint8_t vdev_id, uint8_t *peer_mac_addr);
void dp_rx_peer_unmap_handler(void *soc_handle, uint16_t peer_id);
void dp_rx_sec_ind_handler(void *soc_handle, uint16_t peer_id,
	enum htt_sec_type sec_type, int is_unicast,
	u_int32_t *michael_key, u_int32_t *rx_pn);

#endif /* _DP_PEER_H_ */