#include <trace/events/sched.h>
#include <linux/sched.h>
#include "sched.h"


static inline u64 max_vruntime(u64 min_vruntime, u64 vruntime)
{
        s64 delta = (s64)(vruntime - min_vruntime);
        if (delta > 0)
                min_vruntime = vruntime;

        return min_vruntime;
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
        s64 delta = (s64)(vruntime - min_vruntime);
        if (delta < 0)
                min_vruntime = vruntime;

        return min_vruntime;
}

static void update_curr(struct mycfs_rq *mycfs_rq)
{
  struct sched_entity *curr = mycfs_rq->curr;
  u64 now = mycfs_rq->rq->clock;
  unsigned long delta_exec;
  if(unlikely(!curr)) return;

  delta_exec = (unsigned long)(now - curr->exec_start);


  if(!delta_exec) return;

  curr->vruntime += delta_exec; //we don't have to worry about weighting the runtimes.

  //not sure if we need anything else from the update curr from fair.c.

  //schedstat_set(curr->exec_max, max((u64)delta_exec, curr->exec->max))
}
static void update_min_vruntime(struct mycfs_rq *mycfs_rq)
{
	u64 vruntime = mycfs_rq->min_vruntime;

	if (mycfs_rq->curr)
		vruntime = mycfs_rq->curr->vruntime;

	if (mycfs_rq->rb_leftmost) {
		struct sched_entity *se = rb_entry(mycfs_rq->rb_leftmost,
						   struct sched_entity,
						   run_node);

		if (!mycfs_rq->curr)
			vruntime = se->vruntime;
		else
			vruntime = min_vruntime(vruntime, se->vruntime);
	}

	mycfs_rq->min_vruntime = max_vruntime(mycfs_rq->min_vruntime, vruntime);
}


static void insert_tree(struct mycfs_rq *mycfs_rq, struct sched_entity *se){

	struct rb_node **link = &mycfs_rq->tasks_timeline.rb_node;
	struct rb_node *parent = NULL;
	struct sched_entity *entry;
	int leftmost = 1;
	while(*link)
	{
		parent = *link;
		entry = rb_entry(parent, struct sched_entity, run_node);
		if (se->vruntime < entry->vruntime)
		{
			link = &parent->rb_left;
		} 
		else
		{
			link = &parent->rb_right;
			leftmost = 0;
		}
	}
	
	/* Cache the lefmost tree entries */
	if(leftmost)
	{
		mycfs_rq->rb_leftmost = &se->run_node;
	}	
	
	rb_link_node(&se->run_node, parent, link);
	rb_insert_color(&se->run_node, &mycfs_rq->tasks_timeline);


}
/*
*  Enqueue an entity into the rb-tree
*/
static void enqueue_entity(struct mycfs_rq *mycfs_rq, struct sched_entity *se, int flags)
{
	if(!(flags & ENQUEUE_WAKEUP) /*|| (flags & ENQUEUE_MIGRATE)*/)
	    se->vruntime += mycfs_rq->min_vruntime;
        
        update_curr(mycfs_rq);
        //update_min_vruntime(mycfs_rq);
        if(flags & ENQUEUE_WAKEUP)
	{
	    se->vruntime = max_vruntime(se->vruntime, mycfs_rq->min_vruntime);

	}
	
	insert_tree(mycfs_rq, se);

	
}

/*
   Called when a task enters a runnable state.
   It puts the scheduling entity (task) into the red-black tree and
   increments the nr_running variable.	
 */
static void
enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	/* I am not sure if we need the flags */
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	struct sched_entity *se= &p->se;
	
	/*
	*  in fair.c I don't know why but it iterates over all the parent entities.  
	*  It does the same thing but inside this loop: 
	*  #define for_each_sched_entity(se) for (; se; se = se->parent)
	*  for_each_sched_entity(se){
	*    if(se->on_rq) break;
	*  }
	*/


        mycfs_rq->h_nr_running++; //update the number of proccesses running.  
	inc_nr_running(rq);

	enqueue_entity(mycfs_rq, se, flags);
	se->on_rq = 1;
}



