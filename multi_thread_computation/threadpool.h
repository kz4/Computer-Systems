#ifndef MULTI_THREAD_COMPUTATION_THREADPOOL_H
#define MULTI_THREAD_COMPUTATION_THREADPOOL_H

/* Binary semaphore */
typedef struct bsem {
    pthread_mutex_t mutex;
    pthread_cond_t   cond;
    int v;
} bsem;

/* Job */
typedef struct job{
    struct job*  prev;                   /* pointer to previous job   */
    void*  (*function)(void* arg);       /* function pointer          */
    void*  arg;                          /* function's argument       */
} job;


/* Job queue */
typedef struct jobqueue{
    pthread_mutex_t rwmutex;             /* used for queue r/w access */
    job  *front;                         /* pointer to front of queue */
    job  *rear;                          /* pointer to rear  of queue */
    bsem *has_jobs;                      /* flag as binary semaphore  */
    int   len;                           /* number of jobs in queue   */
} jobqueue;


/* Thread */
typedef struct thread{
    int       id;                        /* friendly id               */
    pthread_t pthread;                   /* pointer to actual thread  */
    struct thpool_* thpool_p;            /* access to thpool          */
} thread;


/* Threadpool */
typedef struct thpool_{
    thread**   threads;                  /* pointer to threads        */
    volatile int num_threads_alive;      /* threads currently alive   */
    volatile int num_threads_working;    /* threads currently working */
    pthread_mutex_t  thcount_lock;       /* used for thread count etc */
    pthread_cond_t  threads_all_idle;    /* signal to thpool_wait     */
    jobqueue*  jobqueue_p;               /* pointer to the job queue  */
} thpool_;

typedef struct thpool_* threadpool;

/*
 * Initialize the thread pool with num of threads
 */
threadpool thpool_init(int num);

/*
 * Add work to the thread pool
 */
int thpool_add_work(threadpool, void *(*function_p)(void*), void* arg_p);

/*
 * Wait for all threads to finish work
 */
void thpool_wait(threadpool);

/*
 * Used in the end to destroy a thread pool
 */
void thpool_destroy(threadpool);


static int  thread_init(thpool_* thpool_p, struct thread** thread_p, int id);
static void* thread_do(struct thread* thread_p);
static void  thread_hold();
static void  thread_destroy(struct thread* thread_p);

static int   jobqueue_init(thpool_* thpool_p);
static void  jobqueue_clear(thpool_* thpool_p);
static void  jobqueue_push(thpool_* thpool_p, struct job* newjob_p);
static struct job* jobqueue_pull(thpool_* thpool_p);
static void  jobqueue_destroy(thpool_* thpool_p);

static void  bsem_init(struct bsem *bsem_p, int value);
static void  bsem_reset(struct bsem *bsem_p);
static void  bsem_post(struct bsem *bsem_p);
static void  bsem_post_all(struct bsem *bsem_p);
static void  bsem_wait(struct bsem *bsem_p);


#endif //MULTI_THREAD_COMPUTATION_THREADPOOL_H
