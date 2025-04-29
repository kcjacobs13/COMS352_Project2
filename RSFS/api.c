/*
    implementation of API
*/

#include "def.h"

pthread_mutex_t mutex_for_fs_stat;//mutex used by RSFS_stat()


//initialize file system - should be called as the first thing before accessing this file system 
int RSFS_init(){
    char *debugTitle = "RSFS_init";

    //initialize data blocks
    for(int i=0; i<NUM_DBLOCKS; i++){
      void *block = malloc(BLOCK_SIZE); //a data block is allocated from memory
      if(block==NULL){
        printf("[%s] fails to init data_blocks\n", debugTitle);
        return -1;
      }
      data_blocks[i] = block;  
    } 

    //initialize bitmaps
    for(int i=0; i<NUM_DBLOCKS; i++) data_bitmap[i]=0;
    pthread_mutex_init(&data_bitmap_mutex,NULL);
    for(int i=0; i<NUM_INODES; i++) inode_bitmap[i]=0;
    pthread_mutex_init(&inode_bitmap_mutex,NULL);    

    //initialize inodes
    for(int i=0; i<NUM_INODES; i++){
        inodes[i].length=0;
    }
    pthread_mutex_init(&inodes_mutex,NULL); 

    //initialize open file table
    for(int i=0; i<NUM_OPEN_FILE; i++){
        struct open_file_entry entry=open_file_table[i];
        entry.used=0; //each entry is not used initially
        pthread_mutex_init(&entry.entry_mutex,NULL);
        entry.position=0;
        entry.access_flag=-1;
        // entry.ref=0;
        entry.inode_number=-1;
    }
    pthread_mutex_init(&open_file_table_mutex,NULL); 

    //initialize root inode
    root_inode_number = allocate_inode();
    if(root_inode_number<0){
        printf("[%s] fails to allocate root inode\n", debugTitle);
        return -1;
    }
    pthread_mutex_init(&root_dir_mutex,NULL); 
    
    
    //initialize mutex_for_fs_stat
    pthread_mutex_init(&mutex_for_fs_stat,NULL);

    //return 0 means success
    return 0;
}


//create file
//if file does not exist, create the file and return 0;
//if file_name already exists, return -1; 
//otherwise (other errors), return -2.
int RSFS_create(char file_name){

    //search root_dir for dir_entry matching provided file_name
    struct dir_entry *dir_entry = search_dir(file_name);

    if(dir_entry){//already exists
        printf("[create] file (%c) already exists.\n", file_name);
        return -1;
    }else{

        if(DEBUG) printf("[create] file (%c) does not exist.\n", file_name);

        //get a free inode 
        char inode_number = allocate_inode();
        if(inode_number<0){
            printf("[create] fail to allocate an inode.\n");
            return -2;
        } 
        if(DEBUG) printf("[create] allocate inode with number:%d.\n", inode_number);

        //insert (file_name, inode_number) to root directory entry
        dir_entry = insert_dir(file_name, inode_number);
        if(DEBUG) printf("[create] insert a dir_entry with file_name:%c.\n", dir_entry->name);
        
        return 0;
    }
}



//delete file
int RSFS_delete(char file_name){

    char debug_title[32] = "[RSFS_delete]";

    //to do: find the corresponding dir_entry
    struct dir_entry *dir_entry = search_dir(file_name);
    if(dir_entry==NULL){
        printf("%s director entry does not exist for file (%c)\n", 
            debug_title, file_name);
        return -1;
    }

    //to do: find the corresponding inode
    int inode_number = dir_entry->inode_number;
    if(inode_number<0 || inode_number>=NUM_INODES){
        printf("%s inode number (%d) is invalid.\n", 
            debug_title, inode_number);
        return -2;
    }
    struct inode *inode = &inodes[inode_number];

    //to do: find the data blocks, free them in data-bitmap
    pthread_mutex_lock(&data_bitmap_mutex);
    for(int i = 0; i <= inode->length/BLOCK_SIZE; i++){
        int block_number = inode->block[i];
        if(block_number>=0) data_bitmap[block_number]=0;    
    }
    pthread_mutex_unlock(&data_bitmap_mutex);

    //to do: free the inode in inode-bitmap
    pthread_mutex_lock(&inode_bitmap_mutex);
    inode_bitmap[inode_number]=0;
    pthread_mutex_unlock(&inode_bitmap_mutex);

    //to do: free the dir_entry
    int ret = delete_dir(file_name);
    
    return 0;
}


