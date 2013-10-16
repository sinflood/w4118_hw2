#include <linux/netlock.h>
#include <linux/syscalls.h>
#include <asm/current.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
//declare and initialize at compile time a network struct 


//TODO uncoment to the line below.
//static NETWORK_STRUCT(network);

static struct network_struct network;

asmlinkage long sys_netlock_acquire (netlock_t type)
{
	
	struct task_struct *task = get_current();
 	int ret = 0;
	
	//check that a task doesn't request a second lock
	if( task->type_lock != NET_LOCK_N )
	{
		return -1;
	}
	
	//check correct netlock_t type request
	if(  type!=NET_LOCK_E && type!=NET_LOCK_R )
	{
		return -1;
	}
	
 	/* This is the return value of netlock_acquire. 
    	0 is success, -1 is error */

 	// Case: Writer
 	if ( type == NET_LOCK_E )
 	{
 		//define a new wait entry
 		DEFINE_WAIT(writers_wait);
	
 		spin_lock_irq(&(network.lock));
 		network.num_waiting_writers++;
 		spin_unlock_irq(&(network.lock));
	
 		for(;;) 
 		{
 			/*
 			*  prepare_to_wait_exclusive() does 3 things:
 			*  First it sets wait->flags to exclusive (wake_up() wakes up only one exclusive task)
 			*  Second it calls __add_wait_queue_tail that insert the new wait entry to the end of the queue
 			*  This is necessary because we want to implement a queue : "If multiple processes request 
 			*  exclusive locks, they should be granted locks in the order of their request"  
 			*  Third it sets the task state to TASK_INTERRUPTIBLE (But the task is not sleeping yet! it will be  
 			*  only after schedule() is called)
 		    */
 			prepare_to_wait_exclusive(&(network.writers_queue), &writers_wait, TASK_INTERRUPTIBLE);

 			//acquire a lock to check network conditions
 			spin_lock_irq(&(network.lock));
 			/*
 			* "When a process requests an exclusive lock, it must wait until processes 
 			*  currently holding regular or exclusive locks release the locks"
 			*/
 			if(network.num_current_writers == 0 && network.num_current_readers == 0)
 			{
 				break;
 			}
 			/*
 			*  The network conditions weren't satisfied. We put the task to sleep.
 			*  In the absence of signal, we call schedule() to schedule another task.
 			*  Before, we release the lock.
 			*  When woken up, the task executes the continue statement that takes it back to the beginning
 			*  of the for loop. 
 			*/
 			if(!signal_pending(task))
 			{
 				spin_unlock_irq(&(network.lock));
 				schedule();
 				continue;
 			}		
 			// this code is executed if we get a signal. we return an error and break out of the loop
 			ret = -ERESTARTSYS;
 			break;
 		}
 		//a task exits the for loop holding a network lock

 		finish_wait(&network.writers_queue, &writers_wait);
 		network.num_waiting_writers--;
	
 		if (ret == 0)
 		{
 			//the writer is now running
 			network.num_current_writers++;
 			spin_unlock_irq(&(network.lock));
 			//set the task as writer
 			task->type_lock = NET_LOCK_E;
 		}
 		else 
 	    {
 			//netlock_acquire fails. release the network lock.
 			spin_unlock_irq(&(network.lock));
 		}
 	}
	
 	// Case: Reader
	 else
 	{
 		//define a new wait entry
 		DEFINE_WAIT(readers_wait);
	
 		spin_lock_irq(&(network.lock));
 		network.num_waiting_readers++;
 		spin_unlock_irq(&(network.lock));
	
 		for(;;) 
 		{
 			/*
 			*  For readers, we don't need prepare_to_wait_exclusive.
 			*  All waiting readers are woken up together. So no need to implement a queue: __add_wait_queue()
 			*  is enough and tasks don't need to be flagged as EXCLUSIVE.
 			*/
 			prepare_to_wait(&(network.readers_queue), &readers_wait, TASK_INTERRUPTIBLE);
		
 			//acquire a lock to check network conditions
 			spin_lock_irq(&(network.lock));
 			/* 
 			*  "The calls to acquire the lock in regular mode should succeed 
 			*  immediately as long as no process is holding an exclusive (write) lock
 			*  or is waiting for an exclusive lock."
 			*/
 			if(network.num_current_writers == 0 && network.num_waiting_writers == 0)
 			{
 				break;
 			}
 			if(!signal_pending(task))
 			{
 				spin_unlock_irq(&(network.lock));
 				schedule();
 				continue;
 			}		
 			// this code is executed if we get a signal. we return an error and break out of the loop
 			ret = -ERESTARTSYS;
 			break;
 		}
 		//a task exits the for loop holding a network lock

 		finish_wait(&network.readers_queue, &readers_wait);
 		network.num_waiting_readers--;
	
 		if (ret == 0)
 		{
 			//the writer is now running
 			network.num_current_readers++;
 			spin_unlock_irq(&(network.lock));
 			//set the task as writer
 			task->type_lock = NET_LOCK_R;
 		}
 		else 
 		{
 			//netlock_acquire fails. release the network lock.
 			spin_unlock_irq(&(network.lock));
 		}
	
 	}
	
	return ret;
}

asmlinkage long sys_netlock_release (void)
{
	struct task_struct *task = get_current();
	
	//check if the task owns a lock
	if ( task->type_lock != NET_LOCK_R && task->type_lock != NET_LOCK_E)
	{
		return -1;
	}
	
	spin_lock_irq(&(network.lock));
	
	/* Case: Reader
	*  We prioritize writers over readers. Since many readers can 
	*  be granted the lock in the same time, we must check that no other 
	*  readers have the lock before waking up writers
	*/
	if ( task->type_lock == NET_LOCK_R )
	{
		//first decrease the number of readers
		network.num_current_readers--;
		//if no other current readers but waiting writers, wake up the writers
		if ( network.num_current_readers == 0 && network.num_waiting_writers != 0 )
		{
			spin_unlock_irq(&(network.lock));
			/*  
			*  all tasks on the writers queue are exclusive tasks. 
			*  and wake_up() wakes up only one exclusive task.
			*  prepare_to_wait_exclusive() and wake_up () used together implement a FIFO
			*/
			wake_up(&(network.writers_queue));
		}
		//No need to wake up readers since they can share a lock
		
	}
	/* Case: Writer
	* "Only one process may hold an exclusive lock at any given time".
	*  Therefore if the unlocking process is a writer, it was the only
	*  process having a lock. We may then immediately wake up writers. 
	*/
	else 
	{
		//first decrease the number of writers
		network.num_current_writers--;
		//Try to wake up writers
		if (network.num_waiting_writers != 0 )
		{
			spin_unlock_irq(&(network.lock));
			/*  
			*  all tasks on the writers queue are exclusive tasks. 
			*  and wake_up() wakes up only one exclusive task.
			*/
			wake_up(&(network.writers_queue));
		}
		// No writers. Wake up readers.
		else if (network.num_waiting_readers != 0)
		{
			spin_unlock_irq(&(network.lock));
			/*
			*  all tasks on the readers queue are non-exclusive tasks.
			*  wake_up() wakes up all non-exclusive task.
			*/
			wake_up(&(network.readers_queue));
		}
		
	}
	
	//the task no longer holds a lock
	task->type_lock = NET_LOCK_N;
	
	return 0;
}



