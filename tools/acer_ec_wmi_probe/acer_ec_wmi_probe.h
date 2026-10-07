/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef ACER_EC_WMI_PROBE_H
#define ACER_EC_WMI_PROBE_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ACER_EC_WMI_MAX_DATA 64

struct acer_ec_wmi_request {
    __u32 method_id;
    __u32 input_len;
    __u32 output_capacity;
    __u32 output_len;
    __s32 acpi_status;
    __u8 input[ACER_EC_WMI_MAX_DATA];
    __u8 output[ACER_EC_WMI_MAX_DATA];
};

#define ACER_EC_WMI_IOC_CALL \
    _IOWR(0xA5, 0, struct acer_ec_wmi_request)

#endif
