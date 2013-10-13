#ifndef __KERNEL_NETLOCK_H
#define __KERNEL_NETLOCK_H

#include <linux/spinlock.h>
#include <linux/wait.h>

struct network_struct {
  int num_current_readers;
  int num_current_writers;
  spinlock_t lock;
  wait_queue_head_t readers_queue;
  wait_queue_head_t writers_queue;
  int num_waiting_readers;
  int num_waiting_writers;
};

#define NETWORK_STRUCT(network) struct network_struct network = NETWORK_STRUCT_INIT(network)

#define NETWORK_STRUCT_INIT(network) {
	.num_current_readers = 0,
	.num_current_writers = 0,
	.lock = __SPIN_LOCK_UNLOCKED(lock),
	.readers_queue = __WAIT_QUEUE_HEAD_INITIALIZER(readers_queue),
	.writers_queue = __WAIT_QUEUE_HEAD_INITIALIZER(writers_queue),
	.num_waiting_readers = 0,
	.num_waiting_writers = 0,
}

enum __netlock_t{
  NET_LOCK_N,
  NET_LOCK_R,
  NET_LOCK_E,
};

typedef enum __netlock_t netlock_t;

#endif /* __KERNEL_NETLOCK_H */
