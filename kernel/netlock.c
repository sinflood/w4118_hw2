#include <linux/netlock.h>
#include <linux/syscalls.h>
#include <asm/current.h>
#include <linux/errno.h>
#include <linux/sched.h>

//TODO:need to be initiliazed
static struct network_struct network;

asmlinkage long sys_netlock_acquire (netlock_t type)
{
	
	struct task_struct *task = get_current();
	
	//check that a task doesn't request a second lock
	if( task->type_lock != NET_LOCK_N )
	{
		return EINVAL;
	}
	
	//check correct netlock_t type request
	if(  type!=NET_LOCK_E && type!=NET_LOCK_R )
	{
		return EINVAL;
	}
	
	DEFINE_WAIT(writers_wait);
	DEFINE_WAIT(readers_wait);
	
	//TODO not sure if this is the good lock or if spin_lock is enough
	spin_lock_irq(&(network.lock);
		
	
	// Case: Writer
	if ( type == NET_LOCK_E ) 
	{			
		add_wait_queue(network.writers_queue,&writers_wait);
		//the writer waits for the while condition to be false
		network.num_waiting_writers++;	
		
		while ( network.num_current_writers != 0 || network.num_current_readers != 0)
		{
			prepare_to_wait(&(network.writers_queue), &writers_wait, TASK_INTERRUPTIBLE);
			//unlock before scheduling another task			
			spin_unlock_irq(&(network.lock));
			if(signal_pending((task))
			{
				//TODO handle signal
			}
			schedule();
			//lock before checking the while loop condition
			spin_lock_irq(&(network.lock));
		}
		
		finish_wait(&network.writers_queue, &writers_wait);
		
		//the writer is now running
		network.num_waiting_writers--;				
		network.num_current_writers++;
		
		spin_unlock_irq(&(network.lock));
		
		//set the task as writer
		task->type_lock = NET_LOCK_E;
	}
	// Case: Reader
	else
	{	
		add_wait_queue(network.readers_queue,&readers_wait);
		//the reader waits for the while condition to be false
		network.num_waiting_readers++;	
		
		while ( network.num_current_writers != 0 || network.num_waiting_writers != 0)
		{
			prepare_to_wait(&(network.readers_queue), &readers_wait, TASK_INTERRUPTIBLE);
			//unlock before scheduling another task			
			spin_unlock_irq(&(network.lock));
			if(signal_pending((task))
			{
				//TODO handle signal
			}
			schedule();
			//lock before checking the while loop condition
			spin_lock_irq(&(network.lock));
		}
		
		finish_wait(&network.readers_queue, &readers_wait);
		
		//the reader is now running
		network.num_waiting_readers--;				
		network.num_current_readers++;
		
		spin_unlock_irq(&(network.lock));
		
		//set the task as reader
		task->type_lock = NET_LOCK_R;
	}
	
	return 0;
}

asmlinkage long sys_netlock_release (void)
{
	struct task_struct *task = get_current();
	
	//check if the task owns a lock
	if ( task->type_lock != NET_LOCK_R && task->type_lock != NET_LOCK_E)
	{
		return EINVAL;
	}
	
	spin_lock_irq(&(network.lock);
	
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
			//TODO not sure where to place this unlock
			spin_unlock_irq(&(network.lock));
			wake_up(&(network.writers_queue));
		}
		//No need to wake up readers since they can share a lock
		
	}
	/* Case: Writer
	* "Only one precess may hold an exclusive lock at any given time".
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
			//TODO not sure where to place this unlock
			spin_unlock_irq(&(network.lock));
			wake_up(&(network.writers_queue));
		}
		// No writers. Wake up readers.
		else if (network.num_waiting_readers != 0)
		{
			//TODO not sure where to place this unlock
			spin_unlock_irq(&(network.lock));
			wake_up(&(network.readers_queue));
		}
		
	}
	
	//the task no longer holds a lock
	task->type_lock = NET_LOCK_N;
	
	return 0;
}



