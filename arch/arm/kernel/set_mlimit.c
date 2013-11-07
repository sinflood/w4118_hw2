#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/cred.h>
#include <linux/errno.h>

asmlinkage int sys_set_mlimit(uid_t uid, long mem_max)
{


  /* I see two ways to get the current process's user_struct
   * First using get_current_user in linux/cred.h
   * Second using find_user in kernel/user.c then free_uid
  */
  struct user_struct *user = get_current_user();

  if(user->uid == uid){
    user->mem_max = mem_max;
    return 0;
  }

  return EINVAL;
}
