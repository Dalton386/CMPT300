#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

#define TOTINS 8
#define SEPINS 16
#define ARGLEN 32

typedef struct WaitingQueue* Ptr2Que;
struct WaitingQueue {
	int pidlist[TOTINS];
	int pcnt;
	int stpid;
	Ptr2Que next;
};

Ptr2Que insertQ(Ptr2Que Wheader, Ptr2Que Wrear, Ptr2Que Wptr);
Ptr2Que searchQ (Ptr2Que Wheader, int jobid);
int initialQ(Ptr2Que Wheader);
int deleteQ(Ptr2Que Wheader, int jobid);
int parse_shell(char (*syscmmd)[SEPINS][ARGLEN]);
Ptr2Que execute_ntrcmmd(char (*syscmmd)[SEPINS][ARGLEN], Ptr2Que Wheader, Ptr2Que Wrear, Ptr2Que Sheader, Ptr2Que Srear, int *chnptr);
int xStr2Num(char s[]);
void initial_syscmmd(char (*syscmmd)[SEPINS][ARGLEN]);
void execute_process(char (*separate_syscmmd)[ARGLEN]);
void perror(const char *s);
// int execute_pipes();


int main (int argc, char const *argv[]) {
	pid_t pid;
	char ntrcmmd[3][ARGLEN] = {"ls", "exit", "jobs"};
	char syscmmd[TOTINS][SEPINS][ARGLEN];
	int ins_cnt, abs_cnt, pipe_cnt;
	int pfd[2*(TOTINS-1)];
	int pidlist[TOTINS];
	int i, j, status, chnptr;
	Ptr2Que Wheader, Wrear, Wptr;
	Ptr2Que Sheader, Srear, Sptr;
	Ptr2Que Nptr;
	const char *errs; 
	char readbuffer[1], inputBuffer[1];

	Wptr = Sptr = NULL;
	Wheader = Wrear = (Ptr2Que)malloc(sizeof(struct WaitingQueue));
	Sheader = Srear = (Ptr2Que)malloc(sizeof(struct WaitingQueue));
	initialQ(Wheader);
	initialQ(Sheader);

	while (1) {	
		chnptr = 0;
		memset(pidlist,0,sizeof(pidlist));
		initial_syscmmd(syscmmd);
		ins_cnt = parse_shell(syscmmd);
		abs_cnt = (ins_cnt > 0) ? ins_cnt : -ins_cnt;
		pipe_cnt = abs_cnt - 1;

		printf("**ins is %d, abs is %d\n", ins_cnt, abs_cnt);
		for (i = 0; i < abs_cnt; ++i)
			for (j = 1; j <= syscmmd[i][0][0]; j++)
				printf("**%dth is %s\n", i, syscmmd[i][j]);
		printf("---------------------\n");

		Nptr = execute_ntrcmmd(syscmmd,Wheader,Wrear,Sheader,Srear,&chnptr);
																	//cmmd is internal and has been executed successfully
		if (chnptr == 1) {
			Wrear = Nptr;
			continue;
		}									
		else if (chnptr == 2) {
			Srear = Nptr;
			continue;
		}
		else if (chnptr == -1) {
			continue;
		}									

		for (i = 0; i < pipe_cnt; ++i)
			pipe(pfd+2*i);	

		for (i = 0; i < abs_cnt; ++i)
		{
			printf("0\n");
			pid = fork();
			printf("1\n");
			pidlist[i] = pid;
			printf("i is %d, pid is %d\n", i, pid);
			if (pid < 0) {
				fprintf(stderr, "Fork Failed");
				return 1;
			}
			else if (pid == 0) {
				kill(getpid(), SIGSTOP);
				write(pfd[1], inputBuffer, 0);
				if (i == 0 && i != abs_cnt-1) {
					printf("2\n");
					dup2(pfd[1], 1);
					for (j = 0; j < pipe_cnt; ++j) {				//prevent the leak of open file descripters
						close(pfd[2*j]);
						close(pfd[2*j + 1]);
					}
				}
				else if (i != 0 && i == abs_cnt-1) {
					printf("3\n");		
					dup2(pfd[2*(i-1)], STDIN_FILENO);
					for (j = 0; j < pipe_cnt; ++j) {
						close(pfd[2*j]);
						close(pfd[2*j + 1]);
					}		
				}
				else if (i != 0 && i != abs_cnt-1) {
					printf("4\n");
					dup2(pfd[2*(i-1)], STDIN_FILENO);
					dup2(pfd[2*i + 1], STDOUT_FILENO);
					for (j = 0; j < pipe_cnt; ++j) {
						close(pfd[2*j]);
						close(pfd[2*j + 1]);
					}
				}

				execute_process(syscmmd[i]);
			}
			printf("xxxxxxxxxxxxxxxxxxxx\n");
		}
		for (i = 0; i < abs_cnt; ++i) {
			kill(pidlist[i], SIGCONT);
			read(pfd[0], readbuffer, 0);
		}
		for (i = 0; i < pipe_cnt; ++i) {
			close(pfd[2*i]);
			close(pfd[2*i + 1]);
		}
		if (ins_cnt > 0){
			for (i = 0; i < abs_cnt; ++i) {
				printf("5\n");
				waitpid(pidlist[i], &status, WUNTRACED);
				printf("6\n");
				if(WIFSTOPPED(status) == true) {
					printf("7\n");
					Sptr = (Ptr2Que)malloc(sizeof(struct WaitingQueue));
					for (j = 0; j < abs_cnt; ++j) 
						Sptr->pidlist[j] = pidlist[j];
					Sptr->pcnt = abs_cnt;
					Sptr->stpid = i;
					Srear = insertQ(Sheader,Srear,Sptr);
					printf("process[%d] has been stopped\n", pidlist[i] );
					break;
				}
				else
					printf("process[%d] has completed\n", pidlist[i]);
				printf("8\n");
			}
		}
		else {
			Wptr = (Ptr2Que)malloc(sizeof(struct WaitingQueue));
			for (i = 0; i < abs_cnt; ++i) 
				Wptr->pidlist[i] = pidlist[i];
			Wptr->pcnt = abs_cnt;
			Wptr->stpid = 0;
			Wrear = insertQ(Wheader,Wrear,Wptr);
			// perror(errs);
			printf("Let's continue!\n");
		}
	}
	return 0;
}

