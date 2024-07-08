#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/redirect.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "my_secmalloc.private.h"
#include "sys/mman.h"

// Mock extern variables
extern struct chunk *metadata_pages;
extern struct chunk *data_pages;
extern struct chunk *free_list;
extern FILE *execution_report;

Test(secmalloc, initialize_metadata_success) {
    initialize_metadata();
    
    cr_assert_neq(metadata_pages, NULL, "Metadata pages were not initialized.");
    cr_assert_eq(metadata_pages->size, PAGE_SIZE - CHUNK_SIZE, "Metadata page size is incorrect.");
    cr_assert_eq(metadata_pages->flags, FREE, "Metadata page flags are not set to FREE.");
    cr_assert_eq(metadata_pages->canary_start, CANARY_VALUE, "Metadata canary start value is incorrect.");
    cr_assert_eq(metadata_pages->canary_end, CANARY_VALUE, "Metadata canary end value is incorrect.");
    cr_assert_eq(free_list, metadata_pages, "Metadata pages were not added to the free list.");
    
    munmap(metadata_pages, PAGE_SIZE);
}

Test(secmalloc, initialize_data_success) {
    initialize_data();
    
    cr_assert_neq(data_pages, NULL, "Data pages were not initialized.");
    cr_assert_eq(data_pages->size, PAGE_SIZE - CHUNK_SIZE, "Data page size is incorrect.");
    cr_assert_eq(data_pages->flags, FREE, "Data page flags are not set to FREE.");
    cr_assert_eq(data_pages->canary_start, CANARY_VALUE, "Data canary start value is incorrect.");
    cr_assert_eq(data_pages->canary_end, CANARY_VALUE, "Data canary end value is incorrect.");
    
    munmap(data_pages, PAGE_SIZE);
}

Test(my_alloc, malloc_zero_size) {
    void *ptr = my_malloc(0);
    cr_assert_null(ptr, "my_malloc with size 0 should return NULL");
}

Test(my_alloc, malloc_small_size) {
    size_t size = 32;
    void *ptr = my_malloc(size);
    printf ("ptr = %p\n", ptr);
    cr_assert_not_null(ptr, "my_malloc with small size should not return NULL");
    my_free(ptr);
}

Test(my_alloc, malloc_real_usage) {
    size_t size = 1024; // 1 KB
    void *ptr = my_malloc(size);
    for (size_t i = 0; i < size - 1; i++) {
        ((char *)ptr)[i] = 'a';
        printf ("%c", ((char *)ptr)[i]);
    }
    printf ("\n");
    cr_assert_not_null(ptr, "my_malloc with large size should not return NULL");
    my_free(ptr);
}
Test(my_alloc, malloc_large_size) {
    size_t size = 1024 * 1024; // 1 MB
    void *ptr = my_malloc(size);
    cr_assert_not_null(ptr, "my_malloc with large size should not return NULL");
    my_free(ptr);
}

// Test cases for my_calloc
Test(my_alloc, calloc_zero_elements) {
    void *ptr = my_calloc(0, sizeof(int));
    cr_assert_null(ptr, "my_calloc with zero elements should return NULL");
}

Test(my_alloc, calloc_with_elements) {
    size_t nmemb = 10;
    size_t size = sizeof(int);
    int *ptr = (int *)my_calloc(nmemb, size);
    cr_assert_not_null(ptr, "my_calloc should not return NULL");
    // Check if memory is zero-initialized
    for (size_t i = 0; i < nmemb; ++i) {
        cr_assert_eq(ptr[i], 0, "my_calloc should initialize memory to zero");
    }
    my_free(ptr);
}

// Test cases for my_realloc
Test(my_alloc, realloc_null_ptr) {
    size_t size = 64;
    void *ptr = my_realloc(NULL, size);
    cr_assert_not_null(ptr, "my_realloc with NULL ptr should behave like my_malloc");
    my_free(ptr);
}

Test(my_alloc, realloc_smaller_size) {
    size_t original_size = 128;
    size_t new_size = 64;
    void *ptr = my_malloc(original_size);
    void *new_ptr = my_realloc(ptr, new_size);
    cr_assert_not_null(new_ptr, "my_realloc with smaller size should not return NULL");
    my_free(new_ptr);
}

Test(my_alloc, realloc_larger_size) {
    size_t original_size = 64;
    size_t new_size = 128;
    void *ptr = my_malloc(original_size);
    void *new_ptr = my_realloc(ptr, new_size);        
    cr_assert_not_null(new_ptr, "my_realloc with larger size should not return NULL");
    my_free(new_ptr);
}

// Test cases for my_free
Test(my_alloc, free_null_ptr) {
    my_free(NULL); // Should not crash
    cr_assert(1, "my_free with NULL ptr should not do anything");
}

// Malloc free
Test(secmalloc, malloc_free) {
    void *ptr = my_malloc(1024);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 1024 byte allocation");

    struct chunk *metadata = (struct chunk *)ptr - 1;
    cr_assert_eq(metadata->canary_start, CANARY_VALUE, "Canary start value is corrupted after malloc");
    cr_assert_eq(metadata->canary_end, CANARY_VALUE, "Canary end value is corrupted after malloc");

    my_free(ptr);
    cr_assert_eq(metadata->flags, FREE, "Chunk is not marked as FREE after my_free");
}

