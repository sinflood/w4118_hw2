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
/*maybe this should be asm_arm not asm>*/


/*NOTE: if fail_num is zero this means we are not in a fault injection session*/

asmlinkage int sys_fail(int n)
{
    if(n>0)
    {
        struct task_struct *task = current();
        task->fail_num = n;
        task->num_sys_calls = 0;
    }
    else if(n == 0)
    {
        /*end the injection session if there is one running*/
        if(task->fail_num != 0)
        {
            task->fail_num = 0; /*when fail_num == zero no fails should happen*/
            task->num_sys_calls = 0;
        }
        else
        {
            return -1; //TODO return invalid argument error code
        }
    }
    return -1; //TODO return invalid argument error code   
}
long should_fail(void)
{
    struct task_struct *task = current();
    task->num_sys_calls++;
    if(task->fail_num != 0 && task->fail_num == task->num_sys_calls)
    {
        /*if fail_num is 0 there is no fail injection, if it's and we reached our fail num, we should fail.*/
	task->fail_num = 0;
	task->num_sys_calls = 0;
        return 1;
    }   
    return 0; /*don't fail*/
}

long fail_syscall(void)
{
    return -1; /*TODO return a correct error code*/
    /*not sure if this is all that is needed here*/
}