//print status of the file system
void RSFS_stat(){

    pthread_mutex_lock(&mutex_for_fs_stat);


    printf("\nCurrent status of the file system:\n\n %16s%10s%10s\n", "File Name", "Length", "iNode #");

    //list files
    for(int i=0; i<BLOCK_SIZE/sizeof(struct dir_entry); i++){
        struct dir_entry *dir_entry = (struct dir_entry *)root_data_block + i;
        if(dir_entry->name==0) continue;
        
        int inode_number = dir_entry->inode_number;
        struct inode *inode = &inodes[inode_number];
        
        printf("%16c%10d%10d\n", dir_entry->name, inode->length, inode_number);
    }
    
    
    //data blocks
    int db_used=0;
    for(int i=0; i<NUM_DBLOCKS; i++) db_used+=data_bitmap[i];
    printf("\nTotal Data Blocks: %4d,  Used: %d,  Unused: %d\n", NUM_DBLOCKS, db_used, NUM_DBLOCKS-db_used);

    //inodes
    int inodes_used=0;
    for(int i=0; i<NUM_INODES; i++) inodes_used+=inode_bitmap[i];
    printf("Total iNode Blocks: %3d,  Used: %d,  Unused: %d\n", NUM_INODES, inodes_used, NUM_INODES-inodes_used);

    //open files
    int of_num=0;
    for(int i=0; i<NUM_OPEN_FILE; i++) of_num+=open_file_table[i].used;
    printf("Total Opened Files: %3d\n\n", of_num);

    pthread_mutex_unlock(&mutex_for_fs_stat);
}





//------ implementation of the following functions is incomplete --------------------------------------------------------- 



//open a file with RSFS_RDONLY or RSFS_RDWR flags
//return a file descriptor if succeed; 
//otherwise return a negative integer value
int RSFS_open(char file_name, int access_flag) {
    if (access_flag != RSFS_RDONLY && access_flag != RSFS_RDWR) {
        printf("[RSFS_open] invalid access flag: %d\n", access_flag);
        return -1;
    }

    struct dir_entry *entry = search_dir(file_name);
    if (entry == NULL) {
        printf("[RSFS_open] fail to find file with name: %c\n", file_name);
        return -2;
    }

    int inode_number = entry->inode_number;
    if (inode_number < 0 || inode_number >= NUM_INODES) {
        printf("[RSFS_open] invalid inode number: %d\n", inode_number);
        return -3;
    }

    int fd = allocate_open_file_entry(access_flag, inode_number);
    if (fd < 0) {
        printf("[RSFS_open] fail to allocate open file entry.\n");
        return -4;
    }

    return fd;
}




