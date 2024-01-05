/**
 * 컴퓨터공학부 2020136129 최수연
*/

#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <dirent.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int initsem(key_t semkey) {
    union semun semunarg;
    int status = 0, semid;

    semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (semid == -1) {
        if (errno == EEXIST)
            semid = semget(semkey, 1, 0);
    }
    else {
        semunarg.val = 1;
        status = semctl(semid, 0, SETVAL, semunarg);
    }

    if (semid == -1 || status == -1) {
        perror("initsem");
        return (-1);
    }

    return semid;
}

int semlock(int semid) {
    struct sembuf buf;

    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;
    if (semop(semid, &buf, 1) == -1) {
        perror("semlock failed");
        exit(1);
    }
    return 0;
}

int semunlock(int semid) {
    struct sembuf buf;

    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    if (semop(semid, &buf, 1) == -1) {
        perror("semunlock failed");
        exit(1);
    }
    return 0;
}

int main(int argc, char** argv)
{
    DIR* dp;
    struct stat sb;
    struct dirent* dep;
    char *cwd;
    char buf[BUFSIZ];
    getcwd(buf, BUFSIZ);
    printf("cur_dir_location: %s\n", buf);
    if((dp = opendir(".")) == NULL) {
        printf("It is not found directory.\n");
    }

    // check current directory location
    cwd = getcwd(NULL, BUFSIZ);
    printf("cur_dir_location: %s\n", cwd);
    system("ls");

    // search file test code
    while (dep = readdir(dp)) {
        lstat(dep->d_name, &sb);
        char* dir1 = dep->d_name;
        if (chdir(dir1) < 0) {
            perror(dir1);
        }
        else {
            cwd = getcwd(NULL, BUFSIZ);
            printf("cur_dir_location: %s\n", cwd);
            int shmid, i;
            char *shmaddr, *shmaddr2;
            
            shmid = shmget(IPC_PRIVATE, 20, IPC_CREAT|0644);
            if (shmid == -1) {
                perror("shmget");
                exit(1);
            }
            
            for (int i = 0; i < 8; i++)
                switch (fork()) {
                    case -1:
                        perror("fork");
                        exit(1);
                        break;
                    case 0:
                        shmaddr = (char *)shmat(shmid, (char *)NULL, 0);
                        printf("Child Process =====\n");
                        int semid;
                        pid_t pid = getpid();

                        if ((semid = initsem(1)) < 0) exit(1);

                        semlock(semid);
                        printf("Lock : Process %d\n", (int)pid);
                        printf("** Lock Mode : Critical Section\n");
                        sleep(1);
                        printf("Unlock : Process %d\n", (int)pid);
                        semunlock(semid);

                        shmdt((char *)shmaddr);
                        exit(0);
                        break;
                    default:
                        wait(0);
                        shmaddr2 = (char *)shmat(shmid, (char *)NULL, 0);
                        printf("Parent Process =====\n");
                        
                        shmdt((char *)shmaddr2);
                        shmctl(shmid, IPC_RMID, (struct shmid_ds *)NULL);
                        break;
                }
            chdir("..");
            printf("cur_dir_location: %s\n", cwd);
        }
    }
}



