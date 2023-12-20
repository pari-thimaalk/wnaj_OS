#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "kbd.h"
#include "term.h"
#include "filesystem.h"
#include "types.h"

#define PASS 1
#define FAIL 0
#define lky 58

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

int zero_test(){
	TEST_HEADER;
	int result = PASS;
	int i = 9;
	int j = 0;
	return i/j;
	return result;
}

// test any exception, change hex number from 0x00-0x13 to check
int exception_test(){
	TEST_HEADER;
	asm("int $0x03");
	return FAIL;
}

// system call test
int syscall_handler_test() {
    TEST_HEADER;
    __asm__("int $0x80");
    return FAIL;
}

//deref null test
int deref_null_test() {
	TEST_HEADER;
	int result = PASS;
	int* i = NULL;
	return *i;
	return result;
}

//deref referenceable memory locations test
int deref_referenceable_locations_test(){
	int result = PASS;
	char* i = (char *) 0xb8000;
	clear();
	printf("\nthe start of video memory is %x", (unsigned char) *i);
	char* random_vidmem_location = (char *) 0xb8020;
	printf("\nrandom location of video memory is %x", (unsigned char) *random_vidmem_location);
	char* end_of_vidmem_location = (char *) 0xb8fff;
	printf("\nthe end of vidmem is %x", (unsigned char) *end_of_vidmem_location);
	char* start_of_kernel_mem = (char *) 0x400000;
	printf("\nthe start of kernel memory is %x", (unsigned char) *start_of_kernel_mem);
	char* random_kernel_mem_loc = (char *) 0x500000;
	printf("\nrandom location in kernel memory is %x", (unsigned char) *random_kernel_mem_loc);
	char* end_of_kernel_mem = (char *) 0x7fffff;
	printf("\nend of kernel memory is %x\n", (unsigned char) *end_of_kernel_mem);
	return result;
}

//dereferencing one location before vidmem
int deref_one_before_vidmem_test() {
	int result = PASS;
	char* one_before_vidmem = (char *) 0xb7fff;
	printf("one befor vidmem (if this passes, i will genuinely kms) is %x", (unsigned char) *one_before_vidmem);
	return result;
}

//derefencing one location after vidmem
int deref_one_after_vidmem_test() {
	int result = PASS;
	char* one_after_vidmem = (char *) 0xb9000;
	printf("one after vidmem (if this passes, i will genuinely kms) is %x", (unsigned char) *one_after_vidmem);
	return result;
}

//derefencing one location before kernel mem
int deref_one_before_kernel_test() {
	int result = PASS;
	char* one_before_kernel = (char *) 0x3fffff;
	printf("one before kernel (if this passes, i will genuinely kms) is %x", (unsigned char) *one_before_kernel);
	return result;
}

//deref one after kernel mem
int deref_one_after_kernel_test() {
	int result = PASS;
	char* one_after_kernel = (char *) 0x800000;
	printf("one after kernel (if this passes, i will genuinely kms) is %x", (unsigned char) *one_after_kernel);
	return result;
}
// add more tests here

/* Checkpoint 2 tests */
// Dir Read test: prints out the contents of the directory and all the file information
// int test_dir_read() {
// 	int result = PASS;
// 	char buf[800];
// 	char* dir_name = ".";
// 	clear();
// 	dir_open((const uint8_t*)dir_name);
// 	dir_read((uint32_t)dir_name, buf, 800);
// 	dir_close((int32_t)dir_name);
// 	return result;
// }

// // File read test: prints out the contents of frame0.txt, if it opens properly
// int test_file_read_text(){
// 	int result = PASS;
// 	int i,return_val;
// 	clear();
	
// 	//187 is the file size of frame0.txt, taken from test_dir_read
// 	char buf[187];
// 	char* file_name = "frame0.txt";
// 	file_open((const uint8_t*) file_name);
// 	return_val = file_read((uint8_t*) file_name, buf, 187);
// 	if(return_val == -1){result = FAIL; return result;}
// 	file_close((int32_t) file_name);
// 	for(i = 0; i < 187; i++){
// 		if(buf[i] != 0){putc(buf[i]);}	
// 	}
// 	puts("\n");
// 	return result;
// }

// // File read test: prints out the contents of frame3.txt, if it opens properly
// // it shouldnt, and will throw an error
// int test_file_invalid_read_text(){
// 	int result = PASS;
// 	int i,return_val;
// 	clear();
	
