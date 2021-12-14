#include "MyLib.h"

int main() {
	
	key_t key = ftok(shm_name, ftok_id);
	check (key == -1);

	int sem_id = semget(key, sem_quantity, IPC_CREAT | 0666);
	check (sem_id == -1);

	int check_val = semctl (sem_id, 0, IPC_RMID, 0); 
	if (check_val == -1) {
        printf ("Couldn't delete!\n");
    }
}