// RSFS_append: Append data from buf to the end of the file.
// Locks the open file entry and inode during update. Allocates data blocks as needed.
// Returns number of bytes successfully appended
//append the content in buf to the end of the file of descriptor fd
//return the number of bytes actually appended to the file
int RSFS_append(int fd, void *buf, int size) {
    // Check the sanity of the arguments
    if (fd < 0 || fd >= NUM_OPEN_FILE || size <= 0) {
        printf("[RSFS_append] invalid fd: %d or size: %d\n", fd, size);
        return 0;
    }
    
    // Get the open file entry corresponding to fd
    struct open_file_entry *entry = &open_file_table[fd];
    
    // Lock the entry mutex to ensure exclusive access
    pthread_mutex_lock(&entry->entry_mutex);
    
    if (!entry->used) {
        printf("[RSFS_append] file descriptor not in use\n");
        pthread_mutex_unlock(&entry->entry_mutex);
        return 0;
    }
    
    // Check if the file is opened with RSFS_RDWR mode
    if (entry->access_flag != RSFS_RDWR) {
        printf("[RSFS_append] file not opened with RSFS_RDWR mode\n");
        pthread_mutex_unlock(&entry->entry_mutex);
        return 0;
    }
    
    // Get the inode
    int inode_number = entry->inode_number;
    struct inode *inode = &inodes[inode_number];
    
    // Lock the inode mutex to ensure exclusive access
    pthread_mutex_lock(&inodes_mutex);
    
    // Save the original file length
    int original_length = inode->length;
    
    // Calculate how many bytes to append
    int bytes_to_append = size;
    
    // Calculate starting block and offset
    int start_block = original_length / BLOCK_SIZE;
    int offset_in_block = original_length % BLOCK_SIZE;
    
    // Calculate how many bytes can be written to the first block
    int bytes_to_first_block = (offset_in_block == 0) ? 0 : (BLOCK_SIZE - offset_in_block);
    bytes_to_first_block = (bytes_to_first_block > bytes_to_append) ? bytes_to_append : bytes_to_first_block;
    
    // Write to the first block if needed
    if (bytes_to_first_block > 0) {
        // Allocate block if needed
        if (inode->block[start_block] < 0) {
            pthread_mutex_lock(&data_bitmap_mutex);
            inode->block[start_block] = allocate_data_block();
            pthread_mutex_unlock(&data_bitmap_mutex);
            
            if (inode->block[start_block] < 0) {
                printf("[RSFS_append] fail to allocate data block\n");
                pthread_mutex_unlock(&inodes_mutex);
                pthread_mutex_unlock(&entry->entry_mutex);
                return 0;
            }
        }
        
        // Copy data to the first block
        void *dst = (char*)data_blocks[inode->block[start_block]] + offset_in_block;
        void *src = buf;
        memcpy(dst, src, bytes_to_first_block);
        
        // Update file length and bytes left to append
        inode->length += bytes_to_first_block;
        bytes_to_append -= bytes_to_first_block;
        
        // Update start_block and buf pointer
        start_block++;
        buf = (char*)buf + bytes_to_first_block;
    }
    
    // Write to remaining blocks
    while (bytes_to_append > 0) {
        // Check if we've reached the maximum number of blocks
        if (start_block >= NUM_POINTERS) {
            printf("[RSFS_append] file is too large, cannot allocate more blocks\n");
            break;
        }
        
        // Allocate a new block
        pthread_mutex_lock(&data_bitmap_mutex);
        inode->block[start_block] = allocate_data_block();
        pthread_mutex_unlock(&data_bitmap_mutex);
        
        if (inode->block[start_block] < 0) {
            printf("[RSFS_append] fail to allocate data block\n");
            break;
        }
        
        // Calculate how many bytes to write to this block
        int bytes_to_block = (bytes_to_append > BLOCK_SIZE) ? BLOCK_SIZE : bytes_to_append;
        
        // Copy data to the block
        memcpy(data_blocks[inode->block[start_block]], buf, bytes_to_block);
        
        // Update file length and bytes left to append
        inode->length += bytes_to_block;
        bytes_to_append -= bytes_to_block;
        
        // Update start_block and buf pointer
        start_block++;
        buf = (char*)buf + bytes_to_block;
    }
    
    // Calculate how many bytes were actually appended
    int bytes_appended = inode->length - original_length;
    
    // Update the current position in open file entry
    entry->position = inode->length;
    
    // Unlock the mutexes
    pthread_mutex_unlock(&inodes_mutex);
    pthread_mutex_unlock(&entry->entry_mutex);
    
    // Return the number of bytes appended to the file
    return bytes_appended;
}





