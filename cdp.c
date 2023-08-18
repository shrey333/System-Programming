#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    int numProcesses = 5; // Number of defunct processes to create
    int i;

    for (i = 0; i < numProcesses; i++)
    {
        pid_t childPid = fork();

        if (childPid == 0)
        {
            // Child process
            pid_t childPid = fork();

            if (childPid == 0)
            {
                // Child process
                pid_t childPid = fork();
                if (childPid == 0)
                {
                    // Child process
                    pid_t childPid = fork();
                    if (childPid == 0)
                    {
                        exit(0);
                    }
                    else if (childPid > 0)
                    {
                        // Parent process
                        printf("Created child process with PID: %d\n", childPid);
                    }
                    else
                    {
                        // Fork failed
                        printf("Failed to fork a child process.\n");
                    }
                }
                else if (childPid > 0)
                {
                    // Parent process
                    printf("Created child process with PID: %d\n", childPid);
                }
                else
                {
                    // Fork failed
                    printf("Failed to fork a child process.\n");
                }
            }
            else if (childPid > 0)
            {
                // Parent process
                printf("Created child process with PID: %d\n", childPid);
            }
            else
            {
                // Fork failed
                printf("Failed to fork a child process.\n");
            }
        }
        else if (childPid > 0)
        {
            // Parent process
            printf("Created child process with PID: %d\n", childPid);
        }
        else
        {
            // Fork failed
            printf("Failed to fork a child process.\n");
        }
    }

    // Parent process does not wait for child processes to exit

    // Sleep for a while to allow defunct processes to accumulate
    sleep(10000);

    return 0;
}
