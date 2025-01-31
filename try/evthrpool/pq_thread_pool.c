#include <stdio.h>

#include <unistd.h>
#include <thread_pool.h>
#include <errno.h>
#include <stdatomic.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <ev_pqueue.h>

struct task_s {
	task_func_type task_function;
	task_argument_type * arg;
};

struct thr_s {
	pthread_t	t;
	atomic_int 	state;
	atomic_int	task_count;
	bool	subscribed;
};
#if defined THREAD_FREE
#undef THREAD_FREE
#endif
#if defined THREAD_BUSY
#undef THREAD_BUSY
#endif
#define THREAD_FREE 0
#define THREAD_BUSY 1

struct thread_pool_s {
	atomic_uint			_cond_count;
	struct thr_s		*_threads;
	int					_num_threads;
	ev_pqueue_type		_task_queue;
	atomic_int			_shutdown;
	atomic_int			_enq_index;
	struct  sigaction	*_o_sigh_ptr;
	ev_pqueue_type		_free_thr_queue;
};

struct thr_free_s {
	pthread_t	t;
	int			thr_index;
};

struct thr_inp_data_s {
	struct thread_pool_s	*pool;
	int						thr_index;
};

void thr_wakeup_sig_handler(int s)
{
	puts("AWAKE");
	return;
}


// The main thread looping to take and execute a task.
//
static void * thread_loop(void *data)
{
	struct thread_pool_s	*pool = NULL;
	struct task_s			*qe = NULL;
	int						s = 0;
	unsigned int			o_c = 0;
	unsigned int			c = 0;
	sigset_t				set;
	sigset_t				o_set;
	sigset_t				r_set;
	int						sig = 0;
	int						thr_index = 0;

	pool = ((struct thr_inp_data_s *)data)->pool;
	thr_index = ((struct thr_inp_data_s *)data)->thr_index;
	free(data);
	data = NULL;

	sigemptyset(&set);
	//sigemptyset(&o_set);
	sigaddset(&set,SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, &o_set);

	pool->_threads[thr_index].state = THREAD_FREE;
	for (;;) {
		s = atomic_load_explicit(&(pool->_shutdown),memory_order_relaxed);
		if (s) break;
		qe = dequeue_ev_pqueue(pool->_task_queue);
		if (!qe) {
			int i = 0;
			while (s == 0) {
				if (i<20) i++;
				pthread_sigmask(SIG_BLOCK, &set, &o_set);

				sig = 0;
				pool->_threads[thr_index].subscribed = true;
				if (pool->_threads[thr_index].subscribed) { usleep(i*10); }
				pool->_threads[thr_index].subscribed = false;

				qe = dequeue_ev_pqueue(pool->_task_queue);
				if (qe) {
					if (!qe->task_function)
						s = 1;
					break;
				}

				s = atomic_load_explicit(&(pool->_shutdown),memory_order_relaxed);
			}
		}
		if (s) break;
		atomic_thread_fence(memory_order_acq_rel);
		//EV_DBGP("pool->_threads[%d]._task_count = [%d]\n",thr_index,pool->_threads[thr_index].task_count);
		pool->_threads[thr_index].state = THREAD_BUSY;
		(*(qe->task_function))(qe->arg);
		free(qe);
		atomic_fetch_add(&(pool->_threads[thr_index].task_count),1);
		pool->_threads[thr_index].state = THREAD_FREE;
	}

	return NULL;
}

struct thread_pool_s * create_thread_pool(int num_threads)
{
	struct thread_pool_s * pool =NULL;
	struct  sigaction sigh;
	struct  sigaction * o_sigh_ptr = NULL;

	if (0>=num_threads) {
		errno = EINVAL;
		return NULL;
	}

	pool = malloc(sizeof(struct thread_pool_s));

