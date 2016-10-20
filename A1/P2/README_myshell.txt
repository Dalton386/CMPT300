Here is a shell supporting pipes and job control.

How to start:	
	enter command "make", "make run" in the terminal;
CLI like this: 
	"dujiand@asb9820u-d04:/home/dujiand/Documents/CMPT300/A1/P2/DING/CMPT300-A1P2$ ";
How to use: 
	1. you can enter any external command and some internal commands, like "jobs", "bg", "fg", "cd", "exit".
	2. if you append & after a command, this process will run in the background; and you can also suspend a forground process with "Ctrl+Z" and enter "bg" to put it into background;
	3. the "bg" and "fg" focus on the first job in the job list by default, which you can see with "jobs" command. you can choose which job you preferred with a parameter like "bg 5";
	4. after each background process completed, the shell will give the prompt like "[0] Done            ls & ";
	5. if you termiante a forground process with "Ctrl+C", the shell will give the prompt like "[0] Termianted            ./lab ";
How to exit: 
	enter command "exit";
Note:
	1. after exiting from shell, you'd better enter "make clean" to clean up files;
	2. this shell can only support at most 8-pipe-command, and each command can contain 7 arguments, each argument can cantain 31 letters. if you enter illegal command, the shell will show "Too many or too long arguments";
	3. if the command is illegal, the shell will show like "Illegal command: No such file or directory";
	4. if you enter bg/fg command with parameter, but there is no such job, the shell will show "No such job";

Test Samples:
1.cat textfile
	command:
		dujiand@asb9820u-d04:/home/dujiand/Documents/CMPT300/A1/P2/DING/CMPT300-A1P2$ cat textfile
	result:
			a
			b
			c
			d
			f
			g
			h
			j
			j

2. cat textfile | gzip -f | gunzip -k | tail -n 4 
	command:
		dujiand@asb9820u-d04:/home/dujiand/Documents/CMPT300/A1/P2/DING/CMPT300-A1P2$ cat textfile | gzip -f | gunzip -k | tail -n 4 
	result:
			g
			h
			j
			j

3. cat textfile | tail -n 4 &
	command:
		dujiand@asb9820u-d04:/home/dujiand/Documents/CMPT300/A1/P2/DING/CMPT300-A1P2$ cat textfile | tail -n 4 &
	result:
		dujiand@asb9820u-d04:/home/dujiand/Documents/CMPT300/A1/P2/DING/CMPT300-A1P2$ g
			h
			j
			j
	// because the background job continue running unless it needs input from the terminal

4. jobs
	command:
		dujiand@asb9820u-d04:/home/dujiand/Documents/CMPT300/A1/P2/DING/CMPT300-A1P2$ jobs
	result:
		[0] Done            cat textfile | tail | -n | 4 | & 


ACKNOWLEDGE:
	1. The idea for shell initialization and job control derive from the GNU C Library, which instructor mentioned in the FAQ part.
	2. During this project, I once have discussed some concept issues with my classmates.

