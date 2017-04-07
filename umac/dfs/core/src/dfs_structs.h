/*
 * Copyright (c) 2011-2012, 2016-2017 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: This file has dfs capability, dfs pulse structures.
 */

#ifndef _DFS_STRUCTS_H_
#define _DFS_STRUCTS_H_

/**
 * This represents the general case of the radar PHY configuration,
 * across all chips.
 *
 * It's then up to each chip layer to translate to/from this
 * (eg to HAL_PHYERR_PARAM for the HAL case.)
 */

#define WLAN_DFS_PHYERR_PARAM_NOVAL   0xFFFF
#define WLAN_DFS_PHYERR_PARAM_ENABLE  0x8000

/**
 * For the dfs_nol_clist_update() method - this is the
 * update command.
 */
enum {
	DFS_NOL_CLIST_CMD_NONE      = 0x0,
	DFS_NOL_CLIST_CMD_UPDATE    = 0x1,
};

/**
 * struct wlan_dfs_caps - DFS capability structure.
 * @wlan_dfs_ext_chan_ok:          Can radar be detected on the extension chan?
 * @wlan_dfs_combined_rssi_ok:     Can use combined radar RSSI?
 * @wlan_dfs_use_enhancement:      This flag is used to indicate if radar
 *                                detection scheme should use enhanced chirping
 *                                detection algorithm. This flag also determines
 *                                if certain radar data should be discarded to
 *                                minimize false detection of radar.
 * @wlan_strong_signal_diversiry:  Strong Signal fast diversity count.
 * @wlan_chip_is_bb_tlv:           Chip is BB TLV?
 * @wlan_chip_is_over_sampled:     Is Over sampled.
 * @wlan_chip_is_ht160:            IS VHT160?
 * @wlan_chip_is_false_detect:     Is False detected?
 * @wlan_fastdiv_val:              Goes with wlan_strong_signal_diversiry: If we
 *                                have fast diversity capability, read off
 *                                Strong Signal fast diversity count set in the
 *                                ini file, and store so we can restore the
 *                                value when radar is disabled.
 */
struct wlan_dfs_caps {
	uint32_t wlan_dfs_ext_chan_ok:1,
			 wlan_dfs_combined_rssi_ok:1,
			 wlan_dfs_use_enhancement:1,
			 wlan_strong_signal_diversiry:1,
			 wlan_chip_is_bb_tlv:1,
			 wlan_chip_is_over_sampled:1,
			 wlan_chip_is_ht160:1,
			 wlan_chip_is_false_detect:1;
	uint32_t wlan_fastdiv_val;
};

/**
 * struct dfs_pulse - DFS pulses.
 * @rp_numpulses:         Num of pulses in radar burst.
 * @rp_pulsedur:          Duration of each pulse in usecs.
 * @rp_pulsefreq:         Frequency of pulses in burst.
 * @rp_max_pulsefreq:     Frequency of pulses in burst.
 * @rp_patterntype:       fixed or variable pattern type.
 * @rp_pulsevar:          Time variation of pulse duration for matched
 *                        filter (single-sided) in usecs.
 * @rp_threshold:         Threshold for MF output to indicate radar match.
 * @rp_mindur:            Min pulse duration to be considered for this pulse
 *                        type.
 * @rp_maxdur:            Min pulse duration to be considered for this pulse
 *                        type.
 * @rp_rssithresh:        Minimum rssi to be considered a radar pulse.
 * @rp_meanoffset:        Offset for timing adjustment.
 * @rp_rssimargin:        rssi threshold margin. In Turbo Mode HW reports
 *                        rssi 3dBm. lower than in non TURBO mode. This
 *                        will be used to offset that diff.
 * @rp_ignore_pri_window: Ignore PRI window.
 * @rp_pulseid:           Unique ID for identifying filter.
 */
struct dfs_pulse {
	uint32_t  rp_numpulses;
	uint32_t  rp_pulsedur;
	uint32_t  rp_pulsefreq;
	uint32_t  rp_max_pulsefreq;
	uint32_t  rp_patterntype;
	uint32_t  rp_pulsevar;
	uint32_t  rp_threshold;
	uint32_t  rp_mindur;
	uint32_t  rp_maxdur;
	uint32_t  rp_rssithresh;
	uint32_t  rp_meanoffset;
	int32_t   rp_rssimargin;
	uint32_t  rp_ignore_pri_window;
	uint32_t  rp_pulseid;
};

