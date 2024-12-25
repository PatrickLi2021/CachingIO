#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include "../io300.h"

/*
    student.c
    Fill in the following stencils
*/

/*
    When starting, you might want to change this for testing on small files.
*/
#ifndef CACHE_SIZE
#define CACHE_SIZE 4096
#endif

#if(CACHE_SIZE < 4)
#error "internal cache size should not be below 4."
#error "if you changed this during testing, that is fine."
#error "when handing in, make sure it is reset to the provided value"
#error "if this is not done, the autograder will not run"
#endif

/*
   This macro enables/disables the dbg() function. Use it to silence your
   debugging info.
   Use the dbg() function instead of printf debugging if you don't want to
   hunt down 30 printfs when you want to hand in
*/
#define DEBUG_PRINT 0
#define DEBUG_STATISTICS 1

struct io300_file {
    /* read,write,seek all take a file descriptor as a parameter */
    int fd;
    /* this will serve as our cache */
    char *cache;

    // Flag representing if the cache contents have changed (1 if yes, 0 if no)
    size_t changed;
    // Index representing where in the file we currently are
    size_t file_ptr;
    // Represents the current usage of the cache (can be smaller than 4096)
    size_t cache_usage;
    // Stores where the cache is in relative to the file (can only be multiples of CACHE_SIZE)
    size_t cache_head;
    // Stores the size of the file
    size_t file_size;

    /* Used for debugging, keep track of which io300_file is which */
    char *description;
    /* To tell if we are getting the performance we are expecting */
    struct io300_statistics {
        int read_calls;
        int write_calls;
        int seeks;
    } stats;
};

/*
    Assert the properties that you would like your file to have at all times.
    Call this function frequently (like at the beginning of each function) to
    catch logical errors early on in development.
*/
static void check_invariants(struct io300_file *f) {
    assert(f != NULL);
    // TODO: Add more invariants
}

/*
    Wrapper around printf that provides information about the
    given file. You can silence this function with the DEBUG_PRINT macro.
*/
static void dbg(struct io300_file *f, char *fmt, ...) {
    (void)f; (void)fmt;
#if(DEBUG_PRINT == 1)
    static char buff[300];
    size_t const size = sizeof(buff);
    int n = snprintf(
        buff,
        size,
        // TODO: Add the fields you want to print when debugging
        "{desc:%s, } -- ",
        f->description
    );
    int const bytes_left = size - n;
    va_list args;
    va_start(args, fmt);
    vsnprintf(&buff[n], bytes_left, fmt, args);
    va_end(args);
    printf("%s", buff);
#endif
}

struct io300_file *io300_open(const char *const path, char *description) {
    if (path == NULL) {
        fprintf(stderr, "error: null file path\n");
        return NULL;
    }

    int const fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        fprintf(stderr, "error: could not open file: `%s`: %s\n", path, strerror(errno));
        return NULL;
    }

    struct io300_file *const ret = malloc(sizeof(*ret));
    if (ret == NULL) {
        fprintf(stderr, "error: could not allocate io300_file\n");
        return NULL;
    }

    ret->fd = fd;
    ret->cache = malloc(CACHE_SIZE);
    if (ret->cache == NULL) {
        fprintf(stderr, "error: could not allocate file cache\n");
        close(ret->fd);
        free(ret);
        return NULL;
    }
    ret->description = description;
    
    // Initializes other file metadata
    ret->changed = 0;
    ret->file_ptr = 0;
    ret->stats.read_calls = 0;
    ret->stats.write_calls = 0;
    ret->stats.write_calls = 0;
    ret->cache_head = 0;
    ret->file_size = io300_filesize(ret);
    ret->stats.seeks = 0;

    // Populate the cache
    int num_bytes_read = pread(ret->fd, ret->cache, CACHE_SIZE, 0);
    ret->stats.read_calls += 1;
    ret->cache_usage = num_bytes_read;

    check_invariants(ret);
    dbg(ret, "Just finished initializing file from path: %s\n", path);
    return ret;
}

int io300_seek(struct io300_file *const f, off_t const pos) {
    check_invariants(f);
    f->stats.seeks++;
    // Calculate the distance between the position to seek to and the current file pointer
    size_t distance_difference = pos - f->file_ptr;
    
    // Set the file pointer equal to that distance
    f->file_ptr += distance_difference;

    if ((f->file_ptr / CACHE_SIZE) * CACHE_SIZE != f->cache_head) {
        // Check if the cache contents have been changed
        if (f->changed == 1) {
            io300_flush(f);
        }
        f->cache_head = (f->file_ptr / CACHE_SIZE) * CACHE_SIZE;
        pread(f->fd, f->cache, CACHE_SIZE, f->cache_head);
    }
    return pos;
}

int io300_close(struct io300_file *const f) {
    check_invariants(f);

#if(DEBUG_STATISTICS == 1)
    printf("stats: {desc: %s, read_calls: %d, write_calls: %d, seeks: %d}\n",
            f->description, f->stats.read_calls, f->stats.write_calls, f->stats.seeks);
#endif
    // TODO: Implement this

    // Flush the contents of the cache to the file by calling write()
    if (f->changed == 1) {
        io300_flush(f);
    }
    close(f->fd);
    free(f->cache);
    free(f);
    return 0;
}