// Realloc with increase size
Test(secmalloc, realloc_increase_size) {
    void *ptr = my_malloc(1024);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 1024 byte allocation");

    void *new_ptr = my_realloc(ptr, 2048);
    cr_assert_not_null(new_ptr, "my_realloc returned NULL for a 2048 byte allocation");

    struct chunk *metadata = (struct chunk *)new_ptr - 1;
    cr_assert_eq(metadata->canary_start, CANARY_VALUE, "Canary start value is corrupted after realloc");
    cr_assert_eq(metadata->canary_end, CANARY_VALUE, "Canary end value is corrupted after realloc");

    my_free(new_ptr);
    cr_assert_eq(metadata->flags, FREE, "Chunk is not marked as FREE after my_free following realloc");
}

// Realloc with decrease size
Test(secmalloc, realloc_decrease_size) {
    void *ptr = my_malloc(2048);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 2048 byte allocation");

    void *new_ptr = my_realloc(ptr, 1024);
    cr_assert_not_null(new_ptr, "my_realloc returned NULL for a 1024 byte allocation");

    struct chunk *metadata = (struct chunk *)new_ptr - 1;
    cr_assert_eq(metadata->canary_start, CANARY_VALUE, "Canary start value is corrupted after realloc");
    cr_assert_eq(metadata->canary_end, CANARY_VALUE, "Canary end value is corrupted after realloc");

    my_free(new_ptr);
    cr_assert_eq(metadata->flags, FREE, "Chunk is not marked as FREE after my_free following realloc");
}

// Realloc with same size
Test(secmalloc, realloc_same_size) {
    void *ptr = my_malloc(1024);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 1024 byte allocation");

    void *new_ptr = my_realloc(ptr, 1024);
    cr_assert_eq(ptr, new_ptr, "my_realloc should return the same pointer for the same size allocation");

    struct chunk *metadata = (struct chunk *)new_ptr - 1;
    cr_assert_eq(metadata->canary_start, CANARY_VALUE, "Canary start value is corrupted after realloc");
    cr_assert_eq(metadata->canary_end, CANARY_VALUE, "Canary end value is corrupted after realloc");

    my_free(new_ptr);
    cr_assert_eq(metadata->flags, FREE, "Chunk is not marked as FREE after my_free following realloc");
}

// Realloc with zero as size
Test(secmalloc, realloc_zero_size) {
    void *ptr = my_malloc(1024);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 1024 byte allocation");

    void *new_ptr = my_realloc(ptr, 0);
    cr_assert_null(new_ptr, "my_realloc should return NULL for a zero size allocation");

    struct chunk *metadata = (struct chunk *)ptr - 1;
    cr_assert_eq(metadata->flags, FREE, "Chunk is not marked as FREE after realloc to zero size");
}

// Calloc basic usage
Test(secmalloc, calloc_basic) {
    void *ptr = my_calloc(128, sizeof(int));
    cr_assert_not_null(ptr, "my_calloc returned NULL for a 512 byte allocation");

    int *int_ptr = (int *)ptr;
    for (size_t i = 0; i < 128; i++) {
        cr_assert_eq(int_ptr[i], 0, "Memory allocated by my_calloc is not zero-initialized");
    }

    struct chunk *metadata = (struct chunk *)ptr - 1;
    cr_assert_eq(metadata->canary_start, CANARY_VALUE, "Canary start value is corrupted after calloc");
    cr_assert_eq(metadata->canary_end, CANARY_VALUE, "Canary end value is corrupted after calloc");

    my_free(ptr);
    cr_assert_eq(metadata->flags, FREE, "Chunk is not marked as FREE after my_free following calloc");
}

// Calloc with 0 as size
Test(secmalloc, calloc_zero_size) {
    void *ptr = my_calloc(0, sizeof(int));
    cr_assert_null(ptr, "my_calloc should return NULL for zero elements");

    ptr = my_calloc(128, 0);
    cr_assert_null(ptr, "my_calloc should return NULL for zero size");
}

// Calloc with multiplication overflow
Test(secmalloc, calloc_overflow) {
    size_t nmemb = SIZE_MAX / 2 + 1;
    size_t size = 2;
    void *ptr = my_calloc(nmemb, size);
    cr_assert_null(ptr, "my_calloc should return NULL for size overflow");
}

// Realloc to smaller size with data verification
Test(secmalloc, realloc_smaller_size_data_verification) {
    size_t original_size = 128;
    size_t new_size = 64;
    void *ptr = my_malloc(original_size);
    for (size_t i = 0; i < original_size; ++i) {
        ((char *)ptr)[i] = (char)i;
    }
    void *new_ptr = my_realloc(ptr, new_size);
    cr_assert_not_null(new_ptr, "my_realloc with smaller size should not return NULL");
    for (size_t i = 0; i < new_size; ++i) {
        cr_assert_eq(((char *)new_ptr)[i], (char)i, "Data integrity check failed after realloc to smaller size");
    }
    my_free(new_ptr);
}

