// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

#include "multihid_descriptors.h"
#include "print_funcs.h"

// #define DEBUG

static multihid_device_t *multihid_descriptors_top = NULL;
static multihid_device_t *multihid_descriptors_current = NULL;
static uhc_device_t *multihid_descriptors_usb = NULL;

static void multihid_descriptors_fetch_next(void);
static void multihid_descriptors_fetch(void);
static void multihid_descriptors_fetch_done(usb_add_t add, uhd_trans_status_t status, uint16_t payload_trans);
static bool multihid_descriptors_get(uint16_t interface, void *out, size_t len, const uhd_callback_setup_end_t callback);

void multihid_descriptors_fetch_start(uhc_device_t *usb, multihid_device_t *devices) {
    multihid_descriptors_top = devices;
    multihid_descriptors_current = devices;
    multihid_descriptors_usb = usb;
    multihid_descriptors_fetch();
}

static void multihid_descriptors_fetch_next(void) {
  multihid_descriptors_current = multihid_descriptors_current->next;
  multihid_descriptors_fetch();
}

static void multihid_descriptors_fetch(void) {
  multihid_device_t *hdev = multihid_descriptors_current;
  if (hdev == NULL) {
#ifdef DEBUG
    print_dbg("\r\n HID device: finished retrieving endpoints, notifying users");
#endif
    multihid_change(multihid_descriptors_top, true);
    return;
  }

#ifdef DEBUG
  print_dbg("\r\n HID device: getting HID descriptor for interface ");
  print_dbg_ulong(hdev->interface_num);
  print_dbg(", size ");
  print_dbg_ulong(hdev->descriptor_size);
#endif

  void *descriptor = malloc(hdev->descriptor_size);
  if (descriptor == NULL) {
#ifdef DEBUG
    print_dbg(": out of memory for HID descriptor, size ");
#endif
    print_dbg_ulong(hdev->descriptor_size);
    multihid_descriptors_fetch_next();
    return;
  }
  hdev->descriptor = descriptor;
  if (!multihid_descriptors_get(hdev->interface_num, descriptor, hdev->descriptor_size, multihid_descriptors_fetch_done)) {
#ifdef DEBUG
    print_dbg(": failed to get HID descriptor");
#endif
    multihid_descriptors_fetch_next();
    return;
  }
}

static void multihid_descriptors_fetch_done(usb_add_t add, uhd_trans_status_t status, uint16_t payload_trans) {
  multihid_device_t *hdev = multihid_descriptors_current;

  if (status != UHD_TRANS_NOERROR) {
#ifdef DEBUG
    print_dbg(": transfer failed: ");
    print_dbg_ulong(status);
#endif
    free(hdev->descriptor);
    hdev->descriptor = NULL;
    hdev->descriptor_size = 0;
  } else {
#ifdef DEBUG
    print_dbg(": success: ");
    for (int i = 0; i < hdev->descriptor_size; i++) {
      uint8_t byte = hdev->descriptor[i];
      print_dbg_char_hex(byte);
    }
#endif
  }
  multihid_descriptors_fetch_next();
}

static bool multihid_descriptors_get(uint16_t interface, void *out, size_t len, const uhd_callback_setup_end_t callback) {
  uhc_device_t *usb = multihid_descriptors_usb;

  usb_setup_req_t req;
  req.bmRequestType = (USB_REQ_DIR_IN) | (USB_REQ_TYPE_STANDARD) | (USB_REQ_RECIP_INTERFACE);
  req.bRequest = USB_REQ_GET_DESCRIPTOR;
  req.wValue = USB_DT_HID_REPORT << 8;
  req.wIndex = interface;
  req.wLength = len;
#ifdef DEBUG
  print_dbg(", req: ");
  uint8_t *dump = (uint8_t *)&req;
  for (int i = 0; i < 8; i++) {
    print_dbg_char_hex(dump[i]);
  }
#endif
  if (!uhd_setup_request(usb->address, &req, out, len, NULL, callback)) {
    return false;
  }
  return true;
}

#undef DEBUG