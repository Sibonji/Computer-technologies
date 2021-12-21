#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include <signal.h>
#include <math.h>
#include <poll.h>
#include <fcntl.h>

#define check(status) {                                                  \
    if (status) {                                                        \
	    perror ("Error");                                          \
        fprintf (stderr, "Error in programm: %s, func: %s, line: %d!\n", \
                                      __FILE__, __FUNCTION__, __LINE__); \
                                                                         \
        exit (EXIT_FAILURE);                                             \
    }                                                                    \
}

#define POISON -1
#define MAX_CHILDREN_QUANTITY 10000

enum MODES{
    READ = 0,
    WRITE = 1,
};

struct cp_info {
    unsigned int child_num;
    pid_t child_pid;
    pid_t parent_pid;

    int C2P_pipe[2];
    int P2C_pipe[2];
};

struct cp_connect {
    int fd_in;
    int fd_out;

    int size;
    char* data;
    
    int is_read;
    int is_write;    
    int busy_place;
    int free_place;
};

const int size = 4096;
const int max_size = 10000;

long int get_num(int argc, char* argv[]);
void is_parent_alive (pid_t parent_pid);
void close_pipes (struct cp_info* child_info, int num);
void child_exec (struct cp_info* children_info, char* file, int children_quantity);
void create_buf (struct cp_connect* connection, struct cp_info* child_info, int child_quantity);
void read_from_fd_in(struct cp_connect* connection);
void write_to_fd_out (struct cp_connect* connection);
void close_curr_pipe (struct cp_info* child_info);
void read_and_write_calculate (int* this_op, int* oppos_op, int ret_value, int* size_a, int* size_b, int buf_size);

///////////////////////////////////////////////////////////////////////////////////

long int get_num(int argc, char* argv[]) {
    long int num = 0;
    char* end;

    if(argc != 3) {
        fprintf(stderr, "Incorrect number of arguments!!!\n");

        exit (EXIT_FAILURE);
    }

    num = strtol(argv[1], &end, 10);

    if(num <= 0 || num > MAX_CHILDREN_QUANTITY) {
        fprintf(stderr, "Write correct number instead of: %s!!!\n", argv[1]);

        exit (EXIT_FAILURE);;
    }
    else if (errno != 0) {
        fprintf (stderr, "Overflow!\n");

        exit (EXIT_FAILURE);
    }
    else {
        return num;
    }
}

void is_parent_alive (pid_t parent_pid) {
    check (prctl(PR_SET_PDEATHSIG, SIGTERM) < 0)

    check (parent_pid != getppid())
}

void close_pipes (struct cp_info* child_info, int num) {
    int check_val = 0;

    for (int i = 0; i < num; i ++)
    {
        check_val = close(child_info[i].C2P_pipe[READ]);
        check (check_val == -1);
        child_info[i].C2P_pipe[READ] = POISON;

        check_val = close(child_info[i].P2C_pipe[WRITE]);
        check (check_val == -1);    
        child_info[i].P2C_pipe[WRITE] = POISON;
    }

    check_val = close(child_info[num].C2P_pipe[READ]);
    check (check_val == -1);
    child_info[num].C2P_pipe[READ] = POISON;

    check_val = close(child_info[num].P2C_pipe[WRITE]);
    check (check_val == -1);
    child_info[num].P2C_pipe[WRITE] = POISON;
}

void child_exec (struct cp_info* children_info, char* file, int children_quantity)
{   
    int check_val = 0;
    int fd_in = 0;
    int fd_out = 0;

    if (children_info -> child_num == 0)
        fd_in = open (file, O_RDONLY);
    else    
		fd_in = children_info -> P2C_pipe[READ];

    if (children_info -> child_num == children_quantity - 1)
        fd_out = STDOUT_FILENO;
    else
        fd_out = children_info -> C2P_pipe[WRITE];
    
    check (fd_in == -1);
    check (fd_out == -1);

    check_val = fcntl(fd_out, F_SETFL, O_WRONLY);
    check (check_val == -1);

    check_val = fcntl(fd_in, F_SETFL, O_RDONLY);
    check (check_val == -1);

    do {
        check_val = splice (fd_in, NULL, fd_out, NULL, size, SPLICE_F_MOVE);
        check (check_val == -1);
    } while (check_val != 0);

    check_val = close (children_info -> P2C_pipe[READ]);
    check (check_val == -1);
    children_info -> P2C_pipe[READ] = POISON;    

    check_val = close (children_info -> C2P_pipe[WRITE]);
    check (check_val == -1);
    children_info -> C2P_pipe[WRITE] = POISON;
}

