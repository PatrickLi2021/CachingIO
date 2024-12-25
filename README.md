# CachingI/O

## Code: 
Available upon request (patrick_li@brown.edu or patrickli2021@gmail.com)

## Motivation:
Due to the design of Hard Disk Drives (HDD, "disks"), interacting with data on disk involves waiting on several slow physical mechanisms. Magnetic hard disks contain a spinning platter with a thin magnetic coating, writing 0's and 1's as tiny areas of magnetic North or South on the platter. These hard disks require requests to wait for the platters to spin and the metal arms to move to the right place where they can read data off the disk.

Caching alows us to sacrifice a small amount of data integrity in order to gain efficiency in performance. To do this, the computer temporarily holds the data you are using inside of its main memory (DRAM) instead of working directly with the disk. In other words, the main memory (DRAM) acts as a cache for the disk. The reason why caching is useful is because the alternative (system calls) are extremely expensive: they require the changing of the processor's privilege mode, a multitude of safety checks, and kernel code. These system calls go directly to disk.

## Overview:
**This project is a file I/O cache written in C designed to speed up the reading and writing of data to and from a filesystem.** The cache is written entirely in software and implements an input/output (I/O) library that supports operations such as `read()` (reading data from files), `write()` (writing data to files), or `seek()` (moving to a different offset within the file). The I/O library uses a cache to prefetch data and reduce the number of hard disk operations required.

When a byte is projected to be written to a file, instead of immediately going to disk, the byte(s) is stored in a cache, and the entire cache is written to disk at some point. This approach significantly reduces the amount of system calls that need to be made and speeds up runtime.

## Initialization and Metadata:
The creation of a cache in software required the introduction of several pieces of metadata:

- `size_t changed`: This is a flag representing whether the contents of the cache have changed (1 if they have changed, 0 if they have NOT changed)
- `size_t file_ptr`: This is a numerical index that represents where in the file our current position is
- `size_t cache_usage`: This is a value that represents how much of the cache is currently being used in bytes.
- `size_t cache_head`: This is a value that represents where the cache is stored relative to the file (the value it takes on can only be multiples of the cache size)
- `size_t file_size`: This value keeps track of the overall size of the file in bytes.

## Cache Operations:
The cache itself is represented with a `char*` type, as each `char` is a byte in size. There are a total of 6 operations that can be performed on the cache:

- `read()`: This function reads _n_ bytes from the file into a buffer, assuming that the buffer is large enough. Upon failure, -1 is returned. Upon success, the number of bytes that were placed into the provided buffer are returned.
- `write()`: This function writes _n_ bytes from the start of a provided buffer into the file, assuming that the buffer is large enough. On failure, -1 is returned. On success, the number of bytes that were writtent oteh file is returned.
- `readc()`: This function reads a single byte from the end of the file and returns it. It returns -1 on failure or if the end of the file has been reached.
- 'writec()': This function simply writes a single byte to the file. It returns the byte that was written upon success and -1 on failure.
- `seek()`: This function repositions the file offset to the given value, which causes subsequent reads and writes to take place from the new offset. It returns the resulting offset location as measured in bytes from the beginning of the file. On error, a value of -1 is returned.
- `open()`: This function allocates and initializes a new file struct that will wrap the given file path. In addition, it gives the file we are operating on a description to be used for debugging purposes
- `close()`: This function closes and cleans up the given file. Additionally, any allocated data is freed and any cached data that resides in RAM is flushed to disk.
- `flush()`: This function removes any in-RAM data from the cache and flushes it to disk.

## Results:
After successful implementation, the software cache was, on average, 2-3 times faster than C's standard file I/O implementation functions.
