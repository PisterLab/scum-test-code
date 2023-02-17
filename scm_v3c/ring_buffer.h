// The ring buffer assumes a single producer and consumer, which can run in
// separate threads. The ring buffer is implemented as a lock-free ring buffer.

#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Maximum number of elements in the ring buffer.
#define RING_BUFFER_MAX_SIZE 512

// Ring buffer element type.
typedef uint8_t ring_buffer_type_t;

// Ring buffer struct.
typedef struct {
    // Ring buffer. The extra element is used to signify a full vs. empty ring
    // buffer.
    ring_buffer_type_t buffer[RING_BUFFER_MAX_SIZE + 1];

    // Head index.
    size_t head;

    // Tail index.
    size_t tail;
} ring_buffer_t;

// Initialize a ring buffer.
void ring_buffer_init(ring_buffer_t* ring_buffer);

// Push an element into the ring buffer if the ring buffer is not full.
// Return whether the element was successfully pushed.
bool ring_buffer_push(ring_buffer_t* ring_buffer,
                      const ring_buffer_type_t* element);

// Pop an element from the ring buffer if the ring buffer is not empty.
// Return whether an element was successfully popped.
bool ring_buffer_pop(ring_buffer_t* ring_buffer, ring_buffer_type_t* element);

// Return whether the ring buffer is empty.
bool ring_buffer_empty(const ring_buffer_t* ring_buffer);

// Return whether the ring buffer is full.
bool ring_buffer_full(const ring_buffer_t* ring_buffer);

#endif  // __RING_BUFFER_H
