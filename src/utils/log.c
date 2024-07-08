#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "my_secmalloc.private.h"

extern FILE *execution_report;

/**
 * @brief Initializes the execution report file.
 * 
 * This function is automatically called when the library is loaded.
 * It attempts to open the file specified by the "MSM_OUTPUT" environment variable
 * for writing and assigns the file pointer to `execution_report`.
 * If the file cannot be opened, it writes an error message to `stderr` and exits.
 */
 __attribute__((constructor))
void init_execution_report() {
    char *report_filename = getenv("MSM_OUTPUT");
    if (report_filename != NULL) {
        if (execution_report == NULL) {
            execution_report = fopen(report_filename, "w");
            if (execution_report == NULL) {          
                write(STDERR_FILENO, "Failed to open execution report file\n", 38);
                exit(EXIT_FAILURE);
            }
        }
    }    
}

/**
 * @brief Closes the execution report file.
 * 
 * This function is automatically called when the library is unloaded.
 * It closes the `execution_report` file if it is not NULL.
 */
__attribute__((destructor))
void close_execution_report() {
    if (execution_report != NULL) {
        fclose(execution_report);
        execution_report = NULL;
    }
}

/**
 * @brief Retrieves code string based on the given value.
 * 
 * @param value The color code value (1 for Error, 2 for OK, 3 for Info).
 * @return const char* The corresponding code string.
 */
static const char* get_code(int value) {
    switch (value) {
        case 1:
            return "Error :";
            break;
        case 2: 
            return "OK : ";
            break;
        case 3:
            return "Info : ";
            break;
        default:
            return "";
            break;
    }
}

/**
 * @brief Logs an execution report message to the execution report file.
 * 
 * @param value Code value (1 for Error, 2 for OK, 3 for Info).
 * @param func_type The name of the function generating the log.
 * @param size The size associated with the log message.
 * @param addr The address associated with the log message.
 * 
 * This function formats and writes a log message to the `execution_report` file.
 * It includes the function type, size, and address in the log message. If the file
 * cannot write the expected number of bytes, it writes an error message to `stderr`.
 */
void log_execution_report(int value, const char *func_type, size_t size, void *addr) {
    char message_buffer[256];
    char full_message_buffer[512];
    const char *reset_code = "";

    snprintf(message_buffer, sizeof(message_buffer), "Function: %s, Size: %zu, Address: %p", func_type, size, addr);
    if (value == 3) {
        snprintf(message_buffer, sizeof(message_buffer), "Function: %s", func_type);
        snprintf(full_message_buffer, sizeof(full_message_buffer), "%s %s%s\n", get_code(value), message_buffer, reset_code);
    } else {
        snprintf(full_message_buffer, sizeof(full_message_buffer), "%s %s%s\n", get_code(value), message_buffer, reset_code);
    }

    if (execution_report != NULL) {
        int fd = fileno(execution_report);
        size_t written = write(fd, full_message_buffer, strlen(full_message_buffer));
        if (written != strlen(full_message_buffer)) {
            write(STDERR_FILENO, "write did not write the expected number of bytes\n", 50);
        }
        fsync(fd);
    }
}

/**
 * @brief Prints a debug message to the standard output.
 * 
 * @param value Code value (1 for Error, 2 for OK, 3 for Info).
 * @param format The format string for the message.
 * @param ... The variable arguments for the format string.
 * 
 * This function formats and writes a debug message to `stdout`.
 * It includes the color code and the formatted message in the output.
 */
void debug_print(int value, const char *format, ...) {
    char message_buffer[256];
    char full_message_buffer[512];
    const char *reset_code = "";

    va_list args;
    va_start(args, format);
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    va_end(args);
    snprintf(full_message_buffer, sizeof(full_message_buffer), " %s %s%s\n", get_code(value), message_buffer, reset_code);
    write(STDOUT_FILENO, full_message_buffer, strlen(full_message_buffer));
}
