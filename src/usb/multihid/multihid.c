// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

#include "multihid.h"
#include "multihid_descriptors.h"

#include "print_funcs.h"
#include "events.h"

// #define DEBUG

typedef struct {
  uhc_device_t *dev;
  multihid_device_t *hid_devices;
} multihid_usb_device_t;

static multihid_usb_device_t multihid_usb_dev = { 0 };

static bool multihid_device_validate(multihid_device_t *hdev);

static void multihid_usb_device_add_hid(multihid_usb_device_t *udev, multihid_device_t *hdev);
static void multihid_on_report(usb_add_t add, usb_ep_t ep, uhd_trans_status_t status, iram_size_t nb_transfered);

uhc_enum_status_t multihid_install(uhc_device_t* dev) {
  uint16_t conf_desc_lgt;
  usb_iface_desc_t *ptr_iface;

  conf_desc_lgt = le16_to_cpu(dev->conf_desc->wTotalLength);
  ptr_iface = (usb_iface_desc_t*)dev->conf_desc;

  multihid_usb_device_t *udev = &multihid_usb_dev;
  if (udev->dev != NULL) {
    return UHC_ENUM_SOFTWARE_LIMIT;
  }
  udev->dev = dev;

  multihid_device_t *hdev = NULL;

  while(conf_desc_lgt) {
    switch (ptr_iface->bDescriptorType) {
    case USB_DT_INTERFACE:
#ifdef DEBUG
      print_dbg("\r\n iface_desc -> bInterfaceNumber : ");
      print_dbg_hex(ptr_iface->bInterfaceNumber);
      print_dbg("\r\n iface_desc -> bInterfaceClass : ");
      print_dbg_hex(ptr_iface->bInterfaceClass);
      print_dbg("\r\n iface_desc -> bInterfaceSubClass : ");
      print_dbg_hex(ptr_iface->bInterfaceSubClass);
      print_dbg("\r\n iface_desc -> bInterfaceProtocol : ");
      print_dbg_hex(ptr_iface->bInterfaceProtocol);
#endif
      if ((!ptr_iface->bInterfaceClass == HID_CLASS)) {
        break;
      }
      // Allocate new multihid device, add to linked list.
      if (hdev != NULL) {
        multihid_usb_device_add_hid(udev, hdev);
      }

      hdev = calloc(sizeof(multihid_device_t), 1);
      if (hdev == NULL) {
          return UHC_ENUM_MEMORY_LIMIT;
      }
      hdev->interface_num = ptr_iface->bInterfaceNumber;
      hdev->interface_subclass = ptr_iface->bInterfaceSubClass;
      hdev->interface_protocol = ptr_iface->bInterfaceProtocol;
      break;

    case USB_DT_HID:
      if (hdev == NULL) {
        break;
      }
      usb_hid_descriptor_t *hid_desc = (usb_hid_descriptor_t *)ptr_iface;

#ifdef DEBUG
      print_dbg("\r\n hid_desc -> wDescriptorLength : ");
      print_dbg_hex(hid_desc->wDescriptorLength);
#endif

      hdev->descriptor_size = le16_to_cpu(hid_desc->wDescriptorLength);
      break;

    case USB_DT_ENDPOINT:
      if (hdev == NULL) {
        break;
      }
      usb_ep_desc_t *ep_desc = (usb_ep_desc_t *)ptr_iface;
      if (!(ep_desc->bEndpointAddress & USB_EP_DIR_IN)) {
        break;
      }
      // TODO(q3k): don't allocate this yet
      if (!uhd_ep_alloc(dev->address, ep_desc)) {
        return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail
      }
#ifdef DEBUG
      print_dbg("\r\n ep_desc -> bEndpointAddress : ");
      print_dbg_hex(ep_desc->bEndpointAddress);
      print_dbg("\r\n ep_desc -> wMaxPacketSize : ");
      print_dbg_hex(ep_desc->wMaxPacketSize);
#endif

      hdev->ep_in = ep_desc->bEndpointAddress;
      hdev->report_size = le16_to_cpu(ep_desc->wMaxPacketSize);
      break;

    default:
      break;
    }

    Assert(conf_desc_lgt>=ptr_iface->bLength);
    conf_desc_lgt -= ptr_iface->bLength;
    ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
  }

  if (hdev != NULL) {
    multihid_usb_device_add_hid(udev, hdev);
  }

  if (udev->hid_devices != NULL) {
    return UHC_ENUM_SUCCESS;
  }
  return UHC_ENUM_UNSUPPORTED;
}

