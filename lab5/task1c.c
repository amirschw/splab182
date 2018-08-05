#include "LineParser.h"
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#define STDIN   0
#define STDOUT  1

void execute(cmdLine *pCmdLine);

int main(int argc, char **argv)
{
    char buf[PATH_MAX];
    char line[LINE_MAX];
    cmdLine* pCmdLine;
    
    for (;;) {
        if (getcwd(buf, PATH_MAX)) {
            printf("%s/", buf); /* Display prompt - current working directory */
            if (fgets(line, LINE_MAX, stdin)) { /* Read a line from stdin */
                if ((pCmdLine = parseCmdLines(line))) { /* Parse the input */
                    execute(pCmdLine); /* Invoke pCmdLine */
                    freeCmdLines(pCmdLine); /* Releases allocated memory */
                }
            } else if (errno) {
                perror("fgets failed");
                exit(EXIT_FAILURE);
            } else { /* EOF */
                printf("\n");
                exit(EXIT_SUCCESS);
            }
        } else {
            perror("getcwd failed");
            exit(EXIT_FAILURE);
        }
    }
}

/* Invokes the program specified in the cmdLine */
/* Exists shell if "quit" is entered */
void execute(cmdLine *pCmdLine)
{
    pid_t pid;
    const char *in_red = pCmdLine->inputRedirect;
    const char *out_red = pCmdLine->outputRedirect;
    char *file = pCmdLine->arguments[0];

    if (!strcmp(file, "quit")) {
        freeCmdLines(pCmdLine); /* Releases allocated memory */
        exit(EXIT_SUCCESS);
    }
    if (-1 == (pid = fork())) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (0 == pid) { /* Code executed by child */
        if (out_red) {
            close(STDOUT);
            if (!fopen(out_red, "w")) {
                perror("open failed");
                _exit(EXIT_FAILURE);
            }
        }
        if (in_red) {
            close(STDIN);
            if (!fopen(in_red, "r")) {
                perror("open failed");
                _exit(EXIT_FAILURE);
            }
        }
        if (execvp(file, pCmdLine->arguments)) {
            perror("execvp failed");
            _exit(EXIT_FAILURE);
        }
    } else { /* Code executed by parent */
        if (pCmdLine->blocking) {
            /* Suspend execution of parent until child is terminated or stopped */
            if (-1 == waitpid(pid, NULL, WUNTRACED)) {
                perror("waitpid failed");
                exit(EXIT_FAILURE);
            }
        }
    }

}
