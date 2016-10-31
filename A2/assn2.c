#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_C_REPEATTIME 100000000
#define MAX_P_REPEATTIME 100000
#define MAX_T_REPEATTIME 100000
#define CPUNUM 1
#define ITRTIME 10

pthread_mutex_t LOCK;
struct timespec start_w;
struct timespec stop_w;
unsigned long long result_raw;

void perror(const char *s);

unsigned long long timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
  	return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

void null_function(){}

void *thread1(void *T_REPEATTIME) {
	int i;
	unsigned int *TT_REPEATTIME;

	TT_REPEATTIME = T_REPEATTIME;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start_w);
	for (i = 0; i < *TT_REPEATTIME; ++i) {
		pthread_yield();
	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&stop_w);
}

void *thread2(void *T_REPEATTIME) {
	int i;
	unsigned int *TT_REPEATTIME;

	TT_REPEATTIME = T_REPEATTIME;
	for (i = 0; i < *TT_REPEATTIME; ++i) {
		pthread_yield();
	}
}

double measure_functioncall(unsigned int C_REPEATTIME) {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_fc, result_overhead; //64 bit integer
	double per_fc;
	int i;

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(i = 0; i < C_REPEATTIME; ++i) 
		null_function();
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result_fc =timespecDiff(&stop,&start);

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(i = 0; i < C_REPEATTIME; ++i) 
		;
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result_overhead =timespecDiff(&stop,&start);

	per_fc = ((double)result_fc - (double)result_overhead) / C_REPEATTIME;
	// printf("C_REPEATTIME is %u: per null_fc is %fns\n", C_REPEATTIME, per_fc);
	return per_fc;
}

double measure_systemcall(unsigned int C_REPEATTIME) {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_sc, result_overhead; //64 bit integer
	double per_sc;
	pid_t pid;
	int i;

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(i = 0; i < C_REPEATTIME; ++i) {
		pid = getpid();
	}
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result_sc =timespecDiff(&stop,&start);

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(i = 0; i < C_REPEATTIME; ++i){
		;
	}
	clock_gettime(CLOCK_MONOTONIC, &stop);
	result_overhead =timespecDiff(&stop,&start);

	per_sc = ((double)result_sc - (double)result_overhead) / C_REPEATTIME;
	// printf("C_REPEATTIME is %u: per null_sc is %fns\n", C_REPEATTIME, per_sc);
	return per_sc;
}

double measure_proceswitch(unsigned int P_REPEATTIME) {
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

		close(fdC[1]);
		close(fdP[0]);
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
		
		close(fdC[0]);
		close(fdP[1]);
		wait(NULL);
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

	close(fdP[0]);close(fdP[1]);
	close(fdC[0]);close(fdC[1]);
	per_ps = ((double)result_ps - (double)result_overhead) / (P_REPEATTIME * 2);
	// printf("P_REPEATTIME is %u: per proc_sw is %fns\n", P_REPEATTIME, per_ps);
	return per_ps;
}

double measure_threadswitch(unsigned int T_REPEATTIME) {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_overhead; //64 bit integer
	double per_ts;
	pthread_t tid1, tid2;
	int i;
	cpu_set_t set_t, set_m;

	CPU_ZERO(&set_t);
	CPU_ZERO(&set_m);
	CPU_SET(CPUNUM, &set_t);
	CPU_SET(CPUNUM+2, &set_m);
	pthread_mutex_init(&LOCK, NULL);

	pthread_mutex_lock(&LOCK);
	pthread_create(&tid1, NULL, thread1, &T_REPEATTIME);
	pthread_create(&tid2, NULL, thread2, &T_REPEATTIME);
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

	// printf("T_REPEATTIME is %u: per thrd_sw is %fns\n", T_REPEATTIME, per_ts);
	pthread_mutex_destroy(&LOCK);
	return per_ts;
}

int main()
{	
	int i;
	unsigned int C_REPEATTIME, P_REPEATTIME, T_REPEATTIME;
	double test_data;
	FILE *fp;

	if ((fp = fopen("data.txt","w")) == NULL){
		perror("fopen: ");
		exit(1);
	}

	for (C_REPEATTIME = 5000000; C_REPEATTIME <= MAX_C_REPEATTIME; C_REPEATTIME += 5000000){
		test_data = 0;
		for (i = 0; i < ITRTIME; ++i)
			test_data += measure_functioncall(C_REPEATTIME);
		fprintf(fp, "%u %f\n", C_REPEATTIME, test_data / ITRTIME);
	}
	fprintf(fp, "\n");

	for (C_REPEATTIME = 5000000; C_REPEATTIME <= MAX_C_REPEATTIME; C_REPEATTIME += 5000000){
		test_data = 0;
		for (i = 0; i < ITRTIME; ++i)
			test_data += measure_systemcall(C_REPEATTIME);
		fprintf(fp, "%u %f\n", C_REPEATTIME, test_data / ITRTIME);
	}
	fprintf(fp, "\n");

	fflush(fp);

	for (P_REPEATTIME = 5000; P_REPEATTIME <= MAX_P_REPEATTIME; P_REPEATTIME += 5000){
		test_data = 0;
		for (i = 0; i < ITRTIME; ++i) {
			test_data += measure_proceswitch(P_REPEATTIME);
		}

		fprintf(fp, "%u %f\n", P_REPEATTIME, test_data / ITRTIME);
		fflush(fp);
	}
	fprintf(fp, "\n");

	for (T_REPEATTIME = 5000; T_REPEATTIME <= MAX_T_REPEATTIME; T_REPEATTIME += 5000){
		test_data = 0;
		for (i = 0; i < ITRTIME; ++i)
			test_data += measure_threadswitch(T_REPEATTIME);
		fprintf(fp, "%u %f\n", T_REPEATTIME, test_data / ITRTIME);
	}
	fprintf(fp, "\n");

	if (fclose(fp)) {
		perror("fclose: ");
		exit(1);
	}

	return 0;
}