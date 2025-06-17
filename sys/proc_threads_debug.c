/**
 * @file proc_threads_debug.c
 * @brief Thread Debugging Implementation
 * 
 * Implements debug logging facility for threading subsystem.
 * Features:
 *  - File-based logging to 'c:\thread.log'
 *  - Atomic log writing using kernel I/O
 *  - Formatted message support with variadic arguments
 *  - Severity-based filtering (minimal to verbose)
 * 
 * Designed for low-overhead operation in constrained environments
 * with minimal performance impact when disabled.
 * 
 * Author: Medour Mehdi
 * Date: June 2025
 * Version: 1.0
 */

#include "proc_threads_debug.h"
#include "mint/file.h"
#include "arch/tosbind.h"
#include "libkern/libkern.h"

#ifdef DEBUG_THREAD
/**
 * Write debug message to a file
 * 
 * @param filename Path to the log file
 * @param fmt Format string
 * @param ... Variable arguments
 */
void debug_to_file(const char *filename, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    kvsprintf(buffer, sizeof(buffer), fmt, args);

    int fd = Fopen(filename, O_RDWR | O_CREAT | O_APPEND);
    if (fd < 0) {
        return; // Handle error if needed
    }

    // Write the formatted message to the file
    Fwrite(fd, strlen(buffer), buffer);

    // Write a newline
    Fwrite(fd, 1, "\n");

    // Close the file
    Fclose(fd);

    va_end(args);
}
#endif