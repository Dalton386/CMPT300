#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void FindInfo(char *delim, char Infoname[], char InfoURL[], int n, int swtch);

int main(int argc, char const *argv[])
{
        char *delim;
        char KernelURL[4][32] = {"/proc/cpuinfo","/proc/version","/proc/meminfo","/proc/uptime"};
        char KernelInfo[4][32] = {"model name","Kernel Version","MemTotal","Uptime"};
        int i;

        delim = ":\n\r";
        for (i = 0; i < 4; ++i)
        {
                switch(i) {
                        case 0:
                                printf("CPU Model Name:          ");
                                break;                          
                        case 1:
                                printf("Kernel Version:           ");
                                break;
                        case 2:
                                printf("Main Memory Total: ");
                                break;
                        case 3:                 
                                printf("Uptime:                   ");   
                                break;
                        default: break;
                }
                FindInfo(delim, KernelInfo[i], KernelURL[i], strlen(KernelInfo[i]), i);
        }

        return 0;
}

void FindInfo(char *delim, char Infoname[], char InfoURL[], int n, int swtch){
        FILE *f;
        char *seg;
        char str[256];
        int is_find = 0;
        double upt1, upt2;

        if (!(f = fopen(InfoURL, "r")))
        {
                printf("cannot open %s!\n", InfoURL);
                exit(0);
        }

        switch(swtch) {
                case 0:
                case 2:
                        while(!is_find) {
                                fgets(str, 256, f);
                                seg = strtok(str,delim);
                                while(seg){
                                        if(is_find) {
                                                printf("%s\n", seg);
                                                break;
                                        }
                                        if(!strncmp(seg,Infoname,n))
                                                is_find = 1;
                                        seg = strtok(NULL,delim);
                                }
                        }       
                        break;
                case 1:
                        fgets(str, 256, f);
                        printf("%s", str);
                        break;
                case 3:
                        fscanf(f, "%lf %lf", &upt1, &upt2);
                        printf("%.2f Seconds\n", upt1);
                        break;
                default:
                        break;
        }

        if (fclose(f)) {
                printf("cannot close the file!\n");
                exit(0);
        }

}
