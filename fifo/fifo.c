#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

const int MAX_NAME_LENGTH = 100;
const int MAX_TRSFER_SIZE = 2;
const int MAX_WAIT_TIME = 5;

int CO_fifo (const char *fifo_name, int flags);

int main (int argc, char* argv[]) {
    if (argc > 2) {
        fprintf (stderr, "Too much arguments for programm: %s,\t func: %s!\n", __FILE__, __func__ );
        exit (EXIT_FAILURE);
    }
    else if (argc == 2) { //writer programm
        pid_t reader_pid = -1;
        int general_fifo = CO_fifo ("Fifo.fifo", O_RDONLY);
        if (general_fifo < 0) {
            fprintf (stderr, "Error occured while opening general fifo!\n");
            exit (EXIT_FAILURE);
        }

        //Criticalsect_1
        //Criticalsect_2

        int check_val = -1;
        do {
            check_val = read (general_fifo, &reader_pid, sizeof (pid_t));
            if (check_val < 0) {
                fprintf (stderr, "Error occured while reading from general fifo!\n");
                exit (EXIT_FAILURE);
            }
        } while (check_val == 0);
        //Criticalsect_1-end

        char unique_fifo_name[MAX_NAME_LENGTH];
        sprintf (unique_fifo_name, "unique_fifo_%d.fifo", reader_pid);

        int unique_fifo = -1;
        unique_fifo = CO_fifo (unique_fifo_name, O_WRONLY | O_NONBLOCK);
        if (unique_fifo < 0) {
            fprintf (stderr, "Error occured while opening unique fifo!\n");
            exit (EXIT_FAILURE);
        }
        //Criticalsect_2-end

        check_val = fcntl (unique_fifo, F_SETFL, O_WRONLY);
        if (check_val < 0) {
            fprintf (stderr, "Error occured when changed flags in unique fifo!\n");
            exit (EXIT_FAILURE);
        }

        int read_file = -1;
        read_file = open (argv[1], O_RDONLY);
        if (read_file < 0) {
            fprintf (stderr, "Error occured while opening txt file!\n");
            exit (EXIT_FAILURE);
        }

        do {
            check_val = splice (read_file, NULL,unique_fifo, NULL, MAX_TRSFER_SIZE, SPLICE_F_MOVE);

            if (check_val < 0) {
                fprintf (stderr, "Error occured while transfering data from txt file to unique fifo!\n");
                exit (EXIT_FAILURE);
            }
        } while (check_val > 0);

        close (general_fifo);
        close (unique_fifo);
        exit (EXIT_SUCCESS);
    }
    else { //reader programm
        int check_val = -1;
        pid_t reader_pid = -1;
        
        int general_fifo = -1;
        general_fifo = CO_fifo ("Fifo.fifo", O_WRONLY);
        if (general_fifo < 0) {
            fprintf (stderr, "Error occured while opening general fifo!\n");
            exit (EXIT_FAILURE);
        }

        reader_pid = getpid ();
        char unique_fifo_name[MAX_NAME_LENGTH];
        sprintf (unique_fifo_name, "unique_fifo_%d.fifo", reader_pid);

        int unique_fifo = -1;
        unique_fifo = CO_fifo (unique_fifo_name, O_RDONLY | O_NONBLOCK);
        if (unique_fifo < 0) {
            fprintf (stderr, "Error occured while opening unique fifo!\n");
            exit (EXIT_FAILURE);
        }

        //Criticalsect_3
        //Criticalsect_4

        check_val = write (general_fifo, &reader_pid, sizeof (pid_t));
        if (check_val < 0) {
            fprintf (stderr, "Error occured while writing reader_pid to general fifo!\n");
            exit (EXIT_FAILURE);
        }
        
        //Criticalsect_3-end

        fd_set unique_fifo_set;
        FD_ZERO (&unique_fifo_set);
        FD_SET (unique_fifo, &unique_fifo_set);

        struct timeval time;
        time.tv_sec = MAX_WAIT_TIME;
        check_val = select (unique_fifo + 1, &unique_fifo_set, NULL, NULL, &time);
        if (check_val <= 0) {
            fprintf (stderr, "Error occured while executing select!\n");
            exit (EXIT_FAILURE);
        }

        //Criticalsect_4-end

        check_val = fcntl (unique_fifo, F_SETFL, O_RDONLY);
        if (check_val < 0) {
            fprintf (stderr, "Error occured when changed flags in unique fifo!\n");
            exit (EXIT_FAILURE);
        }

        if (isatty (STDOUT_FILENO)) {
            int stdout_flags = fcntl (STDOUT_FILENO, F_GETFL);
            stdout_flags = stdout_flags & ~O_APPEND;
            check_val = fcntl (STDOUT_FILENO, F_SETFL, stdout_flags);

            if (check_val < 0) {
                fprintf (stderr, "Error occured while changing stdout flags!\n");
                exit (EXIT_FAILURE);
            }
        }

        do {
            check_val = splice (unique_fifo, NULL, STDOUT_FILENO, NULL, MAX_TRSFER_SIZE, SPLICE_F_MOVE);

            if (check_val < 0) {
                fprintf (stderr, "Error occured while transfering data from txt file to unique fifo!\n");
                exit (EXIT_FAILURE);
            }
        } while (check_val > 0);

        close (general_fifo);
        close (unique_fifo);
        unlink (unique_fifo_name);
        exit (EXIT_SUCCESS);
    }

    return 0;
}

int CO_fifo (const char *fifo_name, int flags) {
    if (mkfifo (fifo_name, 0666) < 0) {
        if (errno != EEXIST)
        {
            fprintf (stderr, "Error in creating fifo!\n");
            exit (EXIT_FAILURE);
        }

        errno = 0;
    }

    return open (fifo_name, flags);
}