Names: Keenan Jacobs

Description:
RSFS (Really Simple File System) is a simple file system implementation that provides basic file operations in a multi-threaded environment. The file system supports:

- File creation and deletion
- File reading and writing
- File seeking and appending
- Concurrent access with proper synchronization
- Basic file system statistics

Files Modified/Added:

1. api.c - Main implementation file containing:
   - RSFS_init() - File system initialization
   - RSFS_create() - File creation
   - RSFS_delete() - File deletion
   - RSFS_open() - File opening
   - RSFS_close() - File closing
   - RSFS_read() - File reading
   - RSFS_write() - File writing
   - RSFS_append() - File appending
   - RSFS_fseek() - File seeking
   - RSFS_stat() - File system statistics

