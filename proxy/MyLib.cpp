#include "MyLib.hpp"

//                                   Standart functions                                   \\

long int get_num(int argc, char* argv[], const int arg_quantity) {
    long int children_quantity = 0;
    char* end;

    if(argc != arg_quantity) {
        fprintf(stderr, "Incorrect children_quantityumber of arguments!!!\n");

        return -1;
    }

    children_quantity = strtol(argv[1], &end, 10);

    if (children_quantity <= 0 || children_quantity > MAX_CHILDREN_QUANTITY) {
        fprintf(stderr, "Write correct children_quantityumber instead of: %s!!!\n", argv[1]);

        return -1;
    }
    if (errno != 0) {
        fprintf (stderr, "Overflow!\n");

        return -1;
    }
    else {
        return children_quantity;
    }
}

void Close (int fd) {
    if (fd != POISON) {
        Close (fd);
        fd = POISON;
    }

    return;
}

bool is_full (cp_buf* buf) {
	return buf->is_full;
}

bool is_empty (cp_buf* buf) {
	return !buf->is_full && (buf->reading == buf->writing);
}

int read_buf (cp_info* cps, int id, int n) {
	cp_buf* cur = &cps[id].buf;

	if (is_full (cur)) {
		fprintf (stderr, "Attempt to read in a full buf in connection %d\n", id);
		clear_buf (cps, n);
		exit (EXIT_FAILURE);
	}

	int check_val = -1;
	
	if (cur->writing > cur->reading) {
		check_val = read (cps[id].C2P_pipe_fd [IS::READ], cur->start_ptr + cur->reading, cur->writing - cur->reading);
	}
	else {
		check_val = read (cps[id].C2P_pipe_fd [IS::READ], cur->start_ptr + cur->reading, cur->size - cur->reading);
	}

	if (check_val < 0) {
		fprintf (stderr, "Error reading to buf in connection %d\n", id);
		clear_buf (cps, n);
		exit (EXIT_FAILURE);
	}

	cur->reading += check_val;
    cur->reading %= cur->size;

	if (check_val != 0) {
		if (cur->reading == cur->writing) {
			cur->is_full = true;
		}
	}

	return check_val;
}

void clear_buf (cp_info* cps, int children_quantity) {
	for (unsigned int i = 0; i < children_quantity; ++i) {
		free (cps[i].buf.start_ptr);
	}
}

int write_buf (cp_info* cps, int id, int n) {
	cp_buf* cur = &cps[id].buf;
	int check_val = -1;

	if (is_empty (cur)) {
		fprintf (stderr, "Attempt to write from an empty buf in connection %d\n", id);
		clear_buf (cps, n);
		exit (EXIT_FAILURE);
	}
	
    if (cur->writing >= cur->reading) {
		if ((cur->writing == cur->reading && is_full (cur)) || cur->writing != cur->reading) {
	    	check_val = write (cps[id].P2C_pipe_fd [IS::WRITE], cur->start_ptr + cur->writing, cur->size - cur->writing);
		}
	}
    else {
        check_val = write (cps[id].P2C_pipe_fd [IS::WRITE], cur->start_ptr + cur->writing, cur->reading - cur->writing);
    }
	
	if (check_val < 0 && errno != EAGAIN) {
		fprintf (stderr, "Error writing from buf in connection %d\n", id);
		clear_buf (cps, n);
		exit (EXIT_FAILURE);
	}

	cur->writing += check_val;
    cur->writing %= cur->size;

	if (check_val != 0) {
		//If something was readed that means that buffer is not full
		cur->is_full = false;
    }

	return check_val;
}

//                                   Parent fucntions                                   \\

int parent_create (cp_info* cps, int child_number, int children_quantity, pid_t child_pid) {
    Close (cps[child_number].C2P_pipe_fd[IS::WRITE]);	
	
    if (child_number == children_quantity - 1) {
		Close (cps[child_number].P2C_pipe_fd[IS::READ]);
		Close (cps[child_number].P2C_pipe_fd[IS::WRITE]);
		
        cps[child_number].P2C_pipe_fd[IS::WRITE] = STDOUT_FILENO;
	}
	
	cps[child_number].child_pid = child_pid;

    return 0;
}

