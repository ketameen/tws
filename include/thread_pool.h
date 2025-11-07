#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

#define DEFAULT_MAX_THREADS 256


// ========================================
// TODO: more comprehnsive error messages |
// ========================================

typedef void (*task_func) (void *arg);

typedef struct
{
        task_func func;
        void     *arg;
} task_t;

typedef struct
{
        pthread_t   *threads;
        unsigned int n_threads;
        // circular queue
        task_t      *queue;
        unsigned int queue_max_size;
        unsigned int queue_size;
        unsigned int head_idx;
        unsigned int tail_idx;
        // sync
        pthread_mutex_t queue_mutex;
        pthread_cond_t  queue_cond;
        //
        int shutdown_state; // 0: no shutdown / 1: graceful / 2: immediate

} thread_pool_t;


static void *launch_worker(void *arg);
thread_pool_t* create_thread_pool(size_t n_threads, size_t n_tasks);
int push_task(thread_pool_t *tp, task_func func, void *arg);
int release_thread_pool(thread_pool_t *tp, bool graceful);




#endif // THREAD_POOL_H