#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <pwd.h>

#define INTCNT 5							//support 5 internal commands
#define TOTINS 8							//support at most 8 pipe commands 
#define SEPINS 16							//each command can contain 15 arguments
#define ARGLEN 32							//each argument can contain 31 letters
#define TERMIN STDIN_FILENO					//the terminal used by this shell is stdin

typedef struct JobQ* Ptr2Job;				//the job record structure
struct JobQ {
	pid_t pgid;								//the pgid of relative process group
	int status;
	int is_completed;						//0-Not, 1-exit or terminated
	int is_stopped;							//0-Not, 1-stopped
	int is_legal;							//0-legal, 1-illegal
	int is_back;							//0-not background, 1-background
	char *cmmd;								//the string of command
	int pidlist[TOTINS];					//track the child process of this job
	int pstatus[TOTINS];					//track the status of each child process 
	int pissorc[TOTINS];					//00-No; 01-stop; 10-exit or terminated
	Ptr2Job next;
};

pid_t shell_pgid;
Ptr2Job Jheader;

int insertQ(Ptr2Job Jheader, Ptr2Job Jptr);
Ptr2Job searchQ (Ptr2Job Jheader, int jobid);
int initialQ(Ptr2Job Jheader);
int parse_shell(char (*syscmmd)[SEPINS][ARGLEN]);
int count_bytes(char (*syscmmd)[SEPINS][ARGLEN]);
int xStr2Num(char s[]);
void append_cmmd(Ptr2Job Jptr, char (*syscmmd)[SEPINS][ARGLEN], char cmmd[]);
void initial_syscmmd(char (*syscmmd)[SEPINS][ARGLEN]);
void execute_process(char (*separate_syscmmd)[ARGLEN], Ptr2Job Jptr);
void init_Shell();
int exe_bg(char (*syscmmd)[ARGLEN], Ptr2Job Jptr);
int exe_fg(char (*syscmmd)[ARGLEN], Ptr2Job Jptr);
int exe_jobs(Ptr2Job Jheader);
int wait_job(Ptr2Job Jptr);
int wait_process(pid_t pid, int status);
int change_status();

