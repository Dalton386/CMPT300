#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int parse_shell(char (*syscmmd)[16][32]);
// void initial_syscmmd(char **syscmmd[]);
// int execute_pipes();


int main (int argc, char const *argv[]) {
	pid_t pid;
	char ntrcmmd[3][32] = {"ls", "exit", "jobs"};
	char syscmmd[8][16][32];
	int ins_cnt, abs_cnt;
	int i,j;

	ins_cnt = parse_shell(syscmmd);
	abs_cnt = (ins_cnt > 0) ? ins_cnt : -ins_cnt;

	printf("ins is %d, abs is %d\n", ins_cnt, abs_cnt);
	for (i = 0; i < abs_cnt; ++i)
		for (j = 1; j <= syscmmd[i][0][0]; j++)
			printf("%dth is %s\n", i, syscmmd[i][j]);

	while (1) {											
		for (i = 0; i < abs_cnt; ++i)
		{
			pid = fork();

			if (pid < 0) {
				fprintf(stderr, "Fork Failed");
				return 1;
			}
			else if (pid == 0) {
			// ....


			
			}
			else if (ins_cnt > 0){
				waitpid(pid,NULL,0);
				printf("Child Complete\n");
			}
			else {
				printf("Child Complete\n");
			}
		}
		initial_syscmmd(syscmmd);
		parse_shell(syscmmd);
	}
	return 0;
}

int parse_shell(char (*syscmmd)[16][32]) {
	char cmmd_input[2048];
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
		for (str2 = token ;; str2 = NULL)
		{
			j++;
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


//how to handle the case we pipe internal commmand in command line?