// 	//using a small buffer size because this file doesn't exist, will throw an error anyway
// 	char buf[10];
// 	char* file_name = "frame3.txt";
// 	file_open((const uint8_t*) file_name);
// 	return_val = file_read((uint8_t*) file_name, buf, 10);
// 	file_close((int32_t) file_name);

// 	if(return_val == -1){result = FAIL; printf("invalid filename, does it exist?"); return result;}

// 	for(i = 0; i < 10; i++){
// 		if(buf[i] != 0){putc(buf[i]);}	
// 	}
// 	puts("\n");
// 	return result;
// }

// //prints the contents of the executable file grep
// //if correct, should see "elf" at the beginning and the magic string at the end
// int test_file_read_executable_grep() {
// 	int result = PASS;
// 	int i;
// 	int chars_thisline = 0;
// 	clear();
	
// 	//6149 is the file size of grep, taken from test_dir_read
// 	char buf[6149];
// 	char* file_name = "grep";
// 	file_open((const uint8_t*) file_name);
// 	file_read((uint8_t*) file_name, buf, 6149);
// 	file_close((int32_t) file_name);
// 	//for each line, we stop printing after 80 chars have been printed
// 	//as it will just go off screen, wrap around and overwrite the chars we already printed
// 	for(i = 0; i < 6149; i++){
// 		if(buf[i] != 0 && chars_thisline < 80){putc(buf[i]);chars_thisline++;}
// 		if(buf[i] == '\n'){chars_thisline = 0;puts("\n");}		
// 	}
// 	puts("\n");
// 	return result;
// }

// //this test will populate buffer with contents of fish executable and print it to screen
// int test_file_read_executable_fish() {
// 	int result = PASS;
// 	int i;
// 	int chars_thisline = 0;
// 	clear();
	
// 	//36164 is the file size of fish, taken from test_dir_read
// 	char buf[36164];
// 	char* file_name = "fish";
// 	file_open((const uint8_t*) file_name);
// 	file_read((uint8_t*) file_name, buf, 36164);
// 	file_close((int32_t) file_name);
// 	//for each line, we stop printing after 80 chars have been printed
// 	//as it will just go off screen, wrap around and overwrite the chars we already printed
// 	//we are only printing first 1000 non-null characters bc the file size is very big, if 
// 	//we print the whole thing it will just overwrite elf
// 	for(i = 0; i < 1000; i++){
// 		if(buf[i] != 0 && chars_thisline < 80){putc(buf[i]);chars_thisline++;}
// 		if(buf[i] == '\n'){chars_thisline = 0;puts("\n");}		
// 	}
// 	puts("\n");
// 	return result;
// }

// //this test will check that trying to read vltwvln.txt will return an error
// //because the filename is too long
// int test_file_read_long_text_shld_fail() {
//   int result = FAIL;
//   int i;
//   int chars_thisline = 0;
//   clear();

//   //buffer size of 5277 gotten from file size in dir read test
//   char buf[5277];
//   char* file_name = "verylargetextwithverylongname.txt";
//   file_open((const uint8_t*) file_name);
//   int read_vaild = file_read((uint8_t*) file_name, buf, 5277);
//   file_close((int32_t) file_name);
//   //it should throw an error here bc the read should not be valid. 
//   if (read_vaild == -1) {return PASS;}
//   //for each line, we stop printing after 80 chars have been printed
//   //as it will just go off screen, wrap around and overwrite the chars we already printed
//   for(i = 0; i < 5277; i++){
//     if(buf[i] != 0 && chars_thisline < 80){putc(buf[i]);chars_thisline++;}
//     if(buf[i] == '\n'){chars_thisline = 0;puts("\n");}    
//   }
//   puts("\n");
//   return result;
// }

// //this test will ensure that trying to read vltwvln.tx will not return an error
// //it should print out the contents of the file
// int test_file_read_long_text_shld_work() {
//   int result = PASS;
//   int i;
//   int chars_thisline = 0;
//   clear();

