#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <limits.h>

#define MAX_CMDLINE_SIZE    (128)
#define MAX_CMD_SIZE        (32)
#define MAX_ARG             (4)

typedef int  (*cmd_func_t)(int argc, char** argv);
typedef void (*usage_func_t)(void);

typedef struct cmd_t {
    char            cmd_str[MAX_CMD_SIZE];
    cmd_func_t      cmd_func;
    usage_func_t   usage_func;
    char            comment[128];
} cmd_t;

#define DECLARE_CMDFUNC(str)    int cmd_##str(int argc, char **argv); \
                                void usage_##str(void)

DECLARE_CMDFUNC(help);
DECLARE_CMDFUNC(mkdir);
DECLARE_CMDFUNC(rmdir);
DECLARE_CMDFUNC(cd);
DECLARE_CMDFUNC(mv);
DECLARE_CMDFUNC(ls);
DECLARE_CMDFUNC(ln);
DECLARE_CMDFUNC(rm);
DECLARE_CMDFUNC(chmod);
DECLARE_CMDFUNC(cat);
DECLARE_CMDFUNC(cp);
DECLARE_CMDFUNC(quit);

/* Command List */
static cmd_t cmd_list[] = {
    {"help",    cmd_help,    usage_help,  "show usage, ex) help <command>"},
    {"mkdir",   cmd_mkdir,   usage_mkdir, "create directory"},
    {"rmdir",   cmd_rmdir,   usage_rmdir, "remove directory"},
    {"cd",      cmd_cd,      usage_cd,    "change current directory"},
    {"mv",      cmd_mv,      usage_mv,    "rename directory & file"},
    {"ls",      cmd_ls,      NULL,        "show directory contents"},
    {"ln",    	cmd_ln,      usage_ln,    "create hard link & symbolic link"},
    {"rm",    	cmd_rm,      usage_rm,    "remove directory & file"},
    {"chmod",   cmd_chmod,   usage_chmod, "change access permissions"},
    {"cat",    	cmd_cat,     usage_cat,   "show file contents"},
    {"cp",    	cmd_cp,      usage_cp,    "copy file"},
    {"quit",    cmd_quit,    NULL,        "terminate shell"},
};

const int command_num = sizeof(cmd_list) / sizeof(cmd_t);
static char* chroot_path = "/tmp/test";
static char* current_dir;

static int search_command(char* cmd)
{
    int i;

    for (i = 0; i < command_num; i++) {
        if (strcmp(cmd, cmd_list[i].cmd_str) == 0) {
            /* found */
            return (i);
        }
    }

    /* not found */
    return (-1);
}

static void get_realpath(char* usr_path, char* result)
{
    char* stack[32];
    int   index = 0;
    char  fullpath[128];
    char* tok;
    int   i;
#define PATH_TOKEN   "/"

    if (usr_path[0] == '/') {
        strncpy(fullpath, usr_path, sizeof(fullpath) - 1);
    }
    else {
        snprintf(fullpath, sizeof(fullpath) - 1, "%s/%s", current_dir + strlen(chroot_path), usr_path);
        // printf("fullpath: %s\n", fullpath);
    }

    /* parsing */
    tok = strtok(fullpath, PATH_TOKEN);
    if (tok == NULL) {
        goto out;
    }

    do {
        if (strcmp(tok, ".") == 0 || strcmp(tok, "") == 0) {
            ; // skip
        }
        else if (strcmp(tok, "..") == 0) {
            if (index > 0) index--;
        }
        else {
            stack[index++] = tok;
        }
    } while ((tok = strtok(NULL, PATH_TOKEN)) && (index < 32));

out:
    strcpy(result, chroot_path);

    // TODO: boundary check
    for (i = 0; i < index; i++) {
        strcat(result, "/");
        strcat(result, stack[i]);
    }
}

