#define _GNU_SOURCE
#include <stdio.h>
#include <alloca.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "my_secmalloc.private.h"

FILE *execution_report = NULL;
struct chunk *metadata_pages = NULL;
struct chunk *data_pages = NULL;
struct chunk *free_list = NULL; 

/**
 * @brief Splits a chunk if it's larger than the requested size.
 *
 * @param chunk Pointer to the chunk to be split (input).
 * @param size Requested size for the allocation (input).
 *
 * This function splits the given chunk into two parts if its size is larger than
 * the requested size plus the size of the chunk metadata. The remaining part
 * becomes a new free chunk.
 */
 static void split_chunk(struct chunk *chunk, size_t size) {
        log_execution_report(3, "splitting chunk ", size, chunk);
    if (chunk->size >= size + CHUNK_SIZE + 1) {
        struct chunk *new_chunk = (struct chunk *)((char *)chunk + CHUNK_SIZE + size);
        new_chunk->size = chunk->size - size - CHUNK_SIZE;
        new_chunk->flags = FREE;
        new_chunk->canary_start = CANARY_VALUE;
        new_chunk->canary_end = CANARY_VALUE;
        new_chunk->next = chunk->next;
        new_chunk->prev = chunk;
        if (chunk->next) {
            chunk->next->prev = new_chunk;
        }
        chunk->next = new_chunk;
        chunk->size = size;
        chunk->canary_end = CANARY_VALUE;
        log_execution_report(2, "split_chunk ", size, chunk);
    }
}

/**
 * @brief Allocates memory of the specified size.
 *
 * @param size Size of the memory to allocate (input).
 * @return Pointer to the allocated memory block, or NULL if allocation fails.
 *
 * This function allocates memory of the specified size. It first checks for a free chunk
 * of sufficient size. If no suitable chunk is found, it allocates a new page of memory
 * and initializes it as a free chunk. If allocation fails, it returns NULL.
 */
 void *my_malloc(size_t size) {
    log_execution_report(3, "my_malloc called", size, NULL);
    if (size == 0) {
        return NULL;
    }
    if (metadata_pages == NULL) {
        initialize_metadata();
    }
    if (data_pages == NULL) {
        initialize_data();
    }
    struct chunk *free_chunk = find_free_chunk(size);
    if (free_chunk == NULL) {
        struct chunk *new_data_page = allocate_page();
        if (new_data_page == NULL) {
            log_execution_report(1, "Failed to allocated memory", 0,new_data_page);
            return NULL;
        }
        new_data_page->size = PAGE_SIZE - CHUNK_SIZE;
        new_data_page->flags = FREE;
        new_data_page->canary_start = CANARY_VALUE;
        new_data_page->canary_end = CANARY_VALUE;
        new_data_page->next = data_pages;
        data_pages = new_data_page;
        free_chunk = new_data_page;
    }
    split_chunk(free_chunk, size);
    free_chunk->flags = BUSY;
    if (free_chunk->prev) {
        free_chunk->prev->next = free_chunk->next;
    } else {
        free_list = free_chunk->next;
    }
    if (free_chunk->next) {
        free_chunk->next->prev = free_chunk->prev;
    }
    void *allocated_memory = (void *)(free_chunk + 1);
    log_execution_report(2, "my_malloc",free_chunk->size , allocated_memory);

    return allocated_memory;
}

#ifdef DYNAMIC

/**
 * @brief Overrides the standard library function malloc with my_malloc.
 *
 * @param size Size of the memory to allocate (input).
 * @return Pointer to the allocated memory block, or NULL if allocation fails.
 *
 * This function overrides the standard library function malloc with my_malloc.
 */
void    *malloc(size_t size)
{
    return my_malloc(size);
}

/**
 * @brief Overrides the standard library function free with my_free.
 *
 * @param ptr Pointer to the memory block to free (input).
 *
 * This function overrides the standard library function free with my_free.
 */

void    free(void *ptr)
{
    my_free(ptr);
}

/**
 * @brief Overrides the standard library function calloc with my_calloc.
 *
 * @param nmemb Number of elements (input).
 * @param size Size of each element (input).
 * @return Pointer to the allocated memory block, or NULL if allocation fails.
 *
 * This function overrides the standard library function calloc with my_calloc.
 */
void    *calloc(size_t nmemb, size_t size)
{
    return my_calloc(nmemb, size);
}

/**
 * @brief Overrides the standard library function realloc with my_realloc.
 *
 * @param ptr Pointer to the previously allocated memory block (input).
 * @param size New size of the memory block (input).
 * @return Pointer to the reallocated memory block, or NULL if reallocation fails.
 *
 * This function overrides the standard library function realloc with my_realloc.
 */
void    *realloc(void *ptr, size_t size)
{
    return my_realloc(ptr, size);

}

#endif