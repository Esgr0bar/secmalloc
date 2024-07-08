
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "my_secmalloc.private.h"


extern struct chunk *free_list;
extern struct chunk *metadata_pages;

/**
 * @brief Finds a free chunk of memory of at least the specified size.
 *
 * @param size Size of the memory chunk to find (input).
 * @return Pointer to the found free chunk, or NULL if no suitable chunk is found.
 *
 * This function iterates through the free list to find a chunk that is marked as free
 * and has a size greater than or equal to the requested size.
 */
 struct chunk *find_free_chunk(size_t size) {

    struct chunk *current = free_list;
    log_execution_report(2, "Find free chunk", size, current);
    while (current != NULL) {
        if (current->flags == FREE && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    log_execution_report(2, "Free chunk found", size, current);
    return NULL;
}

/*
static void check_free_leak() {
    struct chunk *current = metadata_pages;
    while (current != NULL) {
        if (current->flags != FREE) {
            log_execution_report(1, "Memory leak detected: chunk not freed", current->size, (void *)(current + 1));
            my_free(current->next);
        }
        current = current->next;
    }
}*/

/**
 * @brief Frees a memory chunk previously allocated by my_malloc.
 *
 * @param ptr Pointer to the memory chunk to free (input).
 *
 * This function validates the canary value, checks for double free errors,
 * marks the chunk as free, and updates the free list accordingly.
 */
 void my_free(void *ptr) {
    log_execution_report(3, "my_free called", 0, ptr);
    if (ptr == NULL) {
        return;
    }
    struct chunk *metadata_chunk = (struct chunk *)ptr - 1;
    if (metadata_chunk->size == 0) {
        log_execution_report(1, "my_free error: Invalid free or corrupted memory", metadata_chunk->size, metadata_chunk);
        return;
    }
    if (metadata_chunk->flags == FREE) {
        log_execution_report(1, "my_free error: Invalid free: Double free detected", metadata_chunk->size, metadata_chunk);
        return;
    }
    if (metadata_chunk->canary_start != CANARY_VALUE || metadata_chunk->canary_end != CANARY_VALUE) {
        log_execution_report(1, "my_free error: Invalid free or corrupted memory", metadata_chunk->size, metadata_chunk);
        return;
    }
    metadata_chunk->flags = FREE;
    metadata_chunk->next = free_list;
    if (free_list) {
        free_list->prev = metadata_chunk;
    }
    free_list = metadata_chunk;
    metadata_chunk->prev = NULL;

    log_execution_report(2, "my_free freed memory", metadata_chunk->size, ptr);
}
