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
 * DOC: These APIs are used by DFS core functions to call mlme functions.
 */

#ifndef _WLAN_DFS_MLME_API_H_
#define _WLAN_DFS_MLME_API_H_

#include "wlan_dfs_ucfg_api.h"

extern struct dfs_to_mlme global_dfs_to_mlme;

/**
 * dfs_mlme_channel_mark_radar() - mark the channel as radar.
 * @pdev: Pointer to DFS pdev object.
 * @freq: Channel frequency
 * @vhtop_ch_freq_seg2: VHT80 Cfreq2
 * @flags: channel flags.
 */
void dfs_mlme_channel_mark_radar(struct wlan_objmgr_pdev *pdev,
		uint16_t freq,
		uint8_t vhtop_ch_freq_seg2,
		uint32_t flags);

/**
 * dfs_mlme_start_rcsa() - Send RCSA to RootAP.
 * @pdev: Pointer to DFS pdev object.
 */
void dfs_mlme_start_rcsa(struct wlan_objmgr_pdev *pdev);

/**
 * dfs_mlme_mark_dfs() - Mark the channel in the channel list.
 * @pdev: Pointer to DFS pdev object.
 * @ieee: Channel number.
 * @freq: Channel frequency.
 * @vhtop_ch_freq_seg2: VHT80 Cfreq2.
 * @flags: channel flags.
 */
void dfs_mlme_mark_dfs(struct wlan_objmgr_pdev *pdev,
			uint8_t ieee,
			uint16_t freq,
			uint8_t vhtop_ch_freq_seg2,
			uint32_t flags);

/**
 * dfs_mlme_start_csa() - Sends CSA in ieeeChan
 * @pdev: Pointer to DFS pdev object.
 * @ieeeChan: Channel number.
 */
void dfs_mlme_start_csa(struct wlan_objmgr_pdev *pdev, uint8_t ieeeChan);

/**
 * dfs_mlme_proc_cac() - Process the CAC completion event.
 * @pdev: Pointer to DFS pdev object.
 * @vdev_id: vdev id.
 */
void dfs_mlme_proc_cac(struct wlan_objmgr_pdev *pdev, uint32_t vdev_id);

/**
 * dfs_mlme_deliver_event_up_afrer_cac() - Send a CAC timeout, VAP up event to
 *                                         userspace.
 * @pdev: Pointer to DFS pdev object.
 */
void dfs_mlme_deliver_event_up_afrer_cac(struct wlan_objmgr_pdev *pdev);

/**
 * dfs_mlme_get_ic_nchans() - Get number of channels in the channel list
 * @pdev: Pointer to DFS pdev object.
 * @nchans: Pointer to save the channel number.
 */
void dfs_mlme_get_ic_nchans(struct wlan_objmgr_pdev *pdev, int *nchans);

/**
 * dfs_mlme_get_ic_no_weather_radar_chan() - Checks is the channel is weather
 *                                           radar channel.
 * @pdev: Pointer to DFS pdev object.
 * @no_wradar: Pointer to save weather radar filter value.
 */
void dfs_mlme_get_ic_no_weather_radar_chan(struct wlan_objmgr_pdev *pdev,
	uint8_t *no_wradar);

/**
 * dfs_mlme_find_alternate_mode_channel() - Finds the channel.
 * @pdev: Pointer to DFS pdev object.
 * @alt_chan_mode: Input mode.
 * @chan_count: channel count.
 */
int dfs_mlme_find_alternate_mode_channel(struct wlan_objmgr_pdev *pdev,
	uint32_t alt_chan_mode,
	int chan_count);

/**
 * dfs_mlme_find_any_valid_channel() - Finds the channel with the given mode.
 * @pdev: Pointer to DFS pdev object.
 * @chan_mode: Channel mode.
 * @ret_val: Pointer to save the channel index.
 */
void dfs_mlme_find_any_valid_channel(struct wlan_objmgr_pdev *pdev,
	uint32_t chan_mode,
	int *ret_val);

/**
 * dfs_mlme_get_extchan() - Get extension channel.
 * @pdev: Pointer to DFS pdev object.
 * @ic_freq:                Frequency in Mhz.
 * @ic_flags:               Channel flags.
 * @ic_flagext:             Extended channel flags.
 * @ic_ieee:                IEEE channel number.
 * @ic_vhtop_ch_freq_seg1:  Channel Center frequency.
 * @ic_vhtop_ch_freq_seg2:  Channel Center frequency applicable for 80+80MHz
 *                          mode of operation.
 */
QDF_STATUS dfs_mlme_get_extchan(struct wlan_objmgr_pdev *pdev,
		uint16_t *ic_freq,
		uint32_t *ic_flags,
		uint16_t *ic_flagext,
		uint8_t *ic_ieee,
		uint8_t *ic_vhtop_ch_freq_seg1,
		uint8_t *ic_vhtop_ch_freq_seg2);

