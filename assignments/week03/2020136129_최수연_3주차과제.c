#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <limits.h>

#define MAX_CMD_SIZE    (128)

/*
* 컴퓨터공학부 2020136129 최수연
*
* [요구사항 목록]
*
* (1) 기능 요구사항
* - 1. 프로그램을 실행하면 사용자 명령을 입력 받을 수 있는 prompt를 표시한다.
* - 2. 사용자가 명령을 입력하면 명령에 따라 아래 기능이 동작한다.
*       *. help : 명령어 목록과 기능 설명을 보여준다.
*       *. cd <path> : 현재 디렉토리를 path 경로로 이동한다.
*       *. mkdir <path> : path에 해당하는 디렉토리를 생성한다.
*       *. rmdir <path> : path에 해당하는 디렉토리를 삭제한다.
*       *. rename <source> <target> : source 디렉토리를 target 이름으로 변경한다.
*       *. ls : 현재 디렉토리의 내용(파일 및 sub 디렉토리 목록)을 보여준다.
*               이때 파일/디렉토리명과 함께 종류(파일/디렉토리/기타 등등)도 알 수 있도록 표시한다.
* - 3. 모든 기능은 수행이 완료된 후 성공/실패 여부를 출력한다.
*      실패시 어떤 에러가 원인인지 perror()를 사용하여 에러 메시지를 출력한다.
*
* (2) 기능 요구사항
* - 1. 이 프로그램내에서 최 상위 디렉토리(/)가 실제로는 /tmp/test 디렉토리가 된다.
*       *. 프로그램내에서 "cd/" 명령이 수행되면 실제로(프로그램 밖에서)는 /tmp/test 폴더로 이동한다.
*       *. 프로그램내에서 "cd /test2" 명령이 수행되면 실제로는 /tmp/test/test2 폴더로 이동한다.
*       *. 프로그램내에서 표시되는 현재 디렉토리 경로가 "/test2/test3" 라면 실제로는 "/tmp/test/test2/test3" 디렉토리에 위치한다.
*       *. 즉, 프로그램내에서는 실제 /tmp/test 디렉토리보다 상위 디렉토리로 이동 불가,
*          상위 디렉토리의 디렉토리 생성/삭제/읽기 불가 하도록 기능을 제한해야 한다.
* - 2. 만약 프로그램 실행시, /tmp/test 디렉토리가 존재하지 않는다면 /tmp/test 디렉토리를 생성해야 한다.
*/

// 권한 출력 관련 함수(drwxrwxrwx)
void rwx(int mode) {
    if (S_ISDIR(mode)) printf("d");
    else printf("-");
    if (mode & S_IRUSR) printf("r");
    else printf("-");
    if (mode & S_IWUSR) printf("w");
    else printf("-");
    if (mode & S_IXUSR) printf("x");
    else printf("-");
    if (mode & S_IRGRP) printf("r");
    else printf("-");
    if (mode & S_IWGRP) printf("w");
    else printf("-");
    if (mode & S_IXGRP) printf("x");
    else printf("-");
    if (mode & S_IROTH) printf("r");
    else printf("-");
    if (mode & S_IWOTH) printf("w");
    else printf("-");
    if (mode & S_IXOTH) printf("x ");
    else printf("- ");
}

