const int arg_quantity = 3;

#include "MyLib.hpp"

int main (int argc, char* argv[]) {
    int children_quantity = get_num (argc, argv, arg_quantity);
    if (children_quantity < 0) {
        exit (EXIT_FAILURE);
    }

    cp_info* cps = (cp_info*) calloc (children_quantity, sizeof (cp_info));

    int is_parent = -1; //If is_parent == -1 --> this is parent
    int argv_fd = POISON;

    
    for (unsigned int i = 1; i <= children_quantity; i++) { //initializing children and pipes
        pipe (cps[i].C2P_pipe_fd);
        pipe (cps[i].P2C_pipe_fd);

        cps[i].parent_pid = getpid ();
        pid_t child_pid = fork ();
        check (child_pid < 0);

        if (child_pid != 0) { //Parent
            parent_create (cps, i, children_quantity, child_pid);
        }
        else () {             //Child
            is_parent = i;
            child_create (cps, i, children_quantity, argv_fd, argv[1]);
        }
    }

    //Use children and pioes to copy info
    if (is_parent == -1) {	//	PARENT
		
	}
	else { 				// CHILD
		
	}

    return 0;
}