#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_FILES 1000
#define MAX_PATH 1024

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

// 검색 결과를 저장하는 구조체
struct SearchResult {
    char filename[MAX_PATH];
    char fullpath[MAX_PATH];
    int permission;
};

// 세마포어 및 공유 메모리 식별자
int sem_id, shm_id;

// 디렉토리를 검색하고 검색 결과를 공유 메모리에 저장하는 함수
void search_directory(const char *path, int permission) {
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir = opendir(path);

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (lstat(full_path, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                // 디렉토리인 경우 자식 프로세스를 생성하여 검색
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    if (fork() == 0) {
                        search_directory(full_path, permission);
                        exit(0);
                    }
                }
            } else if (S_ISREG(file_stat.st_mode) && (file_stat.st_mode & 07777) == permission) {
                // 파일인 경우 검색 결과를 공유 메모리에 저장
                struct SearchResult *shared_memory = (struct SearchResult *)shmat(shm_id, NULL, 0);
                struct sembuf sem_lock = { 0, -1, SEM_UNDO };
                struct sembuf sem_unlock = { 0, 1, SEM_UNDO };
                // lock
                if (semop(sem_id, &sem_lock, 1) == -1) {
                    perror("sem lock");
                    exit(1);
                }

                for (int i = 0; i < MAX_FILES; i++) {
                    if (shared_memory[i].filename[0] == '\0') {
                        strncpy(shared_memory[i].filename, entry->d_name, sizeof(shared_memory[i].filename));
                        shared_memory[i].filename[sizeof(shared_memory[i].filename) - 1] = '\0';
                        strncpy(shared_memory[i].fullpath, path, sizeof(shared_memory[i].fullpath));
                        shared_memory[i].fullpath[sizeof(shared_memory[i].fullpath) - 1] = '\0';
                        shared_memory[i].permission = permission;
                        break;
                    }
                }
                // unlock
                if (semop(sem_id, &sem_unlock, 1) == -1) {
                    perror("sem unlock");
                    exit(1);
                }

                shmdt(shared_memory);
            }
        }
    }

    while (wait(NULL) != -1);
    closedir(dir);
}

int main(int argc, char *argv[]) {
    union semun semarg;
    semarg.val = 1;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <permission_in_octal>\n", argv[0]);
        return 1;
    }

    // 8진수 권한을 10진수로 변환
    int permission = strtol(argv[1], NULL, 8);

    // 현재 디렉토리 얻기
    char current_directory[MAX_PATH];
    if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
        perror("getcwd");
        return 1;
    }

    // 세마포어 및 공유 메모리 생성
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    shm_id = shmget(IPC_PRIVATE, MAX_FILES * sizeof(struct SearchResult), IPC_CREAT | 0666);

    // 세마포어 초기화
    semctl(sem_id, 0, SETVAL, 1);

    // 디렉토리 검색 시작
    search_directory(current_directory, permission);

    // 공유 메모리에서 검색 결과 출력
    struct SearchResult *shared_memory = (struct SearchResult *)shmat(shm_id, NULL, 0);
    for (int i = 0; i < MAX_FILES; i++) {
        if (shared_memory[i].filename[0] != '\0') {
            printf("%s,%s,%04o\n", shared_memory[i].filename, shared_memory[i].fullpath, shared_memory[i].permission);
        }
    }
    shmdt(shared_memory);

    // 공유 메모리 및 세마포어 삭제
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}
