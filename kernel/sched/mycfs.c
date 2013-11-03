#include <trace/events/sched.h>
#include <linux/sched.h>
#include "sched.h"

/*
*  Enqueue an entity into the rb-tree
*/
static void enqueue_entity(struct mycfs_rq *mycfs_rq, struct sched_entity *se)
{
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

	enqueue_entity(mycfs_rq, se);
	se->on_rq = 1;
}

/*
*  Dequeue an entity from the rb-tree
*/
static void dequeue_entity(struct mycfs_rq *mycfs_rq, struct sched_entity *se)
{
	/* Maintain the lefmost tree */
	if(mycfs->rb_leftmost == &se->run_node)
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
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	struct sched_entity *se= &p->se;

	dequeue_entity(mycfs_rq, se);
	se->on_rq = 0;
}

/*
   This function is basically just a dequeue followed by an enqueue, unless the
   compat_yield sysctl is turned on; in that case, it places the scheduling
   entity at the right-most end of the red-black tree.
*/
static void yield_task_mycfs(struct rq *rq)
{

}

/*
This function chooses the most appropriate task eligible to run next.
*/
static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{

	return NULL;
}

/*
 This function is called when a task changes its scheduling class or changes
   its task group.
*/

static void set_curr_task_mycfs(struct rq *rq)
{


}

/*
This function is mostly called from time tick functions; it might lead to
   process switch.  This drives the running preemption.
*/
static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{


}


/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_mycfs(struct rq *rq, struct task_struct *p, int wake_flags)
{


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
	.yield_to_task      = yield_to_task_mycfs,

	.check_preempt_curr	= check_preempt_mycfs,

	.pick_next_task		= pick_next_task_mycfs,
    .put_prev_task      = put_prev_task_mycfs,
	
	.select_task_rq     = select_task_rq_mycfs,
	.task_waking        = task_waking_mycfs,

	.set_curr_task      = set_curr_task_mycfs,
	.task_tick		    = task_tick_mycfs,
	.task_fork          = task_fork_mycfs,

	.prio_changed       = prio_changed_mycfs,
	.switched_from      = switched_from_mycfs,
	.switched_to        = switched_to_mycfs,

	.get_rr_interval    = get_rr_interval_mycfs,
};