// RSFS_fseek: Update the file's current position (like lseek).
// If offset is valid, update position. Otherwise, leave position unchanged.
// Returns the new position or -1 on error.
int RSFS_fseek(int fd, int offset) {
    // Sanity test of fd
    if (fd < 0 || fd >= NUM_OPEN_FILE) {
        printf("[RSFS_fseek] invalid fd: %d\n", fd);
        return -1;
    }
    
    // Get the corresponding open file entry
    struct open_file_entry *entry = &open_file_table[fd];
    
    // Lock the entry mutex to ensure exclusive access
    pthread_mutex_lock(&entry->entry_mutex);
    
    // Check if the file entry is in use
    if (!entry->used) {
        printf("[RSFS_fseek] file descriptor not in use\n");
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    // Get the current position
    int current_pos = entry->position;
    
    // Get the inode and file length
    int inode_number = entry->inode_number;
    struct inode *inode = &inodes[inode_number];
    
    // Lock the inode mutex to ensure exclusive access
    pthread_mutex_lock(&inodes_mutex);
    
    int file_length = inode->length;
    
    // Check if argument offset is within 0...length
    if (offset < 0 || offset > file_length) {
        printf("[RSFS_fseek] offset %d is outside valid range 0...%d\n", offset, file_length);
        pthread_mutex_unlock(&inodes_mutex);
        pthread_mutex_unlock(&entry->entry_mutex);
        return current_pos; // Return current position without updating
    }
    
    if (inode_number < 0 || inode_number >= NUM_INODES) {
        printf("[RSFS_fseek] invalid inode number: %d\n", inode_number);
        pthread_mutex_unlock(&inodes_mutex);
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    // Update the current position to offset
    entry->position = offset;
    current_pos = offset;
    
    // Unlock mutexes
    pthread_mutex_unlock(&inodes_mutex);
    pthread_mutex_unlock(&entry->entry_mutex);
    
    // Return the new current position
    return current_pos;
}






// RSFS_read: Read data from the file starting at its current position.
// Reads up to `size` bytes or until end of file. Updates file position.
// Returns number of bytes read or -1 on error.
int RSFS_read(int fd, void *buf, int size) {
    // Sanity test of fd and size
    if (fd < 0 || fd >= NUM_OPEN_FILE) {
        printf("[RSFS_read] invalid fd: %d\n", fd);
        return -1;
    }
    
    if (size < 0) {
        printf("[RSFS_read] invalid size: %d\n", size);
        return -1;
    }
    
    // Get the corresponding open file entry
    struct open_file_entry *entry = &open_file_table[fd];
    
    // Lock the entry mutex to ensure exclusive access
    pthread_mutex_lock(&entry->entry_mutex);
    
    // Check if the file entry is in use
    if (!entry->used) {
        printf("[RSFS_read] file descriptor not in use\n");
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    if (entry->access_flag != RSFS_RDONLY) {
        printf("[RSFS_read] file not opened in read mode\n");
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    // Get the current position
    int current_pos = entry->position;
    
    // Get the inode number and validate it
    int inode_number = entry->inode_number;
    if (inode_number < 0 || inode_number >= NUM_INODES) {
        printf("[RSFS_read] invalid inode number: %d\n", inode_number);
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    // Get the corresponding inode
    struct inode *inode = &inodes[inode_number];
    
    // Lock the inode mutex to ensure exclusive access
    pthread_mutex_lock(&inodes_mutex);
    
    // If current position is at or beyond the end of file, return 0 bytes read
    if (current_pos >= inode->length) {
        pthread_mutex_unlock(&inodes_mutex);
        pthread_mutex_unlock(&entry->entry_mutex);
        return 0;
    }
    
    // Calculate how many bytes to read (can't read beyond end of file)
    int bytes_to_read = (current_pos + size > inode->length) ? 
                         (inode->length - current_pos) : size;
    int bytes_read = 0;
    
    // Calculate starting block and offset
    int start_block = current_pos / BLOCK_SIZE;
    int offset_in_block = current_pos % BLOCK_SIZE;
    
    // Calculate how many bytes to read from the first block
    int bytes_from_first_block = BLOCK_SIZE - offset_in_block;
    bytes_from_first_block = (bytes_from_first_block > bytes_to_read) ? 
                             bytes_to_read : bytes_from_first_block;
    
    // Read from the first block
    void *src = (char*)data_blocks[inode->block[start_block]] + offset_in_block;
    void *dst = buf;
    if (buf == NULL) {
        printf("[RSFS_read] invalid buffer\n");
        pthread_mutex_unlock(&inodes_mutex);
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    memcpy(dst, src, bytes_from_first_block);
    
    bytes_read += bytes_from_first_block;
    bytes_to_read -= bytes_from_first_block;
    
    // Move to the next block
    start_block++;
    dst = (char*)dst + bytes_from_first_block;
    
    // Read from remaining blocks
    while (bytes_to_read > 0 && start_block < NUM_POINTERS && 
           inode->block[start_block] >= 0) {
        
        // Calculate how many bytes to read from this block
        int bytes_from_block = (bytes_to_read > BLOCK_SIZE) ? 
                               BLOCK_SIZE : bytes_to_read;
        
        // Read from the block
        src = data_blocks[inode->block[start_block]];
        memcpy(dst, src, bytes_from_block);
        
        bytes_read += bytes_from_block;
        bytes_to_read -= bytes_from_block;
        
        // Move to the next block
        start_block++;
        dst = (char*)dst + bytes_from_block;
    }
    
    // Update the current position in open file entry
    entry->position += bytes_read;
    
    // Unlock the mutexes
    pthread_mutex_unlock(&inodes_mutex);
    pthread_mutex_unlock(&entry->entry_mutex);
    
    // Return the actual number of bytes read
    return bytes_read;
}


// RSFS_close: Closes the file corresponding to the given file descriptor.
// Frees the open file table entry. Returns 0 on success, -1 on error.
int RSFS_close(int fd) {
    // Sanity test of fd
    if (fd < 0 || fd >= NUM_OPEN_FILE) {
        printf("[RSFS_close] invalid fd: %d\n", fd);
        return -1;
    }
    
    // Get the corresponding open file entry
    struct open_file_entry *entry = &open_file_table[fd];
    
    // Lock the entry mutex to ensure exclusive access
    pthread_mutex_lock(&entry->entry_mutex);
    
    // Check if the file entry is in use
    if (!entry->used) {
        printf("[RSFS_close] file descriptor not in use\n");
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    // Get the corresponding inode number
    int inode_number = entry->inode_number;
    if (inode_number < 0 || inode_number >= NUM_INODES) {
        printf("[RSFS_close] invalid inode number: %d\n", inode_number);
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    // Check if the access flag is valid
    if (entry->access_flag != RSFS_RDONLY && entry->access_flag != RSFS_RDWR) {
        printf("[RSFS_close] invalid access flag: %d\n", entry->access_flag);
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }
    
    // Release this open file entry in the open file table
    entry->used = 0;
    entry->inode_number = -1;
    entry->position = 0;
    entry->access_flag = -1;
    
    // Unlock the entry mutex
    pthread_mutex_unlock(&entry->entry_mutex);
    
    return 0;
}



// RSFS_write: Write data to the file starting at its current position.
// Overwrites existing data from the position and truncates the rest.
// Returns number of bytes written or -1 on error.
int RSFS_write(int fd, void *buf, int size) {
    // Sanity check
    if (fd < 0 || fd >= NUM_OPEN_FILE || buf == NULL || size <= 0) {
        printf("[RSFS_write] invalid fd, buf, or size\n");
        return -1;
    }

    struct open_file_entry *entry = &open_file_table[fd];
    
    // Lock the entry mutex to ensure exclusive access
    pthread_mutex_lock(&entry->entry_mutex);

    if (!entry->used || entry->access_flag != RSFS_RDWR) {
        printf("[RSFS_write] file not open for writing\n");
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }

    int inode_number = entry->inode_number;
    if (inode_number < 0 || inode_number >= NUM_INODES) {
        printf("[RSFS_write] invalid inode number: %d\n", inode_number);
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }

    // Lock the inode mutex
    pthread_mutex_lock(&inodes_mutex);
    struct inode *inode = &inodes[inode_number];

    int position = entry->position;
    int file_length = inode->length;

    // If writing in the middle, free blocks beyond current position
    if (position < file_length) {
        int start_block = (position + size) / BLOCK_SIZE;
        for (int i = start_block; i < NUM_POINTERS; i++) {
            if (inode->block[i] >= 0) {
                free_data_block(inode->block[i]);
                inode->block[i] = -1;
            }
        }
    }

    // Now write the new content
    int bytes_written = 0;
    int bytes_to_write = size;
    int start_block = position / BLOCK_SIZE;
    int offset_in_block = position % BLOCK_SIZE;

    char *cbuf = (char *)buf;

    if (buf == NULL) {
        printf("[RSFS_write] invalid buffer\n");
        pthread_mutex_unlock(&inodes_mutex);
        pthread_mutex_unlock(&entry->entry_mutex);
        return -1;
    }

    while (bytes_written < size && start_block < NUM_POINTERS) {
        // Allocate block if necessary
        if (inode->block[start_block] < 0) {
            inode->block[start_block] = allocate_data_block();
            if (inode->block[start_block] < 0) {
                printf("[RSFS_write] fail to allocate data block\n");
                break;
            }
        }

        void *dst = (char *)data_blocks[inode->block[start_block]] + offset_in_block;
        int writable = BLOCK_SIZE - offset_in_block;
        int chunk = (bytes_to_write < writable) ? bytes_to_write : writable;

        memcpy(dst, cbuf, chunk);

        bytes_written += chunk;
        bytes_to_write -= chunk;
        cbuf += chunk;

        start_block++;
        offset_in_block = 0; // after first block, always 0 offset
    }

    // Update inode length and open file entry position
    inode->length = position + bytes_written;
    entry->position = position + bytes_written;

    pthread_mutex_unlock(&inodes_mutex);
    pthread_mutex_unlock(&entry->entry_mutex);

    return bytes_written;
}