int parse_shell(char (*syscmmd)[SEPINS][ARGLEN]) {
	char cmmd_input[TOTINS*SEPINS*ARGLEN];
	char *str1, *str2, *token, *subtoken;
	char *saveptr1, *saveptr2;
	char *delim1, *delim2;
	int i,j;

	delim1 = "|\n\r\f\v";
	delim2 = "\t ";								//maybe not right.

	fgets(cmmd_input, sizeof(cmmd_input), stdin);
	str1 = cmmd_input;
	i = 0;

	for (;;str1 = NULL) {
		j = 0;
		token = strtok_r(str1,delim1,&saveptr1);
		if (!token) 
			break;
		if (i != 0) 
			strcpy (syscmmd[i][2],"STDIN");
		
		for (str2 = token ;; str2 = NULL)
		{
			j++;
			if (j == 2 && i != 0) 
				continue;
			subtoken = strtok_r(str2,delim2,&saveptr2);
			if (!subtoken)
				break;
			strcpy (syscmmd[i][j], subtoken);
		}
		syscmmd[i][0][0] = j-1;						// the number of each sub commands, when piped, the first argument is missed, you can add it when set argv for execvp
		i++;
	}

	j = syscmmd[i-1][0][0];
	if (strcmp(syscmmd[i-1][j], "&") == 0)
	{
		syscmmd[i-1][0][0] = j - 1;
		i = -i;
	}

	return i;
}

void initial_syscmmd(char (*syscmmd)[SEPINS][ARGLEN]) {
	int i, j;

	for (i = 0; i < TOTINS; ++i)
		for (j = 0; j < SEPINS; ++j)
			memset(syscmmd[i][j], 0, sizeof(syscmmd[i][j]));
}

void execute_process(char (*separate_syscmmd)[ARGLEN]) {
	char *arg[SEPINS];
	int j;

	for (j = 1; j <= separate_syscmmd[0][0]; ++j) {
		if (strcmp(separate_syscmmd[j], "STDIN") == 0)
			arg[j-1] = STDIN_FILENO;		
		else
			arg[j-1] = separate_syscmmd[j];		
	}

	arg[j-1] = NULL;
	printf("execute, process is %s\n", separate_syscmmd[1]);
	execvp(separate_syscmmd[1],arg);
}

int initialQ(Ptr2Que Wheader) {
	memset(Wheader->pidlist,0,sizeof(Wheader->pidlist));
	Wheader->next = NULL;
	Wheader->pcnt = 0;
	Wheader->stpid = 0;

	return 0;
}

int deleteQ (Ptr2Que Wheader, int jobid) {
	Ptr2Que Wptr_f, Wptr_l, Wptr_n;
	int i;

	if (Wheader->next == NULL)
		return 0;

	Wptr_f = Wheader;										//the first job 
	Wptr_l = Wptr_f->next;
	if (Wptr_l == NULL)
		return 1;

	for (i = 1; i < jobid; ++i) {
		Wptr_f = Wptr_f->next;
		Wptr_l = Wptr_f->next;
		if (Wptr_l == NULL)
			return 1;										//the target job doesn't exist
	}

	Wptr_n = Wptr_l->next;
	Wptr_f->next = Wptr_n;
	Wheader->pcnt--;

	free(Wptr_l);

	return 0;
}

Ptr2Que insertQ(Ptr2Que Wheader, Ptr2Que Wrear, Ptr2Que Wptr) {
	Wptr->next = NULL;
	Wrear->next = Wptr;
	Wrear = Wptr;
	Wheader->pcnt++;

	return Wrear;
}

