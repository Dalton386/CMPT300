**************************************
EXCUTABLE FILE: assn2

**************************************
How TO USE:

1. enter "make" and "make run" into the terminal, the program will start to run;
2. the data.txt contains the result data, where the first column stands for the trial times, the second column stands for the trial result;
3. after the program exits, you'd better enter "make clean" to clean up files.

**************************************
MEASUREMENT METHODOLOGY:

In this program, I measure the time consumption of FUNCTION_CALL, SYSTEM_CALL, PROCESS_SWITCH and THREAD_SWITCH.
In each part, the trial time increases by certain constant, and the measurement will repeat 10 times under
certain trial time and the average will be calculated as the trial result to be stored in data.txt.

