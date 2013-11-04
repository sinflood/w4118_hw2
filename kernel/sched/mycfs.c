#include <trace/events/sched.h>
#include <linux/sched.h>
#include "sched.h"


/*
   Called when a task enters a runnable state.
   It puts the scheduling entity (task) into the red-black tree and
   increments the nr_running variable.	
 */
static void
enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{

}

/*
   When a task is no longer runnable, this function is called to keep the
   corresponding scheduling entity out of the red-black tree.  It decrements
   the nr_running variable.
*/
static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{

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
