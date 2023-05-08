#ifndef _USB_MULTIHID_PARSER_H_
#define _USB_MULTIHID_PARSER_H_

// Copyright 2023 Serge 'q3k' Bazanski
// SPDX-License-Identifier: GPL-2.0

#include "compiler.h"

typedef struct {
  uint8_t tag;
  uint8_t type;
  uint8_t size;
} multihid_parser_header_t;

typedef void (*multihid_parser_visitor_t)(void *context, multihid_parser_header_t hdr, uint8_t *data);

typedef struct {
  uint8_t *buf;
  uint8_t *buf_end;

  uint8_t *cur;

  multihid_parser_visitor_t visitor;
  void *visitor_context;
} multihid_parser_t;

bool multihid_parse_collection(multihid_parser_t *p);

#endif