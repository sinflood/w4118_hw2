#include <trace/events/sched.h>
#include <linux/sched.h>
#include "sched.h"

void wake_limited(struct mycfs_rq *mycfs_rq, struct rq *rq);

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

        return min_vruntime;if(curr->sched_class != &mycfs_sched_class && rq->mycfs->nr_running)
}

static void update_curr(struct mycfs_rq *mycfs_rq)
{
  struct sched_entity *curr = mycfs_rq->curr;
  u64 now = mycfs_rq->rq->clock;
  //unsigned long delta_exec;
  if(mycfs_rq->lastTickTime == 0) mycfs_rq->lastTickTime = now;//could cause a bug if 0 happens to be the last tick time. but that is unlikely.
  //not sure if necessary if we can't initialize intervalTime
    //even if you can't initialize clock then it would just make the first interval shorter or one tick.
   // if(mycfs_rq->intervalTime == 0)
       // mycfs_rq->intervalTime = rq->clock;

    //if a task is running from mycfs_rq then update the tasks interval runtime.
    if(curr && mycfs_rq->nr_running)
    {
        if(curr->intervalNum != mycfs_rq->intervalNum)
        {
            curr->intervalNum = mycfs_rq->intervalNum;
            curr->intervalTime = 0;
        }
        //running keeps track of if this process was running the previous tick or not.
        //if it was then we know lastTime is accurate but if not lastTime is junk.
        //this misses a tick of time
        /*if(!curr->running)
        {
            curr->lastTime = rq->clock;
            curr->running = 1;//remember to set this when stop running a task.
        }*/
        //somehow we have to update the interval time.
        
        
        curr->intervalTime += now - mycfs_rq->lastTickTime;
        curr->vruntime += now - mycfs_rq->lastTickTime;
        //curr->lastTime = rq->clock;
    }
    mycfs_rq->intervalTime += now - mycfs_rq->lastTickTime;
    if(mycfs_rq->intervalTime >100000)
    {
        mycfs_rq->intervalNum++;
        wake_limited(mycfs_rq, mycfs_rq->rq);
        mycfs_rq->intervalTime = 0;
        if(curr){
            curr->intervalTime = 0;
            curr->intervalNum = mycfs_rq->intervalNum;
        }
    //Remember to update this numbers as we select a new entity to run.
    }
    
    mycfs_rq->lastTickTime = now;


  //if(!delta_exec) return;

  //curr->vruntime += delta_exec; //we don't have to worry about weighting the runtimes.

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
static void enqueue_entity(struct mycfs_rq *mycfs_rq, struct sched_entity *se)
{
	//if(!(flags & ENQUEUE_WAKEUP) /*|| (flags & ENQUEUE_MIGRATE)*/)
	//    se->vruntime += mycfs_rq->min_vruntime;
        
        update_curr(mycfs_rq);
        //update_min_vruntime(mycfs_rq);
        /*if(flags & ENQUEUE_WAKEUP)
	{
	    se->vruntime = max_vruntime(se->vruntime, mycfs_rq->min_vruntime);

	}*/
	if(!se->on_rq)
	    se->vruntime = mycfs_rq->min_vruntime;
	
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


        mycfs_rq->nr_running++; //update the number of proccesses running.  
	inc_nr_running(rq);

	enqueue_entity(mycfs_rq, se);
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
	//mycfs_rq->nr_running--;

	se->on_rq = 0;
	dequeue_entity(mycfs_rq, se);
	mycfs_rq->nr_running--;
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
  //struct sched_entity *se = &curr->se;

  if(unlikely(rq->nr_running==1)) return; //no other processes to yeild to.

  update_rq_clock(rq);
  
  update_curr(mycfs_rq);
  
  rq->skip_clock_update = 1;
  resched_task(curr);
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

void update_interval_data(struct mycfs_rq *mycfs_rq, struct rq *rq, struct task_struct *curr);
/*
This function is mostly called from time tick functions; it might lead to
   process switch.  This drives the running preemption.
*/
static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
  struct mycfs_rq *mycfs_rq = &rq->mycfs;
  //struct sched_entity *se = &curr->se;

  update_curr(mycfs_rq);
  update_interval_data(mycfs_rq, rq, curr);
//TODO check to see if curr has been running for longer than than the leftmost of the tree.  If it has then preempt the current process and then update bookeeping.
  if(!curr) return;
  
  //if there is more than one task and the timeslice is up reschedule.
  //also if the queued we just reschedule because that's what fair says you do.
  if((rq->clock - curr->se.exec_start > 10000/mycfs_rq->nr_running && mycfs_rq->nr_running > 1) || queued)
      resched_task(rq->curr);

}

/////////////////////////////////////////////////////////////////////////////
//                    *PART B CODE BELOW*                                  //
/////////////////////////////////////////////////////////////////////////////

void add_to_wait(struct mycfs_rq *mycfs_rq, struct task_struct *curr)
{
    if(!mycfs_rq->wait_head) mycfs_rq->wait_head = curr;
    else
    {
      curr->next_wait = mycfs_rq->wait_head;
      mycfs_rq->wait_head = curr;
    //NOTE that curr's sched_entity should not be on the rb tree.
    }
    curr.se->on_rq = 0;
    mycfs_rq->nr_running--;
    dec_nr_running(mycfs_rq->rq);
    //curr->running = 0;
}


void wake_limited(struct mycfs_rq *mycfs_rq, struct rq *rq)
{
    struct task_struct *p = mycfs_rq->wait_head;
    if(p)
    {
        struct task_struct *next;
        while(p);
        {
            next = p->next_wait;
            enqueue_task_mycfs(rq, p, 0);
            p->next_wait = NULL;
            p->se.intervalTime = 0;
            p->se.intervalNum = mycfs_rq->intervalNum;
            p = next;
            
        }
        
        mycfs_rq->wait_head = NULL;
    }
}
/*
* we keep track of interval_data on every tick. If a mycfs_rq task is not the current
* then we still call this function with curr equal to NULL. In that case just update the info
* and maybe move all of the items off of the waiting list and reset each entities data.
*/
void update_interval_data(struct mycfs_rq *mycfs_rq, struct rq *rq, struct task_struct *curr)
{
  
    //remember to add intervalTime and intervalNum to the mycfs_rq.
    //static task_struct *wait_head = NULL add to mycfs
    //check to see if the current procces is done.
    if(curr && (curr->se.intervalTime > curr->se.intervalLimit) && (curr->se.intervalLimit > 0))
    {
        //Pick the next task to run, and set it as curr.
        add_to_wait(mycfs_rq, curr);
        mycfs_rq->curr = NULL;
        mycfs_rq->rq->curr = pick_next_task_mycfs(rq);//not sure if this will work but what I think happens here
                           //is that core.c takes over and picks the next task.
                           //this may happen to be in mycfs_rq which should be ok even tho
                           //we are setting the current in mycfs_rq to NULL
        resched_task(mycfs_rq->rq->curr);
    }
    
}



void init_mycfs_rq(struct mycfs_rq *mycfs_rq)
{
  /* I think we need to only initialize these two fields */
  mycfs_rq->tasks_timeline = RB_ROOT;
  mycfs_rq->min_vruntime = 0;
  /*part b stuff. */
  mycfs_rq->intervalTime = 0;
  mycfs_rq-> wait_head = NULL;
  mycfs_rq->lastTickTime = 0;
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

static void put_prev_entity(struct mycfs_rq *mycfs_rq, struct sched_entity *prev)
{
        /*
         * If still on the runqueue then deactivate_task()
         * was not called and update_curr() has to be done:
         */
         //enqueue_entity calls update_curr so no need to call it twice
       // if (prev->on_rq)
         //       update_curr(mycfs_rq);

        /* throttle cfs_rqs exceeding runtime TODO Is this needed?*/
        //check_cfs_rq_runtime(mycfs_rq);

        if (prev->on_rq) {
                //update_stats_wait_start(mycfs_rq, prev);
                /* Put 'current' back into the tree. */
                enqueue_entity(mycfs_rq, prev);
        }
        mycfs_rq->curr = NULL;
}

/*
 * Account for a descheduled task:
 */
static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{
        struct sched_entity *se = &prev->se;
        struct mycfs_rq *mycfs_rq = &rq->mycfs;


        put_prev_entity(mycfs_rq, se);
        
}

static inline struct sched_entity *parent_entity(struct sched_entity *se)
{
        return se->parent;
}
#if BITS_PER_LONG == 32
# define WMULT_CONST        (~0UL)
#else
# define WMULT_CONST        (1UL << 32)
#endif

#define WMULT_SHIFT        32
/*
 * Shift right and round:
 */
#define SRR(x, y) (((x) + (1UL << ((y) - 1))) >> (y))
/*
 * delta *= weight / lw
 */
static unsigned long
calc_delta_mine(unsigned long delta_exec, unsigned long weight,
                struct load_weight *lw)
{
        u64 tmp;

        /*
         * weight can be less than 2^SCHED_LOAD_RESOLUTION for task group sched
         * entities since MIN_SHARES = 2. Treat weight as 1 if less than
         * 2^SCHED_LOAD_RESOLUTION.
         */
        if (likely(weight > (1UL << SCHED_LOAD_RESOLUTION)))
                tmp = (u64)delta_exec * scale_load_down(weight);
        else
                tmp = (u64)delta_exec;

        if (!lw->inv_weight) {
                unsigned long w = scale_load_down(lw->weight);

                if (BITS_PER_LONG > 32 && unlikely(w >= WMULT_CONST))
                        lw->inv_weight = 1;
                else if (unlikely(!w))
                        lw->inv_weight = WMULT_CONST;
                else
                        lw->inv_weight = WMULT_CONST / w;
        }

        /*
         * Check whether we'd overflow the 64-bit multiplication:
         */
        if (unlikely(tmp > WMULT_CONST))
                tmp = SRR(SRR(tmp, WMULT_SHIFT/2) * lw->inv_weight,
                        WMULT_SHIFT/2);
        else
                tmp = SRR(tmp * lw->inv_weight, WMULT_SHIFT);

        return (unsigned long)min(tmp, (u64)(unsigned long)LONG_MAX);
}
/*
 * delta /= w
 */
static inline unsigned long
calc_delta_fair(unsigned long delta, struct sched_entity *se)
{
        if (unlikely(se->load.weight != NICE_0_LOAD))
                delta = calc_delta_mine(delta, NICE_0_LOAD, &se->load);

        return delta;
}
static unsigned long
wakeup_gran(struct sched_entity *curr, struct sched_entity *se)
{
        unsigned long gran = sysctl_sched_wakeup_granularity;

        /*
         * Since its curr running now, convert the gran from real-time
         * to virtual-time in his units.
         *
         * By using 'se' instead of 'curr' we penalize light tasks, so
         * they get preempted easier. That is, if 'se' < 'curr' then
         * the resulting gran will be larger, therefore penalizing the
         * lighter, if otoh 'se' > 'curr' then the resulting gran will
         * be smaller, again penalizing the lighter task.
         *
         * This is especially important for buddies when the leftmost
         * task is higher priority than the buddy.
         */
        return calc_delta_fair(gran, se);
}
static int
wakeup_preempt_entity(struct sched_entity *curr, struct sched_entity *se)
{
        s64 gran, vdiff = curr->vruntime - se->vruntime;

        if (vdiff <= 0)
                return -1;

        gran = wakeup_gran(curr, se);
        if (vdiff > gran)
                return 1;

        return 0;
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_mycfs(struct rq *rq, struct task_struct *p, int wake_flags)
{
        struct task_struct *curr = rq->curr;
        struct sched_entity *se = &curr->se, *pse = &p->se;
        //struct mycfs_rq *cfs_rq = se->mycfs_rq;
        //int scale = cfs_rq->nr_running >= sched_nr_latency;

        if (unlikely(se == pse))
                return;

        /*
         * We can come here with TIF_NEED_RESCHED already set from new task
         * wake up path.
         *
         * Note: this also catches the edge-case of curr being in a throttled
         * group (e.g. via set_curr_task), since update_curr() (in the
         * enqueue of curr) will have resulted in resched being set.  This
         * prevents us from potentially nominating it as a false LAST_BUDDY
         * below.
         */
        if (test_tsk_need_resched(curr))
                return;

        /* Idle tasks are by definition preempted by non-idle tasks. */
        if (unlikely(curr->policy == SCHED_IDLE) &&
            likely(p->policy != SCHED_IDLE))
                goto preempt;

        /*
         * Batch and idle tasks do not preempt non-idle tasks (their preemption
         * is driven by the tick):
         */
        if (unlikely(p->policy != SCHED_NORMAL))
                return;

        //find_matching_se(&se, &pse);
        update_curr(se->mycfs_rq);
        BUG_ON(!pse);
        if (wakeup_preempt_entity(se, pse) == 1) {


                goto preempt;
        }

        return;

preempt:
        resched_task(curr);
        /*
         * Only set the backward buddy when the current task is still
         * on the rq. This can happen when a wakeup gets interleaved
         * with schedule on the ->pre_schedule() or idle_balance()
         * point, either of which can * drop the rq lock.
         *
         * Also, during early boot the idle thread is in the fair class,
         * for obvious reasons its a bad idea to schedule back to it.
         */
        if (unlikely(!se->on_rq || curr == rq->idle))
                return;
}

static int
select_task_rq_mycfs(struct task_struct *p, int sd_flag, int wake_flags)
{

	 return task_cpu(p);
}

static void switched_to_mycfs(struct rq *rq, struct task_struct *p)
{

	if (!p->se.on_rq)
                return;

        /*
         * We were most likely switched from sched_rt, so
         * kick off the schedule if running, otherwise just see
         * if we can still preempt the current task.
         */
        if (rq->curr == p)
                resched_task(rq->curr);
        else
                check_preempt_curr(rq, p, 0);

	//rq->mycfs->curr = &p->se;
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
    	.put_prev_task      = put_prev_task_mycfs,
	
	.select_task_rq     = select_task_rq_mycfs,
	//.task_waking        = task_waking_mycfs,

	.set_curr_task      = set_curr_task_mycfs,
	.task_tick		    = task_tick_mycfs,
	//.task_fork          = task_fork_mycfs,

	//.prio_changed       = prio_changed_mycfs,
	//.switched_from      = switched_from_mycfs,
	.switched_to        = switched_to_mycfs,

        .get_rr_interval    = get_rr_interval_mycfs,
};