void multihid_enable(uhc_device_t* dev) {
  multihid_usb_device_t *udev = &multihid_usb_dev;
  if (udev->dev != dev) {
    return;
  }
  multihid_descriptors_fetch_start(udev->dev, udev->hid_devices);
}

void multihid_uninstall(uhc_device_t* dev) {
  multihid_usb_device_t *udev = &multihid_usb_dev;
  if (udev->dev != dev) {
    return;
  }

  multihid_change(udev->hid_devices, false);

  udev->dev = NULL;

  multihid_device_t *device = udev->hid_devices;
  while (device != NULL) {
      multihid_device_t *next = device->next;
      multihid_device_free(device);
      device = next;
  }
  udev->hid_devices = NULL;
}

static bool multihid_device_validate(multihid_device_t *hdev) {
  if (hdev->ep_in == 0) {
    print_dbg("\r\n HID device: invalid endpoint: no IN endpoint");
    return false;
  }
  if (hdev->report_size == 0) {
    print_dbg("\r\n HID device: invalid endpoint: no report");
    return false;
  }
  if (hdev->descriptor_size == 0) {
    print_dbg("\r\n HID device: invalid endpoint: no HID descriptor");
    return false;
  }
  return true;
}

void multihid_device_free(multihid_device_t *hdev) {
  if (hdev->report != NULL) {
    free(hdev->report);
    hdev->report = NULL;
  }
  if (hdev->descriptor != NULL) {
    free(hdev->descriptor);
    hdev->descriptor = NULL;
  }
  free(hdev);
}


static void multihid_usb_device_add_hid(multihid_usb_device_t *udev, multihid_device_t *hdev) {
  if (!multihid_device_validate(hdev)) {
    multihid_device_free(hdev);
  } else {
    hdev->next = udev->hid_devices;
    udev->hid_devices = hdev;
  }
}

void multihid_start(multihid_device_t *hdev, multihid_callback_t callback, void *context) {
  multihid_usb_device_t *udev = &multihid_usb_dev;

  hdev->callback = callback;
  hdev->callback_context = context;
  if (hdev->report == NULL) {
    hdev->report = malloc(hdev->report_size);
    if (hdev->report == NULL) {
      print_dbg("\r\n HID device: failed to allocate report buffer");
      return;
    }
  }

#ifdef DEBUG
  print_dbg("\r\n HID device: starting first uhd_ep_run, add ");
  print_dbg_ulong(udev->dev->address);
  print_dbg(", ep_in ");
  print_dbg_ulong(hdev->ep_in);
#endif
  if (!uhd_ep_run(udev->dev->address, hdev->ep_in, true, hdev->report,
               hdev->report_size, 0, multihid_on_report)) {
      print_dbg("\r\n HID device: initial uhd_ep_run failed!, ep_in: ");
      print_dbg_ulong(hdev->ep_in);
  }
}

static void multihid_on_report(usb_add_t add, usb_ep_t ep, uhd_trans_status_t status, iram_size_t nb_transfered)
{
  if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
    return; 
  }

  multihid_usb_device_t *udev = &multihid_usb_dev;
  multihid_device_t *hdev = (multihid_device_t *)udev->hid_devices;
  while (hdev != NULL) {
      if (hdev->ep_in == ep) {
          break;
      }
      hdev = hdev->next;
  }
  if (hdev == NULL) {
      return;
  }

  hdev->callback(hdev->callback_context, hdev->report, hdev->report_size);

  if (!uhd_ep_run(add, ep, true, hdev->report, hdev->report_size, 0, multihid_on_report)) {
    print_dbg("\r\n HID device: uhd_ep_run failed!, ep_in: ");
    print_dbg_ulong(hdev->ep_in);
  }
}

void multihid_change(multihid_device_t* devices, bool plug) {
  event_t e;
  if (plug) {
    e.type = kEventMultihidConnect;
  } else {
    e.type = kEventMultihidDisconnect;
  }
  e.data = (s32)devices;
  event_post(&e);
}

#undef DEBUG