/*
*  Dequeue an entity from the rb-tree
*/
static void dequeue_entity(struct mycfs_rq *mycfs_rq, struct sched_entity *se)
{
        update_curr(mycfs_rq);
	update_min_vruntime(mycfs_rq);
	/* Maintain the lefmost tree */
	if(mycfs_rq->rb_leftmost == &se->run_node)
	{
		struct rb_node *next_node;
		next_node = rb_next(&se->run_node);
		mycfs_rq->rb_leftmost = next_node;
	}
	
	rb_erase(&se->run_node, &mycfs_rq->tasks_timeline);
}



/*
   When a task is no longer runnable, this function is called to keep the
   corresponding scheduling entity out of the red-black tree.  It decrements
   the nr_running variable.
*/
static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	/* Do we need the flags? */
	struct sched_entity *se= &p->se;
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	mycfs_rq->h_nr_running--;

	se->on_rq = 0;
	dequeue_entity(mycfs_rq, se);
	mycfs_rq->h_nr_running--;
	dec_nr_running(rq);
}

/*
   This function is basically just a dequeue followed by an enqueue, unless the
   compat_yield sysctl is turned on; in that case, it places the scheduling
   entity at the right-most end of the red-black tree.
*/
static void yield_task_mycfs(struct rq *rq)
{
  struct task_struct *curr = rq->curr;
  struct mycfs_rq *mycfs_rq = &rq->mycfs;
  struct sched_entity *se = &curr->se;

  if(unlikely(rq->nr_running==1)) return; //no other processes to yeild to.

  update_rq_clock(rq);
  
  update_curr(mycfs_rq);
  
  rq->skip_clock_update = 1;
  dequeue_entity(mycfs_rq, se);
  enqueue_entity(mycfs_rq, se, 0);
  //I believe this should work and schedule() call in sched_yeild() shoudl handle the rest.
  
}

/*
This function chooses the most appropriate task eligible to run next.
*/
static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
  /*not really sure what else needs to be done here.
    
  */
  struct mycfs_rq *mycfs_rq = &rq->mycfs;
  struct sched_entity *se;
  struct rb_node *left = mycfs_rq->rb_leftmost;
  if(!left) return NULL;

  se = rb_entry(left, struct sched_entity, run_node);

  dequeue_entity(mycfs_rq, se); //not sure if we need to check on_rq here.
  se->exec_start = rq->clock;
  mycfs_rq->curr = se;

  return container_of(se, struct task_struct, se);
}

/*
 This function is called when a task changes its scheduling class or changes
   its task group.
*/

static void set_curr_task_mycfs(struct rq *rq)
{
  struct sched_entity *se = &rq->curr->se;
  struct mycfs_rq *mycfs_rq = &rq->mycfs;

  se->exec_start = rq->clock;
  mycfs_rq->curr = se;

}

/*
This function is mostly called from time tick functions; it might lead to
   process switch.  This drives the running preemption.
*/
static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
  struct mycfs_rq *mycfs_rq = &rq->mycfs;
  //struct sched_entity *se = &curr->se;

  update_curr(mycfs_rq);
//TODO check to see if curr has been running for longer than than the leftmost of the tree.  If it has then preempt the current process and then update bookeeping.
  

}


/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_mycfs(struct rq *rq, struct task_struct *p, int wake_flags)
{


}


/////////////////////////////////////////////////////////////////////////////
//                    *PART B CODE BELOW*                                  //
/////////////////////////////////////////////////////////////////////////////

