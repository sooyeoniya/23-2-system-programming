#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

#define MAX_BUFFER 1024
#define NUM_PROCESSES 4
#define SHM_SIZE 4096

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void P(int semid) {
    struct sembuf p = {0, -1, SEM_UNDO};
    semop(semid, &p, 1);
}

void V(int semid) {
    struct sembuf v = {0, 1, SEM_UNDO};
    semop(semid, &v, 1);
}

void searchInFile(int shmid, int semid, char *filename, char *searchString, long start, long end) {
    FILE *fp = fopen(filename, "r");
    char buffer[MAX_BUFFER];
    char *shm_ptr;

    fseek(fp, start, SEEK_SET);
    if (start != 0) {
        fgets(buffer, MAX_BUFFER, fp);  // Move to the next line
    }

    shm_ptr = (char *)shmat(shmid, NULL, 0);  // Attach to shared memory
    if (shm_ptr == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }

    while (ftell(fp) < end && fgets(buffer, MAX_BUFFER, fp) != NULL) {
        if (strstr(buffer, searchString) != NULL) {
            P(semid);  // Wait operation on semaphore
            strcat(shm_ptr, buffer);  // Write to shared memory
            V(semid);  // Signal operation on semaphore
        }
    }

    shmdt(shm_ptr);  // Detach from shared memory
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <file> <string>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    char *searchString = argv[2];

    int shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    union semun u;
    u.val = 1;
    if (semctl(semid, 0, SETVAL, u) == -1) {
        perror("semctl");
        exit(1);
    }

    FILE *fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    long chunkSize = fileSize / NUM_PROCESSES;
    fclose(fp);

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            long start = i * chunkSize;
            long end = (i + 1) * chunkSize;
            searchInFile(shmid, semid, filename, searchString, start, end);
            exit(0);
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    // Print results from shared memory
    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    printf("%s", shm_ptr);
    shmdt(shm_ptr);

    // Clean up
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, u);

    return 0;
}
