#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define C_REPEATTIME 100000000
#define P_REPEATTIME 100000
#define T_REPEATTIME 100000
#define CPUNUM 1

// int COUNTER;

pthread_mutex_t LOCK;
pthread_cond_t COND;
struct timespec start_w;
struct timespec stop_w;
// struct timespec start2;
// struct timespec stop2;
unsigned long long result_raw;

void perror(const char *s);

unsigned long long timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
  	return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

void null_function(){}

// void *thread() {
	
// 	pthread_mutex_lock(&LOCK);
// 	for (; COUNTER < T_REPEATTIME; ++COUNTER) {
// 		pthread_cond_signal(&COND);
// 		pthread_cond_wait(&COND, &LOCK);
// 	}
// 	pthread_cond_signal(&COND);
// 	pthread_mutex_unlock(&LOCK);
// }

// void *thread() {
// 	int i;
// 	pthread_mutex_lock(&LOCK);
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		pthread_cond_wait(&COND, &LOCK);
// 	}
// 	pthread_cond_signal(&COND);
// 	pthread_mutex_unlock(&LOCK);
// }

// void *thread1() {
// 	int i;
// 	pthread_mutex_lock(&LOCK);
// 	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&start1);
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		pthread_cond_wait(&COND, &LOCK);
// 	}
// 	pthread_cond_signal(&COND);
// 	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&stop1);
// 	pthread_mutex_unlock(&LOCK);
// }

// void *thread2() {
// 	int i;
// 	pthread_mutex_lock(&LOCK);
// 	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&start2);
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		pthread_cond_wait(&COND, &LOCK);
// 	}
// 	pthread_cond_signal(&COND);
// 	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&stop2);
// 	pthread_mutex_unlock(&LOCK);
// }

// void *thread1() {
// 	int i;
// 	pthread_mutex_lock(&LOCK);
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		clock_gettime(CLOCK_MONOTONIC,&start_w);
// 		pthread_cond_wait(&COND, &LOCK);
// 	}
// 	pthread_cond_signal(&COND);
// 	pthread_mutex_unlock(&LOCK);
// }

// void *thread2() {
// 	int i;
// 	pthread_mutex_lock(&LOCK);
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		pthread_cond_wait(&COND, &LOCK);
// 		clock_gettime(CLOCK_MONOTONIC,&stop_w);
// 		result_raw += timespecDiff(&stop_w,&start_w);
// 	}
// 	pthread_cond_signal(&COND);
// 	pthread_mutex_unlock(&LOCK);
// }

void *thread1() {
	int i;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start_w);
	for (i = 0; i < T_REPEATTIME; ++i) {
		pthread_yield();
	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&stop_w);
}

void *thread2() {
	int i;

	for (i = 0; i < T_REPEATTIME; ++i) {
		pthread_yield();
	}
}

void measure_functioncall() {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_fc, result_overhead; //64 bit integer
	double per_fc;
	int i;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	for(i = 0; i < C_REPEATTIME; ++i)
		null_function();
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	result_fc =timespecDiff(&stop,&start);

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	for(i = 0; i < C_REPEATTIME; ++i)
		;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	result_overhead =timespecDiff(&stop,&start);

	per_fc = ((double)result_fc - (double)result_overhead) / C_REPEATTIME;
	printf("CLOCK_PROCESS Measured: per null_fc is %fns\n",per_fc);
}

void measure_systemcall() {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_sc, result_overhead; //64 bit integer
	double per_sc;
	pid_t pid;
	int i;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	for(i = 0; i < C_REPEATTIME; ++i)
		pid = getpid();
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	result_sc =timespecDiff(&stop,&start);

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	for(i = 0; i < C_REPEATTIME; ++i)
		;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	result_overhead =timespecDiff(&stop,&start);

	per_sc = ((double)result_sc - (double)result_overhead) / C_REPEATTIME;
	printf("CLOCK_PROCESS Measured: per null_sc is %fns\n",per_sc);
}

