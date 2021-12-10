#ifndef MY_LIB
#define MY_LIB

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <errno.h>
#include <limits.h>
#include <cstring>

#define check(status) {                                                  \
    if (status) {                                                        \
        fprintf (stderr, "Error in programm: %s, func: %s, line: %d!\n", \
                                      __FILE__, __FUNCTION__, __LINE__); \
                                                                         \
        exit (EXIT_FAILURE);                                             \
    }                                                                    \
}



#define MAX_CHILDREN_QUANTITY 10000
const int MAX_MSG_SIZE = 373;
const int POISON = 0xDEADBEEF;

enum IS {
    READ  = 0,
    WRITE = 1
};

struct cp_buf {
	char* start_ptr = nullptr;
	size_t size = 0;
	int reading = 0, writing = 0;
	bool is_full = false;
};

struct cp_info { //Connection pipe info
    pid_t parent_pid = 0;
    pid_t child_pid  = 0;
	int C2P_pipe_fd [2];			// Child to parent
	int P2C_pipe_fd [2]; 		    // Parent to child

    cp_buf buf {};
};

//                                   Standart funstions                                   \\

long int get_num (int argc, char* argv[], const int arg_quantity);
void Close (int fd);
bool is_full (cp_buf* cp_buf);
void clear_buf (cp_info* cps, int children_quantity);
int read_buf (cp_info* cps, int id, int n);
int write_buf (cp_info* cps, int id, int n);

//                                   Parent   fucntions                                   \\

int parent_create (cp_info* cps, int child_number, int children_quantity, pid_t child_pid);
void parent_exec (cp_info* cps, int children_quantity);

//                                   Child    functions                                   \\

int child_create  (cp_info* cps, int child_number, int children_quantity, int* argv_fd, char* filepath);
void child_exec (cp_info* cps, int proc_id, int argv_fd);

#endif