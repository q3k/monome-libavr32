#include "edgetrigger.h"

edgetrigger_t *edgetrigger_new(size_t size) {
    edgetrigger_t *t = calloc(sizeof(edgetrigger_t), 1);
    if (t == NULL) {
        return NULL;
    }

    t->buffer = calloc(size, 1);
    if (t->buffer == NULL) {
        free(t);
        return NULL;
    }
    t->buffer_size = size;
    return t;
}

void edgetrigger_free(edgetrigger_t *t) {
    if (t->buffer != NULL) {
        free(t->buffer);
        t->buffer = NULL;
    }
    free(t);
}

void edgetrigger_fill(edgetrigger_t *t, uint8_t *data) {
    for (int i = 0; i < t->buffer_size; i++) {
        if (t->buffer[i] != data[i]) {
            t->dirty = true;
        }
        t->buffer[i] = data[i];
    }
}

bool edgetrigger_is_dirty(edgetrigger_t *t) {
    return t->dirty;
}

void edgetrigger_clear_dirty(edgetrigger_t *t) {
    t->dirty = false;
}

void *edgetrigger_buffer(edgetrigger_t *t) {
    return t->buffer;
}