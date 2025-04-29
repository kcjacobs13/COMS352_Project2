Names: Keenan Jacobs

Description:
RSFS (Really Simple File System) is a simple file system implementation that provides basic file operations in a multi-threaded environment. The file system supports:

- File creation and deletion
- File reading and writing
- File seeking and appending
- Concurrent access with proper synchronization (reader-writer locks)
- Basic file system statistics

Everything is working perfectly and I completed both the mandatory and advanced section on my own

Files Modified/Added:

1. api.c - Main implementation file containing:
   - RSFS_open() - File opening (with reader/writer synchronization)
   - RSFS_close() - File closing (with reader/writer synchronization)
   - RSFS_read() - File reading
   - RSFS_write() - File writing
   - RSFS_append() - File appending
   - RSFS_fseek() - File seeking
     
2. def.h - Modified inode structure to support concurrent access

3. inode.c - Modified to initialize reader-writer synchronization primitives