Ptr2Que searchQ (Ptr2Que Wheader, int jobid) {
	Ptr2Que Wptr;
	int i;

	if (Wheader->next == NULL)
		return NULL;

	Wptr = Wheader;
	for (i = 0; i < jobid; ++i) {
		Wptr = Wptr->next;
		if (Wptr == NULL)
			return NULL;									//the target job doesn't exist
	}

	return Wptr;	
}

int xStr2Num(char s[]) {
	int i;
	int number;

	number = 0;
	for (i = 0; i < strlen(s); ++i) {
			number *= 10;
			number += s[i] - 48;
	}

	return number;
}

Ptr2Que execute_ntrcmmd(char (*syscmmd)[SEPINS][ARGLEN], Ptr2Que Wheader, Ptr2Que Wrear, Ptr2Que Sheader, Ptr2Que Srear, int *chnptr) {
	int i,j;
	int jobid, status, is_stopped;
	Ptr2Que Sptr, Wptr, Nptr, Nrear;

	Sptr = Sheader;
	Wptr = Wheader;
	Nptr = Nrear = NULL;
	is_stopped = 1;
	*chnptr = -1;
	if(strcmp(syscmmd[0][1], "exit") == 0) {					// exit
		exit(0);
	}
	else if(strcmp(syscmmd[0][1], "jobs") == 0) {				//jobs
		for(i = 1; ; ++i) {
			if (Sptr->next == NULL)
				break;
			Sptr = Sptr->next;
			printf("[%d] Stopped     1st process is %d\n", i, Sptr->pidlist[0]);
		}
		for(i = 1; ; ++i) {
			if (Wptr->next == NULL)
				break;
			Wptr = Wptr->next;
			printf("[%d]             1st process is %d\n", i+Sheader->pcnt, Wptr->pidlist[0]);
		}

	}
	else if(strcmp(syscmmd[0][1], "cd") == 0) {					//cd
		chdir(syscmmd[0][2]);

	}
	else if(strcmp(syscmmd[0][1], "fg") == 0) {					//fg, transfer jobs from SQ or WQ to continue
		if (syscmmd[0][0][0] == 1)
			jobid = 1;
		else
			jobid = xStr2Num(syscmmd[0][2]);

		if (jobid <= Sheader->pcnt) {
			Sptr = searchQ(Sheader,jobid);
			if (Sptr == NULL) {
				printf("There is no such job\n");
				return NULL;
			}

			for (i = Sptr->stpid; i < Sptr->pcnt; ++i) {
				waitpid(Sptr->pidlist[0], &status, WUNTRACED);
				if(WIFSTOPPED(status) == true) {
					is_stopped = 0;
					Sptr->stpid = i;
					printf("process[%d] has been stopped\n", Sptr->pidlist[i]);
					break;
				}
				else
					printf("process[%d] has completed\n", Sptr->pidlist[i]);
			}

			if (is_stopped != 0)
				deleteQ(Sheader,jobid);
		}
		else {
			Wptr = searchQ(Wheader,jobid - Sheader->pcnt);
			if (Wptr == NULL) {
				printf("There is no such job\n");
				return NULL;
			}

			for (i = Wptr->stpid; i < Wptr->pcnt; ++i) {
				waitpid(Wptr->pidlist[0], &status, WUNTRACED);
				if(WIFSTOPPED(status) == true) {
					Nptr = (Ptr2Que)malloc(sizeof(struct WaitingQueue));
					for (j = 0; j < Wptr->pcnt; ++j) 
						Nptr->pidlist[j] = Wptr->pidlist[j];
					Nptr->pcnt = Wptr->pcnt;
					Nptr->stpid = i;
					Nrear = insertQ(Sheader,Srear,Nptr);
					*chnptr = 2;
					printf("process[%d] has been stopped\n", Wptr->pidlist[i]);
					break;
				}
				else
					printf("process[%d] has completed\n", Wptr->pidlist[i]);
			}

			deleteQ(Wheader,jobid - Sheader->pcnt);
		}

	}
	else if(strcmp(syscmmd[0][1], "bg") == 0) {					//bg, transfer jobs from SQ to WQ and continue
		if (syscmmd[0][0][0] == 1)
			jobid = 1;
		else
			jobid = xStr2Num(syscmmd[0][2]);

		if (Sheader->pcnt < jobid) {
			printf("There is no such job\n");
			return NULL;
		}

		Sptr = searchQ(Sheader,jobid);
		Nptr = (Ptr2Que)malloc(sizeof(struct WaitingQueue));
		for (j = 0; j < Sptr->pcnt; ++j) 
			Nptr->pidlist[j] = Sptr->pidlist[j];
		Nptr->pcnt = Sptr->pcnt;
		Nptr->stpid = Sptr->stpid;
		Nrear = insertQ(Wheader,Wrear,Nptr);
		*chnptr = 1;
		deleteQ(Sheader,jobid);

		kill(Nptr->pidlist[Nptr->stpid], SIGCONT);
	}
	else {
		*chnptr = 0;
		return NULL;												//cmmd is not internal 
	}

	return Nrear;
}
