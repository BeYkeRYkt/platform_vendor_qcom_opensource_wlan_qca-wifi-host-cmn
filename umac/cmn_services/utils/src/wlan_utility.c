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

/**
 * DOC: This file contains definition for mandatory legacy API
 */

#include "wlan_utility.h"
#include <wlan_cmn.h>

uint32_t wlan_chan_to_freq(uint8_t chan)
{
	/* ch 0 - ch 13 */
	if (chan < WLAN_24_GHZ_CHANNEL_14)
		return WLAN_24_GHZ_BASE_FREQ + chan * WLAN_CHAN_SPACING_5MHZ;
	else if (chan == WLAN_24_GHZ_CHANNEL_14)
		return WLAN_CHAN_14_FREQ;
	else if (chan < WLAN_24_GHZ_CHANNEL_27)
		/* ch 15 - ch 26 */
		return WLAN_CHAN_15_FREQ +
		  (chan - WLAN_24_GHZ_CHANNEL_15) * WLAN_CHAN_SPACING_20MHZ;
	else if (chan == WLAN_5_GHZ_CHANNEL_170)
		return WLAN_CHAN_170_FREQ;
	else
		return WLAN_5_GHZ_BASE_FREQ + chan * WLAN_CHAN_SPACING_5MHZ;
}

bool wlan_is_dsrc_channel(uint16_t center_freq)
{
	if (center_freq >= 5852 &&
	    center_freq <= 5920)
		return true;

	return false;
}