// 권한 출력 함수(e.g. drwxrwxrwx)
static void rwx(int mode)
{
    // 파일 종류
    if (S_ISREG(mode)) printf("-");
    else if (S_ISDIR(mode)) printf("d");
    else if (S_ISCHR(mode)) printf("c");
    else if (S_ISBLK(mode)) printf("b");
    else if (S_ISFIFO(mode)) printf("p");
    else if (S_ISLNK(mode)) printf("l");
    else if (S_ISSOCK(mode)) printf("s");

    // 대상별 파일 권한
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

int main(int argc, char** argv)
{
    char* command, * tok_str;
    char* cmd_argv[MAX_ARG];
    int  cmd_argc, i, ret;

    command = (char*)malloc(MAX_CMDLINE_SIZE);
    if (command == NULL) {
        perror("command malloc");
        exit(1);
    }

    current_dir = (char*)malloc(MAX_CMDLINE_SIZE);
    if (current_dir == NULL) {
        perror("current_dir malloc");
        free(command);
        exit(1);
    }

    if (chdir(chroot_path) < 0) {
        mkdir(chroot_path, 0755);
        chdir(chroot_path);
    }

    // chroot(chroot_path);

    do {
        if (getcwd(current_dir, MAX_CMDLINE_SIZE) == NULL) {
            perror("getcwd");
            break;
        }

        if (strlen(current_dir) == strlen(chroot_path)) {
            printf("/"); // for root path
        }
        printf("%s $ ", current_dir + strlen(chroot_path));

        if (fgets(command, MAX_CMDLINE_SIZE - 1, stdin) == NULL) break;

        /* get arguments */
        tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

        cmd_argv[0] = tok_str;

        for (cmd_argc = 1; cmd_argc < MAX_ARG; cmd_argc++) {
            if (tok_str = strtok(NULL, " \n")) {
                cmd_argv[cmd_argc] = tok_str;
            }
            else {
                break;
            }
        }

        /* search command in list and call command function */
        i = search_command(cmd_argv[0]);
        if (i < 0) {
            printf("%s: command not found\n", cmd_argv[0]);
        }
        else {
            if (cmd_list[i].cmd_func) {
                ret = cmd_list[i].cmd_func(cmd_argc, cmd_argv);
                if (ret == 0) {
                    printf("return success\n");
                }
                else if (ret == -2 && cmd_list[i].usage_func) {
                    printf("usage : ");
                    cmd_list[i].usage_func();
                }
                else {
                    printf("return fail(%d)\n", ret);
                }
            }
            else {
                printf("no command function\n");
            }
        }
    } while (1);

    free(command);
    free(current_dir);

    return 0;
}


int cmd_help(int argc, char** argv)
{
    int i;

    if (argc == 1) {
        for (i = 0; i < command_num; i++) {
            printf("%32s: %s\n", cmd_list[i].cmd_str, cmd_list[i].comment);
        }
    }
    else if (argv[1] != NULL) {
        i = search_command(argv[1]);
        if (i < 0) {
            printf("%s command not found\n", argv[1]);
        }
        else {
            if (cmd_list[i].usage_func) {
                printf("usage : ");
                cmd_list[i].usage_func();
                return (0);
            }
            else {
                printf("no usage\n");
                return (-2);
            }
        }
    }

    return (0);
}

int cmd_mkdir(int argc, char** argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);

        if ((ret = mkdir(rpath, 0755)) < 0) {
            perror(argv[0]);
        }
    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_rmdir(int argc, char** argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);

        if ((ret = rmdir(rpath)) < 0) {
            perror(argv[0]);
        }
    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_cd(int argc, char** argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);

        if ((ret = chdir(rpath)) < 0) {
            perror(argv[0]);
        }
    }
    else {
        ret = -2;
    }

    return (ret);
}

int cmd_mv(int argc, char** argv)
{
    int  ret = 0;
    char rpath1[128];
    char rpath2[128];

    if (argc == 3) {

        get_realpath(argv[1], rpath1);
        get_realpath(argv[2], rpath2);

        if ((ret = rename(rpath1, rpath2)) < 0) {
            perror(argv[0]);
        }
    }
    else {
        ret = -2;
    }

    return (ret);
}

static const char* get_type_str(char type)
{
    switch (type) {
    case DT_BLK:
        return "BLK";
    case DT_CHR:
        return "CHR";
    case DT_DIR:
        return "DIR";
    case DT_FIFO:
        return "FIFO";
    case DT_LNK:
        return "LNK";
    case DT_REG:
        return "REG";
    case DT_SOCK:
        return "SOCK";
    default: // include DT_UNKNOWN
        return "UNKN";
    }
}

