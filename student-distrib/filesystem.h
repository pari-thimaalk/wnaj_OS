#ifndef filesys
#define filesys

#include "types.h" // contains definition for int32_t
#include "lib.h" //contains strncmp and printf

//for filesys
#define INODE_DATA_BLOCK_NUM 1023 //4kb/4 - 1 for length
#define DATA_BLK_SIZE 4096
#define MAX_FILENAME_LEN 32



//necessary structs for file operations, taken from Appendix A
typedef struct __attribute__((packed)) dir_entry {
    uint8_t file_name[32];
    uint32_t file_type;
    uint32_t inode_num;
    char reserved[24];
}dir_entry_t;

typedef struct __attribute__((packed)) boot_blk {
    uint32_t num_dir_entries;
    uint32_t num_inodes;
    uint32_t num_data_blks;
    char reserved[52];
    dir_entry_t dir_entries[63];
}boot_blk_t;

typedef struct __attribute__((packed)) data_blocks {
    char data[DATA_BLK_SIZE];
}data_block_t;

typedef struct __attribute__((packed)) inode {
    uint32_t length;
    uint32_t data_blocks[INODE_DATA_BLOCK_NUM];
}inode_t;

//variables we will use to track/access file system variables
boot_blk_t* boot_block_ptr;
inode_t* inode_array_ptr;
data_block_t* db_array_ptr;

//initializes the above variables by reading from bootblock
void init_filesys(uint8_t* start_address);

//generalized common interface that matches system call parameters
//visit .c file for function definitions
//return 0 for success, -1 for failure

//file operations
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);

//directory operations
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);

//three routines we need in the file system module
extern int32_t read_dentry_by_name(const uint8_t* fname, dir_entry_t* dentry);
extern int32_t read_dentry_by_index(uint32_t index, dir_entry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


//EVERYTHING BELOW IS migrated from syscall_function
#define MAX_FD_ARR_SIZE 8
#define MAX_ARG_LEN 1024 //a bit unsure abt this
#define MAX_PIDs 6
#define USER_MEM_INDEX 32 //entry for programs at 128MB
#define USER_ESP 0x8400000
#define USER_VIRT_MEM 0x8000000
#define USER_SCREEN 0x84b8000

// File ops table
// Contains function pointers to perform type specific actions for each operation
typedef struct file_ops_table {
    int32_t (*read) (int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write) (int32_t fd, const void* buf, int32_t nbytes);
    int32_t (*open) (const uint8_t* filename);
    int32_t (*close) (int32_t fd);
}file_ops_table_t;

// entry of a file descriptor array
typedef struct fd_arr_entry {
    file_ops_table_t file_ops;
    uint32_t inode;
    uint32_t file_pos;
    uint32_t flags;
}fd_arr_entry_t;

typedef struct pcb {
    int32_t process_id;
    int32_t parent_id;
    fd_arr_entry_t fd_arr[MAX_FD_ARR_SIZE];
    uint32_t saved_esp;
    uint32_t saved_ebp;
    uint32_t scheduled_esp;
    uint32_t scheduled_ebp;
    uint32_t active;
    int8_t args[MAX_ARG_LEN];
    void* sig_handlers[5];
    uint8_t pending_sig; //lowest 5 bits will correspond to signal bits
    uint8_t masked_sig;
} pcb_t;

pcb_t* this_pcb;

//dont delete the next line

#endif
