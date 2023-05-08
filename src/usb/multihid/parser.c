// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

#include "parser.h"

bool multihid_parse_collection(multihid_parser_t *p) {
  if (p->cur == NULL) {
    return false;
  }

  while (true) {
    if (p->cur == p->buf_end) {
      return true;
    }
    if (p->cur > p->buf_end) {
      return false;
    }

    uint8_t header = *(p->cur);
    multihid_parser_header_t hdr = {
      .tag = header >> 4,
      .type = (header >> 2) & 0b11,
      .size = header & 0b11,
    };
    p->cur++;
    if (hdr.size == 0) {
      return true;
    }

    (p->visitor)(p->visitor_context, hdr, p->cur);
    p->cur += hdr.size;

    if (hdr.tag == 0xa) {
      // Nested collection.
      if (!multihid_parse_collection(p)) {
        return false;
      } else {
      }
    }
  }
}