/**
 * dfs_mlme_set_no_chans_available() - Set no_chans_available flag.
 * @pdev: Pointer to DFS pdev object.
 * @val: Set this value to no_chans_available flag.
 */
void dfs_mlme_set_no_chans_available(struct wlan_objmgr_pdev *pdev,
		int val);

/**
 * dfs_mlme_ieee2mhz() - Get the frequency from channel number.
 * @pdev: Pointer to DFS pdev object.
 * @ieee: Channel number.
 * @flag: Channel flag.
 */
int dfs_mlme_ieee2mhz(struct wlan_objmgr_pdev *pdev,
		int ieee,
		int flag);

/**
 * dfs_mlme_find_dot11_channel() - Get dot11 channel from ieee, cfreq2 and mode.
 * @pdev: Pointer to DFS pdev object.
 * @ieee: Channel number.
 * @des_cfreq2: cfreq2
 * @mode: Phymode
 * @ic_freq:                Frequency in Mhz.
 * @ic_flags:               Channel flags.
 * @ic_flagext:             Extended channel flags.
 * @ic_ieee:                IEEE channel number.
 * @ic_vhtop_ch_freq_seg1:  Channel Center frequency.
 * @ic_vhtop_ch_freq_seg2:  Channel Center frequency applicable for 80+80MHz
 *                          mode of operation.
 */
void dfs_mlme_find_dot11_channel(struct wlan_objmgr_pdev *pdev,
		uint8_t ieee,
		uint8_t des_cfreq2,
		int mode,
		uint16_t *ic_freq,
		uint32_t *ic_flags,
		uint16_t *ic_flagext,
		uint8_t *ic_ieee,
		uint8_t *ic_vhtop_ch_freq_seg1,
		uint8_t *ic_vhtop_ch_freq_seg2);

/**
 * dfs_mlme_get_ic_channels() - Get channel from channel list.
 * @pdev: Pointer to DFS pdev object.
 * @ic_freq:                Frequency in Mhz.
 * @ic_flags:               Channel flags.
 * @ic_flagext:             Extended channel flags.
 * @ic_ieee:                IEEE channel number.
 * @ic_vhtop_ch_freq_seg1:  Channel Center frequency.
 * @ic_vhtop_ch_freq_seg2:  Channel Center frequency applicable for 80+80MHz
 *                          mode of operation.
 * @index: Index into channel list.
 */
void dfs_mlme_get_ic_channels(struct wlan_objmgr_pdev *pdev,
		uint16_t *ic_freq,
		uint32_t *ic_flags,
		uint16_t *ic_flagext,
		uint8_t *ic_ieee,
		uint8_t *ic_vhtop_ch_freq_seg1,
		uint8_t *ic_vhtop_ch_freq_seg2,
		int index);

/**
 * dfs_mlme_ic_flags_ext() - Get extension channel flags.
 * @pdev: Pointer to DFS pdev object.
 */
uint32_t dfs_mlme_ic_flags_ext(struct wlan_objmgr_pdev *pdev);

/**
 * dfs_mlme_channel_change_by_precac() - Channel change by PreCAC.
 * @pdev: Pointer to DFS pdev object.
 */
void dfs_mlme_channel_change_by_precac(struct wlan_objmgr_pdev *pdev);

/**
 * dfs_mlme_nol_timeout_notification() - NOL timeout notification to userspace.
 * @pdev: Pointer to DFS pdev object.
 */
void dfs_mlme_nol_timeout_notification(struct wlan_objmgr_pdev *pdev);

/**
 * dfs_mlme_clist_update() - Mark the channel as RADAR.
 * @pdev: Pointer to DFS pdev object.
 * @nollist: Pointer to NOL list.
 * @nentries: Number of channels in the NOL list.
 */
void dfs_mlme_clist_update(struct wlan_objmgr_pdev *pdev,
		void *nollist,
		int nentries);

/**
 * dfs_mlme_get_cac_timeout() - Get cac_timeout.
 * @pdev: Pointer to DFS pdev object.
 * @ic_freq:                Frequency in Mhz.
 * @ic_vhtop_ch_freq_seg2:  Channel Center frequency applicable for 80+80MHz
 *                          mode of operation.
 * @ic_flags:               Channel flags.
 */
int dfs_mlme_get_cac_timeout(struct wlan_objmgr_pdev *pdev,
		uint16_t ic_freq,
		uint8_t ic_vhtop_ch_freq_seg2,
		uint32_t ic_flags);

#endif /* _WLAN_DFS_MLME_API_H_ */