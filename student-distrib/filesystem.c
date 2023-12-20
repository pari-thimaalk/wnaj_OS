#include "filesystem.h"
//#include "syscall_functions.h"

//pass in mod->start
/* 
 * init_filesys
 *  DESCRIPTION: Initializes the filesystem variables by reading from bootblock
 *  INPUTS: starting address of the bootblock
 *  OUTPUTS: None
 *  RETURN VALUE: None
 *  SIDE EFFECTS: will set up our local variables with pointers to the bootblock, inodes and data blocks
 */
void init_filesys(uint8_t* start_address){
    //check whether pointer to a byte would work better
    boot_block_ptr = (boot_blk_t *) start_address;
    inode_array_ptr = (inode_t*)(start_address + sizeof(boot_blk_t));
    db_array_ptr = (data_block_t*)(start_address + sizeof(boot_blk_t) + (boot_block_ptr->num_inodes) * sizeof(inode_t));
}

/* 
 * file_read
 *  DESCRIPTION: populates buffer with nbytes of content from the given file
 *  INPUTS: fd num, buffer, and number of bytes to transfer over
 *  OUTPUTS: an integer value indicating success/failure of operation
 *  RETURN VALUE: -1 (failure), or n bytes transferred (success)
 *  SIDE EFFECTS: will populate buffer with file contents
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    //fd should only be between 2 and 7 bc 1 and 2 is reserved for stdin/stdout
    if (fd < 2 || fd >= MAX_FD_ARR_SIZE){return -1;}
    uint32_t read_bytes = read_data(this_pcb->fd_arr[fd].inode, this_pcb->fd_arr[fd].file_pos, (uint8_t*) buf, nbytes);
    int eof_ret_val = inode_array_ptr[this_pcb -> fd_arr[fd].inode].length - this_pcb->fd_arr[fd].file_pos;
    if (read_bytes == 0) {
        this_pcb->fd_arr[fd].file_pos += eof_ret_val;
        return eof_ret_val;
    } 
    else if(read_bytes > 0){this_pcb->fd_arr[fd].file_pos += nbytes;}
    return read_bytes;
}


//read-only file system - just return failure, do not do anything
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}
//initialise temp structures, return 0
int32_t file_open(const uint8_t* filename){
    return 0;
}
//undo changes made in open function
int32_t file_close(int32_t fd){
    return 0;
}


/* 
 * dir_read
 *  DESCRIPTION: populates buffer with the file name of a single directory entry
 *  INPUTS: fd entry of current program, buffer in which names should be filled, nbytes to trf
 *  OUTPUTS: an integer value indicating success/failure of operation
 *  RETURN VALUE: 0 (success)
 *  SIDE EFFECTS: will populate buffer with directory contents
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
    //filter out bad args
    if(fd < 0 || fd >= MAX_FD_ARR_SIZE || buf == NULL || nbytes < 0){return -1;}
    //we have reached end of directory
    if(boot_block_ptr->num_dir_entries<=this_pcb->fd_arr[fd].file_pos){return 0;}

    //get file name using its index, copy over up to 32 chars into buff
    dir_entry_t file_metadata;
    int length = read_dentry_by_index(this_pcb->fd_arr[fd].file_pos,&file_metadata);
    if(length <0){return -1;}
    int name_length = strlen((int8_t*) file_metadata.file_name);
    name_length = name_length > MAX_FILENAME_LEN ? MAX_FILENAME_LEN : name_length;
    strncpy((int8_t*)buf,(int8_t*)file_metadata.file_name,name_length);
    //((uint8_t*)buf)[length] = '\0';
    this_pcb->fd_arr[fd].file_pos ++;
    return name_length;
}

/*
(hello stalker)
8===============D
its called football not soccer
big up spig man

o o
 |
 |
 |
 |
 |
 pinocchio
 read the damn code
*/
// int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
//     int i,j;
//     char* file_name;
//     char name_buff[33];
//     name_buff[32] = '\0';
//     for (i = 0; i < boot_block_ptr -> num_dir_entries; i++) {
//         //get file name and add necessary offsets
//         file_name = (char*)(boot_block_ptr -> dir_entries[i]).file_name;
//         int file_name_length = strlen((int8_t*) file_name);
//         if(file_name_length > 32){strncpy(name_buff,file_name,32);}
//         else{
//             strcpy(name_buff,file_name);
//             for(j = file_name_length; j < 32; j++){name_buff[j] = ' ';}
//         }
//         // printf("file name: %s, file type: %d, file size: %d", name_buff, (boot_block_ptr -> dir_entries[i]).file_type, (inode_array_ptr[(boot_block_ptr -> dir_entries[i]).inode_num].length));
//         // printf("\n");
//     }
//     return 0;
// }