//   //buffer size of 5277 gotten from file size in dir read test
//   char buf[5277];
//   char* file_name = "verylargetextwithverylongname.tx";
//   file_open((const uint8_t*) file_name);
//   int read_vaild = file_read((uint8_t*) file_name, buf, 5277);
//   file_close((int32_t) file_name);
//   if (read_vaild == -1) {return FAIL;}
//   //for each line, we stop printing after 80 chars have been printed
//   //as it will just go off screen, wrap around and overwrite the chars we already printed
//   for(i = 0; i < 5277; i++){
//     if(buf[i] != 0 && chars_thisline < 80){putc(buf[i]);chars_thisline++;}
//     if(buf[i] == '\n'){chars_thisline = 0;puts("\n");}    
//   }
//   puts("\n");
//   return result;
// }

// //checks that the frequency of the rtc can be changed
// //tests rtc open and rtc write
// int rtc_test_freq()
// {
//   int i;
//   int result = PASS;
//   clear();
//   rtc_open(NULL);
//   int freq = 1;
//   int max_rtc = 20; //max number of cycles
//   int count_tracker = 0;
//   for(i = 1; i <= 10; i++){
//     freq = freq << 1; // rtc frequency
//     printf("Current frequency: %d  ||  ", freq);
//     rtc_write(0,&freq,0); // sets frequency
//     while(rtc_count < (max_rtc*i)){
//       if(count_tracker != rtc_count){
//         count_tracker++;
//         printf("%d", count_tracker % 10);
//       }
//     }
//     printf("\n");
//   }

//   printf("After calling rtc_open, frequency should reset back to 0: ");
//   rtc_open(NULL);
//   i = 0;
//   while(i < 10){
//     if(count_tracker != rtc_count){
//       count_tracker++;
//       i++;
//       printf("%d", count_tracker % 10);
//     }
//   }
//   printf("\n");
//   rtc_close(NULL);
//   return result;
// }

// int rtc_test_read(){
//   int i;
//   int result = PASS;
//   clear();
//   rtc_open(NULL);
  
//   printf("%d",1);
//   for(i = 0; i < 10; i++){
//     rtc_read(NULL,NULL,NULL);
//     printf("%d",1);
//   }
//   rtc_close(NULL);
//   return result;
// }

// int keyboard_test() {
// 	static uint8_t buf[INPUT_BUFFER_SIZE];
// 	terminal_init();
// 	// we *love* undefined behaviour! gotta get it on one line though, y'know what I mean?
// 	for (;;) terminal_write(1, (uint8_t*)buf, terminal_read(0, buf, INPUT_BUFFER_SIZE));
// 	// unreachable
// 	return FAIL;
// }

// int keyboard_test_nooverflow() {
// 	static uint8_t buf[INPUT_BUFFER_SIZE];
// 	terminal_init();
// 	(void)terminal_read(0, buf,  10);
// 	terminal_write(1, (uint8_t*)buf, 5);
// 	// unreachable
// 	return PASS;
// }
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */

void launch_tests(){
	//TEST_OUTPUT("idt_test", idt_test());
	TEST_OUTPUT("zero_test", zero_test());
	//TEST_OUTPUT("any exception", exception_test());
	//TEST_OUTPUT("system_call_test", syscall_handler_test());
	//TEST_OUTPUT("deref_null_test", deref_null_test());
	//TEST_OUTPUT("deref_start_of_vidmem_test", deref_referenceable_locations_test());
	//TEST_OUTPUT("deref one before vid mem test", deref_one_before_vidmem_test());
	//TEST_OUTPUT("deref one after vidmem ends test", deref_one_after_vidmem_test());
	//TEST_OUTPUT("deref one before kernel memory starts", deref_one_before_kernel_test());
	//TEST_OUTPUT("deref one after kernel memory starts", deref_one_after_kernel_test());
	//cp2 tests
	// TEST_OUTPUT("testing read dir", test_dir_read());
	// TEST_OUTPUT("test file read", test_file_read_text());
	// TEST_OUTPUT("test file read", test_file_invalid_read_text());
	// TEST_OUTPUT("test file read", test_file_read_executable_grep());
	// TEST_OUTPUT("test file read", test_file_read_executable_fish());
	// TEST_OUTPUT("test invalid file read long name", test_file_read_long_text_shld_fail());
	 // TEST_OUTPUT("test file read long name", test_file_read_long_text_shld_work());
	// TEST_OUTPUT("rtc_test", rtc_test_freq());
  	// TEST_OUTPUT("rtc_test", rtc_test_read());
	//TEST_OUTPUT("terminal driver test", keyboard_test());
	// TEST_OUTPUT("terminal driver test", keyboard_test_nooverflow());
	// launch your tests here
}

