#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>
#include <unistd.h>

#define check(status) {                                                  \
    if (status) {                                                        \
        fprintf (stderr, "Error in programm: %s, func: %s, line: %d!\n", \
                                      __FILE__, __FUNCTION__, __LINE__); \
                                                                         \
        exit (EXIT_FAILURE);                                             \
    }                                                                    \                               
}

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
    check (check_val < 0);

    int msgid = msgget (IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0666);
    check (msgid < 0);

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

            check (msgsnd (msgid, &msg, data_size, IPC_NOWAIT) < 0);

            check (msgrcv (msgid, &msg, data_size, children_quantity + 1, MSG_NOERROR) < 0);
        }

        msgctl (msgid, IPC_RMID, NULL);

        return 0;
    }
    else if (fork_pid == 0) { //Children
        msgbuf msg {num};

        check (msgrcv (msgid, &msg, data_size, num, MSG_NOERROR) < 0);

        fprintf (stdout, "%d ", num);
        msg.mtype = children_quantity + 1;

        check (msgsnd (msgid, &msg, data_size, IPC_NOWAIT) < 0);

        return 0;
    }

    return 0;
}