#include <linux/export.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/current.h> 

asmlinkage int sched_setlimit(pid_t pid, int limit)
{
    struct task_struct *p = find_task_by_vpid(pid);
    
    p->se.intervalLimit = limit*1000; //adjust into nanoseconds

    return 0;
}
