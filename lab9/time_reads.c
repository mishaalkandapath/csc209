/* The purpose of this program is to practice writing signal handling
 * functions and observing the behaviour of signals.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* Message to print in the signal handling function. */
#define MESSAGE "%ld reads were done in %ld seconds.\n"

/* Global variables to store number of read operations and seconds elapsed. 
 */
long num_reads, seconds;

void handler(int signal){
  printf(MESSAGE, num_reads, seconds);
  exit(0);
}


/* The first command-line argument is the number of seconds to set a timer to run.
 * The second argument is the name of a binary file containing 100 ints.
 * Assume both of these arguments are correct.
 */

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: time_reads s filename\n");
        exit(1);
    }
    seconds = strtol(argv[1], NULL, 10);

    FILE *fp;
    if ((fp = fopen(argv[2], "r")) == NULL) {
      perror("fopen");
      exit(1);
    }

    struct sigaction action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGPROF, &action, NULL);

    //start timer
    struct itimerval timer_details;
    timer_details.it_value.tv_sec = seconds;
    timer_details.it_value.tv_usec = 0;
    timer_details.it_interval.tv_sec = 0;
    timer_details.it_interval.tv_usec = 0;
    setitimer(ITIMER_PROF, &timer_details, NULL);

    /* In an infinite loop, read an int from a random location in the file,
     * and print it to stderr.
     */
    int rand_idx;
    int value_at_idx;
    for (;;) {
      rand_idx = (int) (((double) random()/RAND_MAX) * 100);
      fseek(fp, sizeof(int) * rand_idx, SEEK_SET);
      num_reads += fread(&value_at_idx, sizeof(int), 1, fp);
      printf("%i\n", value_at_idx);
    }
    return 1; // something is wrong if we ever get here!
}