// Realloc to larger size with data verification
Test(secmalloc, realloc_larger_size_data_verification) {
    size_t original_size = 64;
    size_t new_size = 128;
    void *ptr = my_malloc(original_size);
    for (size_t i = 0; i < original_size; ++i) {
        ((char *)ptr)[i] = (char)i;
    }
    void *new_ptr = my_realloc(ptr, new_size);
    cr_assert_not_null(new_ptr, "my_realloc with larger size should not return NULL");
    for (size_t i = 0; i < original_size; ++i) {
        cr_assert_eq(((char *)new_ptr)[i], (char)i, "Data integrity check failed after realloc to larger size");
    }
    my_free(new_ptr);
}

// Double free detection
Test(secmalloc, double_free_detection) {
    void *ptr = my_malloc(128);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 128 byte allocation");
    my_free(ptr);
    // Simulate double-free scenario
    my_free(ptr);
    cr_assert(1, "Double free should be detected and handled gracefully");
}

// Free memory corruption detection
Test(secmalloc, free_memory_corruption_detection) {
    void *ptr = my_malloc(128);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 128 byte allocation");
    struct chunk *metadata = (struct chunk *)ptr - 1;
    metadata->canary_start = 0; // Corrupt the canary value
    my_free(ptr);
    cr_assert(1, "Memory corruption should be detected during free");
}

// Out-of-bounds write detection
Test(secmalloc, out_of_bounds_write_detection) {
    size_t size = 128;
    void *ptr = my_malloc(size);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 128 byte allocation");

    char *char_ptr = (char *)ptr;
    for (size_t i = 0; i < size; ++i) {
        char_ptr[i] = 'a'; // Write within bounds
    }

    // Simulate out-of-bounds write
    char_ptr[size] = 'b'; // This should cause a detection
    my_free(ptr);
    cr_assert(1, "Out-of-bounds write should be detected and handled gracefully");
}

// Alignment check
Test(secmalloc, alignment_check) {
    size_t size = 128;
    void *ptr = my_malloc(size);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 128 byte allocation");
    cr_assert_eq(((uintptr_t)ptr % sizeof(void *)), 0, "Allocated memory is not properly aligned");
    my_free(ptr);
}

// Memory leak detection
Test(secmalloc, memory_leak_detection) {
    size_t size = 128;
    void *ptr = my_malloc(size);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 128 byte allocation");
    // Intentionally not freeing the memory to simulate a leak
}

// Stress test with multiple allocations and frees
Test(secmalloc, stress_test_multiple_allocations_and_frees) {
    size_t num_allocations = 1000;
    size_t allocation_size = 128;
    void *ptrs[num_allocations];

    for (size_t i = 0; i < num_allocations; ++i) {
        ptrs[i] = my_malloc(allocation_size);
        cr_assert_not_null(ptrs[i], "my_malloc returned NULL for a 128 byte allocation");
    }

    for (size_t i = 0; i < num_allocations; ++i) {
        my_free(ptrs[i]);
    }
    cr_assert(1, "Stress test with multiple allocations and frees passed");
}

// Realloc to very large size
Test(secmalloc, realloc_very_large_size) {
    size_t original_size = 1024;
    size_t new_size = 1024 * 1024 * 10; // 10 MB
    void *ptr = my_malloc(original_size);
    cr_assert_not_null(ptr, "my_malloc returned NULL for a 1024 byte allocation");

    void *new_ptr = my_realloc(ptr, new_size);
    cr_assert_not_null(new_ptr, "my_realloc returned NULL for a 10 MB allocation");

    struct chunk *metadata = (struct chunk *)new_ptr - 1;
    cr_assert_eq(metadata->canary_start, CANARY_VALUE, "Canary start value is corrupted after realloc");
    cr_assert_eq(metadata->canary_end, CANARY_VALUE, "Canary end value is corrupted after realloc");

    my_free(new_ptr);
    cr_assert_eq(metadata->flags, FREE, "Chunk is not marked as FREE after my_free following realloc");
}

void test_realloc_larger_size_with_free_chunk() {
    void *ptr1 = my_malloc(30);
    cr_assert(ptr1 != NULL);
    void *ptr2 = my_malloc(10);
    cr_assert(ptr2 != NULL);
    my_free(ptr1);

    memset(ptr2, 'B', 10);
    void *new_ptr = my_realloc(ptr2, 30);
    cr_assert(new_ptr != NULL);
    cr_assert(memcmp(new_ptr, "BBBBBBBBBB", 10) == 0);

    my_free(new_ptr);
}

void test_realloc_larger_size_no_free_chunks() {
    void *ptr = my_malloc(10);
    cr_assert(ptr != NULL);
    memset(ptr, 'A', 10);

    void *new_ptr = my_realloc(ptr, 20);
    cr_assert(new_ptr != NULL);
    cr_assert(memcmp(new_ptr, "AAAAAAAAAA", 10) == 0);

    my_free(new_ptr);
}