int main (int argc, char const *argv[]) {
	pid_t pid, pgid;
	pid_t pidlist[TOTINS];
	char ntrcmmd[INTCNT][ARGLEN] = {"exit", "cd", "jobs", "fg", "bg"};
	char syscmmd[TOTINS][SEPINS][ARGLEN];
	char *cmmd;
	int ins_cnt, abs_cnt, pipe_cnt, bytes_cnt;
	int pfd[2*(TOTINS-1)];
	int i, j, k;
	int status, intrnum;
	const char *errs; 
	Ptr2Job Jptr;

	//initial the job queue and shell execution environment
	Jheader = (Ptr2Job)malloc(sizeof(struct JobQ));
	Jptr = NULL;
	initialQ(Jheader);
	init_Shell();

	while(1) {
		//initial the space to store command and pid
		//calculate the command number and bytes space needed
		memset(pidlist,0,sizeof(pidlist));
		initial_syscmmd(syscmmd);
		ins_cnt = parse_shell(syscmmd);
		bytes_cnt = count_bytes(syscmmd);
		abs_cnt = (ins_cnt > 0) ? ins_cnt : -ins_cnt;
		pipe_cnt = abs_cnt - 1;
		intrnum = -1;

		if(bytes_cnt == 0) {
			printf("Too few arguments\n");
			continue;
		}
		//the command has too many arguments
		if(abs_cnt == 0) {
			printf("Too many or too long arguments\n");
			continue;
		}
		//process internal cmmd or not
		for (i = 0; i < INTCNT; ++i) {
			if (strcmp(syscmmd[0][1], ntrcmmd[i]) == 0) {
				intrnum = i;
				break;
			}
		}
		switch(intrnum) {
			case 0: exit(0);continue;
			case 1: 
				if(chdir(syscmmd[0][2]))
					perror("chdir");
				continue;
			case 2: exe_jobs(Jheader);continue;
			case 3: exe_fg(syscmmd[0],Jheader);continue;
			case 4: exe_bg(syscmmd[0],Jheader);continue;
			default: break;
		}
		//create new job record to track this job
		Jptr = (Ptr2Job)malloc(sizeof(struct JobQ));
		initialQ(Jptr);
		cmmd = (char *)malloc(bytes_cnt*sizeof(char));
		memset(cmmd,0,sizeof(cmmd));
		append_cmmd(Jptr,syscmmd,cmmd);
		insertQ(Jheader,Jptr);
		//fork child process and initial pipes
		for (i = 0; i < pipe_cnt; ++i)
			pipe(pfd+2*i);	
		for (i = 0; i < abs_cnt; ++i)
		{
			pid = fork();
			if (pid < 0) {
				fprintf(stderr, "Fork Failed");
				return 1;
			}
			else if (pid == 0) {
				//set process group
				pid = getpid();
				if(Jptr->pgid == 0)
					Jptr->pgid = pid;
				setpgid(pid, Jptr->pgid);
				//set signal
				signal (SIGINT, SIG_DFL);
			    signal (SIGQUIT, SIG_DFL);
			    signal (SIGTSTP, SIG_DFL);
			    signal (SIGTTIN, SIG_DFL);
			    signal (SIGTTOU, SIG_DFL);
			    signal (SIGCHLD, SIG_DFL);
			    //set pipes
				if (i == 0 && i != abs_cnt-1) {
					close(STDOUT_FILENO);
					dup2(pfd[1], STDOUT_FILENO);
					for (j = 0; j < pipe_cnt; ++j) {				//prevent the leak of open file descripters
						close(pfd[2*j]);
						close(pfd[2*j + 1]);
					}
				}
				else if (i != 0 && i == abs_cnt-1) {
					close(STDIN_FILENO);
					dup2(pfd[2*(i-1)], STDIN_FILENO);
					for (j = 0; j < pipe_cnt; ++j) {
						close(pfd[2*j]);
						close(pfd[2*j + 1]);
					}		
				}
				else if (i != 0 && i != abs_cnt-1) {
					close(STDIN_FILENO);
					close(STDOUT_FILENO);
					dup2(pfd[2*(i-1)], STDIN_FILENO);
					dup2(pfd[2*i + 1], STDOUT_FILENO);
					for (j = 0; j < pipe_cnt; ++j) {
						close(pfd[2*j]);
						close(pfd[2*j + 1]);
					}
				}
				execute_process(syscmmd[i],Jptr);
			}
			else if (pid > 0) {
				pidlist[i] = pid;
				if(Jptr->pgid == 0)
					Jptr->pgid = pid;
				setpgid(pid, Jptr->pgid);
			}
		}
		//parent close the pipes
		for (i = 0; i < pipe_cnt; ++i) {
			close(pfd[2*i]);
			close(pfd[2*i + 1]);
		}
		//forground process
		if (ins_cnt > 0){
			for (i = 0; i < abs_cnt; ++i) 
				Jptr->pidlist[i] = pidlist[i];
			exe_fg(NULL,Jptr);
		}
		//background process
		else if(ins_cnt < 0) {
			for (i = 0; i < abs_cnt; ++i)
				Jptr->pidlist[i] = pidlist[i];
			exe_bg(NULL,Jptr);
		}
	}
	
	return 0;
}

int count_bytes(char (*syscmmd)[SEPINS][ARGLEN]){
	int i, j;
	int bytes_cnt;
	//count the needed bytes space for JQ to store
	bytes_cnt = 0;
	for (i = 0; i < TOTINS; ++i) {
		for(j = 1; j < SEPINS; ++j) {
			if (strlen(syscmmd[i][j]) != 0) {
				if (i != 0 )
					bytes_cnt += 2;
				bytes_cnt += strlen(syscmmd[i][j]) + 1;
			}
			else
				break;
		}
		if (j == 1)
			break;
	}

	return bytes_cnt;
}

void append_cmmd(Ptr2Job Jptr, char (*syscmmd)[SEPINS][ARGLEN], char cmmd[]){
	int i, j;
	//append the command string to the JQ record
	for (i = 0; i < TOTINS; ++i) {
		for(j = 1; j < SEPINS; ++j) {
			if (strlen(syscmmd[i][j]) != 0){
				if (i != 0 )
					strcat(cmmd,"| ");
				strcat(cmmd,syscmmd[i][j]);
				strcat(cmmd," ");
			}
			else
				break;
		}
		if(j == 1)
			break;
	}

	Jptr->cmmd = cmmd;
}

