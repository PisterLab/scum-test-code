#include "ring_buffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Return the next index in the ring buffer.
static inline size_t ring_buffer_next_index(const size_t index) {
    return (index + 1) % (RING_BUFFER_MAX_SIZE + 1);
}

void ring_buffer_init(ring_buffer_t* ring_buffer) {
    ring_buffer->head = 0;
    ring_buffer->tail = 0;
}

bool ring_buffer_push(ring_buffer_t* ring_buffer,
                      const ring_buffer_type_t* element) {
    if (ring_buffer_full(ring_buffer)) {
        return false;
    }

    ring_buffer->buffer[ring_buffer->head] = *element;
    ring_buffer->head = ring_buffer_next_index(ring_buffer->head);
    return true;
}

bool ring_buffer_pop(ring_buffer_t* ring_buffer, ring_buffer_type_t* element) {
    if (ring_buffer_empty(ring_buffer)) {
        return false;
    }

    *element = ring_buffer->buffer[ring_buffer->tail];
    ring_buffer->tail = ring_buffer_next_index(ring_buffer->tail);
    return true;
}

bool ring_buffer_empty(const ring_buffer_t* ring_buffer) {
    return ring_buffer->head == ring_buffer->tail;
}

bool ring_buffer_full(const ring_buffer_t* ring_buffer) {
    return ring_buffer_next_index(ring_buffer->head) == ring_buffer->tail;
}
