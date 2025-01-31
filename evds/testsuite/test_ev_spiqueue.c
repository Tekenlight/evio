#include <ev_include.h>
#include <ev_queue.h>
#include <ev_piqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <CuTest.h>

extern void * dequeue_ev_spiqueue(ev_piqueue_type  pq_ptr);
extern ev_piqueue_type create_ev_spiqueue(int n);
extern void enqueue_ev_spiqueue(ev_piqueue_type pq_ptr,void * data);

struct thr_inp {
	ev_piqueue_type pq_ptr;
	int N;
	int slot;
	int dequeued;
};


void * thr(void *param)
{
	struct thr_inp * inp;
	int i = 0;
	int j = 0;
	inp = param;

	ev_piqueue_type pq_ptr = (ev_piqueue_type)inp->pq_ptr;;
	int N = inp->N;
	int slot = inp->slot;
	int COUNT = N/4;

	if (slot == 5) {
		j = 1;
		COUNT = N;
	}
	else {
		j = 1+slot*N/4;
		COUNT = N/4;
	}

	for (i=0;i<COUNT;i++,j++) {
		//printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
		enqueue_ev_spiqueue(pq_ptr,(void*)(long)j);
		//enqueue_ev_piqueue(pq_ptr,(void*)(long)j);
		//usleep(1);
		if (NULL != dequeue_ev_spiqueue(pq_ptr)) inp->dequeued++;
		//dequeue_ev_piqueue(pq_ptr);
		//j++;i++;
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue_ev_piqueue(pq_ptr));fflush(stdout);
	}

	/*
	for (i=0;i<COUNT;i++,j++) {
		//printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
		//enqueue_ev_piqueue(pq_ptr,(void*)(long)j);
		//usleep(1);
		dequeue_ev_piqueue(pq_ptr);
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue_ev_piqueue(pq_ptr));fflush(stdout);
	}
	*/

	//printf("THR1 OVER\n");

	return NULL;
}

void * thr_TEST(void *param)
{
	int i = 0; int j= 0;
	int fd = 0;
	ev_piqueue_type pq_ptr = (ev_piqueue_type)param;
	for (i=0;i<128;i++,j++) {
		enqueue_ev_piqueue(pq_ptr,(void*)(long)j);
		fd = (int)(long)dequeue_ev_piqueue(pq_ptr);
		printf("THREAD_TEST:%d:%d\n",i,fd);fflush(stdout);
	}

	/*
	for (i=0;i<128;i++) {
		printf("THREAD_TEST:%d:%d\n",i,fd);//fflush(stdout);
	}
	*/
	return NULL;
}

static void test_main(CuTest *tc)
{
	int i = 0, fd = 0;;
	pthread_t t0,t1, t2, t3, t4, t5;
	void *retptr = NULL;
	struct thr_inp inp1 ;
	struct thr_inp inp2 ;
	struct thr_inp inp3 ;
	struct thr_inp inp4 ;
	struct thr_inp inp5 ;

	ev_piqueue_type q;

	q = create_ev_spiqueue(4);

	inp1.pq_ptr = q;
	inp1.N = 4*1024*1024;
	inp1.dequeued = 0;
	inp1.slot = 1;

	inp2.pq_ptr = q;
	inp2.N = 4*1024*1024;
	inp2.dequeued = 0;
	inp2.slot = 2;

	inp3.pq_ptr = q;
	inp3.N = 4*1024*1024;
	inp3.dequeued = 0;
	inp3.slot = 3;

	inp4.pq_ptr = q;
	inp4.N = 4*1024*1024;
	inp4.dequeued = 0;
	inp4.slot = 4;

	inp5.pq_ptr = q;
	inp5.N = 4*1024*1024;
	inp5.dequeued = 0;
	inp5.slot = 5;

	/*
	pthread_create(&t0,NULL,thr_TEST,q);
	*/
	inp1.slot = 0;
	pthread_create(&t1, NULL, thr, &inp1);
	inp2.slot = 1;
	pthread_create(&t2, NULL, thr, &inp2);
	inp3.slot = 2;
	pthread_create(&t3, NULL, thr, &inp3);
	inp4.slot = 3;
	pthread_create(&t4, NULL, thr, &inp4);
	/*
	*/
	/*
	inp5.slot = 5;
	pthread_create(&t5, NULL, thr, &inp5);
	*/

	//pthread_join(t0,&retptr);
	pthread_join(t1,&retptr);
	pthread_join(t2,&retptr);
	pthread_join(t3,&retptr);
	pthread_join(t4,&retptr);
	/*
	*/
	/*
	*/
	pthread_join(t5,&retptr);
	//printf("HELLO\n");

	i = 0;
	{
		void * p;
		while (NULL != (p = dequeue_ev_spiqueue(q))) {
			++i;
			fd = (int)(long)p;
			printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
		}
	}


	//destroy_ev_queue(&q);

	//CuAssertTrue(tc, i==0);
	printf("Value 4*1024*1024 = %d\n", 4*1024*1024);
	printf("Dequeued          = %d\n", (i+inp1.dequeued+inp2.dequeued+inp3.dequeued+inp4.dequeued));
	printf("Dequeued          = %d\n", inp5.dequeued);
	CuAssertTrue(tc, ((i+inp1.dequeued+inp2.dequeued+inp3.dequeued+inp4.dequeued) == 4*1024*1024));
}


static int run_ev_test()
{
	CuSuite* suite = CuSuiteNew();
	CuString *output = CuStringNew();

	SUITE_ADD_TEST(suite,test_main);

	CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
	printf("Count = %d\n",suite->count);
	return suite->failCount;
}

int main()
{
	return run_ev_test();
}
