#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>

#define NUM_GROUPS 3
#define NUM_SECTIONS 3
#define TREASURE_FOUND 101

int sem_id, shm_id;
int *sections;

struct sembuf start_work = {0, -1, SEM_UNDO};
struct sembuf stop_work = {0, 1, SEM_UNDO};
struct sembuf init_sections = {0, 1, SEM_UNDO};

void init_semaphore() {
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Error in semget\n");
        exit(1);
    }
    if (semop(sem_id, &init_sections, 1) == -1) {
        perror("Error in semop\n");
        exit(1);
    }
}

void init_shared_memory() {
    struct shmid_ds buf;
    shm_id = shmget(IPC_PRIVATE, sizeof(int)*NUM_GROUPS*NUM_SECTIONS,
                    IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error in shmget\n");
        exit(1);
    }
    sections = shmat(shm_id, NULL, 0);
    if (*sections == -1) {
        perror("Error in shmat\n");
        exit(1);
    }
    if (shmctl(shm_id, IPC_STAT, &buf) == -1) {
        perror("Error in shmctl\n");
        exit(1);
    }
    buf.shm_perm.mode = 0666;
    if (shmctl(shm_id, IPC_SET, &buf) == -1) {
        perror("Error in shmctl\n");
        exit(1);
    }
}

void semaphore_wait() {
    if (semop(sem_id, &start_work, 1) == -1) {
        perror("Error in semop\n");
        exit(1);
    }
}

void semaphore_signal() {
    if (semop(sem_id, &stop_work, 1) == -1) {
        perror("Error in semop\n");
        exit(1);
    }
}

void pirate_work(int pirate_num) {
    int start_sec = pirate_num * NUM_SECTIONS / NUM_GROUPS;
    int end_sec = start_sec + NUM_SECTIONS / NUM_GROUPS;
    printf("Pirate %d is searching sections %d to %d...\n",
           pirate_num, start_sec, end_sec-1);
    sleep(5);
    for (int i = start_sec; i < end_sec; i++) {
        if (sections[i] == TREASURE_FOUND) {
            printf("Pirate %d found treasure at section %d!\n", pirate_num, i);
            semaphore_signal();
            exit(0);
        }
    }
}

void wait_for_pirates() {
    int num_pirates = NUM_GROUPS * NUM_SECTIONS / 2;
    pid_t pid;
    for (int i = 0; i < num_pirates; i++) {
        pid = fork();
        if (pid == -1) {
            perror("Error in fork\n");
            exit(1);
        }
        else if (pid == 0) {
            semaphore_wait();
            pirate_work(i);
            exit(0);
        }
    }
}

void cleanup(int sig) {
    shmdt(sections);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    init_semaphore();
    init_shared_memory();
    wait_for_pirates();

    // pirate work is complete, but no treasure was found
    printf("No treasure was found!\n");
    cleanup(0);
    return 0;
}