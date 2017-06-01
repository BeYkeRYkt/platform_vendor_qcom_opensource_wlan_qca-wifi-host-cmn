/*
 * Copyright (c) 2016-2017 The Linux Foundation. All rights reserved.
 *
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

/**
 * DOC: This file has the DFS dispatcher API which is exposed to outside of DFS
 * component.
 */

#ifndef _WLAN_DFS_TGT_API_H_
#define _WLAN_DFS_TGT_API_H_

#include "wlan_dfs_ucfg_api.h"
#include "wlan_dfs_utils_api.h"

extern struct dfs_to_mlme global_dfs_to_mlme;

/**
 * tgt_dfs_set_current_channel() - Fill dfs channel structure from
 *                                 dfs_ieee80211_channel structure.
 * @pdev: Pointer to DFS pdev object.
 * @ic_freq: Frequency in Mhz.
 * @ic_flags: Channel flags.
 * @ic_flagext: Extended channel flags.
 * @ic_ieee: IEEE channel number.
 * @ic_vhtop_ch_freq_seg1: Channel Center frequency1.
 * @ic_vhtop_ch_freq_seg2: Channel Center frequency2.
 */
QDF_STATUS tgt_dfs_set_current_channel(struct wlan_objmgr_pdev *pdev,
		uint16_t ic_freq,
		uint32_t ic_flags,
		uint16_t ic_flagext,
		uint8_t ic_ieee,
		uint8_t ic_vhtop_ch_freq_seg1,
		uint8_t ic_vhtop_ch_freq_seg2);

/**
 * tgt_dfs_reset() - DFS reset
 * @pdev: Pointer to DFS pdev object.
 *
 * Wrapper function for dfs_reset(). This function called from outside of DFS
 * component.
 */
QDF_STATUS tgt_dfs_reset(struct wlan_objmgr_pdev *pdev);

/**
 * tgt_dfs_get_radars() - Based on the chipset, calls init radar table functions
 * @pdev: Pointer to DFS pdev object.
 *
 * Wrapper function for dfs_get_radars(). This function called from
 * outside of DFS component.
 */
QDF_STATUS tgt_dfs_get_radars(struct wlan_objmgr_pdev *pdev);

/**
 * tgt_dfs_process_phyerr() - Process phyerr.
 * @pdev: Pointer to DFS pdev object.
 * @buf: Phyerr buffer.
 * @datalen: phyerr buffer length.
 * @r_rssi: RSSI.
 * @r_ext_rssi: Extension channel RSSI.
 * @r_rs_tstamp: Timestamp.
 * @r_fulltsf: TSF64.
 *
 * Wrapper function for dfs_process_phyerr(). This function called from
 * outside of DFS component.
 */
QDF_STATUS tgt_dfs_process_phyerr(struct wlan_objmgr_pdev *pdev,
	void *buf,
	uint16_t datalen,
	uint8_t r_rssi,
	uint8_t r_ext_rssi,
	uint32_t r_rs_tstamp,
	uint64_t r_fulltsf);

/**
 * tgt_dfs_destroy_object() - Destroys the DFS object.
 * @pdev: Pointer to DFS pdev object.
 *
 * Wrapper function for  dfs_destroy_object(). This function called from
 * outside of DFS component.
 */
QDF_STATUS tgt_dfs_destroy_object(struct wlan_objmgr_pdev *pdev);

/**
 * tgt_dfs_radar_enable() - Enables the radar.
 * @pdev: Pointer to DFS pdev object.
 * @no_cac: If no_cac is 0, it cancels the CAC.
 *
 * This is called each time a channel change occurs, to (potentially) enable
 * the radar code.
 */
QDF_STATUS tgt_dfs_radar_enable(struct wlan_objmgr_pdev *pdev,
	int no_cac, uint32_t opmode);

/**
 * tgt_dfs_attach() - Allocates memory for wlan_dfs members.
 * @pdev: Pointer to DFS pdev object.
 *
 * Wrapper function for dfs_attach(). This function called from
 * outside of DFS component.
 */