int main(int argc, char** argv) {
    DIR* dp; // 디렉토리 포인터 선언
    // dirent, stat 구조체 선언
    struct dirent* dent;
    struct stat sb;
    // 문자열 포인터 선언
    char* command, * tok_str;
    char* current_dir = NULL;
    char* real_dir = "/tmp/test";

    // 문자열 메모리 동적 할당
    command = (char*)malloc(MAX_CMD_SIZE);
    current_dir = (char*)malloc(MAX_CMD_SIZE);
    sprintf(current_dir, "%s", "/"); // current_dir에 "/" 문자열 복사

    // command가 NULL일 경우 malloc 관련 에러 출력 후 프로그램 종료
    if (command == NULL) {
        perror("malloc");
        exit(1);
    }

    // 디렉토리 "/tmp/test"가 없으면 권한 0755로 생성
    if ((dp = opendir("/tmp/test")) == NULL) mkdir("/tmp/test", 0755);
    else closedir(dp);

    chdir("/tmp/test"); // 디렉토리 "/tmp/test"로 이동

    do {
        printf("%s $ ", current_dir); // 현재 경로 출력
        if (fgets(command, MAX_CMD_SIZE - 1, stdin) == NULL) break; // 명령어 입력 받기
        tok_str = strtok(command, " \n"); // 명령어를 공백 or \n으로 토큰화
        if (tok_str == NULL) continue; // 토큰 없으면 계속 진행
        if (strcmp(tok_str, "quit") == 0) { // quit를 입력하면 프로그램 종료
            break;
        }
        // 입력한 값이 help일 때 명령어 목록과 기능 설명을 보여줌
        else if (strcmp(tok_str, "help") == 0) {
            printf("help: 명령어 목록과 기능 설명을 보여준다.\n");
            printf("cd <path>: 현재 디렉토리를 path 경로로 이동한다.\n");
            printf("mkdir <path>: path에 해당하는 디렉토리를 생성한다.\n");
            printf("rmdir <path>: path에 해당하는 디렉토리를 삭제한다.\n");
            printf("rename <source> <target>: source 디렉토리를 target 이름으로 변경한다.\n");
            printf("ls: 현재 디렉토리의 내용(파일 및 sub 디렉토리 목록)을 보여준다.\n");
            printf("pwd: 현재 디렉토리를 보여준다.\n");
        }
        else if (strcmp(tok_str, "cd") == 0) { // 경로 이동
            bool isAuthorized = true; // 권한 여부
            char* current_path = strtok(NULL, " \n"); // 현재 경로에서 다음 토큰 추출하여 저장
            char* path2 = NULL;
            path2 = (char*)malloc(MAX_CMD_SIZE);
            char path[MAX_CMD_SIZE];

            // 절대 경로일 경우 앞에 /tmp/test를 붙임
            if (current_path[0] == '/') sprintf(path, "/tmp/test%s", current_path);
            else {
                char* tok;
                tok = (char*)malloc(MAX_CMD_SIZE);
                realpath(current_path, path2); // current_path 절대 경로로 변환 후 path2로 저장
                sprintf(path, "%s", path2); // path에 경로 저장

                // 현재 경로의 앞에 "/tmp/test"가 있는지 확인, 없으면 false
                tok = strtok(path2, "/");
                if (tok == NULL || strcmp(tok, "tmp") != 0) isAuthorized = false;
                tok = strtok(NULL, "/");
                if (tok == NULL || strcmp(tok, "test") != 0) isAuthorized = false;
                tok = strtok(NULL, "/");
            }
            // 권한이 없을 경우 없다고 화면에 출력 후 디렉토리 이동하지 않음
            if (isAuthorized == false) {
                printf("권한이 없습니다.\n");
                continue;
            }
            if (chdir(path) == -1) { // 디렉토리 이동
                perror("chdir"); // 에러 처리
            }
            else {
                // 현재 경로와 실제 경로에 현재 작업 경로 입력
                current_dir = getcwd(NULL, 0);
                real_dir = getcwd(NULL, 0);

                //printf("current_dir1: %s\n", current_dir);
                char new_dir[MAX_CMD_SIZE];
                char* tok_dir1 = strtok(current_dir, "/"); // 현재 경로의 "/"를 기준으로 토큰화
                int i = 0;
                while (tok_dir1 != NULL) {  // /tmp/test를 뗀 현재 경로를 추출하기 위한 작업
                    if (i >= 2) {
                        strcat(new_dir, "/");
                        strcat(new_dir, tok_dir1);
                    }
                    tok_dir1 = strtok(NULL, "/");
                    i++;
                }
                strcpy(current_dir, new_dir); // new_dir에 저장된 현재 경로를 current_dir에 저장
                memset(new_dir, 0, sizeof(new_dir)); // new_dir 메모리 초기화

                // 만약 current_dir이 아무것도 없으면 "/" 절대경로 넣어주기
                // 이는 실제로 "tmp/test"에 해당
                if (strcmp(current_dir, "") == 0) {
                    free(current_dir);
                    current_dir = "/";
                }
                //printf("current_dir2: %s\n", current_dir);
                //printf("real_dir: %s\n", real_dir);
            }
            memset(path2, 0, sizeof(path2)); // path2 메모리 초기화
        }
        else if (strcmp(tok_str, "mkdir") == 0) { // 디렉토리 생성
            bool isAuthorized = true; // 권한 여부
            char* current_path = strtok(NULL, " \n"); // 현재 경로에서 다음 토큰 추출하여 저장
            char* path2 = NULL;
            path2 = (char*)malloc(MAX_CMD_SIZE);
            char path[MAX_CMD_SIZE];

            // 절대 경로일 경우 앞에 /tmp/test를 붙임
            if (current_path[0] == '/') sprintf(path, "/tmp/test%s", current_path);
            else {
                char* tok;
                tok = (char*)malloc(MAX_CMD_SIZE);
                realpath(current_path, path2); // current_path 절대 경로로 변환 후 path2로 저장
                sprintf(path, "%s", path2); // path에 경로 저장

                // 현재 경로의 앞에 "/tmp/test"가 있는지 확인, 없으면 false
                tok = strtok(path2, "/");
                if (tok == NULL || strcmp(tok, "tmp") != 0) isAuthorized = false;
                tok = strtok(NULL, "/");
                if (tok == NULL || strcmp(tok, "test") != 0) isAuthorized = false;
                tok = strtok(NULL, "/");
            }
            // 권한이 없을 경우 없다고 화면에 출력 후 디렉토리 이동하지 않음
            if (isAuthorized == false) {
                printf("권한이 없습니다.\n");
                continue;
            }
            // 디렉토리 생성
            if (mkdir(path, 0755) == -1) {
                perror("mkdir");
            }
            else {
                // 최하위 디렉토리명 추출 및 출력
                char* tok_str2 = strtok(path, "/");
                char* find_tok = NULL;
                while (tok_str2 != NULL) {
                    find_tok = tok_str2;
                    tok_str2 = strtok(NULL, "/");
                }
                if (find_tok != NULL) {
                    printf("%s 디렉토리 생성됨\n", find_tok);
                }
            }
            memset(path, 0, sizeof(path)); // path 메모리 초기화
        }
        else if (strcmp(tok_str, "rmdir") == 0) { // 디렉토리 삭제
            bool isAuthorized = true; // 권한 여부
            char* current_path = strtok(NULL, " \n"); // 현재 경로에서 다음 토큰 추출하여 저장
            char* path2 = NULL;
            path2 = (char*)malloc(MAX_CMD_SIZE);
            char path[MAX_CMD_SIZE];

            // 절대 경로일 경우 앞에 /tmp/test를 붙임
            if (current_path[0] == '/') sprintf(path, "/tmp/test%s", current_path);
            else {
                char* tok;
                tok = (char*)malloc(MAX_CMD_SIZE);
                realpath(current_path, path2); // current_path 절대 경로로 변환 후 path2로 저장
                sprintf(path, "%s", path2); // path에 경로 저장

                // 현재 경로의 앞에 "/tmp/test"가 있는지 확인, 없으면 false
                tok = strtok(path2, "/");
                if (tok == NULL || strcmp(tok, "tmp") != 0) isAuthorized = false;
                tok = strtok(NULL, "/");
                if (tok == NULL || strcmp(tok, "test") != 0) isAuthorized = false;
                tok = strtok(NULL, "/");
            }
            // 권한이 없을 경우 없다고 화면에 출력 후 디렉토리 이동하지 않음
            if (isAuthorized == false) {
                printf("권한이 없습니다.\n");
                continue;
            }
            // 디렉토리 삭제
            if (rmdir(path) == -1) {
                perror("rmdir");
            }
            else {
                // 최하위 디렉토리명 추출 및 출력
                char* tok_str2 = strtok(path, "/");
                char* find_tok = NULL;
                while (tok_str2 != NULL) {
                    find_tok = tok_str2;
                    tok_str2 = strtok(NULL, "/");
                }
                if (find_tok != NULL) {
                    printf("%s 디렉토리 삭제됨\n", find_tok);
                }
            }
            memset(path, 0, sizeof(path)); // path 메모리 초기화
        }
        else if (strcmp(tok_str, "rename") == 0) { // 디렉토리 이름 변경
            char old_path[MAX_CMD_SIZE]; // 이전 경로
            char new_path[MAX_CMD_SIZE]; // 바꿀 경로
            // rename <old_path> <new_path> 각 위치 별 이전 경로, 바꿀 경로를 절대 경로로 저장
            snprintf(old_path, sizeof(old_path), "%s/%s", real_dir, strtok(NULL, " \n"));
            snprintf(new_path, sizeof(new_path), "%s/%s", real_dir, strtok(NULL, " \n"));
            // 디렉토리 이름 변경
            if (rename(old_path, new_path) == -1) {
                perror("rename");
            }
            else {
                // 최하위 디렉토리명 추출 및 출력
                char* tok_old = strtok(old_path, "/");
                char* find_old = NULL;
                while (tok_old != NULL) {
                    find_old = tok_old;
                    tok_old = strtok(NULL, "/");
                }
                char* tok_new = strtok(new_path, "/");
                char* find_new = NULL;
                while (tok_new != NULL) {
                    find_new = tok_new;
                    tok_new = strtok(NULL, "/");
                }
                printf("디렉토리 이름 %s에서 %s로 변경됨\n", find_old, find_new);
            }
            // 메모리 초기화
            memset(old_path, 0, sizeof(old_path));
            memset(new_path, 0, sizeof(new_path));
        }
        else if (strcmp(tok_str, "ls") == 0) { // 디렉토리 내용 읽기
            char buf[80];
            bool isAuthorized = true; // 권한 여부
            char* current_path = strtok(NULL, " \n"); // 현재 경로에서 다음 토큰 추출하여 저장
            char* path2 = NULL;
            path2 = (char*)malloc(MAX_CMD_SIZE);
            char path[MAX_CMD_SIZE];
            if (current_path == NULL) { // ls 명령어 뒤에 아무것도 없을 경우
                strcpy(path, getcwd(NULL, 0)); // 현재 디렉토리 경로 사용
            }
            else { // ls 뒤에 경로 붙을 경우
                // 절대 경로일 경우 앞에 /tmp/test를 붙임
                if (current_path[0] == '/') sprintf(path, "/tmp/test%s", current_path);
                else {
                    char* tok;
                    tok = (char*)malloc(MAX_CMD_SIZE);
                    realpath(current_path, path2); // current_path 절대 경로로 변환 후 path2로 저장
                    sprintf(path, "%s", path2); // path에 경로 저장
                    // 현재 경로의 앞에 "/tmp/test"가 있는지 확인, 없으면 false
                    tok = strtok(path2, "/");
                    if (tok == NULL || strcmp(tok, "tmp") != 0) isAuthorized = false;
                    tok = strtok(NULL, "/");
                    if (tok == NULL || strcmp(tok, "test") != 0) isAuthorized = false;
                    tok = strtok(NULL, "/");
                }
            }
            // 권한이 없을 경우 없다고 화면에 출력 후 디렉토리 이동하지 않음
            if (isAuthorized == false) {
                printf("권한이 없습니다.\n");
                continue;
            }
            if ((dp = opendir(path)) == NULL) { // 디렉토리 열기
                printf("opendir() error\n");
            }
            //printf("현재 디렉토리 위치 : %s\n", getcwd(NULL, 0));
            while ((dent = readdir(dp))) {
                stat(dent->d_name, &sb); // 해당 디렉토리 내 정보 가져오기
                rwx(sb.st_mode); // 권한 출력
                printf("%-8s ", dent->d_name); // 이름 출력
                printf("%-4o ", (unsigned int)sb.st_nlink); // 링크 수 출력
                printf("%-8d ", (int)dent->d_ino); // inode 번호 출력
                printf("%-8s ", (getpwuid(sb.st_uid))->pw_name); // 소유자 이름 출력
                printf("%-5d ", (int)sb.st_uid); // 소유자 UID 출력
                printf("%-5d ", (int)sb.st_size); // 디렉토리 or 파일 크기 출력
                strftime(buf, sizeof(buf), "%m월%d일 %H:%M\n", localtime(&(sb.st_atime))); // 접근 시간 출력
                printf("%s", buf);
                memset(buf, 0, sizeof(buf));
            }
            closedir(dp); // 디렉토리 닫기
            free(path2);
            memset(path, 0, sizeof(path)); // path 메모리 초기화
        }
        else if (strcmp(tok_str, "pwd") == 0) { // 현재 디렉토리 출력
            printf("프로그램 내 디렉토리 위치: %s\n", current_dir);
            printf("실제 디렉토리 위치: %s\n", real_dir);
        }
        // 없는 명령어일 경우 출력
        else {
            printf("your command: %s\n", tok_str);
            printf("and argument is ");
            tok_str = strtok(NULL, " \n");
            if (tok_str == NULL) {
                printf("NULL\n");
            }
            else {
                printf("%s\n", tok_str);
            }
        }
    } while (1);
    free(command); // 메모리 해제
    return 0;
}