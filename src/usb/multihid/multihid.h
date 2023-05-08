#ifndef _USB_MULTIHID_H_
#define _USB_MULTIHID_H_

// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

// multihid implements a HID/USB API with support for multiple HID reports on
// a single device, ie. composite USB devices.

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "usb_protocol_hid.h"
#include "uhi.h"

typedef void (*multihid_callback_t)(void *context, uint8_t *report, size_t report_size);

typedef struct multihid_device {
  usb_ep_t ep_in;
  uint8_t interface_num;
  uint8_t interface_subclass;
  uint8_t interface_protocol;

  size_t report_size;
  uint8_t *report;

  size_t descriptor_size;
  uint8_t *descriptor;

  multihid_callback_t callback;
  void *callback_context;

  struct multihid_device *next;
} multihid_device_t;

multihid_device_t *multihid_find_keyboard(multihid_device_t *devices);
void multihid_start(multihid_device_t *device, multihid_callback_t callback, void *context);
void multihid_device_free(multihid_device_t *hdev);
void multihid_change(multihid_device_t* devices, bool plug);

#endif
