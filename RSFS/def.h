/*
    definitions of constants, data structures, and API
*/


#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>


//global constants
#define NUM_INODES 8 //total number of inodes
#define NUM_DBLOCKS 64 //total number of data blocks
#define NUM_POINTERS 8 //total number of (direct) pointers for each inode; i.e., each file can have at most this number of data blocks
#define BLOCK_SIZE 32 //size of each data block (unit: byte)
#define NUM_OPEN_FILE 8 //maximum number of files that can be open at a time in the whole system

#define RSFS_RDONLY 0 //a value for access_flag in RSFS_open(): file is open for read only
#define RSFS_RDWR 1 //a value for access_flag in RSFS_open(): file is open for read and write  

#define RSFS_SEEK_SET 0 //a value for whence in RSFS_fseek()
#define RSFS_SEEK_CUR 1 //a value for whence in RSFS_fseek()
#define RSFS_SEEK_END 2 //a value for whence in RSFS_fseek()

#define DEBUG 0 //1-enable debug, 0-disable debug prints

//directory entry
struct dir_entry{
    char name; //file name must be at most three characters
    char inode_number; //inode_number identifying the inode of the file
};
extern int root_inode_number; //initial value
extern pthread_mutex_t root_dir_mutex;


//inode data structure: inodes implemented in inode.c
struct inode {
    char block[NUM_POINTERS]; //(direct) pointers to data blocks; note: value<0 means the block is not used
    int length; //length of the file of the inode
};
extern struct inode inodes[NUM_INODES]; //global array of inodes
extern pthread_mutex_t inodes_mutex; //mutex to guard mutually-exclusive access of inodes

//inode bitmap: implemented in inode.c
extern int inode_bitmap[NUM_INODES]; //global inode bitmap
extern pthread_mutex_t inode_bitmap_mutex; //mutex to guard mutually-exclusive access of the bitmap

//data bitmap: implemented in data_block.c
extern int data_bitmap[NUM_DBLOCKS]; //global data-block bitmap
extern pthread_mutex_t data_bitmap_mutex; //mutex to guard mutually-exclusive access of the bitmap

//data blocks: implemented in data_block.c
extern void *data_blocks[NUM_DBLOCKS]; //global array of pointers to the data blocks
extern void *root_data_block;

//open file entry: open_file_table implemented in open_file_table.c 
struct open_file_entry{
    char used; //0-the entry is not in use, or 1- it is in use (already allocated)
    pthread_mutex_t entry_mutex; //mutex to guard M.E. access to this entry
    int inode_number;
    int position; //current position of the file
    char access_flag; //RSFS_RDONLY or RSFS_RDWR - how the file can be accessed by the process/thread openning this file
};
extern struct open_file_entry open_file_table[NUM_OPEN_FILE]; //global table (array) of open_file_entries 
extern pthread_mutex_t open_file_table_mutex; //mutex to guard M.E. access to the table


//routines for directory management: implemented in dir.c
struct dir_entry *search_dir(char file_name); //get the dir_entry for file_name
struct dir_entry *insert_dir(char file_name, char inode_number); //create a dir_entry for file_name and its inode_number; the dir_entry is returned
int delete_dir(char file_name); //delete the dir_entry for the given file name from the global directory


//routines for inode management: implemented in inode.c
int allocate_inode(); //allocate an unused inode, and the inode_number is returned
void free_inode(int inode_number); //free (release) an inode


//routines for data block management: implemented in data_block.c
int allocate_data_block(); //allocate an unused data block, and the block_number is returned
void free_data_block(int block_number); //free (release) a data block


//routines for open file entry management: implemented in open_file_table.c
int allocate_open_file_entry(int access_flag, int inode_number); 
        //allocate_open_file_entry: allocate an open file entry and initialize it with provided parameters
void free_open_file_entry(int fd); //free (release) an open file entry



//api - basic: already implemented in api.c
int RSFS_init(); //initialize thesystem (provided)
void RSFS_stat(); //print the file's stat (provided)

//api - basic: required to be implemented in api.c
int RSFS_create(char file_name); //create an empty file and return the file handler (i.e., index of the entry in open_file_table)
int RSFS_open(char file_name, int access_flag); //open an existing file and return the file handler
int RSFS_append(int fd, void *buf, int size); //append to the end of the file, and return the actual number of bytes appended
int RSFS_fseek(int fd, int offset); //change the current location of the file
int RSFS_read(int fd, void *buf, int size); //read from file, and return the actual number of bytes read
int RSFS_close(int fd); //close the file

//api - advanced: to be implemented in api.c
int RSFS_write(int fd, void *buf, int size);
int RSFS_cut(int fd, int size); 
int RSFS_delete(char file_name); //delete the file with the provided file_name




