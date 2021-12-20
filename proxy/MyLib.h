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

    size_t size;
    char* data;

    size_t is_read;
    size_t is_write;    

    size_t full;
    size_t empty;
};

const int MAX_CHILDREN_QUANTITY = 10000;
const int size = 4096;
const int max_size = 10000;

long int get_num(int argc, char* argv[]);
void is_parent_alive (pid_t parent_pid);
void close_pipes (struct cp_info* child_info, int num);
void child_exec (struct cp_info* children_info, char* file, int children_quantity);
void create_buf (struct cp_connect* connection, struct cp_info* child_info, int num, int child_quantity);
void read_in_buf(struct cp_connect* connection, int id);
void write_from_buf (struct cp_connect* connection, int id);


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
    for (int i = 0; i < num; i ++)
    {
        check (close(child_info[i].C2P_pipe[READ]) == -1);
        child_info[i].C2P_pipe[READ] = -1;

        check (close(child_info[i].P2C_pipe[WRITE]) == -1);    
        child_info[i].P2C_pipe[WRITE] = -1;
    }

    check (close(child_info[num].C2P_pipe[READ]) == -1);
    child_info[num].C2P_pipe[READ] = -1;

    check (close(child_info[num].P2C_pipe[WRITE]) == -1);
    child_info[num].P2C_pipe[WRITE] = -1;
}

void child_exec (struct cp_info* children_info, char* file, int children_quantity)
{   
    int check_val = 0;
    int fd_in = 0;
    int fd_out = 0;
    if (children_info -> child_num == 0)
    {
        fd_in = open (file, O_RDONLY);

        check_val = close (children_info -> P2C_pipe[READ]);
        check (check_val == -1);
    }
    else    
		fd_in = children_info -> P2C_pipe[READ];

    if (children_info -> child_num == children_quantity - 1)
    {
        fd_out = STDOUT_FILENO;

        check_val = close (children_info -> C2P_pipe[WRITE]);
        check (check_val == -1);
    }
    else
        fd_out = children_info -> C2P_pipe[WRITE];
    
    check (fd_in == -1);

    check (fd_out == -1);

    check_val = fcntl(fd_out, F_SETFL, O_WRONLY);
    check (check_val == -1);

    check_val = fcntl(fd_in, F_SETFL, O_RDONLY);
    check (check_val == -1);

    int is_readed = -1;
    char data[size];

    while (true)
    {
        is_readed = read (fd_in, data, size);
        check (is_readed == -1);

        check (is_readed == 0) 
            break;
        
        check_val = write (fd_out, data, is_readed);
        check (check_val == -1);
    }

    check_val = close (fd_in);
    check (check_val == -1);
    fd_in = children_info -> P2C_pipe[READ] = -1;    

    check_val = close (fd_out);
    check (check_val == -1);
    fd_out = children_info -> C2P_pipe[WRITE] = -1;
}

void parent_exec (struct cp_info* child_info, int children_quantity)
{
    int check_val = 0;
    int fd = -1;
	fd_set fd_out, fd_in;
    struct cp_connect* connection = (struct cp_connect*) calloc (children_quantity - 1, sizeof(struct cp_connect));
	check (connection == NULL);

	for (int i = 0; i < children_quantity - 1; i++) {
		create_buf (connection, child_info, i, children_quantity);

		check_val = fcntl(connection[i].fd_in, F_SETFL, O_RDONLY | O_NONBLOCK);
        check (check_val == -1);
        
		check_val = fcntl(connection[i].fd_out, F_SETFL, O_WRONLY | O_NONBLOCK);
        check (check_val == -1);
	}	

	size_t start_num = 0;
    int connections_quantity = children_quantity - 1;
	while (start_num < connections_quantity){
		FD_ZERO(&fd_out);
		FD_ZERO(&fd_in);

		for (int i = start_num; i < connections_quantity; i++){

			if ((connection[i].fd_in != -1) && connection[i].empty > 0)
				FD_SET(connection[i].fd_in, &fd_in);
			if ((connection[i].fd_out != -1) && connection[i].full > 0)
				FD_SET(connection[i].fd_out, &fd_out);

			if (connection[i].fd_in > fd)
				fd = connection[i].fd_in;
			if (connection[i].fd_out > fd)
				fd = connection[i].fd_out;
		}	

		check_val = select(fd + 1, &fd_in, &fd_out, NULL, NULL);
        check (check_val < 0);
	    fd = -1;			    

	    for (size_t i = start_num; i < connections_quantity; i++){
			if (FD_ISSET(connection[i].fd_in, &fd_in) && (connection[i].empty > 0))
				read_in_buf(&connection[i], i);
            if (FD_ISSET(connection[i].fd_out, &fd_out) && (connection[i].full > 0))
				write_from_buf(&connection[i], i);
			
	        if (connection[i].fd_in == -1 && connection[i].fd_out != -1 && connection[i].full == 0) {
				close(connection[i].fd_out);								
				connection[i].fd_out = -1;

				check (start_num != i);

	            start_num++;
				free(connection[i].data);
			}
		}
	}	
    for (size_t i = 0; i < children_quantity; i++){
        check_val = waitpid(child_info[i].child_pid, NULL, 0);
        check (check_val == -1);
    }
        
    free(connection);
}

void create_buf (struct cp_connect* connection, struct cp_info* child_info, int num, int child_quantity) {
    connection[num].size = pow (3, child_quantity - num) * 9;
	
    connection[num].data = (char*) calloc (1, connection[num].size);
	check (connection[num].data == NULL);

	connection[num].is_read = 0;
	connection[num].is_write = 0;

	connection[num].full = 0;
	connection[num].empty = connection[num].size;

	connection[num].fd_in = child_info[num].C2P_pipe[READ];
	connection[num].fd_out = child_info[num + 1].P2C_pipe[WRITE];
}

void read_in_buf(struct cp_connect* connection, int id) {
    int ret_read = read(connection -> fd_in, &connection -> data[connection -> is_read], connection -> empty);
    check (ret_read < 0);

    if (ret_read == 0) {
        close(connection -> fd_in);
        connection -> fd_in = -1;
        return;
    }

    if (connection -> is_read >= connection -> is_write)
        connection -> full += ret_read;

    if (connection -> is_read + ret_read == connection -> size) {
        connection -> is_read = 0;
        connection -> empty = connection -> is_write;
    }
    else {
        connection -> is_read += ret_read;
        connection -> empty -= ret_read;
    }
}

void write_from_buf (struct cp_connect* connection, int id) {
    errno = 0;
    int ret_write = write(connection -> fd_out, &connection -> data[connection -> is_write], connection -> full);
    check ((ret_write < 0) && (errno != EAGAIN));

    if (connection -> is_write >= connection -> is_read)
        connection->empty += ret_write;

    if (connection -> is_write + ret_write == connection -> size) {
        connection -> full = connection -> is_read;
        connection -> is_write = 0; 
    }
    else {
        connection -> full -= ret_write;
        connection -> is_write += ret_write; 
    }
}