void measure_proceswitch() {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_ps, result_overhead; //64 bit integer
	char buffer[] = "a";
	double per_ps;
	pid_t pid;
	int fdP[2], fdC[2], i;
	cpu_set_t set;

	pipe(fdP);
	pipe(fdC);

	CPU_ZERO(&set);

	if ((pid = fork()) == -1) {
		perror("fork error: ");
		exit(1);
	}

	if (pid == 0) {
		CPU_SET(CPUNUM, &set);
		if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) {
			perror("sched_setaffinity error: ");
			exit(1);
		}
		close(fdC[0]);
		close(fdP[1]);

		for (i = 0; i < P_REPEATTIME; i++) {
			read(fdP[0],buffer,sizeof(buffer));
			write(fdC[1],buffer,strlen(buffer));								
		}
		exit(0);
	}		
	else {
		CPU_SET(CPUNUM, &set);
		if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) {
			perror("sched_setaffinity error: ");
			exit(1);
		}
		close(fdC[1]);
		close(fdP[0]);

		clock_gettime(CLOCK_MONOTONIC, &start);
		for (i = 0; i < P_REPEATTIME; i++) {
			write(fdP[1],buffer,strlen(buffer));				
			read(fdC[0],buffer,sizeof(buffer));
		}		
		clock_gettime(CLOCK_MONOTONIC, &stop);
	}
	result_ps = timespecDiff(&stop, &start);

	pipe(fdP);
	pipe(fdC);
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < P_REPEATTIME; i++) {
		write(fdP[1],buffer,strlen(buffer));				
		read(fdP[0],buffer,sizeof(buffer));	
	}
	for (i = 0; i < P_REPEATTIME; i++) {
		write(fdC[1],buffer,strlen(buffer));				
		read(fdC[0],buffer,sizeof(buffer));	
	}	
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result_overhead = timespecDiff(&stop, &start);

	double raw_ps, raw_overhead;
	raw_ps = result_ps * 1.0 / P_REPEATTIME;
	raw_overhead = result_overhead * 1.0 / P_REPEATTIME;
	printf("result_ps is %f, result_overhead is %f\n", raw_ps, raw_overhead);
	per_ps = ((double)result_ps - (double)result_overhead) / (P_REPEATTIME * 2);
	printf("CLOCK_MONOTONIC Measured: per proc_sw is %fns\n", per_ps);
}

// void measure_threadswitch() {
// 	struct timespec start;
// 	struct timespec stop;
// 	unsigned long long result_ts, result_overhead; //64 bit integer
// 	double per_ts;
// 	pthread_t tid;
// 	int i;
// 	cpu_set_t set;

// 	CPU_ZERO(&set);
// 	CPU_SET(CPUNUM, &set);
// 	pthread_mutex_init(&LOCK, NULL);
// 	pthread_cond_init(&COND, NULL);

// 	pthread_mutex_lock(&LOCK);
// 	pthread_create(&tid, NULL, thread, NULL);
// 	pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
// 	pthread_setaffinity_np(tid, sizeof(set), &set);
// 	pthread_mutex_unlock(&LOCK);

// 	pthread_mutex_lock(&LOCK);
// 	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start);
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		pthread_cond_wait(&COND, &LOCK);
// 	}
// 	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&stop);
// 	pthread_mutex_unlock(&LOCK);
// 	result_ts = timespecDiff(&stop, &start);

// 	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start);
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		pthread_mutex_lock(&LOCK);
// 		pthread_cond_signal(&COND);
// 		pthread_mutex_unlock(&LOCK);
// 	}
// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		pthread_cond_signal(&COND);
// 		pthread_mutex_lock(&LOCK);
// 		pthread_cond_signal(&COND);
// 		pthread_mutex_unlock(&LOCK);
// 	}
// 	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&stop);
// 	result_overhead = timespecDiff(&stop, &start);

// 	printf("result_ts is %llu, result_overhead is %llu\n", result_ts, result_overhead);
// 	per_ts = ((double)result_ts - (double)result_overhead) / (T_REPEATTIME * 2);
// 	printf("CLOCK_PROCESS   Measured: per thrd_sw is %fns\n", per_ts);

// 	pthread_join(tid, NULL);
// 	pthread_mutex_destroy(&LOCK);
// 	pthread_cond_destroy(&COND);
// }

// void measure_threadswitch() {
// 	struct timespec start;
// 	struct timespec stop;
// 	unsigned long long result_ts, result_overhead; //64 bit integer
// 	double per_ts;
// 	pthread_t tid1, tid2;
// 	int i;
// 	cpu_set_t set_t, set_m;

// 	// COUNTER = 0;
// 	CPU_ZERO(&set_t);
// 	CPU_ZERO(&set_m);
// 	CPU_SET(CPUNUM, &set_t);
// 	CPU_SET(CPUNUM+2, &set_m);
// 	pthread_mutex_init(&LOCK, NULL);
// 	pthread_cond_init(&COND, NULL);

// 	pthread_mutex_lock(&LOCK);
// 	pthread_create(&tid1, NULL, thread1, NULL);
// 	pthread_create(&tid2, NULL, thread2, NULL);
// 	pthread_setaffinity_np(tid1, sizeof(set_t), &set_t);
// 	pthread_setaffinity_np(tid2, sizeof(set_t), &set_t);
// 	pthread_setaffinity_np(pthread_self(), sizeof(set_m), &set_m);

// 	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start);
// 	pthread_mutex_unlock(&LOCK);
// 	pthread_join(tid1, NULL);
// 	pthread_join(tid2, NULL);
// 	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&stop);
// 	result_ts = timespecDiff(&stop, &start);