int exe_fg(char (*syscmmd)[ARGLEN], Ptr2Job Jptr){
	int jobid = 0;
	int i;
	//execute the forground process
	//find the relative JQ record
	if (syscmmd != NULL) {
		jobid = xStr2Num(syscmmd[2]);
		if (jobid == 0) 
			jobid = 1;
		Jptr = searchQ(Jptr,jobid);
	}
	if (Jptr == NULL)
		return -1;
	//if it exists, than give it terminal control
	tcsetpgrp (TERMIN, Jptr->pgid);
	Jptr->is_back = 0;
	if (Jptr->is_stopped)
		if (kill (- Jptr->pgid, SIGCONT) < 0) {
			perror("SIGCONT ERROR");
			return -1;
		}
		else {
			Jptr->is_stopped = 0;
			for (i = 0; Jptr->pidlist[i] != 0; ++i)
				Jptr->pissorc[i] &= 2;
		}
	//wait the job until it suspended or exited (terminated)
	wait_job (Jptr);
	//when the job stop running, let shell control terminal again
	tcsetpgrp (TERMIN, shell_pgid);
	return 0;
}

int exe_bg(char (*syscmmd)[ARGLEN], Ptr2Job Jptr){
	int jobid = 0;
	int i;
	//execute job in background
	//find the relative JQ record
	if (syscmmd != NULL) {
		jobid = xStr2Num(syscmmd[2]);
		if (jobid == 0) 
			jobid = 1;
		Jptr = searchQ(Jptr,jobid);
	}
	if (Jptr == NULL)
		return -1;
	//if the job has been stopped, then give it continue signal
	//and update the job status
	if (Jptr->is_stopped)
		if (kill (- Jptr->pgid, SIGCONT) < 0) {
			perror("SIGCONT ERROR");
			return -1;
		}
		else {
			Jptr->is_stopped = 0;
			for (i = 0; Jptr->pidlist[i] != 0; ++i)
				Jptr->pissorc[i] &= 2;
		}

	Jptr->is_back = 1;

	return 0;
}

int exe_jobs(Ptr2Job Jheader) {
	int i;
	Ptr2Job Jptr;
	//execute "jobs" command
	//update all jobs' status and print the job information
	i = 1;
	change_status();
	for (Jptr = Jheader->next; Jptr != NULL; Jptr = Jptr->next) {
		if (Jptr->is_completed != 1) {
			if (Jptr->is_stopped == 1)
				printf("[%d] Stopped         %s\n", i, Jptr->cmmd);
			else
				printf("[%d] Running         %s\n", i, Jptr->cmmd);
			i++;
		}
	}

	return 0;
}

int wait_job(Ptr2Job Jptr) {
	int status;
	pid_t pid;
	//wait until certain job finished or suspended
	//update all running jobs' status in the meanwhile
	do {
		pid = waitpid (WAIT_ANY, &status, WUNTRACED);
	}
	while (wait_process(pid,status) == 0 && Jptr->is_stopped != 1 && Jptr->is_completed != 1);

	return 0;
}

int wait_process(pid_t pid, int status){
	Ptr2Job Jptr;
	int i;
	//update certain process and relative job's status
	//print job information if necessary
	if (pid > 0){
		for (Jptr = Jheader->next; Jptr != NULL; Jptr = Jptr->next) {
			for (i = 0; Jptr->pidlist[i] != 0; ++i) {
				if (Jptr->pidlist[i] == pid) {
					Jptr->pstatus[i] = status;
					if (WIFSTOPPED(status)) {
						Jptr->pissorc[i] |= 1;
						Jptr->is_stopped = 1;
					}
					else {
						Jptr->pissorc[i] |= 2;
						if (WIFSIGNALED(status)) {
							Jptr->is_completed = 1;
							printf("[0] Terminated      %s\n", Jptr->cmmd);
						}
						else if (i+1 >= TOTINS || Jptr->pidlist[i+1] == 0) {
							Jptr->is_completed = 1;
							if(Jptr->is_legal == 0 && Jptr->is_back == 1)
								printf("[0] Done            %s\n",Jptr->cmmd);
						}
					}
					return 0;
				}
			}
		}

		printf("No such process [%d]\n", pid);
		return -1;
	}
	else {
		return -1;
	}
}

int change_status() {
	pid_t pid;
	int status;
	//update all jobs' status without waiting until certain job finished
	do {
		pid = waitpid(WAIT_ANY, &status, WNOHANG|WUNTRACED);
	}
	while (wait_process(pid,status) == 0);

	return 0;
}

int insertQ(Ptr2Job Jheader, Ptr2Job Jptr){
	Ptr2Job Jrear;
	//insert new job record into JQ
	for (Jrear = Jheader; Jrear->next != NULL; Jrear = Jrear->next)
		;
	Jrear->next = Jptr;

	return 0;
}

Ptr2Job searchQ (Ptr2Job Jheader, int jobid){
	Ptr2Job Jptr;
	int i = 0;
	//find the jobid-th uncompleted job in the JQ
	for (Jptr = Jheader->next; Jptr != NULL; Jptr = Jptr->next){
		if (Jptr->is_completed != 1)
			i++;
		if (i == jobid)
			return Jptr;
	}

	printf("No such job\n");
	return NULL;
}

