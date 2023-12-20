#include "syscall_functions.h"
#include "lib.h"
#include "x86_desc.h"
#include "paging.h"
#include "term.h"
#include "rtc.h"
#include "signals.h"

//hex values
#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000
#define FOUR_MB_PAGE 0x400000

//max 6 processes can be running concurrently
static int pid[MAX_PIDs] = {0, 0, 0, 0, 0, 0};
//initialize current pid to zero first, zero indexed
static int current_pid = 0;

//this is declared initially in filesystem.h, we want this to be a shared variable hence extern
extern pcb_t* this_pcb;

//stdin/out operations basically use terminal operations
//this type of initialization should work but it depends on the version of C compiler used
static file_ops_table_t stdin_ops = {&terminal_read, &dummy_write, &terminal_open, &terminal_close};
static file_ops_table_t stdout_ops = {&dummy_read, &terminal_write, &terminal_open, &terminal_close};
static file_ops_table_t dir_ops = {&dir_read, &dir_write, &dir_open, &dir_close};
static file_ops_table_t file_ops = {&file_read, &file_write, &file_open, &file_close};
static file_ops_table_t rtc_ops = {&rtc_read, &rtc_write, &rtc_open, &rtc_close};

//dummy functions to return error bc cannot write to stdin, and cannot read from stdout
int32_t dummy_read(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}
int32_t dummy_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

//function signatures in definition below
static bool is_valid_exec(uint8_t* file_name, dir_entry_t* direntryobject);
void setup_fd(fd_arr_entry_t *fd);

/* 
 * syscall_halt
 *  DESCRIPTION: Halts the program, and goes back to parent process. If no parent, reboots shell
 *  INPUTS: status msg
 *  OUTPUTS: None
 *  RETURN VALUE: None
 *  SIDE EFFECTS: will erase values from current pcb and remove pid from the array. returns control to parent process
 */
int32_t syscall_halt (uint8_t status) {
    cli();
    // int ret = status == 100 ? 256 : (uint32_t) status;
    
    //if no parent, just reboot shell. if parent exists, clear current pcb
    //then move to parent pcb at end of function
    this_pcb = (pcb_t*) (EIGHT_MB - (terms[scheduled_term].pid+1)*EIGHT_KB);

    if(this_pcb->parent_id == -1){
        pid[terms[scheduled_term].pid] = 0;
        sti();
        syscall_execute((uint8_t*)"shell");
    }
    current_pid = this_pcb->parent_id;
    terms[scheduled_term].pid = current_pid;
    pid[this_pcb->process_id] = 0;
    asm volatile ("\
        movl %0, %%ebp;  \
        movl %1, %%esp;  \
        movl %2, %%eax"  \
        :
        : "g"(this_pcb->saved_ebp), "g" (this_pcb->saved_esp), "g" (status)
    );

    this_pcb->active = 0;
    this_pcb->process_id = -1;

    //restore parent paging
    page_directory[USER_MEM_INDEX].addr_31_12 = (EIGHT_MB + (current_pid * FOUR_MB_PAGE)) >> ADDRESS_SHIFT_LEFT;
    flush_tlb();

    //close relevant FDs
    int i;
    for(i = 2; i < 8; i++){
        // this_pcb->fd_arr[i].inode = 0;
        
        // this_pcb->fd_arr[i].file_pos = 0;
        // this_pcb->fd_arr[i].flags = 0;
        syscall_close(i);
        this_pcb->fd_arr[i].file_ops.close = 0;
        this_pcb->fd_arr[i].file_ops.open = 0;
        this_pcb->fd_arr[i].file_ops.write = 0;
        this_pcb->fd_arr[i].file_ops.read = 0;
    }

    //point to parent pcb before returning   
    this_pcb = (pcb_t*) (EIGHT_MB - (current_pid+1)*EIGHT_KB);

    //set tss to parent
    tss.esp0 = EIGHT_MB - ((current_pid) * EIGHT_KB);
    tss.ss0 = KERNEL_DS;

    //i hope this works
    asm("JMP LUMETTA");
    return 0;
}


