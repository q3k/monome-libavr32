#ifndef _USB_MULTIHID_KEYBOARD_H_
#define _USB_MULTIHID_KEYBOARD_H_

// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

#include "multihid.h"

multihid_device_t *multihid_find_keyboard(multihid_device_t *hid_devices);

#endif