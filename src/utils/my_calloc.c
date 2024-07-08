#define _GNU_SOURCE

#include <string.h>
#include "my_secmalloc.private.h"
/**
 * @brief Allocates memory for an array of nmemb elements of size bytes each and initializes all bytes to zero.
 *
 * @param nmemb Number of elements to allocate (input).
 * @param size Size of each element in bytes (input).
 * @return Pointer to the allocated memory, or NULL if the allocation fails or if there is a multiplication overflow.
 *
 * This function combines the functionality of malloc and memset, ensuring that all allocated memory is zeroed out.
 * It logs the operation and any potential errors, such as multiplication overflow or allocation failure.
 */
void *my_calloc(size_t nmemb, size_t size) {
    log_execution_report(3,"my_calloc called", size, NULL);
    if (size != 0 && nmemb > SIZE_MAX / size) {
        log_execution_report(1,"my_calloc error: Multiplication overflow",size,NULL);
        return NULL;
    }
    size_t total_size = nmemb * size;
    void *ptr = my_malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    } else {
        log_execution_report(1,"my_calloc error: Allocation failed",size,ptr);
    }
    log_execution_report(2,"my_calloc allocated and zeroed memory", size,ptr);
    return ptr;
}
