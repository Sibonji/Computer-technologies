#ifndef MY_LIB
#define MY_LIB

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

#define MAX_CHILDREN_COUNT 10000;
const int MAX_MSG_SIZE = 373;
const int POISON = 0xDEADBEEF;

struct cp_buf {
	char* start_ptr = nullptr;
	size_t size = 0;
	int reading = 0, writing = 0;
	bool is_full = false;
};

struct cp_info { //Connection pipe info
    pid_t parent_pid = 0, child_pid = 0;
	int C2P_pipe_fd [2];			// Child to parent
	int P2C_pipe_fd [2]; 		// Parent to child

    cp_buf {};
};

long int get_num(int argc, char* argv[], const int arg_quantity);

#endif