void parent_exec (cp_info* cps, int children_quantity) {
	for (unsigned int i = 0; i < children_quantity; ++i) { //Close children file descriptors instead of last
		Close (cps[i].P2C_pipe_fd[IS::READ]);
	}
    
    for (unsigned int i = 0; i < children_quantity; ++i) {
			cp_info* cur = &cps[i];
			cur->buf.size = pow (3, children_quantity - i);
			cur->buf.start_ptr = (char *) calloc (cur->buf.size, sizeof (char));

			if (!cur->buf.start_ptr) {
				fprintf (stderr, "Error allocating buf\n");
				clear_buf (cps, children_quantity);
				exit (EXIT_FAILURE);
			}

			fcntl (cur->C2P_pipe_fd[IS::READ],  F_SETFL, O_NONBLOCK);
			fcntl (cur->P2C_pipe_fd[IS::WRITE], F_SETFL, O_NONBLOCK);
		}
		
	fd_set readFd {}, writeFd {};
	int dead_children_quant = 0;
	
	while (dead_children_quant != children_quantity) {
		FD_ZERO (&readFd);
		FD_ZERO (&writeFd);
		int max_fd = 0;
		
		for (unsigned i = dead_children_quant; i < children_quantity; ++i) {
			int read_fd  = cps[i].C2P_pipe_fd[IS::READ];
			int write_fd = cps[i].P2C_pipe_fd[IS::WRITE];

			if (!is_full (&cps[i].buf) && (read_fd != POISON)) {
				FD_SET (read_fd, &readFd);
				max_fd = (read_fd > max_fd ? read_fd : max_fd);
			}
			
			if (!is_empty (&cps[i].buf) && (write_fd != POISON)) {
				FD_SET (write_fd, &writeFd);
				max_fd = (write_fd > max_fd ? write_fd : max_fd);
			}
		}

		int check_val = select (max_fd + 1, &readFd, &writeFd, nullptr, nullptr);
		if (check_val < 0) {
			fprintf (stderr, "Select error\n");
			
            if (errno == EINTR) {
				fprintf (stderr, "Signal interrupt caught\n");
				continue;
			}
			
            clear_buf (cps, children_quantity);
			exit (EXIT_FAILURE);
		}
        else if (check_val == 0) {
			continue;
		}

		for (int i = dead_children_quant; i < children_quantity; ++i) {
			if (FD_ISSET (cps[i].C2P_pipe_fd[IS::READ], &readFd)) { 
            	int check_val = read_buf (cps, i, children_quantity);

				if (check_val == 0) {
            	    Close (cps[i].C2P_pipe_fd[IS::READ]);
            	}
			}
        
        	if (FD_ISSET (cps[i].P2C_pipe_fd[IS::WRITE], &writeFd)) {
        	    int check_val = write_buf (cps, i, children_quantity);
        	}
			
        	if (is_empty (&cps[i].buf) && (cps[i].C2P_pipe_fd[IS::READ] == POISON)) {
				waitpid (cps[i].child_pid, nullptr, 0);
				
        	    if (i != dead_children_quant++) {
        	        fprintf (stderr, "Wrong child death sequence\n");
        	        clear_buf (cps, children_quantity);
        	        exit (EXIT_FAILURE);
        	    }

				Close (cps[i].P2C_pipe_fd[IS::WRITE]);
        	}
		}
    }
}

//                                   Child  functions                                   \\

int child_create  (cp_info* cps, int child_number, int children_quantity, int* argv_fd, char* filepath) {
    if (child_number == 0) {	//First child is reading from file argv[2]
		*argv_fd = open (filepath, O_RDONLY);
		
        if (*argv_fd < 0) {
			fprintf (stderr, "Error opening file\n");
			clear_buf (cps, children_quantity);
    		exit (EXIT_FAILURE);
		}
	}
    else {
		Close (cps[child_number - 1].P2C_pipe_fd[IS::WRITE]); //This pipe belong to the last connection
    }

	Close (cps[child_number].C2P_pipe_fd[IS::READ]);

	//This two file descriptors are belong to the parent and we don't children_quantityeed them
	Close (cps[child_number].P2C_pipe_fd[IS::READ]);
	Close (cps[child_number].P2C_pipe_fd[IS::WRITE]);

	for (unsigned int i = 0; i < child_number; ++i) { //Because of fork we children_quantityeed to Close unnecessary file descriptors
		Close (cps[i].P2C_pipe_fd[IS::WRITE]);
		Close (cps[i].C2P_pipe_fd[IS::READ]);
	}

	prctl (PR_SET_PDEATHSIG, SIGKILL);
    check (getpid () != cps[child_number].parent_pid);

    return 0;
}

void child_exec (cp_info* cps, int proc_id, int argv_fd) {
    int check_val = 0;

	while (true) {
		if (proc_id == 0) {
			check_val = splice (argv_fd,  NULL, \
							 cps[proc_id].C2P_pipe_fd[IS::WRITE], NULL, MAX_MSG_SIZE, SPLICE_F_MOVE);
		}
		else {
        	check_val = splice (cps[proc_id - 1].P2C_pipe_fd[IS::READ],  NULL, \
							 cps[proc_id].C2P_pipe_fd[IS::WRITE], NULL, MAX_MSG_SIZE, SPLICE_F_MOVE);
		}

		if (check_val == 0) {
            Close (cps[proc_id].C2P_pipe_fd[IS::WRITE]);
			if (proc_id == 0) {
				Close (argv_fd);
			}
			else {				
            	Close (cps[proc_id - 1].P2C_pipe_fd[IS::READ]);
			}

            exit (EXIT_SUCCESS);
        }

        check (check_val < 0);
    }
}