/* 
 * syscall_execute
 *  DESCRIPTION: parses the command string to find valid file, which is then executed
 *  INPUTS: command (may contain args)
 *  OUTPUTS: None
 *  RETURN VALUE: None
 *  SIDE EFFECTS: if we are given a valid executable file, will create a new pcb and assign it with values, then execute file
 */
int32_t syscall_execute (const uint8_t* command){
    cli();
    int i,j; //for loop

    /* parse arguments */
    uint32_t cmd_len = strlen((int8_t*)command);
	cmd_len = cmd_len > 32 ? 32 : cmd_len;
    uint32_t num_args = 0;
    //32+1, max filename length
    uint8_t file_name[33];
    uint8_t args[MAX_ARG_LEN];

    //first argument will be the (executable) file name
    for(i = 0; i < cmd_len; i++){
		if(command[i] == ' '){
			num_args++;
			break;
		} else if(command[i] == '\0'){
			break;
		}
	}
    strncpy((int8_t*)file_name,(int8_t*)command,i);
    file_name[i] = '\0';

    //check if there is an arg, if so store it in args
    uint32_t curr_arg_len = 0;
    if(num_args){
        for(j = 1; j < cmd_len+1; j++){
            if(command[i+j] == '\0'){
				break;
            }
        }
        strncpy((int8_t*)args,(int8_t*)(command+i+1),j);
        curr_arg_len = j;
        args[j] = '\0';
    }
    
    //check file validity
    //helper function to check if file is a valid executable
    dir_entry_t file_data;
    if(is_valid_exec(file_name,&file_data) == false) return -1;

    // find new pid
    int old_pid = terms[scheduled_term].pid;
    for (i = 0; i < MAX_PIDs; i++) {
        if (pid[i] == 0) {
            pid[i] = 1;
            current_pid = i;
            break;
        }
    }
    if (i == MAX_PIDs) return -1;

    /* set up paging */
    page_directory[USER_MEM_INDEX].addr_31_12 = (EIGHT_MB + (current_pid*FOUR_MB_PAGE)) >> ADDRESS_SHIFT_LEFT;
    flush_tlb();    

    //load file into memory using dir entry object
    int file_size = inode_array_ptr[file_data.inode_num].length;
    read_data(file_data.inode_num,0, (uint8_t*)0x08048000, file_size);

    //create pcb
    this_pcb = (pcb_t*) (EIGHT_MB - (current_pid+1)*EIGHT_KB);
    this_pcb->process_id = current_pid;
    terms[scheduled_term].pid = current_pid;
    //if the pid is 0,1,2 we are in boot process: parent -1
    if(current_pid >= 0 && current_pid <= 2){this_pcb->parent_id = -1;} //dumb issue. should fix the exiting fault
    else{this_pcb->parent_id = old_pid;}

    setup_fd(this_pcb->fd_arr);
    asm("movl %%ebp,%0" : "=r"(this_pcb->saved_ebp));
    asm("movl %%esp,%0" : "=r"(this_pcb->saved_esp));
    this_pcb->active = 1;
    memset(this_pcb->args,'\0',MAX_ARG_LEN);
    memcpy(this_pcb->args,args,curr_arg_len);

    /* prepare for context switch */
    tss.ss0 = KERNEL_DS; //storing segment selector for kernel stack
    tss.esp0 = EIGHT_MB - ((current_pid) * EIGHT_KB); //bottom of process block
    sti();

    /* push iret context onto stack */
    uint8_t entry_pt_buf[4];
    //buffer contains address
    read_data(file_data.inode_num,24, entry_pt_buf,4);
    int32_t* entry_pt = (int32_t*) entry_pt_buf;
    
    //pushing in order of DS, ESP, EFLAGS, CS, EIP as stated in slides
    asm volatile ("\
        pushl %0;  \
        pushl %1;  \
        pushfl;    \
        pushl %2;  \
        pushl %3;  \
        iret;"
        :
        : "g"(USER_DS), "g" (USER_ESP), "g" (USER_CS), "g" (*entry_pt)
    );

    asm("LUMETTA: "); 

    //iret and return   
    return 0;
}

