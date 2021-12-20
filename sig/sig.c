#include "MyLib.h"

int main (int argc, char* argv[]) {
    if (argc != 2) {
        fprintf (stderr, "Incoorect number of arguments!\n");

        exit (EXIT_FAILURE);
    }

    int check_val = 0;

    sigset_t mask;
    check_val = sigfillset (&mask);
    check (check_val != 0);
    
    check_val = sigdelset (&mask, SIGCHLD);
    check (check_val != 0);

    check_val = sigprocmask (SIG_SETMASK, &mask, NULL);
    check (check_val != 0);

    sigset_t child_is_dead;
    check_val = sigfillset (&child_is_dead);
    check (check_val != 0);

    struct sigaction child_check_sig;
    sigaction_configure (&child_check_sig, child_dead, child_is_dead, SA_NOCLDWAIT, SIGCHLD, NULL);

    pid_t pid = fork ();
    check (pid == -1);

    if (pid == 0) { //Child
        check_val = prctl (PR_SET_PDEATHSIG, SIGKILL);
        check (check_val != 0);
        
        int file = open (argv[1], O_RDONLY);
        check (file == -1);

        sigset_t transfer_signal;
        check_val = sigfillset (&transfer_signal);

        struct sigaction usr1;
        struct sigaction usr2;
        sigaction_configure (&usr1, first_child, transfer_signal, 0, SIGUSR1, NULL);
        sigaction_configure (&usr2, second_child, transfer_signal, 0, SIGUSR2, NULL);

        sigset_t check_parent;
        check_val = sigfillset (&check_parent);
        check (check_val != 0);
        
        check_val = sigdelset (&check_parent, SIGUSR1);
        check (check_val != 0);

        check_val = sigsuspend (&check_parent);
        check (check_val != -1);

        char data[size];
        char elem = '\0';
        int quantity = 0;

        pid_t parent_pid = getppid ();
        check (parent_pid == 1);

        while (1) {

            quantity = read (file, data, size);
            //check (quantity == -1);

            int cur_bit = 0;

            for (int i = 0; i < quantity; i++) {
                elem = data[i];
    
                for (int j = 0; j < 8; j++) {
                    cur_bit = elem & 1;
                    elem = elem >> 1;

                    if (cur_bit == 0) {
                        check_val = kill (parent_pid, SIGUSR1);
                        check (check_val == -1);
                    }
                    else if (cur_bit == 1) {
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
            }

            if (quantity == 0) break;
        }
    }
    else {
        sigset_t transfer_signal;
        check_val = sigfillset (&transfer_signal);
        check (check_val != 0);

        check_val = sigdelset (&transfer_signal, SIGCHLD);
        check (check_val != 0);

        struct sigaction usr1;
        struct sigaction usr2;
        sigaction_configure (&usr1, first_parent, transfer_signal, 0, SIGUSR1, NULL);
        sigaction_configure (&usr2, second_parent, transfer_signal, 0, SIGUSR2, NULL);

        check_val = kill (pid, SIGUSR1);

        char elem = '\0';

        while (1) {
            elem = receive_from_parent (pid);
           
            printf ("%c", elem);
            if (elem == 0) break;
        }
    }

    return 0;
}