int cmd_ls(int argc, char** argv)
{
    int ret = 0;
    DIR* dp;
    struct dirent* dep;
    struct stat sb;

    if (argc != 1) {
        ret = -2;
        goto out;
    }

    if ((dp = opendir(".")) == NULL) {
        ret = -1;
        goto out;
    }


    void print_time(time_t time) { // 시간 출력
        char buf[PATH_MAX];
        strftime(buf, sizeof(buf), "%m/%d %H:%M ", localtime(&time));
        printf("%s", buf);
    }

    while (dep = readdir(dp)) {
        lstat(dep->d_name, &sb);
        rwx(sb.st_mode); // 파일 종류 및 권한 출력
        printf("%-4s ", get_type_str(dep->d_type)); // file type 출력
        printf("%-4o ", (unsigned int)sb.st_nlink); // 하드 링크 개수 출력
        printf("%-8d ", (int)dep->d_ino); // inode 번호 출력
        printf("%-8s ", (getpwuid(sb.st_uid))->pw_name); // 소유자 이름 출력
        printf("%-5d ", (int)sb.st_uid); // UID 출력
        printf("%-5d ", (int)sb.st_gid); // GID 출력
        printf("%-5d ", (int)sb.st_size); // 디렉토리 or 파일 크기 출력

        // 접근 시간, 수정 시간, 생성 시간
        print_time(sb.st_atime);
        print_time(sb.st_mtime);
        print_time(sb.st_ctime);

        printf("%-s ", dep->d_name); // 이름 출력
        if (S_ISLNK(sb.st_mode)) { // 심볼릭 링크 내용 출력
            char link_target[PATH_MAX];
            // readlink 함수 사용
            ssize_t len = readlink(dep->d_name, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf("-> %s", link_target);
            }
        }
        printf("\n");
    }

    closedir(dp);

out:
    return (ret);
}

