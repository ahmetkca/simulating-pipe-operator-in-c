#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>

typedef struct command
{
    int argc;
    char **argv;
} command_t;

command_t *createCommand(int argc, ...);

void run_command(const command_t *cmd, const int fd[2]);
int *get_fd(int num_commands, int cmd_pos, const int **fds);

int main(int argc, char **argv) {

    int num_commands = 3;
    command_t *commands;
    commands = (command_t *) calloc(num_commands, sizeof(command_t));
    commands[0] = *createCommand(3, "ls", "-l", "-a");
    commands[1] = *createCommand(3, "grep", "-n", "Make");
    commands[2] = *createCommand(3, "tail", "-n", "1");

//    run_command(&commands[0], NULL);
    int **fd = (int**) calloc(num_commands, sizeof(int*));
    for(int i=0; i < num_commands-1;i++){
        fd[i] = (int*)calloc(2, sizeof(int*));
        pipe(fd[i]);
    }
    for (int i = 0; i < num_commands; i++) {
        run_command(&commands[i], get_fd(num_commands, i, (const int **) fd));
    }
//    run_command(&commands[0], fd1);
//    printf("Between run_command\n");
//    run_command(&commands[1], fd2);
//    printf("After 2 run_command\n");
    return 0;
}

int *get_fd(int num_commands, int cmd_pos, const int **fds) {
    int *fd = calloc(2, sizeof(int*));
    if (cmd_pos == 0) {
        // first command
        fd[0] = -1;
        fd[1] = fds[0][1];
    } else if (cmd_pos == num_commands - 1){
        // last command
        fd[0] = fds[num_commands-1-1][0];
        fd[1] = -1;
    } else {
        fd[0] = fds[cmd_pos-1][0];
        fd[1] = fds[cmd_pos][1];
    }
    return fd;
}

command_t *createCommand(int argc, ...){
    va_list argv;
    va_start(argv, argc);
    command_t *cmd = (command_t*)calloc(1, sizeof(command_t));
    cmd->argc=argc;
    cmd->argv = (char **) calloc(cmd->argc + 1, sizeof(char *));
    for (int i=0; i < argc; i++){
        cmd->argv[i] = va_arg(argv, char *);
    }
    cmd->argv[argc] = NULL;
    return cmd;
}

void run_command(const command_t *cmd, const int fd[2])
{
    pid_t pid = fork();
    if (pid == 0) {
        if (fd[0] != -1) {
//            printf("%s, Close stdin and replace with %d\n", cmd->argv[0], fd[0]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
        }
        if (fd[1] != -1) {
//            printf("%s, Close stdout and replace with %d\n",cmd->argv[0], fd[1]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        }
//        printf("%s before execvp\n", cmd->argv[0]);
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (fd[0] != -1) {
            close(fd[0]);
        }
        if (fd[1] != -1) {
            close(fd[1]);
        }
//        printf("%s waiting...\n", cmd->argv[0]);
        waitpid(pid, NULL , 0);
//        printf("%s exit.\n", cmd->argv[0]);
    } else {
        fprintf(stderr, "Error: cannot create child process\n");
        exit(EXIT_FAILURE);
    }
//    printf("Command: %s, parent: %d, child: %d\n", cmd->argv[0], getpid(), pid);
}

//void run_command(const command_t *cmd, const int fd[2], const int **fds)
//{
//    pid_t pid = fork();
//    if (pid == 0) {
//        if (fd[0] != -1) {
////            printf("%s, Close stdin and replace with %d\n", cmd->argv[0], fd[0]);
//            dup2(fd[0], STDIN_FILENO);
//            close(fd[0]);
//        }
//        if (fd[1] != -1) {
////            printf("%s, Close stdout and replace with %d\n",cmd->argv[0], fd[1]);
//            dup2(fd[1], STDOUT_FILENO);
//            close(fd[1]);
//        }
////        printf("%s before execvp\n", cmd->argv[0]);
//        execvp(cmd->argv[0], cmd->argv);
//        perror("execvp");
//        exit(EXIT_FAILURE);
//    } else if (pid > 0) {
//        if (fd[0] != -1) {
//            close(fd[0]);
//        }
//        if (fd[1] != -1) {
//            close(fd[1]);
//        }
////        printf("%s waiting...\n", cmd->argv[0]);
//        waitpid(pid, NULL , 0);
////        printf("%s exit.\n", cmd->argv[0]);
//    } else {
//        fprintf(stderr, "Error: cannot create child process\n");
//        exit(EXIT_FAILURE);
//    }
////    printf("Command: %s, parent: %d, child: %d\n", cmd->argv[0], getpid(), pid);
//}