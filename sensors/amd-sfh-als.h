/* SPDX-License-Identifier: GPL-2.0 */
/*
 * AMD Sensor Fusion Hub ambient light sensor interface
 *
 * Author:	Richard Neumann <mail@richard-neumann.de>
 */

#ifndef AMD_SFH_ALS_H
#define AMD_SFH_ALS_H

#include <linux/hid.h>
#include <linux/types.h>

int amd_sfh_get_als_feature_report(int reportnum, u8 *buf, size_t len);
int amd_sfh_get_als_input_report(int reportnum, u8 *buf, size_t len,
				 u32 *cpu_addr);
int amd_sfh_parse_als(struct hid_device *hid);

#endif