//read-only file system - just return failure, do not do anything
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

//open directory file, directory 0
int32_t dir_open(const uint8_t* filename){
    
    return 0;
}

//close directory file, return 0
int32_t dir_close(int32_t fd){
    return 0;
}

/* 
 * read_dentry_by_name
 *  DESCRIPTION: finds the dentry object with file name we are looking for, and populates our dentry object with it
 *  INPUTS: file name, empty dentry object
 *  OUTPUTS: an integer value indicating success/failure of operation
 *  RETURN VALUE: 0 (success), -1 (failure)
 *  SIDE EFFECTS: will populate our dentry object with file information, if successful
 */
int32_t read_dentry_by_name(const uint8_t* fname, dir_entry_t* dentry){
    int i; //counter for for loops
    int num_entries = boot_block_ptr -> num_dir_entries;
    //check if fname is not longer than 32 chars
    if (strlen((int8_t*) fname) > MAX_FILENAME_LEN) return -1;
    for (i = 0; i < num_entries; i++) {
        int file_found = strncmp((int8_t*) fname, (int8_t*) (boot_block_ptr -> dir_entries[i].file_name), MAX_FILENAME_LEN);
        if (file_found == 0) {
            read_dentry_by_index(i, dentry);
            return 0;
        }
    }
    return -1;
}

/* 
 * read_dentry_by_index
 *  DESCRIPTION: points our dentry object to that of the file we found
 *  INPUTS: index, empty dentry object
 *  OUTPUTS: an integer value indicating success/failure of operation
 *  RETURN VALUE: 0 (success), -1 (failure)
 *  SIDE EFFECTS: will populate our dentry object with file information, if successful
 */
int32_t read_dentry_by_index(uint32_t index, dir_entry_t* dentry){
    //check if index is valid first
    if (index < boot_block_ptr -> num_dir_entries) {
        *dentry = (boot_block_ptr -> dir_entries[index]);
        return 0;
    }
    return -1;
}

/* 
 * read_data
 *  DESCRIPTION: reads the contents of the file we are trying to open and stores it in buffer
 *  INPUTS: inode, offset, buffer, num bytes
 *  OUTPUTS: an integer value indicating success/failure of operation
 *  RETURN VALUE: 0 (EOF), -1 (failure), n bytes successfully transferred
 *  SIDE EFFECTS: will populate our buffer with file data, if successful
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    //to avoid confusion: db_index is the index of data block inside inode
    //curr_db_number is the data block number between 0-D-1 inside whole filesystem
    inode_t* curr_inode;
    uint32_t total_data_blocks, inode_db_idx, file_size, buf_counter, curr_db_number;
    data_block_t* curr_datablock;

    //invalid inode number, error
    if(inode > (boot_block_ptr->num_inodes)-1){return -1;}
    curr_inode = &inode_array_ptr[inode];   //using pointer bc of debugging ptsd with idt in cp1
    
    //if the offset is greater than the file length, eof
    //im making a guess that offset is in bytes
    file_size = curr_inode->length;
    if(offset > file_size){return 0;}
    inode_db_idx = offset / DATA_BLK_SIZE;

    //get the datablock number which will be used for error checking as we traverse
    //through the inode datablocks, until buffer is filled
    total_data_blocks = boot_block_ptr->num_data_blks;
    buf_counter = 0;
    curr_db_number = curr_inode->data_blocks[inode_db_idx];
    if(curr_db_number > total_data_blocks - 1){return -1;}

    curr_datablock = &db_array_ptr[curr_inode->data_blocks[inode_db_idx]]; //again, using ptr bc of debugging ptsd
    while(buf_counter < length && offset < file_size){
        //check if we have reached the beginning of new block (and not at start) in which case
        //go to inode, find next data block number, load into curr_datablock
        //trying to avoid loading curr_datablock every iteration bc its an expensive process
        if(buf_counter != 0 && (offset % DATA_BLK_SIZE == 0)){
            inode_db_idx ++;
            curr_db_number = curr_inode->data_blocks[inode_db_idx];
            if(curr_db_number > total_data_blocks - 1){return -1;}  //before accessing datablocks array, check if index is legit
            curr_datablock = &db_array_ptr[curr_inode->data_blocks[inode_db_idx]];
        }
        buf[buf_counter] = curr_datablock->data[(offset % DATA_BLK_SIZE)];
        offset ++;
        buf_counter ++;
    }

    //check if buf counter has been filled with required number of bytes. if so,
    //return length, otherwise we know eof so return 0
    if(buf_counter == length){return length;}else{return 0;}
}