void parent_exec (struct cp_info* child_info, int children_quantity)
{
    int check_val = 0;
    int fd = POISON;
	fd_set fd_out, fd_in;

    int connections_quantity = children_quantity - 1;

    struct cp_connect* connection = (struct cp_connect*) calloc (connections_quantity, sizeof(struct cp_connect));
	check (connection == NULL);

	for (int i = 0; i < connections_quantity; i++) {
		create_buf (&connection[i], &child_info[i], children_quantity);

		check_val = fcntl(connection[i].fd_in, F_SETFL, O_RDONLY | O_NONBLOCK);
        check (check_val == -1);
        
		check_val = fcntl(connection[i].fd_out, F_SETFL, O_WRONLY | O_NONBLOCK);
        check (check_val == -1);
	}	

	int start_num = 0;

	while (start_num < connections_quantity){
		FD_ZERO(&fd_out);
		FD_ZERO(&fd_in);

		for (int i = start_num; i < connections_quantity; i++) {
            if ((connection[i].fd_in != -1) && connection[i].free_place > 0)
				FD_SET(connection[i].fd_in, &fd_in);

			if ((connection[i].fd_out != -1) && connection[i].busy_place > 0)
				FD_SET(connection[i].fd_out, &fd_out);

			if (connection[i].fd_in > fd)
				fd = connection[i].fd_in;

			if (connection[i].fd_out > fd)
				fd = connection[i].fd_out;
		}	

		check_val = select(fd + 1, &fd_in, &fd_out, NULL, NULL);
        check (check_val < 0);			    

	    for (int i = start_num; i < connections_quantity; i++){
			if (FD_ISSET(connection[i].fd_in, &fd_in) && (connection[i].free_place > 0))
				read_from_fd_in(&connection[i]);

            if (FD_ISSET(connection[i].fd_out, &fd_out) && (connection[i].busy_place > 0))
				write_to_fd_out(&connection[i]);
			
	        if (connection[i].fd_in == POISON && connection[i].fd_out != POISON && connection[i].busy_place == 0) {
				check (start_num != i);
                
                close(connection[i].fd_out);				
				connection[i].fd_out = POISON;

	            start_num++;
				free (connection[i].data);
			}
		}
	}	

    for (int i = 0; i < children_quantity; i++){
        check_val = waitpid (child_info[i].child_pid, NULL, 0);
        check (check_val == -1);
    }
        
    free(connection);
}

void create_buf (struct cp_connect* connection, struct cp_info* child_info, int child_quantity) {
    connection -> size = pow (3, child_quantity - child_info -> child_num) * 9;
	
    connection -> data = (char*) calloc (1, connection -> size);
	check (connection -> data == NULL);

	connection -> is_read = 0;
	connection -> is_write = 0;

	connection -> busy_place = 0;
	connection -> free_place = connection -> size;

	connection -> fd_in = child_info -> C2P_pipe[READ];
	connection -> fd_out = (child_info + 1) -> P2C_pipe[WRITE];
}

void read_from_fd_in(struct cp_connect* connection) {
    int ret_read = read(connection -> fd_in, &connection -> data[connection -> is_read], connection -> free_place);
    check (ret_read < 0);

    if (ret_read == 0) {
        close(connection -> fd_in);
        connection -> fd_in = POISON;
        return;
    }

    read_and_write_calculate (&(connection -> is_read), &(connection -> is_write), ret_read, \
                              &(connection -> busy_place), &(connection -> free_place), connection -> size);
}

void write_to_fd_out (struct cp_connect* connection) {
    errno = 0;
    int ret_write = write(connection -> fd_out, &connection -> data[connection -> is_write], connection -> busy_place);
    check ((ret_write < 0) && (errno != EAGAIN));

    read_and_write_calculate (&(connection -> is_write), &(connection -> is_read), ret_write, \
                              &(connection -> free_place), &(connection -> busy_place), connection -> size);
}

void read_and_write_calculate (int* this_op, int* oppos_op, int ret_value, int* size_a, \
                               int* size_b, int buf_size) {
    if (*this_op >= *oppos_op)
        *size_a += ret_value;

    if (*this_op + ret_value == buf_size) {
        *size_b = *oppos_op;
        *this_op = 0;
    }
    else {
        *size_b -= ret_value;
        *this_op += ret_value;
    }
}

void close_curr_pipe (struct cp_info* child_info) {
    int check_val = 0;
    
    check_val = close (child_info -> P2C_pipe[READ]);
    check (check_val == -1);
    child_info -> P2C_pipe[READ] = POISON;

    check_val = close (child_info -> C2P_pipe[WRITE]);
    check (check_val == -1);
    child_info -> C2P_pipe[WRITE] = POISON;
}