int cmd_ln(int argc, char** argv)
{
    int  ret = 0;
    char rpath1[128];
    char rpath2[128];

    if (argc == 3) { // 하드 링크 생성

        get_realpath(argv[1], rpath1);
        get_realpath(argv[2], rpath2);

        if ((ret = link(rpath1, rpath2)) < 0) { // link 함수 사용
            perror(argv[0]);
        }

    }
    else if (argc == 4) { // 심볼릭 링크 생성

        get_realpath(argv[2], rpath1);
        get_realpath(argv[3], rpath2);

        if ((strcmp(argv[1], "-s")) == 0) {

            if ((ret = symlink(rpath1, rpath2)) < 0) { // symlink 함수 사용
                perror(argv[0]);
            }

        }
        else perror(argv[0]);

    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_rm(int argc, char** argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) { // 파일 삭제
        get_realpath(argv[1], rpath);

        if ((ret = remove(rpath)) < 0) { // remove 함수 사용
            perror(argv[0]);
        }
    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_chmod(int argc, char** argv)
{
    int  ret = 0;
    char rpath[128];
    mode_t mode;
    struct stat sb;

    if (argc == 3) {

        stat(argv[2], &sb);
        mode = (unsigned int)sb.st_mode;

        get_realpath(argv[2], rpath);

        for (int i = 0; argv[1][i]; i++) {
            if (argv[1][i] >= '0' && argv[1][i] <= '7') { // 8진수인지 검사
                
                // 3 or 4자리 수인지 검사
                // 4자리 수일 경우 맨 앞 숫자가 0 이어야 함
                int len = strlen(argv[1]);
                if (len == 3 || (len == 4 && argv[1][0] == 48)) {

                    int num = atoi(argv[1]); // 문자열 -> 정수로 변환
                    mode = num % 10; // 사용자(user) 권한 저장
                    for (int i = 1; i <= 3; i++) { // 그룹(group), 기타(other), 파일 유형(type) 권한 저장
                        num /= 10;
                        mode = (mode | ((num % 10) << (i * 3))); // 비트 OR 연산을 통해 각 권한 설정
                    }
                }
                else {
                    ret = -2;
                }
            }
            else {
                int target[3] = { 0, 0, 0 }; // u, g, o
                char operator; // +, -, =
                int access[3] = { 0, 0, 0 }; // r, w, x
                int macros[3][3] = { {S_IRUSR, S_IWUSR, S_IXUSR},   // 사용자(user) 권한 매크로
                                     {S_IRGRP, S_IWGRP, S_IXGRP},   // 그룹(group) 권한 매크로
                                     {S_IROTH, S_IWOTH, S_IXOTH} }; // 기타(other) 권한 매크로

                for (int i = 0; i < strlen(argv[1]); i++) {
                    char current_char = argv[1][i];

                    switch (current_char) {
                    case 'u':
                        target[0] = 1; // 사용자(user) 대상 설정
                        break;
                    case 'g':
                        target[1] = 1; // 그룹(group) 대상 설정
                        break;
                    case 'o':
                        target[2] = 1; // 기타(other) 대상 설정
                        break;
                    case '+':
                    case '-':
                    case '=':
                        operator = current_char; // 연산자(operator) 설정
                        break;
                    case 'r':
                        access[0] = 1; // 읽기(read) 권한 설정
                        break;
                    case 'w':
                        access[1] = 1; // 쓰기(write) 권한 설정
                        break;
                    case 'x':
                        access[2] = 1; // 실행(execute) 권한 설정 
                        break;
                    }
                }

                // u, g, o 모두 없을 시 전체 권한 부여, (e.g. +rwx)
                if (!target[0] && !target[1] && !target[2])
                    for (int i = 0; i < 3; i++) target[i] = 1;

                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        if (operator == '=') { // '=' 연산자일 때 해당 권한 지정 또는 삭제
                            if (target[i] && access[j]) mode |= macros[i][j];
                            else mode &= ~(macros[i][j]);
                        }
                        else {
                            // 해당 대상 혹은 해당 접근권한이 없을 때 넘어감
                            if (!target[i] || !access[j]) continue;
                            else {
                                // '+' 연산자일 때 해당 권한 지정
                                if (operator == '+') mode |= macros[i][j];
                                // '-' 연산자일 때 해당 권한 삭제
                                else if (operator == '-') mode &= ~(macros[i][j]);
                            }
                        }
                    }
                }
            }
        }

        if ((ret = chmod(rpath, mode)) < 0) {
            perror(argv[0]);
        }
    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_cat(int argc, char** argv)
{
    int  ret = 0;
    char rpath[128];
    FILE* fp;
    int c;

    if (argc == 2) {
        get_realpath(argv[1], rpath);

        if ((fp = fopen(rpath, "r")) == NULL) { // 파일 열기
            perror(argv[0]);
        }

        while ((c = fgetc(fp)) != EOF) { // 문자가 끝날 때까지 문자 가져와 출력
            printf("%c", c);
        }

        fclose(fp);// 파일 닫기

    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_cp(int argc, char** argv)
{
    int  ret = 0;
    char rpath1[128];
    char rpath2[128];
    FILE* rfp, * wfp;
    int c;

    if (argc == 3) {

        get_realpath(argv[1], rpath1);
        get_realpath(argv[2], rpath2);

        if ((rfp = fopen(argv[1], "r")) == NULL) { // 파일 읽기
            perror(argv[0]);
        }

        if ((wfp = fopen(argv[2], "w")) == NULL) { // 파일 쓰기
            perror(argv[0]);
        }

        while ((c = fgetc(rfp)) != EOF) { // 다른 파일로 내용 복사
            fputc(c, wfp);
        }

        // 파일 닫기
        fclose(rfp);
        fclose(wfp);

    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}


int cmd_quit(int argc, char** argv)
{
    exit(1);
    return 0;
}

void usage_help(void)
{
    printf("help <command>\n");
}

void usage_mkdir(void)
{
    printf("mkdir <directory>\n");
}

void usage_rmdir(void)
{
    printf("rmdir <directory>\n");
}

void usage_cd(void)
{
    printf("cd <directory>\n");
}

void usage_mv(void)
{
    printf("mv <old_name> <new_name>\n");
}

void usage_ln(void)
{
    printf("ln <original file> <new file>\n");
    printf("ln -s <original file> <new file>\n");
}

void usage_rm(void)
{
    printf("rm <filename>\n");
}

void usage_chmod(void)
{
    printf("chmod <permission: '000~777' or '0000~0777' or 'ugo+-=rwx'> <filename>\n");
}

void usage_cat(void)
{
    printf("cat <filename>\n");
}

void usage_cp(void)
{
    printf("cp <original file> <new file>\n");
}