/* 
 *  syscall_read
 *  DESCRIPTION: reads data of file specified by fd and populates buffer
 *  INPUTS: file descriptor number, buffer, number of bytes to copy over
 *  OUTPUTS: None
 *  RETURN VALUE: number of bytes copied over
 *  SIDE EFFECTS: will populate buffer with data of file
 */
int32_t syscall_read (int32_t fd, void* buf, int32_t nbytes){
    //ensure no bad arguments
    if(fd < 0 || fd >= MAX_FD_ARR_SIZE || buf == NULL || nbytes < 0 || this_pcb->fd_arr[fd].flags == 0){return -1;}
    return this_pcb->fd_arr[fd].file_ops.read(fd,buf,nbytes);
}

/* 
 *  syscall_write
 *  DESCRIPTION: reads from buffer and writes into the file
 *  INPUTS: file descriptor number, buffer, number of bytes to copy over
 *  OUTPUTS: writes nbytes into file
 *  RETURN VALUE: a certain value
 *  SIDE EFFECTS: writes to file
 */ 
int32_t syscall_write (int32_t fd, const void* buf, int32_t nbytes){
    //ensure no bad arguments
    if(fd < 0 || fd >= MAX_FD_ARR_SIZE || buf == NULL || nbytes < 0 || this_pcb->fd_arr[fd].flags == 0){return -1;}
    return this_pcb->fd_arr[fd].file_ops.write(fd,buf,nbytes);
}

/* 
 *  syscall_open
 *  DESCRIPTION: opens file specified by filename
 *  INPUTS: filename
 *  OUTPUTS: None
 *  RETURN VALUE: fd num
 *  SIDE EFFECTS: sets file pos to 0
 */
int32_t syscall_open (const uint8_t* filename){
    dir_entry_t file_metadata;
    //if file does not exist, return error
    if(read_dentry_by_name(filename,&file_metadata) == -1){return -1;}

    //check for vacant file descriptor entry - if none, return -1
    int fd_idx = 0;
    while(this_pcb->fd_arr[fd_idx].flags){fd_idx++; if(fd_idx == MAX_FD_ARR_SIZE){return -1;}}

    //assign fd entry with relevant values
    this_pcb->fd_arr[fd_idx].flags = 1;
    this_pcb->fd_arr[fd_idx].inode = file_metadata.inode_num;
    this_pcb->fd_arr[fd_idx].file_pos = 0;
    switch (file_metadata.file_type)
    {
    case 0: //rtc
        this_pcb->fd_arr[fd_idx].file_ops = rtc_ops;
        break;
    case 1: //directory
        this_pcb->fd_arr[fd_idx].file_ops = dir_ops;
        break;
    case 2:
        this_pcb->fd_arr[fd_idx].file_ops = file_ops;
        break;
    default:
        clear();
        printf("We have a strange file type %d\n",file_metadata.file_type);
        break;
    }

    if(this_pcb->fd_arr[fd_idx].file_ops.open(filename) < 0){return -1;}
    return fd_idx;
}

/* 
 *  syscall_close
 *  DESCRIPTION: closes file in fd arr
 *  INPUTS: fd num
 *  OUTPUTS: None
 *  RETURN VALUE: return value of close function
 *  SIDE EFFECTS: closes file in fd
 */
