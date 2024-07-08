#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

#include <stdint.h>
#include "my_secmalloc.h"


/**
 * @brief Canary value for detecting buffer overflows.
 */
#define CANARY_VALUE 0xDEADBEEF

/**
 * @brief Size of a page in bytes.
 */
#define PAGE_SIZE 4096

/**
 * @brief Size of a chunk in bytes.
 */
#define CHUNK_SIZE sizeof(struct chunk)

/**
 * @brief Number of metadata chunks per page.
 */
#define METADATA_CHUNKS_PER_PAGE (PAGE_SIZE / CHUNK_SIZE)

/**
 * @brief Enumeration for chunk types.
 */
enum chunk_type {
    FREE = 0,  /**< Indicates a free chunk. */
    BUSY = 1   /**< Indicates a busy (allocated) chunk. */
};

/**
 * @brief Structure representing a memory chunk.
 */
struct chunk {
    size_t size;            /**< Size of the chunk's data area. */
    uint32_t canary_start;  /**< Canary value at the start of the chunk. */
    uint32_t canary_end;    /**< Canary value at the end of the chunk. */
    struct chunk *next;     /**< Pointer to the next chunk in the linked list. */
    struct chunk *prev;     /**< Pointer to the previous chunk in the linked list. */
    enum chunk_type flags;  /**< Flags indicating the status of the chunk (FREE or BUSY). */
};
void initialize_metadata();
void check_free_leak();
void initialize_data();
struct chunk *find_free_chunk(size_t size);
struct chunk *allocate_page();
void log_execution_report(int color_value, const char *func_type, size_t size, void *addr);
void debug_print(int color_value, const char *format, ...);
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);
void init_execution_report();
#endif
