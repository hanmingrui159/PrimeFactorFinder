#include <stdio.h> 
#include <math.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <stdlib.h> 
#include <sys/wait.h>

//filter() reads input numbers from parent pipe and write to child pipe, 
//excluding those that are multiples of m.
//filter() also write m into child pipe if m is a prime factor of n. 
int filter(int m, int read_end, int write_end, int n) {
    int buf;
    int read_result;
    int write_result;
    //read integer from pipe read_end to buf 
    while ((read_result = read(read_end, & buf, sizeof(int))) != 0) {
        //check if read() call failed        
        if (read_result == -1) {
            return 1;
        }

        //write integer buf to pipe write_end if it is not a multiple of m
        if (buf % m != 0) {
            if ((write_result = write(write_end, & buf, sizeof(int))) != -1) {
                if (write_result == 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

//this helper reads the rest of the data from pipe readend to prevent the pipe from overflow
int decongest(int readend) {
    int buf1;
    int read_result1;
    //read each int from pipe's readfd
    while ((read_result1 = read(readend, & buf1, sizeof(int))) != 0) {
        //check if read() call failed
        if (read_result1 == -1) {
            return 1;
        }
    }
    return 0;
}

//main function takes in a command-line argument integer n
//and determine if n is prime, has two prime factors, or multiple prime factors, and print the number of filters used during this process.
int main(int argc, char * * argv) {
    int status;
    int res;
    int fd1[2];
    int fd2[2];
    int m;
    int num_bytes;
    int i = 1; // used to indicate if should read from fd1 or fd2
    int *readfd;
    int *writefd;
    int prime_factor = 0;
    int write_result1;

    //check if exactly one commandline argument is provided
    if (!(argc == 2)) {
        fprintf(stderr, "Usage:\n\tpfact n\n");
        exit(1);
    }

    char * illegal_chars;
    int n = strtol(argv[1], & illegal_chars, 10);

    //check if input n is a valid positive integer
    if (!(n > 0) || illegal_chars[0] != '\0') {
        fprintf(stderr, "Usage:\n\tpfact n\n");
        exit(1);
    }

    //handles the exception case when input n=1
    if (n == 1) {
        fprintf(stderr, "1 is reserved for Mr.Hawking\n");
        fprintf(stderr, "Number of filters = %d\n", 0);
        exit(1);
    }

    //create pipe fd1
    if ((pipe(fd1)) == -1) {
        fprintf(stderr, "pipe fd1");
        exit(1);
    }
    //create pipe fd2
    if ((pipe(fd2)) == -1) {
        fprintf(stderr, "pipe fd2");
        exit(1);
    }
    //close pipe fd2's write end
    if (close(fd2[1]) == -1) {
        fprintf(stderr, "close pipe fd2[1]");
        exit(1);
    }
    //fork first child
    res = fork();
    if (res < 0) { // case: a system call error
        fprintf(stderr, "fork");
        exit(1);
    }
    //original parent's actions after fork() call
    if (res > 0) {
        //close pipe fd2
        if (close(fd2[0]) == -1) {
            fprintf(stderr, "close pipe fd2[0]");
            exit(1);
        }
        //close pipe fd1
        if (close(fd1[0]) == -1) {
            fprintf(stderr, "close pipe fd[0]");
            exit(1);
        }
        //write integers from
        for (int j = 2; j < n + 1; j++) {
            if ((write_result1 = write(fd1[1], & j, sizeof(int))) != -1) {
                if (write_result1 == 0) {
                    fprintf(stderr, "wrote 0 bytes to first pipe");
                }
            } else {
                fprintf(stderr, "write to first pipe");
                exit(1);
            }
        }
        //close pipefd1's write end after original parent's writing
        if (close(fd1[1]) == -1) {
            fprintf(stderr, "close pipe fd1[1]");
            exit(1);
        }
        //waiting for first to finish;
        if (waitpid(res, & status, 0) == -1) {
            fprintf(stderr, "waitpid");
            exit(1);
        }
        //if child exited, check if child exited with a normal value. 
        //if the child exited normally, print the result; exit(1) otherwise 
        if (WIFEXITED(status)) {
            int result = WEXITSTATUS(status);
	    //check if child exit abnormally
	    if (result== 0){
	        exit(1);
	    }	
            printf("Number of filters = %d\n", result-1);
            exit(0);
        } else {//else, a signal has occurred during writing
            fprintf(stderr, "main process child exit abnormally");
            exit(1);
        }
    }

    while (1) {
        //child's action after fork() call
        if (res == 0) {
            //set read and write pipes in order;
            if (i == 1) {
                readfd = fd1;
                writefd = fd2;
                i = 2;
            } else if (i == 2) {
                readfd = fd2;
                writefd = fd1;
                i = 1;
            } else { //handle exception case
                fprintf(stderr, "i's assigned value not equal to 1 or 2");
                exit(0);
            }

            //close writefd pipe because B doesn't need the old one 
            if (close(writefd[0]) == -1) {
                fprintf(stderr, "close pipe");
                exit(0);
            }

            //obtain m
            if (close(readfd[1]) == -1) {
                fprintf(stderr, "close pipe");
                exit(0);
            }
            num_bytes = read(readfd[0], & m, sizeof(int)); //debug change
            if (num_bytes == -1) {//handle exception
                fprintf(stderr, "reading first int m from pipe");
            }
            // if m is less than sqrt(n), we keep on checking
            if (m < sqrt(n)) {
                //if m is a prime factor of n, records it if it is the first one found; otherwise, return 
                if (n % m == 0) {
                    if (prime_factor == 0) { //debug change this later with stricter conditions
                        prime_factor = m;
                    } else { //we have found two prime factors smaller than sqrt(n), thus n has multiple factors
                        if (decongest(readfd[0]) == 1) {
                            fprintf(stderr, "decongest");
                        } //read the remaining ints from pipe to make sure pipe closes normally even though they aren't needed
                        if (close(readfd[0]) == -1) {
                            fprintf(stderr, "close pipe");
                            exit(0);
                        }
                        printf("%d is not the product of two primes\n", n);
                        exit(1);
                    }
                }
		//create new pipe at writefd
                if ((pipe(writefd)) == -1) {
                    fprintf(stderr, "pipe");
                    exit(0);
                }
                //fork a child once again
                res = fork();
                if (res < 0) { // case: a system call error
                    fprintf(stderr, "fork");
                    exit(0);
                }
            }
            //else if m>= sqrt(n), the process terminates, check which conditions 
            //that apply(for sure not perfect square at this point) and write to fdparent
            else {
                if (decongest(readfd[0]) == 1) {
                    fprintf(stderr, "decongest");
                } //read the remaining ints from pipe to make sure pipe closes normally even though they aren't needed
                if (close(readfd[0]) == -1) {
                    fprintf(stderr, "close pipe");
                    exit(0);
                }
                ///perfect square                
                if ((m * m) == n) {
                    printf("%d %d %d\n", n, m, m);
                    exit(1);
                }
                //n is prime if no prime factors are found
                else if (prime_factor == 0) {
                    printf("%d is prime\n", n);
                    exit(1);
                }
                //n has exactly two primes or multiples of the same number
                else if (prime_factor > 0) {
                    if (n % (prime_factor * prime_factor) == 0) {
                        printf("%d is not the product of two primes\n", n);
                    } else {
                        printf("%d %d %d\n", n, prime_factor, n / prime_factor);
                    }
                }
                exit(1);
            }
        }

        //in B after fork
        if (res > 0) {
            if (close(writefd[0]) == -1) {
                fprintf(stderr, "close pipe");
                exit(0);
            }
            //transfer input numbers from old pipe to child pipe 
            if (filter(m, readfd[0], writefd[1], n) == 1) { //debug
                fprintf(stderr, "filter");
                exit(0);
            };
            if (close(readfd[0]) == -1) {
                fprintf(stderr, "close pipe");
                exit(0);
            }
            if (close(writefd[1]) == -1) {
                fprintf(stderr, "close pipe");
                exit(0);
            }
            //wait for process to finish, and exit with filter numbers
            if (waitpid(res, & status, 0) == -1) {
                fprintf(stderr, "waitpid");
                exit(0);
            } //upgrade to waitpid,check third parameter's usage
            if WIFEXITED(status){//child exited normally
                int result = WEXITSTATUS(status);
		if (result == 0){
		    exit(0);
		}
                exit(result+1);
            } 
	    else {//child exited abnormally
                fprintf(stderr, "child exited abnormally");
                exit(0);
            }
            //exiting in abnormal cases

        }
    }

    return 0;
}
