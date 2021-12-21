#include "MyLib.h"

int main (int argc, char* argv[]) { //argv[1] - num, argv[2] - file
    int children_quantity = get_num (argc, argv); //check number of args in this func

    pid_t parent_pid;
    struct cp_info* child_info = (struct cp_info*) calloc (children_quantity, sizeof (struct cp_info));

    int check_val = 0;

    for (int i = 0; i < children_quantity; i++) {
        child_info[i].child_num = i;

        check_val = pipe (child_info[i].P2C_pipe);
        check (check_val == -1);

        check_val = pipe (child_info[i].C2P_pipe);
        check (check_val == -1);

        pid_t child_pid = fork ();
        check (child_pid == -1);

        if (child_pid == 0) {
            pid_t parent_pid = getppid ();
            is_parent_alive (parent_pid);
            close_pipes (child_info, i);
            child_exec (child_info + i, argv[2], children_quantity);
            free (child_info);
            exit (0);
            break;
        }
        else {
            check_val = close (child_info[i].P2C_pipe[READ]);
            check (check_val == -1);
            child_info[i].P2C_pipe[READ] = -1;

            check_val = close (child_info[i].C2P_pipe[WRITE]);
            check (check_val == -1);
            child_info[i].C2P_pipe[WRITE] = -1;
        }
    }

    parent_exec (child_info, children_quantity);
    free (child_info);

    return 0;
}