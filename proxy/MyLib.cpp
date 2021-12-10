#include "MyLib.hpp"

//                                   Standart funstions                                   \\

long int get_num(int argc, char* argv[], const int arg_quantity) {
    long int num = 0;
    char* end;

    if(argc != arg_quantity) {
        fprintf(stderr, "Incorrect number of arguments!!!\n");

        return -1;
    }

    num = strtol(argv[1], &end, 10);

    #ifdef MAX_CHILDREN_QUANTITY
    if (num <= 0 || num > MAX_CHILDREN_QUANTITY) {
        fprintf(stderr, "Write correct number instead of: %s!!!\n", argv[1]);

        return -1;
    }
    #elif
    if (num <= 0) {
        fprintf(stderr, "Write correct number instead of: %s!!!\n", argv[1]);

        return -1;
    }
    #endif
    if (errno != 0) {
        fprintf (stderr, "Overflow!\n");

        return -1;
    }
    else {
        return num;
    }
}

void close (int fd) {
    if (fd != POISON) {
        close (fd);
        fd = POISON;
    }

    return;
}

//                                   Parent fucntions                                   \\

int parent_create (cp_info cps, int child_number, int children_quantity, pid_t child_pid) {
    close (cps[child_number].C2PPipeFds[IS::WRITE]);	
	
    if (child_number == children_quantity - 1) {
		close (cps[child_number].P2C_pipe_fd[IS::READ]);
		close (cps[child_number].P2C_pipe_fd[IS::WRITE]);
		
        cps[child_number].P2C_pipe_fd[IS::WRITE] = STDOUT_FILENO;
	}
	
	cps[child_number].child_pid = child_pid;
}

//                                   Child  functions                                   \\