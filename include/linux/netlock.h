#ifndef __KERNEL_NETLOCK_H
#define __KERNEL_NETLOCK_H

#include <linux/spinlock.h>
#include <linux/wait.h>

struct network_struct {
  int num_readers;
  int num_writers;
  spinlock_t lock;
  wait_queue_head_t readers_queue;
  wait_queue_head_t writers_queue;
};

enum __netlock_t{
  NET_LOCK_N,
  NET_LOCK_R,
  NET_LOCK_E,
};

typedef enum __netlock_t netlock_t;

#endif /* __KERNEL_NETLOCK_H */