	pool->_threads = malloc(num_threads*sizeof(struct thr_s));
	pool->_num_threads = 0;
	pool->_cond_count = 0;
	pool->_task_queue = create_ev_pqueue(4);
	pool->_shutdown = 0;
	pool->_enq_index = 0;
	pool->_free_thr_queue = create_ev_pqueue(4);
	{
		sigset_t				set;
		sigset_t				o_set;
		int						sig = 0;
		sigemptyset(&set);
		sigemptyset(&o_set);
		sigaddset(&set,SIGINT);
		//pthread_sigmask(SIG_BLOCK, &set, &o_set);

		for (int i=0; i < num_threads;i++) {
			struct thr_inp_data_s * data = malloc(sizeof(struct thr_inp_data_s));
			data->pool = pool;
			data->thr_index = i;
			if (pthread_create(&pool->_threads[i].t,NULL,thread_loop,data)) {
				destroy_thread_pool(pool);
				return NULL;
			}
			pool->_threads[i].state = THREAD_FREE;
			pool->_threads[i].task_count = 0;
			pool->_num_threads++;
		}

		/*
		for (int i = 0; i < pool->_num_threads; i++)
			printf("CREATED %s:%d %d %p\n",__FILE__,__LINE__,i,pool->_threads[i].t);
		*/
		//o_sigh_ptr = malloc(sizeof(struct  sigaction));
		memset(&sigh,0,sizeof(struct  sigaction));
		sigh.sa_handler = thr_wakeup_sig_handler;
		//sigaction(SIGINT,&sigh,NULL);
		//pthread_sigmask(SIG_SETMASK, &o_set, NULL);
	}


	return pool;
}

void free_thread_pool(struct thread_pool_s *pool)
{
	struct task_s * qe = NULL;

    free(pool->_threads);
	destroy_ev_pqueue(&(pool->_task_queue));
	destroy_ev_pqueue(&(pool->_free_thr_queue));

	return;
}

static void wake_all_threads(struct thread_pool_s *pool, int immediate)
{
	struct task_s * qe = NULL;
	for (int i = 0; i < pool->_num_threads; i++) {
		qe = malloc(sizeof(struct task_s));
		qe->task_function = NULL;
		enqueue_ev_pqueue(pool->_task_queue,qe);
	}
	return;
}

static void wake_any_one_thread(struct thread_pool_s *pool)
{
	return;
}

void enqueue_task(struct thread_pool_s *pool, task_func_type func,
										task_argument_type argument)
{
	struct task_s * qe = NULL;
	qe = malloc(sizeof(struct task_s));
	qe->task_function = func;
	qe->arg = argument;
	enqueue_ev_pqueue(pool->_task_queue,qe);

	atomic_fetch_add(&pool->_cond_count,1);
	wake_any_one_thread(pool);
}

int destroy_thread_pool(struct thread_pool_s *pool)
{
	int s = 0;

	if (!pool) {
		errno = EINVAL;
		return -1;
	}

	s = atomic_load_explicit(&pool->_shutdown,memory_order_relaxed);
	if (s > 0) {
		errno = EBUSY;
		return -1;
	}

	if (!atomic_compare_exchange_strong_explicit(&(pool->_shutdown),&s,s+1,memory_order_relaxed,memory_order_relaxed)) {
		errno = EBUSY;
		return -1;
	}

	wake_all_threads(pool,0);

	for (int i=0;i<pool->_num_threads ; i++) {
		pthread_join(pool->_threads[i].t,NULL);
	}

	free_thread_pool(pool);

	return 0;
}

#ifdef NEVER

#include <stdio.h>

void task_func(void * data)
{
	char * s = data;
	//usleep(10);
	//usleep(25);
	return;
}

struct __s {
	int N;
	struct thread_pool_s *pool;
};

void * enq(void *data)
{
	int i = 0;
	struct __s * sptr = data;
	int N = sptr->N;
	for (i=0; i< N/4; i++) {
		enqueue_task(sptr->pool, task_func, (void*)("WORLD HELLO"));;
		//usleep(25);
	}
	//printf("ENQ complete [%p]\n",pthread_self());
	return NULL;
}

int main()
{
	struct thread_pool_s *pool = NULL;
	int N = 2000000;
	int THREADS = 4;
	int total = 0;
	//int N = 20;
	struct __s s;
	pthread_t t0,t1, t2, t3, t4;
	pool = create_thread_pool(THREADS);
	s.N = N;
	s.pool = pool;

	pthread_create(&t1, NULL, enq, &s);
	pthread_create(&t2, NULL, enq, &s);
	pthread_create(&t3, NULL, enq, &s);
	pthread_create(&t4, NULL, enq, &s);
	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	pthread_join(t3,NULL);
	pthread_join(t4,NULL);
	{
		int i = 0;
		total = 0;
		for (i=0;i<THREADS;i++) {
			total += atomic_load_explicit(&pool->_threads[i].task_count,memory_order_relaxed);
		}
	}

	while (total < N) {
		pthread_yield_np();
		{
			int i = 0;
			total = 0;
			for (i=0;i<THREADS;i++) {
				total += atomic_load_explicit(&pool->_threads[i].task_count,memory_order_relaxed);
			}
		}
	}
	printf("Number of exec times = [%d]\n", total);
	destroy_thread_pool(pool);

	return 0;
}

#endif
