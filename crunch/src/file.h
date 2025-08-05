#ifndef FILE_H_
#define FILE_H_

#include <stdint.h>
#include "arena.h"
#include "byte_array.h"


enum file_error_type_t {
    file_error_none,
    file_error_not_found,
    file_error_read,
    file_error_write
};


typedef struct file_error_t {
    uint32_t type;
} file_error_t;


typedef struct file_read_result_t {
    byte_view_t data;
    file_error_t error;
} file_read_result_t;


// Read a binary file into a memory buffer allocated from the given arena
file_read_result_t file_read(const char *filename, arena_t *arena);

// Write a binary file from the given memory buffer
file_error_t file_write(const char *filename, byte_view_t data);


#endif // ifndef FILE_H_
