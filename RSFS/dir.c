/*
    allocation of global variable root_dir (root directory); 
    routines for directory management
*/


#include "def.h"

//global variable
pthread_mutex_t root_dir_mutex;
struct inode *root_inode = NULL;
void *root_data_block = NULL;



//helper function: search for dir entry matching provided file_name
struct dir_entry *search_dir_internal(char file_name){

    //get the inode for root directory if not assigned yet
    if(root_inode == NULL){
        root_inode = &inodes[root_inode_number];

        int root_data_block_number = allocate_data_block();
        if(root_data_block_number<0){
            printf("[search_dir_internal] fail to get root_data_block_number.\n");
            return NULL;
        }
        root_data_block = data_blocks[root_data_block_number];
        root_inode->block[0]=root_data_block;
        printf("[search_dir_internal] got root_data_block_number = %d\n", root_data_block_number);
    } 

    //get the data block for root directory if not assigned to variable root_data_block yet
    if(root_data_block == NULL){
        int root_data_block_number = root_inode->block[0];
        root_data_block = data_blocks[root_data_block_number];
    }

    //search file_name in the entries 
    for(int i=0; i<BLOCK_SIZE/sizeof(struct dir_entry); i++){
        struct dir_entry *dir_entry = (struct dir_entry *)root_data_block + i;
        if(dir_entry->name == file_name) return dir_entry;     
    }
    
    return NULL;

}


//search for the dir_entry for provided file_name
struct dir_entry *search_dir(char file_name){    

    pthread_mutex_lock(&root_dir_mutex);

    struct dir_entry *dir_entry = search_dir_internal(file_name);
    
    pthread_mutex_unlock(&root_dir_mutex);

    return dir_entry;
}


//insert an entry with provided (file_name, inode_number) and return it;
//if such entry exists already, return it directly
struct dir_entry *insert_dir(char file_name, char inode_number){
    
    pthread_mutex_lock(&root_dir_mutex);


    //search for the entry
    struct dir_entry *dir_entry = search_dir_internal(file_name);
    
    if(!dir_entry && root_inode->length<BLOCK_SIZE/sizeof(struct dir_entry)){//if not found and there is space to add an entry
        
        //find an empty entry
        dir_entry = search_dir_internal(0); //find an entry where name = 0 or '\0'

        //construct a new dir_entry
        if(dir_entry==NULL){
            printf("[insert_dir] fail to allocate a space for dir_entry.\n");
            pthread_mutex_unlock(&root_dir_mutex);
            return NULL;
        }
        dir_entry->name = file_name;
        dir_entry->inode_number = inode_number;
        
        //update the inode
        root_inode->length += 1;
    } 

    pthread_mutex_unlock(&root_dir_mutex);

    return dir_entry;
}

//delete the entry matching provided file_name if it exists;
//return 0 if succeed (found and deleted) or -1 if errs
int delete_dir(char file_name){

    pthread_mutex_lock(&root_dir_mutex);

    int ret = -1;

    //search for the matching dir_entry
    struct dir_entry *dir_entry = search_dir_internal(file_name); 

    //if found, delete it
    if(dir_entry){
        
        //mark this entry as not used (empty)
        dir_entry->name = 0;
        dir_entry->inode_number = 0;

        //update the inode
        root_inode->length -= 1;

        ret = 0;
    }

    pthread_mutex_unlock(&root_dir_mutex);

    return ret;
}

