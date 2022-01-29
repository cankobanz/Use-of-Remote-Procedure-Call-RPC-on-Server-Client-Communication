 /* @file part_a.c
 * @author Can Koban
 *
 * @brief Project 1 part_a solution is following. In this part, Parent capture the inputs. 
 * Directs inputs (two integer in string format) to child through a pipe. Child calls blackbox 
 * executable with execl() function. With dup2() method stdin is replaced with read end for child pipe. 
 * Therefore, child reads from pipe end (Actually blackbox reads thanks to the execl function).
 * Result of the child redirected with dup2() method to the pipe where stdout and stderr file descriptors are 
 * replaced with write pipe end for child. Parent read results from the pipe and redirected to the TXT file 
 * where path is given from command line.
 * In summary, the aim the project was executing unknown executable file by knowing only inputs and outputs type.
 * Here, child process is created because execl() function replaces everything with its content. 
 * Therefore, we protected tasks of our program which takes the inputs and writes to the file by creating 
 * a child and doing remaining tasks in the parent.
 * @Reference: Implentatiton is learned from PS 6 notes, especially redirect.c and io_capture.c files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

/*
 * O_WRONLY: Writing only
 * O_CREAT: Create file. If it is created already, no effect.
 * O_APPEND: Append end of the file.
*/
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
/**
 * S_IRUSR: Read permission for the file owner, 
 * S_IWUSR: Write permission for the file owner, 
 * S_IRGRP: Read permission for the file's group, 
 * S_IROTH: Read permission for users other than the file owner.
*/
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int main(int argc, char const *argv[]) {
    /* 
    * Two pipe is created.
    * p2p: Parent to child pipe. p2c[0]: Read end for child. p2c[1]:Write end for parent.
    * c2p: Child to parent pipe. c2p[0]: Read end for parent. c2p[1]:Write end for child.
    */
    int     p2c[2], c2p[2];
    pid_t   pid;
    char    w_buf[2048], r_buf[2048]; //Write buffer and Read buffer.

    if (argc < 3) {
        printf ("usage: %s blackboxPath outputPath\n", argv[0]);
        exit (1);
    }


    if(pipe(p2c)==-1){
        fprintf(stderr, "pipe() Error: %s\n",strerror(errno));
        exit(1);
    }  

    if(pipe(c2p)==-1){
        fprintf(stderr, "pipe() Error: %s\n",strerror(errno));
        exit(1);
    }

    if((pid=fork()) == -1) {
        fprintf(stderr, "fork() failed.\n");
        exit(1);
    }

    //Parent process
    if(pid > 0) {

        if(close(p2c[0])==-1){// Parent will not read from p2c
        fprintf(stderr, "close() Error: %s\n",strerror(errno));
        exit(1);
        }    
        if(close(c2p[1])==-1){// Parent will not write to c2p
        fprintf(stderr, "close() Error: %s\n",strerror(errno));
        exit(1);
        }

        //Two integer is scanned from user.
        int a;
        int b;
        scanf("%d", &a);
        scanf("%d", &b);
 
        char s1[2048];
        char s2[2048];

        sprintf(s1, "%d", a);
        sprintf(s2, " %d\n", b);//"\n" important to give inputs to blackbox.
 
        strcat(s1, s2);
        strcpy(w_buf,s1);//To packet two integer converted to string.

        //Inputs are sent to child through p2c pipe.
        if(write(p2c[1], w_buf, strlen(w_buf))==-1){
        fprintf(stderr, "write() Error: %s\n",strerror(errno));
        exit(1);
        }  
        /*For the reader, it is better to continue from child process from here,
        * since parent will wait here until something is written to the c2p[0].
        */

        //Parent reads the output from coming from child.
        if(read(c2p[0], r_buf, sizeof(r_buf))==-1){
        fprintf(stderr, "read() Error: %s\n",strerror(errno));
        exit(1);
        }

        //Opent .txt file, the complete path come from command line.
        int fd_output=open(argv[2],CREATE_FLAGS,CREATE_MODE);

        if(fd_output==-1){
        fprintf(stderr, "open() Error: %s\n",strerror(errno));
        exit(1);
        } 

        //Redirect stdout and stderr file descriptors to created txt file.
        if(dup2(fd_output, STDOUT_FILENO)==-1){
        fprintf(stderr, "dup2() Error: %s\n",strerror(errno));
        exit(1);
        } 

        if(dup2(fd_output, STDERR_FILENO)==-1){
        fprintf(stderr, "dup2() Error: %s\n",strerror(errno));
        exit(1);
        } 


        //Wait child to end. Record termination state of child (actually blackbox) to wstatus.
        int wstatus;

        if(wait(&wstatus)==-1){
        fprintf(stderr, "wait() Error: %s\n",strerror(errno));
        exit(1);
        }

        //According to termination state of child print SUCCESS: or FAIL:
        //Note that, print will go to txt file not to stdout or stderr.
        if (WIFEXITED(wstatus)){
	        if(WEXITSTATUS(wstatus)==0){
	            printf("SUCCESS:\n");
	            printf("%s", r_buf);
	        }else{
	            printf("FAIL:\n");
	            printf("%s", r_buf);
	        }
		}
    }
    //Child Process
    else {
        //Redirect stdin to p2c[0] to scanf() inputs. Scanf is come from blackbox (it is guarenteed.)
        if(dup2(p2c[0], STDIN_FILENO)==-1){
        fprintf(stderr, "dup2() Error: %s\n",strerror(errno));
        exit(1);
        }

        //Redirect stdout to c2p[1] to send output to parent.  
        if(dup2(c2p[1], STDOUT_FILENO)==-1){
        fprintf(stderr, "dup2() Error: %s\n",strerror(errno));
        exit(1);
        }
        //Redirect stderr to c2p[1] to send error to parent.
        if(dup2(c2p[1], STDERR_FILENO)==-1){
        fprintf(stderr, "dup2() Error: %s\n",strerror(errno));
        exit(1);
        }

        //// Child will not use any of the pipes. They are related ends redirected above. Close all.
        if(close(c2p[0])==-1){
        fprintf(stderr, "close() Error: %s\n",strerror(errno));
        exit(1);
        }
        if(close(c2p[1])==-1){
        fprintf(stderr, "close() Error: %s\n",strerror(errno));
        exit(1);
        }
        if(close(p2c[0])==-1){
        fprintf(stderr, "close() Error: %s\n",strerror(errno));
        exit(1);
        }
        if(close(p2c[1])==-1){
        fprintf(stderr, "close() Error: %s\n",strerror(errno));
        exit(1);
        }

        //Here, execl() replace everything with the scripts of blackbox. It's path come from command line.
        if(execl(argv[1],argv[1], NULL)==-1){
        fprintf(stderr, "execl() Error: %s\n",strerror(errno));
        exit(1);
        }
    }
    return(0);
}