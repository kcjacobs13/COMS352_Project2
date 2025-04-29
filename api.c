int RSFS_append(int fd, void *buf, int size) {
    if (fd < 0 || fd >= NUM_OPEN_FILE || size <= 0) {
        return 0;
    }
    
    struct open_file_entry *entry = &open_file_table[fd];
    pthread_mutex_lock(&entry->entry_mutex);
    
    if (!entry->used) {
        pthread_mutex_unlock(&entry->entry_mutex);
        return 0;
    }
    
    if (entry->access_flag == RSFS_RDONLY) {
        pthread_mutex_unlock(&entry->entry_mutex);
        return 0;
    }
    
    int inode_number = entry->inode_number;
    struct inode *inode = &inodes[inode_number];
    pthread_mutex_lock(&inodes_mutex);
    
    int original_length = inode->length;
    int bytes_to_append = size;
    int start_block = original_length / BLOCK_SIZE;
    int offset_in_block = original_length % BLOCK_SIZE;
    
    int bytes_to_first_block;
    if (offset_in_block == 0) {
        bytes_to_first_block = (bytes_to_append > BLOCK_SIZE) ? BLOCK_SIZE : bytes_to_append;
    } else {
        bytes_to_first_block = (BLOCK_SIZE - offset_in_block);
        bytes_to_first_block = (bytes_to_first_block > bytes_to_append) ? bytes_to_append : bytes_to_first_block;
    }
    
    if (bytes_to_first_block > 0) {
        int new_block = allocate_data_block();
        inode->block[start_block] = new_block;
        
        if (inode->block[start_block] < 0) {
            pthread_mutex_unlock(&inodes_mutex);
            pthread_mutex_unlock(&entry->entry_mutex);
            return 0;
        }
        
        void *dst = (char*)data_blocks[inode->block[start_block]] + offset_in_block;
        void *src = buf;
        memcpy(dst, src, bytes_to_first_block);
        
        inode->length += bytes_to_first_block;
        bytes_to_append -= bytes_to_first_block;
        
        start_block++;
        buf = (char*)buf + bytes_to_first_block;
    }
    
    while (bytes_to_append > 0) {
        if (start_block >= NUM_POINTERS) {
            break;
        }
        
        int new_block = allocate_data_block();
        inode->block[start_block] = new_block;
        
        if (inode->block[start_block] < 0) {
            break;
        }
        
        int bytes_to_block = (bytes_to_append > BLOCK_SIZE) ? BLOCK_SIZE : bytes_to_append;
        memcpy(data_blocks[inode->block[start_block]], buf, bytes_to_block);
        
        inode->length += bytes_to_block;
        bytes_to_append -= bytes_to_block;
        
        start_block++;
        buf = (char*)buf + bytes_to_block;
    }
    
    int bytes_appended = inode->length - original_length;
    entry->position = inode->length;
    
    pthread_mutex_unlock(&inodes_mutex);
    pthread_mutex_unlock(&entry->entry_mutex);
    
    return bytes_appended;
} 