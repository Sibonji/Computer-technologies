#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>
#include <unistd.h>

const int MAX_CHILDREN_QUANTITY = 10000;

long int get_num(int argc, char* argv[]) {
    long int num = 0;
    char* end;

    if(argc != 2) {
        fprintf(stderr, "Incorrect number of arguments!!!\n");

        return -1;
    }

    num = strtol(argv[1], &end, 10);

    if(num <= 0 || num > MAX_CHILDREN_QUANTITY) {
        fprintf(stderr, "Write correct number instead of: %s!!!\n", argv[1]);

        return -1;
    }
    else if (errno != 0) {
        fprintf (stderr, "Overflow!\n");

        return -1;
    }
    else {
        return num;
    }
}

int main (int argc, char* argv[]) {
    int children_quantity = get_num (argc, argv);
    if (children_quantity <= 0) {
        exit (EXIT_FAILURE);
    }

    int check_val = setvbuf (stdout, NULL, _IONBF, 0);
    if (check_val < 0) {
        fprintf (stderr, "Error occured while set buf!\n");
        
        exit (EXIT_FAILURE);
    }

    int msgid = msgget (IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0666);
    if (msgid < 0) {
        fprintf (stderr, "Error occured while creating msg!\n");

        exit (EXIT_FAILURE);
    }

    unsigned int num = 0;
    pid_t fork_pid = 0;
    unsigned int data_size = sizeof (msgbuf) - sizeof (long);

    for (int i = 1; i <= children_quantity; i++) {
        fork_pid = fork ();

        if (fork_pid == 0) { //Children
            num = i;
            break;
        } 
    }

    if (fork_pid != 0) {//Parent
        for (int i = 1; i <= children_quantity; i++) {
            msgbuf msg {i};

            if (msgsnd (msgid, &msg, data_size, IPC_NOWAIT) < 0) {
                fprintf (stderr, "Parent sending error!\n");
                exit (EXIT_FAILURE);
            }

            if (msgrcv (msgid, &msg, data_size, children_quantity + 1, MSG_NOERROR) < 0) {
                fprintf (stderr, "Error occured while receiving messafe!\n");

                exit (EXIT_FAILURE);
            }
        }

        msgctl (msgid, IPC_RMID, NULL);

        return 0;
    }
    else if (fork_pid == 0) { //Children
        msgbuf msg {num};

        if (msgrcv (msgid, &msg, data_size, num, MSG_NOERROR) < 0) {
            fprintf (stderr, "Child %d receiving error!\n", num);
            exit (EXIT_FAILURE);
        }

        fprintf (stdout, "%d\n", num);
        msg.mtype = children_quantity + 1;

        if (msgsnd (msgid, &msg, data_size, IPC_NOWAIT) < 0) {
            fprintf (stderr, "Error occured while sending message!\n");

            exit (EXIT_FAILURE);
        }

        return 0;
    }

    return 0;
}