/**
 * struct dfs_bin5pulse - DFS bin5 pulse.
 * @b5_threshold:    Number of bin5 pulses to indicate detection.
 * @b5_mindur:       Min duration for a bin5 pulse.
 * @b5_maxdur:       Max duration for a bin5 pulse.
 * @b5_timewindow:   Window over which to count bin5 pulses.
 * @b5_rssithresh:   Min rssi to be considered a pulse.
 * @b5_rssimargin:   rssi threshold margin. In Turbo Mode HW reports rssi 3dB
 */
struct dfs_bin5pulse {
	uint32_t  b5_threshold;
	uint32_t  b5_mindur;
	uint32_t  b5_maxdur;
	uint32_t  b5_timewindow;
	uint32_t  b5_rssithresh;
	uint32_t  b5_rssimargin;
};

/**
 * struct dfs_nol_chan_entry - DFS NOL representation.
 * @nol_chfreq:      Centre frequency, MHz .
 * @nol_chwidth:     Width, MHz.
 * @nol_start_ticks: Start ticks, OS specific.
 * @nol_timeout_ms:  Timeout, ms
 */
struct dfs_nol_chan_entry {
	uint32_t      nol_chfreq;
	uint32_t      nol_chwidth;
	unsigned long nol_start_ticks;
	uint32_t      nol_timeout_ms;
};

/**
 * struct wlan_dfs_phyerr_param - DFS Phyerr structure.
 * @pe_firpwr:     FIR pwr out threshold.
 * @pe_rrssi:      Radar rssi thresh.
 * @pe_height:     Pulse height thresh.
 * @pe_prssi:      Pulse rssi thresh.
 * @pe_inband:     Inband thresh.
 * @pe_relpwr:     Relative power threshold in 0.5dB steps.
 * @pe_relstep:    Pulse Relative step threshold in 0.5dB steps.
 * @pe_maxlen:     Max length of radar sign in 0.8us units.
 * @pe_usefir128:  Use the average in-band power measured over 128 cycles.
 * @pe_blockradar: Enable to block radar check if pkt detect is done via OFDM
 *                 weak signal detect or pkt is detected immediately after tx
 *                 to rx transition.
 * @pe_enmaxrssi:  Enable to use the max rssi instead of the last rssi during
 *                 fine gain changes for radar detection.
 */
struct wlan_dfs_phyerr_param {
	int32_t    pe_firpwr;
	int32_t    pe_rrssi;
	int32_t    pe_height;
	int32_t    pe_prssi;
	int32_t    pe_inband;
	uint32_t   pe_relpwr;
	uint32_t   pe_relstep;
	uint32_t   pe_maxlen;
	bool       pe_usefir128;
	bool       pe_blockradar;
	bool       pe_enmaxrssi;
};

/**
 * wlan_dfs_phyerr_init_noval() - Fill wlan_dfs_phyerr_param with 0xFF.
 * @pe: Pointer to wlan_dfs_phyerr_param structure.
 */
static inline void wlan_dfs_phyerr_init_noval(struct wlan_dfs_phyerr_param *pe)
{
	pe->pe_firpwr = WLAN_DFS_PHYERR_PARAM_NOVAL;
	pe->pe_rrssi = WLAN_DFS_PHYERR_PARAM_NOVAL;
	pe->pe_height = WLAN_DFS_PHYERR_PARAM_NOVAL;
	pe->pe_prssi = WLAN_DFS_PHYERR_PARAM_NOVAL;
	pe->pe_inband = WLAN_DFS_PHYERR_PARAM_NOVAL;
	pe->pe_relpwr = WLAN_DFS_PHYERR_PARAM_NOVAL;
	pe->pe_relstep = WLAN_DFS_PHYERR_PARAM_NOVAL;
	pe->pe_maxlen = WLAN_DFS_PHYERR_PARAM_NOVAL;
}

/**
 * struct wlan_dfs_radar_tab_info - Radar table information.
 * @dfsdomain:         DFS domain.
 * @numradars:         Number of radars.
 * @dfs_radars:        Pointer to dfs_pulse structure.
 * @numb5radars:       NUM5 radars.
 * @b5pulses:          BIN5 radars.
 * @dfs_defaultparams: phyerr params.
 */
struct wlan_dfs_radar_tab_info {
	uint32_t          dfsdomain;
	int               numradars;
	struct dfs_pulse *dfs_radars;
	int               numb5radars;
	struct dfs_bin5pulse *b5pulses;
	struct wlan_dfs_phyerr_param dfs_defaultparams;
};

#endif  /* _DFS_STRUCTS_H_ */