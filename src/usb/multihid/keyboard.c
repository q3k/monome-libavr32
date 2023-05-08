// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

#include "keyboard.h"
#include "parser.h"

#include "print_funcs.h"

// #define DEBUG

typedef struct {
  bool in_collection;
  uint8_t collection;
  uint8_t usage;
  uint8_t usage_minimum;
  uint8_t usage_maximum;
  uint8_t logical_minimum;
  uint8_t logical_maximum;
  uint8_t report_size;
  uint8_t report_count;

  bool have_modifiers;
  bool have_padding;
  bool have_leds;
  bool have_keys;

  bool not_a_keyboard;
} multihid_keyboard_ctx_t;


static void multihid_keyboard_visitor(void *context, multihid_parser_header_t hdr, uint8_t *data) {
  multihid_keyboard_ctx_t *ctx = (multihid_keyboard_ctx_t *)context;
  if (ctx->not_a_keyboard) {
    return;
  }

  if (hdr.tag == 0x0a) {
    ctx->collection = *data;
    ctx->in_collection = true;
    return;
  }

  if (!ctx->in_collection) {
    switch (hdr.tag) {
      case 0: { // Usage
        uint8_t usage = *data;
        switch (hdr.type) {
        case 1: // Global
          if (usage != 1) {
#ifdef DEBUG
            print_dbg(": not generic desktop controls");
#endif
            ctx->not_a_keyboard = true;
          }
          break;
        case 2: // Local
          if (usage != 6) {
#ifdef DEBUG
            print_dbg(": not a keyboard descriptor");
#endif
            ctx->not_a_keyboard = true;
          }
          break;
        }
        break;
      }
    }
  } else if (ctx->in_collection && ctx->collection == 1) {
    switch (hdr.tag) {
    case 0: // Usage
      if (hdr.type == 1)
        ctx->usage = *data;
      break;
    case 1: // Minimum
      if (hdr.type == 1)
        ctx->logical_minimum = *data;
      if (hdr.type == 2)
        ctx->usage_minimum = *data;
      break;
    case 2: // Maximum
      if (hdr.type == 1)
        ctx->logical_maximum = *data;
      if (hdr.type == 2)
        ctx->usage_maximum = *data;
      break;
    case 7: // Report size
      ctx->report_size = *data;
      break;
    case 9: // Report count
      ctx->report_count = *data;
      break;
    }

    if (hdr.tag == 8 && hdr.type == 0) { // Input
      uint8_t input = *data;
      if (!ctx->have_modifiers) {
        if (ctx->usage_minimum != 0xe0
         || ctx->usage_maximum != 0xe7
         || ctx->logical_minimum != 0
         || ctx->logical_maximum != 1
         || ctx->report_size != 1
         || ctx->report_count != 8
         || ctx->usage != 7
         || input != 2) {

#ifdef DEBUG
          print_dbg(": wrong modifiers");
#endif
          ctx->not_a_keyboard = true;
          return;
        }
        ctx->have_modifiers = true;
        return;
      }
      if (!ctx->have_padding) {
        if (ctx->report_size != 8
         || ctx->report_count != 1
         || ctx->usage != 7
         || input != 3) {
#ifdef DEBUG
          print_dbg(": wrong padding");
#endif
          ctx->not_a_keyboard = true;
          return;
        }
        ctx->have_padding = true;
        return;
      }
      if (!ctx->have_keys) {
        if (ctx->usage_minimum != 0
         || ctx->logical_minimum != 0
         || ctx->report_size != 8
         || ctx->report_count < 6
         || ctx->usage != 7
         || input != 00) {
#ifdef DEBUG
          print_dbg(": wrong keys");
#endif
          ctx->not_a_keyboard = true;
          return;
        }
        ctx->have_keys = true;
        return;
      }
    }
    if (hdr.tag == 9 && hdr.type == 0) { // Output
      uint8_t output = *data;
      if (!ctx->have_leds) {
        if (ctx->usage_minimum != 1
         || ctx->usage_maximum != 3
         || ctx->report_size != 1
         || ctx->report_count < 1
         || ctx->usage != 8
         || output != 2) {
          print_dbg(": wrong leds");
          ctx->not_a_keyboard = true;
          return;
        }
        ctx->have_leds = true;
        return;
      }
    }
  }
}

static bool multihid_is_boot_keyboard_shaped(multihid_device_t *hid_device) {
  multihid_keyboard_ctx_t ctx = { 0 };
  multihid_parser_t parser = {
    .buf = hid_device->descriptor,
    .buf_end = hid_device->descriptor + hid_device->descriptor_size,
    .cur = hid_device->descriptor,
    .visitor = multihid_keyboard_visitor,
    .visitor_context = &ctx,
  };

#ifdef DEBUG
  print_dbg("\r\n HID keyboard: parsing as a boot keyboard");
#endif
  if (!multihid_parse_collection(&parser)) {
#ifdef DEBUG
    print_dbg(": parse failed");
#endif
    return false;
  }

  if (ctx.not_a_keyboard) {
    return false;
  }
  if (!ctx.have_keys) {
#ifdef DEBUG
    print_dbg(": not a boot 6KRO keyboard");
#endif
    return false;
  }
  if (!ctx.have_leds) {
#ifdef DEBUG
    print_dbg(": no LEDs");
#endif
    return false;
  }

#ifdef DEBUG
  print_dbg(": seems about right");
#endif
  return true;
}

multihid_device_t *multihid_find_keyboard(multihid_device_t *hid_devices) {
  // First try to find a keyboard with a boot protocol subclass.
  size_t num_keyboards = 0;
  multihid_device_t *device = hid_devices;
  while (device != NULL) {
    // Keyboard
    if (device->interface_protocol != 1) {
      device = device->next;
      continue;
    }
    num_keyboards += 1;

    // Boot keyboard
    if (device->interface_subclass != 1) {
      device = device->next;
      continue;
    }

#ifdef DEBUG
    print_dbg("\r\n HID keyboard: found boot protocol keyboard");
#endif
    return device;
  }

  // If we have exactly one keyboard, return that.
  if (num_keyboards == 1) {
#ifdef DEBUG
    print_dbg("\r\n HID keyboard: falling back to only keyboard");
#endif
    device = hid_devices;
    while (device != NULL) {
      if (device->interface_protocol == 1) {
        return device;
      }
      device = device->next;
    }
  }

  // Otherwise, find the most boot-shaped keyboard.
  device = hid_devices;
  while (device != NULL) {
    if (multihid_is_boot_keyboard_shaped(device)) {
      return device;
    }
    device = device->next;
  }

  return NULL;
}

#undef DEBUG