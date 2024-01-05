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
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>

#define MAX_CMDLINE_SIZE    (128)
#define MAX_CMD_SIZE        (32)
#define MAX_ARG             (4)

extern char** environ;

typedef int  (*cmd_func_t)(int argc, char** argv);
typedef void (*usage_func_t)(void);

typedef struct cmd_t {
    char            cmd_str[MAX_CMD_SIZE];
    cmd_func_t      cmd_func;
    usage_func_t      usage_func;
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
DECLARE_CMDFUNC(ps);
DECLARE_CMDFUNC(run_env);
DECLARE_CMDFUNC(env);
DECLARE_CMDFUNC(set);
DECLARE_CMDFUNC(unset);
DECLARE_CMDFUNC(echo);
DECLARE_CMDFUNC(quit);

/* Command List */
static cmd_t cmd_list[] = {
    {"help",    cmd_help,    usage_help,     "show usage, ex) help <command>"},
    {"mkdir",   cmd_mkdir,   usage_mkdir,    "create directory"},
    {"rmdir",   cmd_rmdir,   usage_rmdir,    "remove directory"},
    {"cd",      cmd_cd,      usage_cd,       "change current directory"},
    {"mv",      cmd_mv,      usage_mv,       "rename directory & file"},
    {"ls",      cmd_ls,      NULL,           "show directory contents"},
    {"ln",      cmd_ln,      usage_ln,       "create hard link & symbolic link"},
    {"rm",      cmd_rm,      usage_rm,       "remove directory & file"},
    {"chmod",   cmd_chmod,   usage_chmod,    "change access permissions"},
    {"cat",     cmd_cat,     usage_cat,      "show file contents"},
    {"cp",      cmd_cp,      usage_cp,       "copy file"},
    {"ps",      cmd_ps,      NULL,           "show process list"},
    {"run_env", cmd_run_env, usage_run_env,  "run current env"},
    {"env",     cmd_env,     NULL,           "show all envs"},
    {"set",     cmd_set,     usage_set,      "set env"},
    {"unset",   cmd_unset,   usage_unset,    "unset env"},
    {"echo",    cmd_echo,    usage_echo,     "show current env"},
    {"quit",    cmd_quit,    NULL,           "terminate shell"},
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

// ���� ��� �Լ�(e.g. drwxrwxrwx)
static void rwx(int mode)
{
    // ���� ����
    if (S_ISREG(mode)) printf("-");
    else if (S_ISDIR(mode)) printf("d");
    else if (S_ISCHR(mode)) printf("c");
    else if (S_ISBLK(mode)) printf("b");
    else if (S_ISFIFO(mode)) printf("p");
    else if (S_ISLNK(mode)) printf("l");
    else if (S_ISSOCK(mode)) printf("s");

    // ��� ���� ����
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

        // ��ó�� ���� ���ڿ��� '$'�� ��� cmd_run_env �Լ� ����
        if (cmd_argv[0][0] == '$') {
            cmd_run_env(cmd_argc, cmd_argv);
        }
        // ���ڿ� �߰��� '='�� ���� ��� cmd_set �Լ� ����
        else if (strstr(cmd_argv[0], "=") != NULL) {
            cmd_set(cmd_argc, cmd_argv);
        }
        else if (i < 0) {
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

    void print_time(time_t time) { // �ð� ���
        char buf[PATH_MAX];
        strftime(buf, sizeof(buf), "%m/%d %H:%M ", localtime(&time));
        printf("%s", buf);
    }

    while (dep = readdir(dp)) {
        lstat(dep->d_name, &sb);
        rwx(sb.st_mode); // ���� ���� �� ���� ���
        printf("%-4s ", get_type_str(dep->d_type)); // file type ���
        printf("%-4o ", (unsigned int)sb.st_nlink); // �ϵ� ��ũ ���� ���
        printf("%-8d ", (int)dep->d_ino); // inode ��ȣ ���
        printf("%-8s ", (getpwuid(sb.st_uid))->pw_name); // ���� �̸� ���
        printf("%-8s ", (getgrgid(getgid())->gr_name)); // �׷� �̸� ���
        printf("%-5d ", (int)sb.st_uid); // UID ���
        printf("%-5d ", (int)sb.st_gid); // GID ���
        printf("%-5d ", (int)sb.st_size); // ���丮 or ���� ũ�� ���

        // ���� �ð�, ���� �ð�, ���� �ð�
        print_time(sb.st_atime);
        print_time(sb.st_mtime);
        print_time(sb.st_ctime);

        printf("%-s ", dep->d_name); // �̸� ���
        if (S_ISLNK(sb.st_mode)) { // �ɺ��� ��ũ ���� ���
            char link_target[PATH_MAX];
            // readlink �Լ� ���
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

    if (argc == 3) { // �ϵ� ��ũ ����

        get_realpath(argv[1], rpath1);
        get_realpath(argv[2], rpath2);

        if ((ret = link(rpath1, rpath2)) < 0) { // link �Լ� ���
            perror(argv[0]);
        }

    }
    else if (argc == 4) { // �ɺ��� ��ũ ����

        get_realpath(argv[2], rpath1);
        get_realpath(argv[3], rpath2);

        if ((strcmp(argv[1], "-s")) == 0) {

            if ((ret = symlink(rpath1, rpath2)) < 0) { // symlink �Լ� ���
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

    if (argc == 2) { // ���� ����
        get_realpath(argv[1], rpath);

        if ((ret = remove(rpath)) < 0) { // remove �Լ� ���
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
            if (argv[1][i] >= '0' && argv[1][i] <= '7') { // 8�������� �˻�

                // 3 or 4�ڸ� ������ �˻�
                // 4�ڸ� ���� ��� �� �� ���ڰ� 0 �̾�� ��
                int len = strlen(argv[1]);
                if (len == 3 || (len == 4 && argv[1][0] == 48)) {

                    int num = atoi(argv[1]); // ���ڿ� -> ������ ��ȯ
                    mode = num % 10; // �����(user) ���� ����
                    for (int i = 1; i <= 3; i++) { // �׷�(group), ��Ÿ(other), ���� ����(type) ���� ����
                        num /= 10;
                        mode = (mode | ((num % 10) << (i * 3))); // ��Ʈ OR ������ ���� �� ���� ����
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
                int macros[3][3] = { {S_IRUSR, S_IWUSR, S_IXUSR},   // �����(user) ���� ��ũ��
                                     {S_IRGRP, S_IWGRP, S_IXGRP},   // �׷�(group) ���� ��ũ��
                                     {S_IROTH, S_IWOTH, S_IXOTH} }; // ��Ÿ(other) ���� ��ũ��

                for (int i = 0; i < strlen(argv[1]); i++) {
                    char current_char = argv[1][i];

                    switch (current_char) {
                    case 'u':
                        target[0] = 1; // �����(user) ��� ����
                        break;
                    case 'g':
                        target[1] = 1; // �׷�(group) ��� ����
                        break;
                    case 'o':
                        target[2] = 1; // ��Ÿ(other) ��� ����
                        break;
                    case '+':
                    case '-':
                    case '=':
                        operator = current_char; // ������(operator) ����
                        break;
                    case 'r':
                        access[0] = 1; // �б�(read) ���� ����
                        break;
                    case 'w':
                        access[1] = 1; // ����(write) ���� ����
                        break;
                    case 'x':
                        access[2] = 1; // ����(execute) ���� ���� 
                        break;
                    }
                }

                // u, g, o ��� ���� �� ��ü ���� �ο�, (e.g. +rwx)
                if (!target[0] && !target[1] && !target[2])
                    for (int i = 0; i < 3; i++) target[i] = 1;

                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        if (operator == '=') { // '=' �������� �� �ش� ���� ���� �Ǵ� ����
                            if (target[i] && access[j]) mode |= macros[i][j];
                            else mode &= ~(macros[i][j]);
                        }
                        else {
                            // �ش� ��� Ȥ�� �ش� ���ٱ����� ���� �� �Ѿ
                            if (!target[i] || !access[j]) continue;
                            else {
                                // '+' �������� �� �ش� ���� ����
                                if (operator == '+') mode |= macros[i][j];
                                // '-' �������� �� �ش� ���� ����
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

        if ((fp = fopen(rpath, "r")) == NULL) { // ���� ����
            perror(argv[0]);
        }

        while ((c = fgetc(fp)) != EOF) { // ���ڰ� ���� ������ ���� ������ ���
            printf("%c", c);
        }

        fclose(fp);// ���� �ݱ�

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

        if ((rfp = fopen(argv[1], "r")) == NULL) { // ���� �б�
            perror(argv[0]);
        }

        if ((wfp = fopen(argv[2], "w")) == NULL) { // ���� ����
            perror(argv[0]);
        }

        while ((c = fgetc(rfp)) != EOF) { // �ٸ� ���Ϸ� ���� ����
            fputc(c, wfp);
        }

        // ���� �ݱ�
        fclose(rfp);
        fclose(wfp);

    }
    else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_ps(int argc, char** argv)
{
    int ret = 0;
    int fd, fd_self = open("/proc/self/fd/0", O_RDONLY);
    char* tty, cmd[256], tty_self[256], path[512];
    struct dirent* dep;
    FILE* fp;
    DIR* dp = opendir("/proc"); // /proc ���͸� ���� ������ dp�� ����

    // ���� ���μ����� tty �̸��� ������ tty_self�� ����
    sprintf(tty_self, "%s", ttyname(fd_self));
    printf("%-5s %-6s %-5s\n", "PID", "TTY", "CMD");

    while ((dep = readdir(dp)) != NULL) {
        for (int i = 0; dep->d_name[i]; i++) {
            // ���ڰ� �ƴ� ��� ����
            if (!isdigit(dep->d_name[i])) break;
            else {
                // ���� ��ũ���� ��� ����
                sprintf(path, "/proc/%s/fd/0", dep->d_name);
                fd = open(path, O_RDONLY);
                tty = ttyname(fd);

                // ���� tty�� ������ tty�� ����ϴ� ���μ����� ���
                if (tty && strcmp(tty, tty_self) == 0) {
                    // �ش� ���μ����� stat ���� ��� ����
                    sprintf(path, "/proc/%s/stat", dep->d_name);
                    fp = fopen(path, "r");
                    fscanf(fp, "%d%s", &i, cmd); // ���μ����� PID�� CMD �б�
                    cmd[strlen(cmd) - 1] = '\0';
                    printf("%-5s %-6s %-5s\n", dep->d_name, tty + 5, cmd + 1); // PID, TTY, CMD ���
                    fclose(fp); // ���� �ݱ�
                }
                close(fd);
            }
        }
    }
    close(fd_self);
    return (ret);
}

int cmd_run_env(int argc, char** argv) { // $<ȯ�溯���̸�> ���·� ȯ�溯���� ���(������ ġȯ)�ϴ� ���
    int ret = 0;
    if (argv[0][0] == '$') {
        char* tmp = strtok(argv[0], "$");
        char* val = getenv(tmp);
        if (val == NULL) printf("\n");
        else printf("%s\n", val);
    }
    else ret = -2;
    return (ret);
}

int cmd_env(int argc, char** argv) { // ��� ȯ�� �����̸��� �� ���� ����ϴ� ���
    int ret = 0;
    char** env = environ;
    while (*env) {
        printf("%s\n", *env);
        env++;
    }
    return (ret);
}

int cmd_set(int argc, char** argv) { // <ȯ�溯���̸�>=<��> ���·� ȯ�溯���� �����ϴ� ���
    int ret = 0;
    char* tmp = NULL;
    char* val = NULL;
    if (argc == 1) {
        // ���� "="�� �������� ��ū �и�
        char* input = strdup(argv[0]);
        tmp = strtok(input, "=");
        val = strtok(NULL, "=");

        // ���ڿ� �յڷ� ū����ǥ Ȥ�� ��������ǥ�� ���� ��� ��� ����
        if ((val[0] == '"' && val[strlen(val) - 1] == '"')
            || (val[0] == '\'' && val[strlen(val) - 1] == '\'')) {
            strncpy(val, val + 1, strlen(val) - 2);
            val[strlen(val) - 2] = '\0';
        }

        // ȯ�溯�� ����
        if (val == NULL) ret = -2;
        else if ((ret = setenv(tmp, val, 1)) < 0) {
            perror(argv[0]);
        }
    }
    else if (argc == 2) {
        // ���� ������ ���� ���, ���� ó��
        char* input = strdup(argv[0]);
        strcat(input, " ");
        strcat(input, argv[1]);

        // ���� "="�� �������� ��ū �и�
        tmp = strtok(input, "=");
        val = strtok(NULL, "=");

        // ���ڿ� �յڷ� ū����ǥ Ȥ�� ��������ǥ�� ���� ��� ��� ����
        if ((val[0] == '"' && val[strlen(val) - 1] == '"')
            || (val[0] == '\'' && val[strlen(val) - 1] == '\'')) {
            strncpy(val, val + 1, strlen(val) - 2);
            val[strlen(val) - 2] = '\0';
        }
        else ret = -2;

        // ȯ�溯�� ����
        if (val == NULL) ret = -2;
        else if ((ret = setenv(tmp, val, 1)) < 0) {
            perror(argv[0]);
        }
    }
    else {
        ret = -2;
    }
    return (ret);
}

int cmd_unset(int argc, char** argv) { // ȯ�溯���� �����ϴ� ���
    int ret = 0;
    if (argc == 2) {
        if ((ret = unsetenv(argv[1])) < 0) {
            perror(argv[0]);
        }
    }
    else {
        ret = -2;
    }
    return (ret);
}

int cmd_echo(int argc, char** argv) { // ���ڷ� �Ѿ�� ���ڿ��� �״�� ���
    int ret = 0;
    char* val;
    if (argc == 2) {

        // �յڷ� ū����ǥ Ȥ�� ��������ǥ�� ���� ��� ��� ����
        char* val = argv[1];
        if ((val[0] == '"' && val[strlen(val) - 1] == '"')
            || (val[0] == '\'' && val[strlen(val) - 1] == '\'')) {
            strncpy(val, val + 1, strlen(val) - 2);
            val[strlen(val) - 2] = '\0';
        }

        // �߰��� ū����ǥ�� ¦������ ���� ��� ��� ����
        int cnt1 = 0;
        for (int i = 0; i < strlen(val); i++) {
            if (val[i] == '"') cnt1++;
        }
        if (cnt1 % 2 == 0) {
            int i = 0;
            for (int j = 0; j < strlen(val); j++) {
                if (val[j] != '"') { val[i++] = val[j]; }
            }
            val[i] = '\0';
        }

        // �߰��� ��������ǥ�� ¦������ ���� ��� ��� ����
        int cnt2 = 0;
        for (int i = 0; i < strlen(val); i++) {
            if (val[i] == '\'') cnt2++;
        }
        if (cnt1 % 2 == 0) {
            int i = 0;
            for (int j = 0; j < strlen(val); j++) {
                if (val[j] != '\'') { val[i++] = val[j]; }
            }
            val[i] = '\0';
        }

        // ȯ�溯���� ���, ȯ�溯���� �� �״�� ���
        if (val[0] == '$') {
            char* tmp = strtok(val, "$");
            char* val = getenv(tmp);
            if (val == NULL) printf("\n");
            else printf("%s\n", val);
        }

        // ���ڿ� ���
        else printf("%s\n", val);
    }
    else {
        ret = -2;
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
    printf("\nln <original file> <new file>\n");
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

void usage_run_env(void)
{
    printf("$<env name>\n");
}

void usage_set(void)
{
    printf("<env name>=<value>\n");
    printf("<env name>=\"<value>\"\n");
}

void usage_unset(void)
{
    printf("unset <env name>\n");
}

void usage_echo(void)
{
    printf("\necho $<env name>\n");
    printf("echo <string>\n");
    printf("echo \"<string>\"\n");
}