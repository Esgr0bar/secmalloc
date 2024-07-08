#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdlib.h>
#include "my_secmalloc.private.h"



extern struct chunk *metadata_pages;
extern struct chunk *data_pages;
extern struct chunk *free_list;

/**
 * @brief Allocates a new memory page for metadata or data.
 * 
 * @return struct chunk* Pointer to the newly allocated page, or NULL if allocation fails.
 * 
 * This function uses `mmap` to allocate a new memory page. It logs an error message 
 * if the allocation fails and a success message if the allocation is successful.
 */
struct chunk *allocate_page() {
    struct chunk *page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) {
        log_execution_report(1,"Failed to allocate page", PAGE_SIZE - CHUNK_SIZE, page);
        return NULL;
    }
    log_execution_report(2,"Allocated page :",PAGE_SIZE - CHUNK_SIZE, page);
    return page;
}

/**
 * @brief Initializes the metadata pages.
 * 
 * This function allocates a new page for metadata and initializes its fields.
 * If the allocation fails, the function terminates the program.
 * The new metadata page is also added to the free list.
 */
void initialize_metadata() {
    metadata_pages = allocate_page();
    if (metadata_pages == NULL) {
        exit(EXIT_FAILURE);
    }
    metadata_pages->size = PAGE_SIZE - CHUNK_SIZE;
    metadata_pages->flags = FREE;
    metadata_pages->canary_start = CANARY_VALUE;
    metadata_pages->canary_end = CANARY_VALUE;
    metadata_pages->next = NULL;
    metadata_pages->prev = NULL;
    free_list = metadata_pages;
}

/**
 * @brief Initializes the data pages.
 * 
 * This function allocates a new page for data and initializes its fields.
 * If the allocation fails, the function terminates the program.
 */
void initialize_data() {
    data_pages = allocate_page();
    if (data_pages == NULL) {
        exit(EXIT_FAILURE);
    }
    data_pages->size = PAGE_SIZE - CHUNK_SIZE;
    data_pages->flags = FREE;
    data_pages->canary_start = CANARY_VALUE;
    data_pages->canary_end = CANARY_VALUE;
    data_pages->next = NULL;
    data_pages->prev = NULL;
}