QDF_STATUS tgt_dfs_attach(struct wlan_objmgr_pdev *pdev);

/**
 * tgt_sif_dfs_detach() - DFS detach.
 * @pdev: Pointer to DFS pdev object.
 *
 * Wrapper function for sif_dfs_attach(). This function called from
 * outside of DFS component.
 */
QDF_STATUS tgt_sif_dfs_detach(struct wlan_objmgr_pdev *pdev);

/**
 * tgt_dfs_control()- Used to process ioctls related to DFS.
 * @pdev: Pointer to DFS pdev object.
 * @id: Command type.
 * @indata: Input buffer.
 * @insize: size of the input buffer.
 * @outdata: A buffer for the results.
 * @outsize: Size of the output buffer.
 */
QDF_STATUS tgt_dfs_control(struct wlan_objmgr_pdev *pdev,
	u_int id,
	void *indata,
	uint32_t insize,
	void *outdata,
	uint32_t *outsize,
	int *error);

/**
 * tgt_nif_dfs_reset() - DFS reset.
 * @pdev: Pointer to DFS pdev object.
 *
 * Wrapper function for nif_dfs_reset(). This function called from
 * outside of DFS component.
 */
QDF_STATUS tgt_nif_dfs_reset(struct wlan_objmgr_pdev *pdev);

/**
 * tgt_dfs_is_precac_timer_running() - Check whether precac timer is running.
 * @pdev: Pointer to DFS pdev object.
 * @is_precac_timer_running: Pointer to save precac timer value.
 *
 * Wrapper function for dfs_is_precac_timer_running(). This function called from
 * outside of DFS component.
 */
QDF_STATUS tgt_dfs_is_precac_timer_running(struct wlan_objmgr_pdev *pdev,
	bool *is_precac_timer_running);

/**
 * utils_dfs_find_vht80_chan_for_precac() - Find VHT80 channel for precac.
 * @pdev: Pointer to DFS pdev object.
 * @chan_mode: Channel mode.
 * @ch_freq_seg1: Segment1 channel freq.
 * @cfreq1: cfreq1.
 * @cfreq2: cfreq2.
 * @phy_mode: Precac phymode.
 * @dfs_set_cfreq2: Precac cfreq2
 * @set_agile: Agile mode flag.
 *
 * wrapper function for  dfs_find_vht80_chan_for_precacdfs_cancel_cac_timer().
 * This function called from outside of dfs component.
 */
QDF_STATUS tgt_dfs_find_vht80_chan_for_precac(struct wlan_objmgr_pdev *pdev,
		uint32_t chan_mode,
		uint8_t ch_freq_seg1,
		uint32_t *cfreq1,
		uint32_t *cfreq2,
		uint32_t *phy_mode,
		bool *dfs_set_cfreq2,
		bool *set_agile);

/**
 * tgt_dfs_process_radar_ind() - Process radar found indication.
 * @pdev: Pointer to DFS pdev object.
 * @radar_found: radar found info.
 *
 * Process radar found indication.
 *
 * Return QDF_STATUS.
 */
QDF_STATUS tgt_dfs_process_radar_ind(struct wlan_objmgr_pdev *pdev,
		struct radar_found_info *radar_found);

/**
 * tgt_dfs_cac_complete() - Process cac complete indication.
 * @pdev: Pointer to DFS pdev object.
 * @vdev_id: vdev id.
 *
 * Process cac complete indication from firmware.
 *
 * Return QDF_STATUS.
 */
QDF_STATUS tgt_dfs_cac_complete(struct wlan_objmgr_pdev *pdev,
		uint32_t vdev_id);

/**
 * tgt_dfs_reg_ev_handler() - Register dfs events.
 * @psoc: Pointer to psoc.
 * @dfs_offload: phy err processing offloaded to firmware.
 *
 * Register dfs events.
 *
 * Return: QDF_STATUS.
 */
QDF_STATUS tgt_dfs_reg_ev_handler(struct wlan_objmgr_psoc *psoc,
		bool dfs_offload);
#endif /* _WLAN_DFS_TGT_API_H_ */