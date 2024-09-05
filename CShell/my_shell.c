//
// Created by colton on 2/9/2024.
//
#include  <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokens
*
*/

char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for(i =0; i < strlen(line); i++){

        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t' || readChar == '\r'){ //issue resolved ; was getting '\r' read into batch mode for some reason
            token[tokenIndex] = '\0';
            if (tokenIndex != 0){
                tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }

        
    free(token);
    tokens[tokenNo] = NULL ;
    return tokens;
}


int main(int argc, char* argv[]) {
    char  line[MAX_INPUT_SIZE];
    char  **tokens;
    int i;

    FILE* fp;
    if(argc == 2) {
        fp = fopen(argv[1],"r");
        if(fp < 0) {
            printf("File doesn't exists.");
            return -1;
        }
    }
    pid_t *bg = (pid_t*) malloc(64*sizeof(pid_t)); //pointer to hold pids of all background processes; max 64
    int num_bg = 0; //number of background processes
    while(1) {
        //signal(SIGINT, SIG_IGN);
        /*Background Process; clean up zombie processes from background execution
        */
        if(waitpid(-1,NULL,WNOHANG) > 0){
            printf("Shell: Background process finished \n");
            num_bg--; //background process has terminated so decrement
        }

        signal(SIGINT, SIG_IGN); //Shell process ignore CTRL-C

        /* BEGIN: TAKING INPUT */
        bzero(line, sizeof(line));
        if(argc == 2) { // batch mode
            if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
                break;
            }
            line[strlen(line) - 1] = '\0';
        } else { // interactive mode
            printf("$ ");
            scanf("%[^\n]", line);
            getchar();
        }
        //printf("Command entered: %s (remove this debug output later)\n", line);
        /* END: TAKING INPUT */

        line[strlen(line)] = '\n'; //terminate with new line
        tokens = tokenize(line);
        
        int parallel = 0;         //if the child processes need to execute in parallel in the foreground
        int run_background = 0;  //if the child processes need to operate in the background
        int run_queue = 0;       //number of processes or commands we need to fork for, i.e. serial mode
        int parallel_child = 1; //number of commands to be run in parallel
        int index[64];          //holds number of locations after each '&&'
        int idx = 0;            // For index of next arg
        index[idx] = 0;         //initialized the first value in index array to 0 in case of just 1 command token

        //do whatever you want with the commands, here we just print them
        int tokens_length = 0;
        for(i=0;tokens[i]!=NULL;i++){
            //printf("found token %s (remove this debug output later)\n", tokens[i]);   
            tokens_length++;
            //background
            if(!strcmp(tokens[i],"&")){
                run_background = 1;
                tokens[i] = NULL;
            }
            //serial
            else if(!strcmp(tokens[i],"&&")){    
                run_queue++;
                idx++;
                index[idx] = i+1;
                tokens[i] = NULL;

            }
            //parallel
            else if(!strcmp(tokens[i],"&&&")){
                run_background = 1;
                parallel = 1;
                parallel_child++;
                idx++;
                index[idx] = i+1;
                tokens[i] = NULL;

            }

        }
        
        /*Parse tokens for non-exec commands
        */
        if(tokens[0] == NULL){
            //Do nothing
        }
        /*Exit command for shell; kill processes and clean up
        */
        else if(!strcmp(tokens[0],"exit")){

            //Free memory
            for(i=0;tokens[i]!=NULL;i++){
                free(tokens[i]);
            }
            free(tokens);
            
            if(num_bg > 0){ //KILL all background processes
                for(int i = 0; i < num_bg; i++){
                    kill(bg[i],SIGKILL);
                }
            }
            free(bg); //free bg array
            exit(0); //Exit program
        }
        
        /*cd command calls chdir(dest) instead of forking and calling execvp
        */
        else if(!strcmp(tokens[0],"cd")){
                if(tokens[1] == NULL){
                    //Do nothing
                }
                //if there are too many args alert user and continue
                else if(tokens[2] != NULL){  
                    printf("Shell: cd: too many arguments\n");
                }
                else{
                    char  *cwdDir;
                    char  *src;
                    char  *dest;
                    char cwd[MAX_TOKEN_SIZE];

                    cwdDir = getcwd(cwd, sizeof(cwd));
                    src = strcat(cwdDir, "/");
                    dest = strcat(src, tokens[1]);

                    int cdRet = chdir(dest);
                    if(cdRet != 0){
                        printf("Shell: cd: %s: %s\n",tokens[1],strerror(errno));
                    }
                }
        }

        /*Regular input, fork() and send input to execvp()*/
        else{
            run_queue++; //number of commands needed to be run
            int t = 0; //Index for index array holding indices in tokens to the next argument needed to execute
            while(run_queue > 0){
                signal(SIGINT, SIG_DFL);
                int status;
                pid_t pid;
                int x; //count for amount of times we need to fork for parallel processing
                if(parallel_child == 1){ //default ;not in parallel execution
                    x = 1;
                }
                else{
                    x = parallel_child;
                } 
                for(int j = 0; j < x; j++){
                    pid=fork();
                    if(run_background && (parallel == 0)){ //save background process PID to array for killing and signal handling
                        bg[num_bg++] = pid;
                        signal(SIGINT,SIG_IGN);
                    }
                    switch((pid)){   
                        case 0: //child
                        //signal(SIGINT, SIG_DFL);
                        //printf("\nChild %d : %d\n",j,getpid());   
                            if(run_background && (parallel == 0)){ //for Ctrl+C signal handling
                                setpgid(pid,0);
                                //signal(SIGINT, SIG_IGN);
                            }
                            //printf("PPID: %d  PID: %d  PGID %d\n",getppid(),getpid(),getpgrp()); //debug
                            if(execvp(tokens[index[t]],tokens+index[t])){ //If execvp returns non-0, raise error and exit(errno)
                                printf("Shell: Incorrect Command\n");
                                exit(errno);
                            }
                            break;
                        case -1:
                            printf("Shell: Fork Failed\n");
                            break;
                        
                        default: //parent
                            signal(SIGINT, SIG_IGN);
                            if(!run_background){
                                waitpid(pid,&status,0);
                                if(status != 0){
                                    //printf("Shell: child: %d: %s\n",pid,strerror(errno)); //Debug
                                }
                            }

                    }
            

                t++; //increment the index of index array + 1 to the index of the next value
                run_queue--; //decrement run_queue for while loop
                }
            }
            //If we are in parallel mode, wait for all children after each of them have started execution
            if(parallel){
                for(int a = 0; a < parallel_child; a++){
                    //printf("WAITING\n");
                    wait(NULL);
                    }
                }

        }
        // Freeing the allocated memory
        for(i=0;tokens[i]!=NULL;i++){
            free(tokens[i]);
        }
        free(tokens);                
    }
    return 0;
}

