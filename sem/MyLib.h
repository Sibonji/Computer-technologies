#ifndef MY_LIB
#define MY_LIB

#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define mem_size 5
#define shm_name "my_shm"
#define ftok_id 123456
#define sem_quantity 9

#define check(status) {                                                  \
    if (status) {                                                        \
	    perror ("Anything!!!");                                          \
        fprintf (stderr, "Error in programm: %s, func: %s, line: %d!\n", \
                                      __FILE__, __FUNCTION__, __LINE__); \
                                                                         \
        exit (EXIT_FAILURE);                                             \
    }                                                                    \
}

enum {
	is_writer = 0,
	is_reader = 1,
	is_connect_writer = 2,
	is_connect_reader = 3,
	is_sum_reader_connect = 4,
	is_sum_writer_connect = 5,
	is_sum_connect = 6,
	is_empty = 7,
	is_full = 8
};

union semun {
	int val; 				
	struct semid_ds* buf; 	
	unsigned short* array; 
};

int sem_get (int sem_id, int sem_num) {
    int get_val = semctl(sem_id, sem_num, GETVAL);

	return get_val;
}

int init_sem (int sem_id, int sem_num, int init_val) {
	union semun semopts;
	semopts.val = init_val;
	return semctl(sem_id, sem_num, SETVAL, semopts);
}

void print_sem(int sem_id) {

	printf("is_writer = %d\n", sem_get(sem_id, is_writer));
	printf("is_reader = %d\n", sem_get(sem_id, is_reader));
	printf("is_connect_writer = %d\n", sem_get(sem_id, is_connect_writer));
	printf("is_connect_reader = %d\n", sem_get(sem_id, is_connect_reader));
	printf("is_sum_reader_connect = %d\n", sem_get(sem_id, is_sum_reader_connect));
	printf("is_sum_writer_connect = %d\n", sem_get(sem_id, is_sum_writer_connect));
	printf("is_sum_connect = %d\n", sem_get(sem_id, is_sum_connect));
	printf("is_empty = %d\n", sem_get(sem_id, is_empty));
	printf("is_full = %d\n", sem_get(sem_id, is_full));
}

void work_with_data (struct sembuf* sem_data, int num, int sem_num, int sem_op, int sem_flag) {
    sem_data[num].sem_num = sem_num;
    sem_data[num].sem_op = sem_op;
    sem_data[num].sem_flg = sem_flag;

    return;
}

#endif