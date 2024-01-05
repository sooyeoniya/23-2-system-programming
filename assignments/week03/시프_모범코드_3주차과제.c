#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CMDLINE_SIZE    (128)
#define MAX_CMD_SIZE        (32)
#define MAX_ARG             (4)

typedef int  (*cmd_func_t)(int argc, char **argv);
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
DECLARE_CMDFUNC(quit);

/* Command List */
static cmd_t cmd_list[] = {
    {"help",    cmd_help,    usage_help,  "show usage, ex) help <command>"},
    {"mkdir",   cmd_mkdir,   usage_mkdir, "create directory"},
    {"rmdir",   cmd_rmdir,   usage_rmdir, "remove directory"},
    {"cd",      cmd_cd,      usage_cd,    "change current directory"},
    {"mv",      cmd_mv,      usage_mv,    "rename directory & file"},
    {"ls",      cmd_ls,      NULL,        "show directory contents"},
    {"quit",    cmd_quit,    NULL,        "terminate shell"},
};

const int command_num = sizeof(cmd_list) / sizeof(cmd_t);
static char *chroot_path = "/tmp/test";
static char *current_dir;

static int search_command(char *cmd)
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

static void get_realpath(char *usr_path, char *result)
{
    char *stack[32];
    int   index = 0;
    char  fullpath[128];
    char *tok;
    int   i;
#define PATH_TOKEN   "/"

    if (usr_path[0] == '/') {
        strncpy(fullpath, usr_path, sizeof(fullpath)-1);
    } else {
        snprintf(fullpath, sizeof(fullpath)-1, "%s/%s", current_dir + strlen(chroot_path), usr_path);
    }

    /* parsing */
    tok = strtok(fullpath, PATH_TOKEN);
    if (tok == NULL) {
        goto out;
    }

    do {
        if (strcmp(tok, ".") == 0 || strcmp(tok, "") == 0) {
            ; // skip
        } else if (strcmp(tok, "..") == 0) {
            if (index > 0) index--;
        } else {
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

int main(int argc, char **argv)
{
    char *command, *tok_str;
    char *cmd_argv[MAX_ARG];
    int  cmd_argc, i, ret;

    command = (char*) malloc(MAX_CMDLINE_SIZE);
    if (command == NULL) {
        perror("command malloc");
        exit(1);
    }

    current_dir = (char*) malloc(MAX_CMDLINE_SIZE);
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

        if (fgets(command, MAX_CMDLINE_SIZE-1, stdin) == NULL) break;

        /* get arguments */
        tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

        cmd_argv[0] = tok_str;

        for (cmd_argc = 1; cmd_argc < MAX_ARG; cmd_argc++) {
            if (tok_str = strtok(NULL, " \n")) {
                cmd_argv[cmd_argc] = tok_str;
            } else {
                break;
            }
        }

        /* search command in list and call command function */
        i = search_command(cmd_argv[0]);
        if (i < 0) {
            printf("%s: command not found\n", cmd_argv[0]);
        } else {
            if (cmd_list[i].cmd_func) {
                ret = cmd_list[i].cmd_func(cmd_argc, cmd_argv);
                if (ret == 0) {
                    printf("return success\n");
                } else if (ret == -2 && cmd_list[i].usage_func) {
                    printf("usage : ");
                    cmd_list[i].usage_func();
                } else {
                    printf("return fail(%d)\n", ret);
                }
            } else {
                printf("no command function\n");
            }
        }
    } while (1);

    free(command);
    free(current_dir);

    return 0;
}


int cmd_help(int argc, char **argv)
{
    int i;

    if (argc == 1) {
        for (i = 0; i < command_num; i++) {
            printf("%32s: %s\n", cmd_list[i].cmd_str, cmd_list[i].comment);
        }
    } else if (argv[1] != NULL) {
        i = search_command(argv[1]);
        if (i < 0) {
            printf("%s command not found\n", argv[1]);
        } else {
            if (cmd_list[i].usage_func) {
                printf("usage : ");
                cmd_list[i].usage_func();
                return (0);
            } else {
                printf("no usage\n");
                return (-2);
            }
        }
    }

    return (0);
}

int cmd_mkdir(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);
        
        if ((ret = mkdir(rpath, 0755)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_rmdir(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);
        
        if ((ret = rmdir(rpath)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2; // syntax error
    }

    return (ret);
}

int cmd_cd(int argc, char **argv)
{
    int  ret = 0;
    char rpath[128];

    if (argc == 2) {
        get_realpath(argv[1], rpath);

        if ((ret = chdir(rpath)) < 0) {
            perror(argv[0]);
        }
    } else {
        ret = -2;
    }

    return (ret);
}

int cmd_mv(int argc, char **argv)
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
    } else {
        ret = -2;
    }

    return (ret);
}

static const char *get_type_str(char type)
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

int cmd_ls(int argc, char **argv)
{
    int ret = 0;
    DIR *dp;
    struct dirent *dep;

    if (argc != 1) {
        ret = -2;
        goto out;
    }

    if ((dp = opendir(".")) == NULL) {
        ret = -1;
        goto out;
    }

    while (dep = readdir(dp)) {
        printf("%10ld %4s %s\n", dep->d_ino, get_type_str(dep->d_type), dep->d_name);
    }

    closedir(dp);

out:
    return (ret);
}

int cmd_quit(int argc, char **argv)
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


