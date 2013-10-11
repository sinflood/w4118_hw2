#include <linux/netlock.h>
#include <linux/syscalls.h>
#include <asm/current.h>
#include <linux/errno.h>
#include <linux/sched.h>

//TODO:need to be initiliazed
static struct network_struct network;

asmlinkage long sys_netlock_acquire (netlock_t type)
{
	DEFINE_WAIT(my_wait);
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
	
	spin_lock_irq(&(network.lock);
		
	
	// case: writer
	if ( type == NET_LOCK_E ) 
	{		
		//can access the network only if there isn't any writers or readers
		if( network.num_writers == 0 && network.num_readers == 0 )
		{
			network.num_writers++;
			spin_unlock_irq(&(network.lock));
		}
		//if there are readers or writers
		else 
		{ 
			prepare_to_wait(&(network.writers_queue), &my_wait, TASK_INTERRUPTIBLE);
			spin_unlock_irq(&(network.lock));
			
			//TODO: needs to wait. Wake up when there are no writers nor readers
			
			spin_lock_irq(&(network.lock);
			network.num_writers++;
			spin_unlock_irq(&(network.lock));
		}
		
		task->type_lock = NET_LOCK_E;
			
	}
	// case: reader
	else ( type == NET_LOCK_R )
	{	
		// good case: no writers, any number of readers
		if( network.num_writers == 0 )
		{
			network.num_readers++;
			spin_unlock_irq(&(network.lock));
		}
		// bad case: there are writers
		else
		{
			prepare_to_wait(&(network.readers_queue), &my_wait, TASK_INTERRUPTIBLE);
			spin_unlock_irq(&(network.lock));
			
			
			//TODO: needs to wait. Wake up when there are no writers 
			
			spin_lock_irq(&(network.lock);
			network.num_readers++;
			spin_unlock_irq(&(network.lock));
			
		}
		
		task->type_lock = NET_LOCK_R;
	}
	return 0;
}

asmlinkage long sys_netlock_release (void)
{
	struct task_struct *task = get_current();
	spin_lock_irq(&(network.lock);
	
	// case: reader
	if ( task->type_lock = NET_LOCK_R )
	{
		//first decrease the number of readers
		network.num_readers--;
		//wake up reader or writer with priority for writers
		if (  )
		{
			//TODO 
		}
		spin_unlock_irq(&(network.lock));
	}
	// case: writer
	else ( task->type_lock = NET_LOCK_E )
	{
		//first decrease the number of writers
		network.num_writers--;
		//check that there are no writers (only one writer at a time) 
		if ( network.num_writers== 0 ) 
		{
			//TODO Wake up writers first
		}
		spin_unlock_irq(&(network.lock));
	}
	//the task no longer holds a lock
	task->type_lock = NET_LOCK_N;
	
	return 0;
}



