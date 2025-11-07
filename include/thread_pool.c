#include "thread_pool.h"


thread_pool_t* create_thread_pool(size_t n_threads, size_t n_tasks)
{
        if(n_threads <= 0 || n_tasks <= 0) 
                return NULL;
        
        thread_pool_t *tp = calloc(1, sizeof(thread_pool_t));
        if(!tp)
                return NULL;
        
        tp->queue_max_size = n_tasks;
        tp->n_threads      = n_threads;
        tp->queue          = calloc(n_tasks, sizeof(task_t));
        if(!tp->queue)
        {
                free(tp);
                return NULL;
        }

        tp->threads = calloc(n_threads, sizeof(pthread_t));
        if(!tp->threads)
        {
                free(tp->threads);
                free(tp->queue);
                free(tp);
        }

        if (pthread_mutex_init(&tp->queue_mutex, NULL) != 0 ||
            pthread_cond_init(&tp->queue_cond, NULL) != 0) 
        {
                free(tp->threads);
                free(tp->queue);
                free(tp);
                return NULL;
        }

        // unecessary since we used calloc
        // but increases readability
        tp->head_idx       = 0;
        tp->tail_idx       = 0;
        tp->queue_size     = 0;
        tp->shutdown_state = 0;

        for(unsigned int i = 0; i < tp->n_threads; i++)
        {
                if(pthread_create(&tp->threads[i], NULL, launch_worker, (void *)tp) != 0)
                {
                        // immediate
                        tp->shutdown_state = 2;

                        pthread_cond_broadcast(&tp->queue_cond);
                        for (int j = 0; j < i; ++j)
                                pthread_join(tp->threads[j], NULL);
                        
                        pthread_cond_destroy(&tp->queue_cond);
                        pthread_mutex_destroy(&tp->queue_mutex);
                        free(tp->threads);
                        free(tp->queue);
                        free(tp);
                        return NULL;
                } 
        }
        return tp;
}

static void* launch_worker(void *arg)
{
        thread_pool_t *tp = (thread_pool_t*)arg;
        while(1)
        {
                // Protect shared queue
                pthread_mutex_lock(&tp->queue_mutex);
                while(tp->queue_size == 0)
                {
                        // release mutex and wait for condition (task submission)
                        pthread_cond_wait(&tp->queue_cond, &tp->queue_mutex);
                }
                // process shutdown
                // immediate: exit without executing remaining tasks
                if (tp->shutdown_state == 2) {
                        pthread_mutex_unlock(&tp->queue_mutex);
                        break;
                }
                // graceful: shutdown only when tasks are done
                if (tp->shutdown_state == 1 && tp->queue_size == 0) 
                {
                        pthread_mutex_unlock(&tp->queue_mutex);
                        break;
                }
                // pop
                task_t task = {0};
                if(tp->queue_size > 0)
                {
                        task = tp->queue[tp->head_idx];
                        tp->head_idx = ++tp->head_idx % tp->queue_max_size;
                        tp->queue_size--;
                }
                pthread_mutex_unlock(&tp->queue_mutex);

                if(task.func)
                        task.func(task.arg);
        }
        return NULL;
}

int push_task(thread_pool_t *tp, task_func func, void *arg)
{
        if(!tp || !func)
                return -1;
        
        pthread_mutex_lock(&tp->queue_mutex);

        if(tp->shutdown_state != 0)
        {
                pthread_mutex_unlock(&tp->queue_mutex);
                return -1;
        }

        if(tp->queue_max_size == tp->queue_size)
        {
                pthread_mutex_unlock(&tp->queue_mutex);
                return -1;
        }

        // push
        tp->queue[tp->tail_idx].func = func;
        tp->queue[tp->tail_idx].arg  = arg;
        tp->tail_idx = ++tp->tail_idx % tp->queue_max_size;
        tp->queue_size++;

        // signal one worker thread
        pthread_cond_signal(&tp->queue_cond);
        pthread_mutex_unlock(&tp->queue_mutex);

        return 0;
}

int release_thread_pool(thread_pool_t *tp, bool graceful)
{
        if(!tp)
                return -1;
        
        pthread_mutex_lock(&tp->queue_mutex);
        if(tp->shutdown_state != 0)
        {
                pthread_mutex_unlock(&tp->queue_mutex);
                return -1; // already shutdown
        }

        tp->shutdown_state = graceful ? 1 : 2;
        // wake all to shutdown
        pthread_cond_broadcast(&tp->queue_cond);
        pthread_mutex_unlock(&tp->queue_mutex);

        for (int i = 0; i < tp->n_threads; ++i) 
                pthread_join(tp->threads[i], NULL);

        // clean
        pthread_mutex_destroy(&tp->queue_mutex);
        pthread_cond_destroy(&tp->queue_cond);
        free(tp->threads);
        free(tp->queue);
        free(tp);

        return 0;
}