int initialQ(Ptr2Job Jptr){
	//initial the JQ record
	Jptr->pgid = 0;
	Jptr->status = 0;
	Jptr->is_stopped = 0;
	Jptr->is_completed = 0;
	Jptr->is_legal = 0;
	Jptr->is_back = 0;
	Jptr->cmmd = NULL;
	memset(Jptr->pidlist,0,sizeof(Jptr->pidlist));
	memset(Jptr->pstatus,0,sizeof(Jptr->pstatus));
	memset(Jptr->pissorc,0,sizeof(Jptr->pissorc));
	Jptr->next = NULL;

	return 0;
}

void init_Shell() {
  	//initial the shell's execution environment
  	//this part is nearly from the GNU C Library 
  	//without some unnecessary error detection

  	while (tcgetpgrp (TERMIN) != (shell_pgid = getpgrp ()))
    	kill (-shell_pgid, SIGTTIN);

  	signal (SIGINT, SIG_IGN);
  	signal (SIGQUIT, SIG_IGN);
  	signal (SIGTSTP, SIG_IGN);
  	signal (SIGTTIN, SIG_IGN);
  	signal (SIGTTOU, SIG_IGN);

  	shell_pgid = getpid ();
  	setpgid (shell_pgid, shell_pgid);
  	tcsetpgrp (TERMIN, shell_pgid);
}

int xStr2Num(char s[]) {
	int i;
	int number;
	//translate a digit-string into relative number
	number = 0;
	for (i = 0; i < strlen(s); ++i) {
			number *= 10;
			number += s[i] - 48;
	}

	return number;
}

int parse_shell(char (*syscmmd)[SEPINS][ARGLEN]) {
	char cmmd_input[TOTINS*SEPINS*ARGLEN];
	char *str1, *str2, *token, *subtoken;
	char *saveptr1, *saveptr2;
	char *delim1, *delim2;
	int i,j;
	char hostname[80];
	char username[80];
	char diretory[80];
	//get the input from user and return both
	//command string and the number of separate commands
	delim1 = "|\n";
	delim2 = "\t ";								
	//get the configuration and print prompt
	gethostname(hostname,sizeof(hostname));
	getlogin_r(username,sizeof(username));
	getcwd(diretory,sizeof(diretory));
	printf("%s@%s:%s$ ", username, hostname, diretory);
	fgets(cmmd_input, sizeof(cmmd_input), stdin);
	str1 = cmmd_input;
	i = 0;
	//syscmmd[i] means each separate command
	//syscmmd[i][j] means each argument
	//syscmmd[i][0][0] store the argument count of each 
	//separate command
	for (;;str1 = NULL) {
		j = 0;
		token = strtok_r(str1,delim1,&saveptr1);
		if (!token) 
			break;
		
		for (str2 = token ;; str2 = NULL)
		{
			j++;
			subtoken = strtok_r(str2,delim2,&saveptr2);
			if (!subtoken)
				break;
			if(j >= SEPINS || strlen(subtoken) >= ARGLEN)
				return 0;
			strcpy (syscmmd[i][j], subtoken);
		}
		syscmmd[i][0][0] = j-1;						
		i++;
	}
	if (i > TOTINS)
		return 0;
	j = syscmmd[i-1][0][0];
	if (strcmp(syscmmd[i-1][j], "&") == 0) {
		syscmmd[i-1][0][0] = j - 1;
		i = -i;
	}

	return i;
}

void execute_process(char (*separate_syscmmd)[ARGLEN], Ptr2Job Jptr) {
	char *arg[SEPINS];
	int j;
	//execute the external command
	//if the command is illegal, then update the 
	//relative record in JQ and print error information
	for (j = 1; j <= separate_syscmmd[0][0]; ++j) 
		arg[j-1] = separate_syscmmd[j];		

	arg[j-1] = NULL;
	if (Jptr->is_legal == 1 || Jptr->is_completed == 1)
		exit(1);
	if(execvp(separate_syscmmd[1],arg)) {
		perror("Illegal command");
		Jptr->is_legal = 1;
		Jptr->is_completed = 1;
		exit(1);
	}
}

void initial_syscmmd(char (*syscmmd)[SEPINS][ARGLEN]) {
	int i, j;
	//initial the command buffer
	for (i = 0; i < TOTINS; ++i)
		for (j = 0; j < SEPINS; ++j)
			memset(syscmmd[i][j], 0, sizeof(syscmmd[i][j]));
}
