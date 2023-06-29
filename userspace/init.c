#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    setenv("TERM", "linux", 1);
    setenv("USER", "root", 1);
    setenv("HOME", "/root", 1);

    if (access("/usr/bin/bash", X_OK) == -1)
    {
        perror("init: /usr/bin/bash");
        return EXIT_FAILURE;
    }

    while (1)
    {
        int pid = fork();
        if (pid == -1)
        {
            perror("init: fork failed");
            return EXIT_FAILURE;
        }
        else if (pid == 0)
        {
            char *argv[] = { "/usr/bin/bash", "-l", NULL };

            chdir(getenv("HOME"));
            execvp("/usr/bin/bash", argv);
            return EXIT_FAILURE;
        }
        waitpid(pid, NULL, 0);
    }

    return EXIT_SUCCESS;
}
