
#include <stdio.h>
#include "threadpool.h"
#include <unistd.h>
#include <stdlib.h>

threadpool* create_threadpool(int num_threads_in_pool){

    if (num_threads_in_pool >= MAXT_IN_POOL){
        printf ("more than the max threads number\n");
        return NULL;
    }


    threadpool *tp = (threadpool *)malloc(sizeof(threadpool));
    if (tp == NULL) {
        return NULL;
    }

    tp->num_threads = num_threads_in_pool;
    tp->qsize = 0;
    tp->qhead = tp->qtail = NULL;
    tp->shutdown = tp->dont_accept = 0;


    pthread_mutex_init(&(tp->qlock), NULL);
    pthread_cond_init(&(tp->q_not_empty), NULL);
    pthread_cond_init(&(tp->q_empty), NULL);


    // creat an array for the threades
    tp->threads = (pthread_t *)malloc(num_threads_in_pool * sizeof(pthread_t));
    if (tp->threads == NULL) {
        return NULL;
    }

    for (int i = 0; i < num_threads_in_pool; i++) {
        pthread_create(tp->threads+i, NULL, do_work, (void *)tp);
    }

    return tp;

}


void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg){

    if (from_me == NULL){
        exit(-1);
    }

    pthread_mutex_lock(&from_me->qlock);

    if (from_me->dont_accept ==1 )
        pthread_mutex_unlock(&from_me->qlock);

    work_t *work = (work_t *)malloc (sizeof (work_t));

    if (work  == NULL) {
        printf("Memory allocation failed!\n");
        exit(-1);
    }

    work->routine = dispatch_to_here;
    work->arg = arg;
    work->next = NULL;

    if (from_me->qhead == NULL && from_me->qtail == NULL) {
        from_me->qhead = from_me->qtail = work;

    } else {
        from_me->qtail->next = work;
        from_me->qtail = work;
    }

    from_me->qsize++;

    pthread_cond_signal(&(from_me->q_not_empty));
    pthread_mutex_unlock(&(from_me->qlock));
}


void* do_work(void* p) {

    threadpool  *tp = (threadpool *)p;

    while (1) {

        pthread_mutex_lock(&tp->qlock);

        if (tp->shutdown) {
            pthread_mutex_unlock(&(tp->qlock));
            pthread_exit(NULL);
        }

        while (tp->qsize == 0 ) {
            pthread_cond_wait(&(tp->q_not_empty),&(tp->qlock));

        }

        ///// to check if in queue have one item

        work_t *work = tp->qhead;

        if (tp->qhead == tp->qtail) {
            // If there's only one task in the queue
            tp->qhead = tp->qtail = NULL;
        } else {
            tp->qhead = work->next;
        }

        tp->qsize--;


        if (tp->qsize == 0 && tp->dont_accept)
            pthread_cond_signal(&(tp->q_empty));

        pthread_mutex_unlock(&(tp->qlock));
        work->routine(work->arg);
        free(work);
    }

}


void destroy_threadpool(threadpool* destroyme){

    pthread_mutex_lock(&destroyme->qlock);
    destroyme->dont_accept = 1 ;

    while (destroyme->qsize > 0 ){
        pthread_cond_wait(&destroyme->q_empty ,&destroyme->qlock );
    }

    destroyme->shutdown = 1;
    pthread_cond_broadcast(&destroyme->q_not_empty);
    pthread_mutex_unlock(&destroyme->qlock);

    for(int i = 0 ; i < destroyme->num_threads ; i++){
        pthread_join(destroyme->threads[i], NULL);
    }

    pthread_mutex_destroy(&(destroyme->qlock));
    pthread_cond_destroy(&(destroyme->q_empty));
    pthread_cond_destroy(&(destroyme->q_not_empty));


    free(destroyme->threads);
    free(destroyme);
}