int32_t syscall_close (int32_t fd){
    //ensure no bad arguments
    if(fd < 2 || fd >= MAX_FD_ARR_SIZE || this_pcb->fd_arr[fd].flags == 0){return -1;}
    this_pcb->fd_arr[fd].flags = 0;
    this_pcb->fd_arr[fd].file_pos = 0;
    this_pcb->fd_arr[fd].inode = 0;
    return this_pcb->fd_arr[fd].file_ops.close(fd);
}

// (｢•-•)｢ ʷʱʸ?
/* 
 *  syscall_getargs
 *  DESCRIPTION: gets args specified by cmd
 *  INPUTS: buffer to copy into, nbytes to copy
 *  OUTPUTS: None
 *  RETURN VALUE: 0
 *  SIDE EFFECTS: passes args into buffer
 */
int32_t syscall_getargs(uint8_t* buf, int32_t nbytes){
    if(nbytes > MAX_ARG_LEN || (strlen(this_pcb->args) == 0 && nbytes > 0)){return -1;}
    memset(buf, 0, nbytes);
    memcpy(buf,this_pcb->args,nbytes);
    return 0;
}

/* 
 *  syscall_vidmap
 *  DESCRIPTION: map user virtual video address to physical video memory
 *  INPUTS: pointer to virtual user vid mem
 *  OUTPUTS: None
 *  RETURN VALUE: 0
 *  SIDE EFFECTS: stores physical video memory into user virtual address
 */
int32_t syscall_vidmap(uint8_t** screen_start){
    //check if mem location provided by caller is valid
    //this happens when we are in shell (first program), so shld be within 8-12MB region
    if((uint32_t)screen_start < USER_VIRT_MEM || (uint32_t)screen_start >= USER_VIRT_MEM+FOUR_MB_PAGE){return -1;}
    *screen_start = (uint8_t*) USER_SCREEN;
    return 0;
}
 
//specs require we are able to handle these two system calls, immediately return failure if we are not handling it
int32_t syscall_sethandler(int32_t signum, void* handler_address){
    if(signum > 4 || signum < 0){return -1;}
    if(handler_address){this_pcb->sig_handlers[signum] = handler_address;}
    else{this_pcb->sig_handlers[signum] = sig_handlers[signum];}     //default option
    return 0;
}

int32_t syscall_sigreturn(void){
    return -1;
}

/* 
 *  setup_fd
 *  DESCRIPTION: helper function to set up file descriptor array
 *  INPUTS: fd array
 *  OUTPUTS: None
 *  RETURN VALUE: none
 *  SIDE EFFECTS: populates fd array
 */
void setup_fd(fd_arr_entry_t *fd){
    int i;
    for(i = 0; i < 2; i++){
        fd[i].flags = 1;        //marked as in use
        fd[i].file_pos = 0;     //read from beginning of file
        fd[i].inode = 0;        
    }

    fd[0].file_ops = stdin_ops;
    fd[1].file_ops = stdout_ops;

    for(i = 2; i < MAX_FD_ARR_SIZE; i++){
        fd[i].flags = 0;
        fd[i].file_pos = 0;
        fd[i].inode = 0;
    }
}

/* 
 *  is_valid_exec
 *  DESCRIPTION: checks if executable is valid
 *  INPUTS: file name, dire entry
 *  OUTPUTS: None
 *  RETURN VALUE: 1 (if valid), 0 (else)
 *  SIDE EFFECTS: populates direntry with file metadata if file is valid
 */
//const char nexists[] = "executable file does not exist\n";
bool is_valid_exec(uint8_t* file_name, dir_entry_t* direntryobject){
    //searching if the file exists, and if it does, the first four bytes must be .elf
    int8_t buf[4]; // not null-terminated

    if (read_dentry_by_name(file_name, direntryobject) == -1) {
		// PRINTLN("executable file does not exist\n");
        return false;
    }
    read_data(direntryobject->inode_num, 0, (uint8_t*) buf, 4);
    return !strncmp(buf, "\177ELF", 4); //its 'DEL'ELF not .ELF ; 177 is DEL in Octal
}
