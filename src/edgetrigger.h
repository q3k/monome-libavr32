#ifndef _EDGETRIGGER_H_
#define _EDGETRIGGER_H_

#include "compiler.h"

typedef struct {
    uint8_t *buffer;
    size_t buffer_size;
    bool dirty;
} edgetrigger_t;

edgetrigger_t *edgetrigger_new(size_t size);
void edgetrigger_free(edgetrigger_t *t);
void edgetrigger_fill(edgetrigger_t *t, uint8_t *data);
bool edgetrigger_is_dirty(edgetrigger_t *t);
void edgetrigger_clear_dirty(edgetrigger_t *t);
void *edgetrigger_buffer(edgetrigger_t *t);

#endif