off_t io300_filesize(struct io300_file *const f) {
    check_invariants(f);
    struct stat s;
    int const r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode)) {
        return s.st_size;
    } else {
        return -1;
    }
}

int io300_readc(struct io300_file *const f) {
    check_invariants(f);

    // Check to see if the file pointer is beyond the file boundary
    if (f->file_ptr >= f->file_size) {
        return -1;
    }
    // Check to see if the file pointer is beyond the cache
    if (f->cache_head + CACHE_SIZE <= f->file_ptr) {
        // Flush if the contents have been changed
        if (f->changed == 1) {
            io300_flush(f);
        }
        f->cache_head += CACHE_SIZE;
        // Re-populate the cache from the file
        int new_bytes = pread(f->fd, f->cache, CACHE_SIZE, f->cache_head);
        if (new_bytes == -1) {
            return -1;
        }
        f->changed = 0;
    }
    // Assign the char equal to new item from the cache
    char c = f->cache[f->file_ptr % CACHE_SIZE];
    f->file_ptr += 1;
    return (unsigned char)c;
}

int io300_writec(struct io300_file *f, int ch) {
    check_invariants(f);

    // Check to see if the file pointer is beyond the boundary of the file
    if (f->file_ptr >= f->file_size) {
        f->file_size += 1;
    }
    // Check to see if the file pointer is beyond the cache
    if (f->file_ptr >= f->cache_head + CACHE_SIZE) {
        // Flush if the cache has been changed
        if (f->changed == 1) {
            io300_flush(f);
        }
        f->cache_head += CACHE_SIZE;
        // Read new bytes into the cache
        int new_bytes = pread(f->fd, f->cache, CACHE_SIZE, f->cache_head);
        if (new_bytes == -1) {
            return -1;
        }
    }
    // Write the character into the cache
    f->cache[f->file_ptr % CACHE_SIZE] = ch;
    f->file_ptr += 1;
    f->changed = 1;
    return (unsigned char)ch;
}

ssize_t io300_read(struct io300_file *const f, char *const buff, size_t const sz) {
    
    // The number of bytes we read into the buffer is sz
    int bytes_to_read_in = sz;
    // If we're close to the end of the file and can only read in <sz bytes, we set that equal to 
    // bytes_to_read_in
    if (f->file_size - f->file_ptr < sz) {
        bytes_to_read_in = f->file_size - f->file_ptr;
    } 
    // If the area to read in is located ENTIRELY within the cache, we memcpy directly to the cache
    if (f->cache_head + CACHE_SIZE >= f->file_ptr + bytes_to_read_in) {
        memcpy(buff, f->cache + (f->file_ptr % CACHE_SIZE), bytes_to_read_in);
        f->file_ptr += bytes_to_read_in;
    }
    // Otherwise, we seek to the file pointer and read the bytes in directly from the buffer
    else {
        lseek(f->fd, f->file_ptr, SEEK_SET);
        read(f->fd, buff, bytes_to_read_in);
        f->file_ptr += bytes_to_read_in;
        // Reset cache_head
        f->cache_head = (f->file_ptr / CACHE_SIZE) * CACHE_SIZE;
        // Flush if contents have been changed
        if (f->changed == 1) {
            io300_flush(f);
        }
        // Read in new cache contents
        pread(f->fd, f->cache, CACHE_SIZE, f->cache_head);
    }
    return bytes_to_read_in;
}

ssize_t io300_write(struct io300_file *const f, const char *buff, size_t const sz) {
    check_invariants(f);
    
    // If our file pointer is beyond the cache boundaries, then we want to increase it by sz (i.e. we are 
    // writing bytes past the current bounds of the file)
    if (f->file_ptr >= f->file_size) {
        f->file_size += sz;
    }
    // If the file pointer is within the cache, we memcpy directly from the buffer to the cache
    if (f->cache_head + CACHE_SIZE >= f->file_ptr + sz) {
        memcpy(f->cache + (f->file_ptr % CACHE_SIZE), buff, sz);
        if (f->changed == 0) {
            f->cache_usage = f->file_ptr % CACHE_SIZE;
        }
        f->file_ptr += sz;
        f->changed = 1;
    }
    // If the file pointer is beyond the cache, then we want to read directly to the file from the buffer
    else {
        if (f->changed == 1) {
            io300_flush(f);
        }
        pwrite(f->fd, buff, sz, f->file_ptr);
        f->file_ptr += sz;
        f->cache_head = (f->file_ptr / CACHE_SIZE) * CACHE_SIZE;
        int used_bytes = read(f->fd, f->cache, CACHE_SIZE);
        f->changed = 0;
        f->cache_usage = sz;
    }
    return sz;
}

int io300_flush(struct io300_file *const f) {
    check_invariants(f);
    if (f->cache_head + CACHE_SIZE >= f->file_size) {
        pwrite(f->fd, f->cache + f->cache_usage, f->file_ptr - f->cache_head - f->cache_usage, f->cache_head + f->cache_usage);
    }
    else {
        pwrite(f->fd, f->cache + f->cache_usage, CACHE_SIZE - f->cache_usage, f->cache_head + f->cache_usage);
    }
}