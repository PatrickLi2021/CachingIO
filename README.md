Project 3: Caching I/O
======================

<!-- TODO: Fill this out. -->

## Design Overview:
The overall design of this project hinged around creating a cache that would drastically speed up the runtime of certain file operations (mainly `read`, `write`, `readc`, `seek`, and `writec`). When the program asks my I/O library to write a byte to a file, instead of immediately going to the disk, we put that byte (or bytes) into a cache, then write the entire cache to disk at some point. All of the functions that were implemented follow the public API that the stencil code provides. This approach significantly reduces the amount of system calls that need to be made and speeds up the runtime.

To begin, I design my file struct to incorporate 4 main pieces of metadata:
- A `size_t` value that represents the whether or not the cache has been changed
- A `file_ptr` that represents the index of where in the file we currently are
- A `cache_usage` variable that represents how much of the cache is currently being used
- A `cache_head` that stores where the cache is relative to the file
- A `file_size` variable that stores the size of the file

For the `io300_readc` function, I check to see if the file pointer is beyond the file boundaries. Afterwards, I check to see if the file pointer is beyond the cache. If it is and the cache contents have been changed, I flush to the file, increment my cache_head, and re-populate the cache. After that, I assign the char to return equal to the new item from the cache.

For the `io300_writec` function, I implement it similarly to the io300_readc function except I write the new character into the cache and return that.

For the `io300_read` function, I first check if the area to read in is located entirely within the cache. Otherwise, I seek to the file pointer and read the bytes in directly to the buffer.

For the `io300_write` function, I also make a check to see if the file pointer is beyond the cache boundaries. If not and the file pointer is within the cache, we memcpy directly from the buffer to the cache. If it’s beyond the boundaries, then we read directly from the file to the buffer.

## Collaborators:
Angela Li

## Extra Credit attempted:
N/A

## How long did it take to complete Caching I/O?
30 hours

<!-- Enter an approximate number of hours that you spent actively working on the project. -->
