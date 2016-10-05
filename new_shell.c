#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <termios.h>

#define TOTINS 8
#define SEPINS 16
#define ARGLEN 32

typedef struct ProL* Ptr2Pro;			//status:
struct ProL {							//
	pid_t pid;
	int status;
	int is_stopped;
	int is_completed;
	char *arg[];
	Ptr2Pro next;
};


typedef struct JobQ* Ptr2Job;
struct JobQ {
	pid_t pgid;
	int status;
	int is_completed;
	int is_stopped;
	Ptr2Pro first_pro;
	Ptr2Job next;
};

struct termios shell_tmodes;

Ptr2Que insertQ(Ptr2Que Wheader, Ptr2Que Wrear, Ptr2Que Wptr);
Ptr2Que searchQ (Ptr2Que Wheader, int jobid);
Ptr2Que deleteQ(Ptr2Que Wheader, Ptr2Que Wrear, int jobid);
int initialQ(Ptr2Que Wheader);
int parse_shell(char (*syscmmd)[SEPINS][ARGLEN]);
int execute_ntrcmmd(char (*syscmmd)[SEPINS][ARGLEN], Ptr2Que Wheader, Ptr2Que Wrear, Ptr2Que Sheader, Ptr2Que Srear, Ptr2Que Nrlist[], int *chnptrW, int *chnptrS);
int xStr2Num(char s[]);
void initial_syscmmd(char (*syscmmd)[SEPINS][ARGLEN]);
void execute_process(char (*separate_syscmmd)[ARGLEN]);
void init_Shell();
void perror(const char *s);
// int execute_pipes();


int main (int argc, char const *argv[]) {
	pid_t pid, pgid;
	char ntrcmmd[5][ARGLEN] = {"ls", "exit", "jobs", "fg", "bg"};
	char syscmmd[TOTINS][SEPINS][ARGLEN];
	int ins_cnt, abs_cnt, pipe_cnt;
	int pfd[2*(TOTINS-1)];
	int pidlist[TOTINS];
	int i, j, k;



	
	return 0;
}

