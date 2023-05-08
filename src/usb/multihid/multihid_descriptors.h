#ifndef _USB_MULTIHID_DESCRIPTORS_H_
#define _USB_MULTIHID_DESCRIPTORS_H_

#include "multihid.h"

// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

void multihid_descriptors_fetch_start(uhc_device_t *usb, multihid_device_t *devices);

#endif