//remeber to add a struct task_struct *next_wait to task_struct.
/*
void add_to_wait(struct mycfs_rq *mycfs_rq, struct task_struct *curr)
{
    if(!mycfs_rq->wait_head)) *wait_head = curr;
    else
    {
        curr->next_wait = mycfs_rq->wait_head;
        mycfs_rq->wait_head = curr;
        //NOTE that curr's sched_entity should not be on the rb tree.
    }
    curr->running = 0;
}


void wake_limited(struct mycfs_rq *mycfs_rq, struct rq *rq)
{
    struct task_struct *p = mycfs_rq->wait_head;
    
    if(p)
    {
        struct task_struct *next;
        do
        {
            next = p->next_wait;
            enqueue_task_mycfs(rq, p, 0);
            p->next_wait = NULL;
            p->intervalTime = 0;
            p->intervalNum = mycfs_rq->intervalNum;
            p = next;
        }while(p);
        mycfs_rq->wait_head = NULL;
    }    
}
/*  
 * we keep track of interval_data on every tick.  If a mycfs_rq task is not the current
 * then we still call this function with curr equal to NULL.  In that case just update the info
 * and maybe move all of the items off of the waiting list and reset each entities data.
 * 
 
void update_interval_data(struct mycfs_rq *mycfs_rq, struct rq *rq, struct task_struct *curr)
{   
    
    //not sure if necessary if we can't initialize intervalTime
    //even if you can't initialize clock then it would just make the first interval shorter on one tick.
    if(mycfs_rq->intervalTime == 0)
        mycfs_rq->intervalTime = rq->clock;

    //if a task is running from mycfs_rq then update the tasks interval runtime.
    if(curr)
    {
        //running keeps track of if this process was running the previous tick or not.
        //if it was then we know lastTime is accurate but if not lastTime is junk.
        //this misses a tick of time 
        if(!curr->running)
        {
            curr->lastTime = rq->clock;
            curr->running = 1;//remember to set this when stop running a task.
        }
        //somehow we have to update the interval time.
        curr->intervalTime += rq->clock - curr->lastTime;
        curr->lastTime = rq->clock;
        
        
    }
    
        
    if(rq->clock - mycfs_rq->intervalTime >100000)
    {
        mycfs_rq->intervalNum++;
        wake_limited(mycfs_rq, rq);
        
        mycfs->intervalTime = rq->clock;
        if(curr){
            curr->intervalTime = 0;
            curr->intervalNum = mycfs->intervalNum;
        }
        //Remember to update this numbers as we select a new entity to run.
    }
    
    //remember to add intervalTime and intervalNum to the mycfs_rq.
    //static task_struct *wait_head = NULL add to mycfs
    //check to see if the current procces is done.
    if(curr && (curr->intervalTime > curr->intervalLimit) && (curr->intervalLimit > 0))
    {
        //Pick the next task to run, and set it as curr.
        
        
        add_to_wait(mycfs, curr, &wait_head);
        mycfs_rq->curr = NULL;
        pick_next_task(rq);//not sure if this will work but what I think happens here
                           //is that core.c takes over and picks the next task.
                           //this may happen to be in mycfs_rq which should be ok even tho
                           //we are setting the current in mycfs_rq to NULL
    }
    
    
    
}
*/


void init_mycfs_rq(struct mycfs_rq *mycfs_rq)
{
  /* I think we need to only initialize these two fields */
  mycfs_rq->tasks_timeline = RB_BOOT;
  mycfs_rq->min_vruntime = 0;
}

/*
 *  called by SYSCALL_DEFINE2(sched_rr_get_interval) in core.c
 *  this syscall writes the default timeslice of a given process into
 *  the user-space timespec buffer
 *
 *  The timeslice is the latency divided by the number of processes on mycfs_rq
 *
 */

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
  struct mycfs_rq *mycfs_rq = &rq->mycfs;
  unsigned int num_processes = mycfs_rq->nr_running;
  /* latency = 10msec = 10,000,000 nanosec */
  unsigned int latency = 10000000;
  unsigned int timeslice = latency/num_processes;
  return NS_TO_JIFFIES(timeslice);
}

/*
 * the scheduling class methods:
 */
const struct sched_class mycfs_sched_class = {
	/* mycfs is between lower than fair but higher than idle */ 
	.next			    = &idle_sched_class,
	.enqueue_task		= enqueue_task_mycfs,
	.dequeue_task		= dequeue_task_mycfs,
	.yield_task		    = yield_task_mycfs,
	//.yield_to_task      = yield_to_task_mycfs,

	.check_preempt_curr	= check_preempt_mycfs,

	.pick_next_task		= pick_next_task_mycfs,
    	//.put_prev_task      = put_prev_task_mycfs,
	
	//.select_task_rq     = select_task_rq_mycfs,
	//.task_waking        = task_waking_mycfs,

	.set_curr_task      = set_curr_task_mycfs,
	.task_tick		    = task_tick_mycfs,
	//.task_fork          = task_fork_mycfs,

	//.prio_changed       = prio_changed_mycfs,
	//.switched_from      = switched_from_mycfs,
	//.switched_to        = switched_to_mycfs,

        .get_rr_interval    = get_rr_interval_mycfs,
};
