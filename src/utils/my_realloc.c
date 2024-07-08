#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <unistd.h> // For sysconf
#include "my_secmalloc.private.h"

extern struct chunk *free_list;

/**
 * @brief Reallocates a memory block previously allocated by my_malloc or my_calloc.
 *
 * @param ptr Pointer to the previously allocated memory block (input).
 * @param size New size of the memory block (input).
 * @return Pointer to the reallocated memory block, or NULL if reallocation fails.
 *
 * This function reallocates the memory block pointed to by ptr to the new size specified by size.
 * If ptr is NULL, it behaves like my_malloc(size).
 * If size is 0, it behaves like my_free(ptr).
 * It validates the canary value of the metadata chunk to ensure memory integrity.
 * If the new size is the same as the current size, it returns ptr without reallocation.
 * Otherwise, it allocates a new memory block of the requested size, copies the data from the
 * old block to the new block, frees the old block, and returns the new block.
 */
void *my_realloc(void *ptr, size_t size) {
    log_execution_report(3, "my_realloc called", size, ptr);

    if (ptr == NULL) {
        return my_malloc(size);
    }

    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    struct chunk *metadata_chunk = (struct chunk *)ptr - 1;

    if (metadata_chunk->canary_start != CANARY_VALUE || metadata_chunk->canary_end != CANARY_VALUE) {
        log_execution_report(1, "my_realloc error: Invalid realloc or corrupted memory", metadata_chunk->size, metadata_chunk);
        return NULL;
    }

    if (metadata_chunk->size == size) {
        return ptr;
    }

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        log_execution_report(1, "my_realloc error: Unable to determine page size", 0, NULL);
        return NULL;
    }

    struct chunk *prev = NULL;
    struct chunk *curr = free_list;
    struct chunk *free_fit = NULL;

    while (curr) {
        if (curr->size >= size) {
            if (!free_fit || curr->size < free_fit->size) {
                free_fit = curr;
            }
        }
        prev = curr;
        curr = curr->next;
    }

    if (free_fit) {
        if (prev) {
            prev->next = free_fit->next;
        } else {
            free_list = free_fit->next;
        }

        void *new_ptr = (void *)(free_fit + 1);
        size_t copy_size = 0;
        if (metadata_chunk->size < size) {
            copy_size = metadata_chunk->size;
        } else {
            copy_size = size;
        }
        memcpy(new_ptr, ptr, copy_size);
        my_free(ptr);

        free_fit->canary_start = CANARY_VALUE;
        free_fit->canary_end = CANARY_VALUE;

        log_execution_report(2, "my_realloc reallocated memory to existing free chunk: %p", 0, new_ptr);
        return new_ptr;
    }

    void *new_ptr = my_malloc(size);
    if (new_ptr) {
        size_t copy_size = 0;
        if (metadata_chunk->size < size) {
            copy_size = metadata_chunk->size;
        } else {
            copy_size = size;
        }

        memcpy(new_ptr, ptr, copy_size);
        my_free(ptr);
        ptr = NULL;
    } else {
        log_execution_report(1, "my_realloc error: Allocation failed", 0, new_ptr);
    }

    log_execution_report(2, "my_realloc reallocated memory to: %p", 0, new_ptr);
    return new_ptr;
}