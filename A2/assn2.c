#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>


#define REPEATTIME 10000000

void perror(const char *s);

unsigned long long timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){

	// printf("size of is %d", sizeof(timeA_p->tv_sec));
  return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

void null_function(){}

void measure_functioncall() {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_fc, result_for; //64 bit integer
	double per_fc;
	int i;

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for(i = 0; i < REPEATTIME; ++i)
		null_function();
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stop);
	result_fc =timespecDiff(&stop,&start);

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for(i = 0; i < REPEATTIME; ++i)
		;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stop);
	result_for =timespecDiff(&stop,&start);

	per_fc = ((double)result_fc - (double)result_for)*1.0 / REPEATTIME;
	printf("per null_fc is %fns\n",per_fc);
}

void measure_systemcall() {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_sc, result_for; //64 bit integer
	double per_sc;
	pid_t pid;
	int i;

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for(i = 0; i < REPEATTIME; ++i)
		pid = getpid();
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stop);
	result_sc =timespecDiff(&stop,&start);

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for(i = 0; i < REPEATTIME; ++i)
		;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stop);
	result_for =timespecDiff(&stop,&start);

	per_sc = ((double)result_sc - (double)result_for)*1.0 / REPEATTIME;
	printf("per null_sc is %fns\n",per_sc);
}

void measure_proceswitch() {
	struct timespec start;
	struct timespec stop;
	unsigned long long result_ps, result_overhead; //64 bit integer
	char buffer[1];
	double per_ps;
	pid_t pid;
	int fdP[2], fdC[2], i;
	cpuset

	pipe(fdP);
	pipe(fdC);

	if ((pid = fork()) == -1) {
		perror("fork error: ");
		exit(1);
	}

	// clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	// for (i = 0; i < REPEATTIME; i++) {
	// 	if (pid == 0) {
	// 		close(fdC[0]);
	// 		close(fdP[1]);
	// 		if (i < REPEATTIME) {
	// 			read(fdP[0],buffer,sizeof(buffer));
	// 			write(fdC[1],buffer,strlen(buffer));				
	// 		}
	// 		exit(0);
	// 	}		
	// 	else {
	// 		close(fdC[1]);
	// 		close(fdP[0]);
	// 		if (i < REPEATTIME) {
	// 			read(fdP[0],buffer,sizeof(buffer));
	// 			write(fdC[1],buffer,strlen(buffer));				
	// 		}			
	// 	}
	// }
	// clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stop);
	// result_ps = timespecDiff(&stop, &start);


}

int main()
{
	int j;

	printf("CLOCK_THREAD_CPUTIME_ID Measured: \n");
	j = 0;
	while (j++ != 100) {	
		measure_functioncall();
		measure_systemcall();
	}

	return 0;
}