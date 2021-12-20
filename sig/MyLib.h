#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#define check(status) {                                                  \
    if (status) {                                                        \
	    perror ("Error name");                                          \
        fprintf (stderr, "Error in programm: %s, func: %s, line: %d!\n", \
                                      __FILE__, __FUNCTION__, __LINE__); \
                                                                         \
        exit (EXIT_FAILURE);                                             \
    }                                                                    \
}

const int size = 10;
int bit_val;

void child_dead (int sig) {
    exit (0);
}
void first_child (int sig) {}
void second_child (int sig) {}
void first_parent (int sig) {
    bit_val = 0;
    //printf ("Receive 0!\n");

    return;
}
void second_parent (int sig) {
    bit_val = 1;
    //printf ("Receive 1!\n");

    return;
}

void sigaction_configure (struct sigaction* signal, void handler (int sig), sigset_t mask, int flags, int how,struct sigaction* old_signal) {
    int check_val;
    
    signal -> sa_handler = handler;
    signal -> sa_mask = mask;
    signal -> sa_flags = flags;
    check_val = sigaction (how, signal, old_signal);
    check (check_val != 0);
}

void send_to_parent (char elem, pid_t parent_pid) {
    int cur_bit = 0;
    int check_val = 0;
    
    for (int i = 0; i < size; i++) {
        cur_bit = elem & 1;
        elem = elem >> 1;

        if (cur_bit == 0) {
            check_val = kill (parent_pid, SIGUSR1);
            check (check_val == -1);
        }
        else if (cur_bit == -1) {
            check_val = kill (parent_pid, SIGUSR2);
            check (check_val == -1);
        }

        sigset_t set_bit;
		check_val = sigfillset (&set_bit);
        check (check_val != 0);
		
        check_val = sigdelset (&set_bit, SIGUSR1);
        check (check_val != 0);

		check_val = sigsuspend (&set_bit);
        check (check_val != -1);
    }

    return;
}

char receive_from_parent (pid_t pid) {
    char elem =  '\0';
    int check_val = 0;
    
    for (int i = 0; i < 8; i++) {
        sigset_t get_bit;
		check_val = sigfillset (&get_bit);
        check (check_val != 0);

		check_val = sigdelset (&get_bit, SIGUSR1);
        check (check_val != 0);

		check_val = sigdelset (&get_bit, SIGUSR2);
        check (check_val != 0);

		check_val = sigdelset (&get_bit, SIGCHLD);
        check (check_val != 0);

        check_val = sigsuspend (&get_bit);
        check (check_val != -1);

        elem = elem | (bit_val << i);

        check_val = kill (pid, SIGUSR1);
    }

    return elem;
}