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

    int num_commands = 4;
    command_t *commands;
    commands = (command_t *) calloc(num_commands, sizeof(command_t));
    commands[0] = *createCommand(3, "ls", "-l", "-a");
    commands[1] = *createCommand(3, "grep", "-n", "Make");
    commands[2] = *createCommand(3, "tail", "-n", "6");
    commands[3] = *createCommand(2, "sort", "--random-sort");

    /* number of pipes is equals to number of processes minus one */
    /*
     * number of pipes = number of processes - 1
     *
     * Create (number of processes - 1) pipes and store them in the 2-d array
     *
     * fd[i][0] reading side of the pipe.
     * fd[i][1] writing side of the pipe.
     */
    int **fd = (int**) calloc(num_commands, sizeof(int*));
    for(int i=0; i < num_commands-1;i++){
        fd[i] = (int*)calloc(2, sizeof(int*));
        pipe(fd[i]);
    }
    for (int i = 0; i < num_commands; i++) {
        run_command(&commands[i], get_fd(num_commands, i, (const int **) fd));
    }
    return 0;
}

/*  Given the command position and number of commands
 *  find the appropriate pipe
 *
 *      For example:
 *          number of commands = 6, command position = 3
 *          fd for this command is as follows;
 *              fd[command_position-1][0]   (read from previous command's pipe)
 *              fd[command_position][1]     (write to current pipe)
 * */
int *get_fd(int num_commands, int cmd_pos, const int **fds) {
    int *fd = calloc(2, sizeof(int*));
    if (cmd_pos == 0) {
        // special treatment for first command
        fd[0] = -1;
        fd[1] = fds[0][1];
    } else if (cmd_pos == num_commands - 1){
        // special treatment for last command
        fd[0] = fds[num_commands-1-1][0];
        fd[1] = -1;
    } else {
        // if the command is neither first nor last then treat it as command between first and second command
        fd[0] = fds[cmd_pos-1][0];
        fd[1] = fds[cmd_pos][1];
    }
    return fd;
}

/* Creates pointer to command struct with number of arguments and list of arguments */
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

/* run given pointer to command struct and handle any given file descriptor for pipe operation */
/*
 *  fd[0] is for reading from pipe.
 *  fd[1] is for writing to pipe.
 * */
void run_command(const command_t *cmd, const int fd[2])
{
    pid_t pid = fork();
    if (pid == 0) {
        /*
         * if reading side of the pipe is valid file descriptor
         * close stdin and duplicate the reading fd to STDIN_FILENO
         * */
        if (fd[0] != -1) {
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
        }

        /*
         * if writing side of the pipe is valid file descriptor
         * close stdout and duplicate the writing fd to STDOUT_FILENO
         * */
        if (fd[1] != -1) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
        }

        /* execute given command with its arguments */
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        /* Parent process */

        /*
         * if fd[0] (reading side of the pipe) is valid file descriptor. CLOSE in the parent process
         * */
        if (fd[0] != -1) {
            close(fd[0]);
        }

        /*
         * if fd[0] (writing side of the pipe) is valid file descriptor. CLOSE in the parent process
         * */
        if (fd[1] != -1) {
            close(fd[1]);
        }
        /* wait for child process to execute the command and exit */
        waitpid(pid, NULL , 0);
    } else {
        fprintf(stderr, "Error: cannot create child process\n");
        exit(EXIT_FAILURE);
    }
}