// 	result_overhead = (timespecDiff(&stop1, &start1) + timespecDiff(&stop2, &start2));

// 	printf("result_ts is %llu, result_overhead is %llu\n", result_ts, result_overhead);
// 	per_ts = ((double)result_ts - (double)result_overhead) / (T_REPEATTIME);
// 	printf("CLOCK_PROCESS Measured: per thrd_sw is %fns\n", per_ts);

// 	pthread_mutex_destroy(&LOCK);
// 	pthread_cond_destroy(&COND);
// }

// void measure_threadswitch() {
// 	struct timespec start;
// 	struct timespec stop;
// 	unsigned long long result_overhead; //64 bit integer
// 	double per_ts;
// 	pthread_t tid1, tid2;
// 	int i;
// 	cpu_set_t set_t, set_m;

// 	// COUNTER = 0;
// 	CPU_ZERO(&set_t);
// 	CPU_ZERO(&set_m);
// 	CPU_SET(CPUNUM, &set_t);
// 	CPU_SET(CPUNUM+2, &set_m);
// 	pthread_mutex_init(&LOCK, NULL);
// 	pthread_cond_init(&COND, NULL);

// 	pthread_mutex_lock(&LOCK);
// 	pthread_create(&tid1, NULL, thread1, NULL);
// 	pthread_create(&tid2, NULL, thread2, NULL);
// 	pthread_setaffinity_np(tid1, sizeof(set_t), &set_t);
// 	pthread_setaffinity_np(tid2, sizeof(set_t), &set_t);
// 	pthread_setaffinity_np(pthread_self(), sizeof(set_m), &set_m);
// 	pthread_mutex_unlock(&LOCK);

// 	result_raw = 0;
// 	result_overhead = 0;

// 	pthread_join(tid1, NULL);
// 	pthread_join(tid2, NULL);

// 	for (i = 0; i < T_REPEATTIME; ++i) {
// 		clock_gettime(CLOCK_MONOTONIC,&start);
// 		pthread_mutex_lock(&LOCK);
// 		pthread_cond_signal(&COND);
// 		pthread_mutex_unlock(&LOCK);
// 		clock_gettime(CLOCK_MONOTONIC,&stop);
// 		result_overhead += timespecDiff(&stop, &start);
// 	}


// 	per_ts = (result_raw - result_overhead) * 1.0 / T_REPEATTIME;

// 	printf("CLOCK_MONOTONIC Measured: per thrd_sw is %fns\n", per_ts);

// 	pthread_mutex_destroy(&LOCK);
// 	pthread_cond_destroy(&COND);
// }

void measure_threadswitch() {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_overhead; //64 bit integer
	double per_ts;
	pthread_t tid1, tid2;
	int i;
	cpu_set_t set_t, set_m;

	// COUNTER = 0;
	CPU_ZERO(&set_t);
	CPU_ZERO(&set_m);
	CPU_SET(CPUNUM, &set_t);
	CPU_SET(CPUNUM+2, &set_m);
	pthread_mutex_init(&LOCK, NULL);
	pthread_cond_init(&COND, NULL);

	pthread_mutex_lock(&LOCK);
	pthread_create(&tid1, NULL, thread1, NULL);
	pthread_create(&tid2, NULL, thread2, NULL);
	pthread_setaffinity_np(tid1, sizeof(set_t), &set_t);
	pthread_setaffinity_np(tid2, sizeof(set_t), &set_t);
	pthread_setaffinity_np(pthread_self(), sizeof(set_m), &set_m);
	pthread_mutex_unlock(&LOCK);

	result_raw = 0;
	result_overhead = 0;

	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);

	clock_gettime(CLOCK_MONOTONIC,&start);
	for (i = 0; i < T_REPEATTIME; ++i) 
		;
	for (i = 0; i < T_REPEATTIME; ++i) 
		;
	clock_gettime(CLOCK_MONOTONIC,&stop);

	result_raw = timespecDiff(&stop_w, &start_w);
	result_overhead = timespecDiff(&stop, &start);

	per_ts = (result_raw - result_overhead) * 1.0 / (T_REPEATTIME * 2);

	printf("CLOCK_MONOTONIC Measured: per thrd_sw is %fns\n", per_ts);

	pthread_mutex_destroy(&LOCK);
	pthread_cond_destroy(&COND);
}

int main()
{
	int j;

	for (j = 0; j < 10; ++j)
		measure_functioncall();
	printf("\n");
	for (j = 0; j < 10; ++j)
		measure_systemcall();
	printf("\n");
	for (j = 0; j < 10; ++j)
		measure_proceswitch();
	printf("\n");
	for (j = 0; j < 10; ++j)
		measure_threadswitch();

	return 0;
}
