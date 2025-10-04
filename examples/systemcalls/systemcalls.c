#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int ret = system(cmd);
    return ret > -1;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    pid_t pid = fork();

    if (pid < 0)
    {
        va_end(args);
        return false;
    } 
    else if (pid == 0) // Child process
    {
        char* filepath = command[0];
        command[0] = basename(filepath);
        execv(filepath, &command[0]);
        va_end(args);
        exit(-1);
    }
    else // Parent process
    {
        int status;
        int ret = waitpid(pid, &status, 0);

        if (ret < 0)
        {
            va_end(args);
            return false;
        }
        va_end(args);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    int fd = open(outputfile, O_TRUNC | O_CREAT | O_WRONLY, 0644);

    if (fd < 0)
    {
        va_end(args);
        return false;
    }

    int pid = fork();
    switch (pid) 
    {
        case -1: 
            close(fd);
            va_end(args);
            return false;
        case 0:
            if (dup2(fd, 1) < 0) 
            { 
                va_end(args);
                return false;
            }
            close(fd);
            char* filepath = command[0];
            command[0] = basename(command[0]);
            execv(filepath, &command[0]);
            va_end(args);
            exit(-1);
        default:
            int status;
            int ret = wait(&status);
            close(fd);
            va_end(args);
            return ret >= 0;
    }
}
