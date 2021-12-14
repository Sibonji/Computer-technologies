#include "MyLib.h"

int main (int argc, char* argv[]) {
    int check_val = 0;

    key_t key = ftok (shm_name, ftok_id);
    check (key == -1);

    int sem_id = semget (key, sem_quantity, IPC_CREAT | 0666);
	check (sem_id == -1);

    int shm_id = shmget (key, mem_size, IPC_CREAT | 0666);
	check (shm_id == -1);

    struct sembuf sem_data[2];
    struct sembuf is_connected[3];
    struct sembuf wait_connection[2];
    struct sembuf check_overflow[7];
  	struct sembuf switch_control[5]; 

    if (argc == 1) { //Reader
        work_with_data (sem_data, 0, is_reader, 0, 0);
        work_with_data (sem_data, 1, is_reader, 1, SEM_UNDO);

        check_val = semop(sem_id, sem_data, 2);
        check (check_val == -1);

        work_with_data (is_connected, 0, is_sum_reader_connect, 1, 0);
        work_with_data (is_connected, 1, is_sum_connect, 1, 0);
        work_with_data (is_connected, 2, is_connect_reader, 1, SEM_UNDO);

        check_val = semop (sem_id, is_connected, 3);
        if (check_val != -1);

        char* shm = shmat (shm_id, NULL, 0);
        check (shm == -1);

        char data[mem_size];
	    int is_readed = 1;
        int is_writed = -1;

        work_with_data (wait_connection, 0, is_connect_reader, -1, 0);
        work_with_data (wait_connection, 1, is_connect_reader, 1, 0);

        check_val = semop(sem_id, wait_connection, 2);
        check (check_val == -1);

        int sem_connect = sem_get (sem_id, is_sum_writer_connect);

        while (1) {
            work_with_data (switch_control, 0, is_sum_connect, -sem_connect * 2, IPC_NOWAIT);
            work_with_data (switch_control, 1, is_sum_connect, sem_connect * 2, 0);
            work_with_data (switch_control, 2, is_connect_writer, -1, IPC_NOWAIT);
            work_with_data (switch_control, 3, is_connect_writer, 1, 0);
            work_with_data (switch_control, 4, is_full, -1, 0);

            check_val = semop (sem_id, switch_control, 5);
            check (check_val != 0);

            is_writed = *((int *) shm);
            shm += 4;
            strncpy (data, shm, is_writed);

            for (int i = is_writed; i != mem_size; i++) {
                shm[i] = 0;
            }

            if (is_writed > 0) {
                printf ("%s", shm);
            }

            work_with_data (switch_control, 0, is_sum_connect, -sem_connect * 2, IPC_NOWAIT);
            work_with_data (switch_control, 1, is_sum_connect, sem_connect * 2, 0);
            work_with_data (switch_control, 2, is_connect_writer, -1, IPC_NOWAIT);
            work_with_data (switch_control, 3, is_connect_writer, 1, 0);
            work_with_data (switch_control, 4, is_empty, 1, 0);

            check_val = semop (sem_id, switch_control, 5);
            check (check_val == -1 && is_writed != 0);

            if (is_writed == 0) break;
        }

        check_val = shmdt (shm);
        check (check_val == -1);
    }
    else if (argc == 2) {	//	WRITER
        work_with_data (sem_data, 0, is_writer, 0, 0);
        work_with_data (sem_data, 1, is_writer, 1, SEM_UNDO);

        check_val = semop(sem_id, sem_data, 2);
        check (check_val == -1);

        work_with_data (is_connected, 0, is_sum_writer_connect, 1, 0);
        work_with_data (is_connected, 1, is_sum_connect, 1, 0);
        work_with_data (is_connected, 2, is_connect_writer, 1, SEM_UNDO);

        check_val = semop (sem_id, is_connected, 3);
        check (check_val == -1);

        init_sem (sem_id, is_full, 0);
        init_sem (sem_id, is_full, 1);

        char* shm = shmat (shm_id, NULL, 0);
        check (shm == -1);

        int fd = open (argv[1], O_RDONLY);
        check (fd == -1);

        char data[mem_size + sizeof (int)];
        int is_readed = 0;
        int is_writed = 0;

        work_with_data (wait_connection, 0, is_connect_reader, -1, 0);
        work_with_data (wait_connection, 1, is_connect_reader, 1, 0);

        check_val = semop(sem_id, wait_connection, 2);
        check (check_val != -1);

        work_with_data (check_overflow, 0, is_sum_connect, -100, IPC_NOWAIT);
        work_with_data (check_overflow, 1, is_sum_connect, 100, 0);
        work_with_data (check_overflow, 2, is_connect_reader, -1, IPC_NOWAIT);
        work_with_data (check_overflow, 3, is_connect_reader, 1, 0);
        work_with_data (check_overflow, 4, is_sum_connect, -sem_get(sem_id, is_sum_connect), 0);
        work_with_data (check_overflow, 5, is_sum_reader_connect, -sem_get(sem_id, is_sum_reader_connect), 0);
        work_with_data (check_overflow, 6, is_sum_writer_connect, -sem_get(sem_id, is_sum_writer_connect), 0);

        check_val = semop (sem_id, check_overflow, 7);
        check (check_val != -1);

        int sem_connect = sem_get (sem_id, is_sum_writer_connect);

        while (1) {
            work_with_data (switch_control, 0, is_sum_connect, -sem_connect * 2, IPC_NOWAIT);
            work_with_data (switch_control, 1, is_sum_connect, sem_connect * 2, 0);
            work_with_data (switch_control, 2, is_connect_reader, -1, IPC_NOWAIT);
            work_with_data (switch_control, 3, is_connect_reader, 1, 0);
            work_with_data (switch_control, 4, is_empty, -1, 0);

            check_val = semop (sem_id, switch_control, 5);
            check (check_val != 0);

            is_readed = read (fd, data + sizeof (int), mem_size);
            check (is_readed == -1);

            *((int *) shm) = is_readed;
            strncpy (shm + 4, data + 4, mem_size);

            work_with_data (switch_control, 0, is_sum_connect, -sem_connect * 2, IPC_NOWAIT);
            work_with_data (switch_control, 1, is_sum_connect, sem_connect * 2, 0);
            work_with_data (switch_control, 2, is_connect_writer, -1, IPC_NOWAIT);
            work_with_data (switch_control, 3, is_connect_writer, 1, 0);
            work_with_data (switch_control, 4, is_full, 1, 0);

            check_val = semop (sem_id, switch_control, 5);
            check (check_val == -1 && is_writed != 0);

            if (is_writed == 0) break;
        }

        check_val = shmdt (shm);
        check (check_val == -1);

        close (fd);
	}
    else {
        fprintf (stderr, "Incorrect input!\n");
        exit (EXIT_FAILURE);
    }

    

    return 0;
}