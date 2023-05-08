#ifndef _USB_UHI_MULTIHID_H_
#define _USB_UHI_MULTIHID_H_

// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

#include "uhi.h"
#include "usb_protocol.h"

// UHI API. Register this in USB_HOST_UHI to start this subsystem.
#define UHI_MULTIHID { \
	.install = multihid_install, \
	.enable = multihid_enable, \
	.uninstall = multihid_uninstall, \
	.sof_notify = NULL, \
}
uhc_enum_status_t multihid_install(uhc_device_t* dev);
void multihid_enable(uhc_device_t* dev);
void multihid_uninstall(uhc_device_t* dev);


#endif