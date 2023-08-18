#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

bool isDefunctProcess(pid_t processId)
{
    char statFilePath[100];
    sprintf(statFilePath, "/proc/%d/stat", processId);

    FILE *stateFile = fopen(statFilePath, "r");
    if (stateFile == NULL)
    {
        printf("Error opening stat stateFile for process %d.\n", processId);
        return -1;
    }

    char status;
    if (fscanf(stateFile, "%*d %*s %c", &status) != 1)
    {
        fclose(stateFile);
        printf("Failed to current status for process %d.\n", processId);
        return false;
    }

    fclose(stateFile);
    return (status == 'Z');
}

pid_t getParentProcessId(pid_t processId)
{
    char statFilePath[100];
    sprintf(statFilePath, "/proc/%d/stat", processId);
    FILE *stateFile = fopen(statFilePath, "r");
    if (stateFile == NULL)
        return -1;

    pid_t ppid;
    if (fscanf(stateFile, "%*d %*s %*c %d", &ppid) != 1)
    {
        fclose(stateFile);
        return -1;
    }

    fclose(stateFile);
    return ppid;
}

bool findAndCheckParent(pid_t rootProcessId, pid_t currentProcessId)
{
    while (true)
    {
        pid_t ppid = getParentProcessId(currentProcessId);

        if (ppid == rootProcessId)
        {
            return true;
        }

        if (ppid == 1 || ppid == -1)
        {
            return false;
        }

        currentProcessId = ppid;
    }
    return false;
}

double getProcessElapsedTime(pid_t processId)
{   
    // read the process stst stateFile
    char statFilePath[100];
    sprintf(statFilePath, "/proc/%d/stat", processId);

    FILE *stateFile = fopen(statFilePath, "r");
    if (stateFile == NULL)
    {
        printf("Error opening stat file for process %d.\n", processId);
        return -1;
    }

    unsigned long long startTime;
    // initialised process starttime in startTime var
    if (fscanf(stateFile, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %llu ", &startTime) != 1)
    {
        fclose(stateFile);
        printf("Failed to read start time from stat file for process %d.\n", processId);
        return -1;
    }

    fclose(stateFile);

    stateFile = fopen("/proc/uptime", "r");
    if (stateFile == NULL)
    {
        printf("Error opening stat file for process %d.\n", processId);
        return -1;
    }

    unsigned long long currentTime;
    if (fscanf(stateFile, "%lld", &currentTime) != 1)
    {
        fclose(stateFile);
        printf("Failed to read current time\n");
        return -1;
    }

    fclose(stateFile);
    // linux time parameter
    clock_t ticksPerSecond = sysconf(_SC_CLK_TCK);
    // convert it in a minutes and find elaspedTime
    double elapsedTime = (double)(currentTime - startTime/ticksPerSecond) / 60;
    printf("-----> %llu %llu %f %ld\n", startTime, currentTime, elapsedTime, ticksPerSecond);

    return elapsedTime;
}

void terminateProcess(pid_t processId)
{
    printf("Terminating parent process: PID=%d\n", processId);
    kill(processId, SIGTERM);
    waitpid(processId, NULL, 0);
}

void defunctChildren(pid_t processId, int numDefuncts)
{
    int noOfDefuntChildren = 0;
    DIR *processDirectory = opendir("/proc");
    if (processDirectory == NULL)
    {
        printf("Error opening /proc directory.\n");
        return;
    }
    
    // traverse through /proc directory and look for defunc childern count
    struct dirent *processEntry;
    while ((processEntry = readdir(processDirectory)) != NULL)
    {
        if (processEntry->d_type != DT_DIR)
            continue;

        char *processName = processEntry->d_name;
        if (processName[0] < '0' || processName[0] > '9')
            continue;

        pid_t pid = atoi(processName);
        pid_t ppid = getParentProcessId(pid);
        if (isDefunctProcess(pid) && ppid == processId)
        {
            noOfDefuntChildren++;
        }
    }

    printf("Iddd --> %d %d %d\n", processId, noOfDefuntChildren, numDefuncts);
    // if defunct childern count is == or > given defunc num then terminate that parent process
    if (noOfDefuntChildren >= numDefuncts)
    {
        terminateProcess(processId);
    }
}

void killDefunctProcesses(pid_t rootProcessId, pid_t excludeProcessId, int elapsedTime, int numDefuncts, int option)
{
    DIR *processDirectory = opendir("/proc");
    if (processDirectory == NULL)
    {
        printf("Error opening /proc directory.\n");
        return;
    }

    // Traverse through each process in /proc directory
    struct dirent *processEntry;
    while ((processEntry = readdir(processDirectory)) != NULL)
    {
        if (processEntry->d_type != DT_DIR)
            continue;

        char *processName = processEntry->d_name;
        if (processName[0] < '0' || processName[0] > '9')
            continue;

        pid_t pid = atoi(processName);
        pid_t ppid = getParentProcessId(pid);
        bool isDefunt = isDefunctProcess(pid);
        bool isChild = findAndCheckParent(rootProcessId, pid);
        // terminate the defunct processes in process tree if the option is not given
        if (option == 0 && isChild && isDefunt && ppid != excludeProcessId)
        {
            terminateProcess(ppid);
        }
        // check for the process elaped time
        else if (option == 1 && isChild && isDefunt && ppid != excludeProcessId && getProcessElapsedTime(pid) >= elapsedTime)
        {
            terminateProcess(ppid);
        }
        else if (option == 2 && isChild && pid != excludeProcessId)
        {
            defunctChildren(pid, numDefuncts);
        }
    }
}

int main(int numberOfArgs, char *arguments[])
{
    if (numberOfArgs < 2)
    {
        printf("Error processing arguments.\n");
        printf("Try: deftreeminus [root_process] [OPTION1] [OPTION2] [-processid]\n");
        return 1;
    }

    pid_t rootProcessId = atoi(arguments[1]);
    int elapsedTime = 0;
    int numDefuncts = 0;
    int option = 0;

    if (numberOfArgs > 3)
    {
        // string comparision for options and initialization of elasedtime, num of defuncts
        if (strcmp(arguments[2], "-t") == 0)
        {
            option = 1;
            elapsedTime = atoi(arguments[3]);
        }
        else if (strcmp(arguments[2], "-b") == 0)
        {
            option = 2;
            numDefuncts = atoi(arguments[3]);
        }
        else
        {
            printf("Error processing arguments.\n");
            return 1;
        }
    }

    pid_t excludeProcessId = -1;
    if (numberOfArgs == 3)
    {
        excludeProcessId = -atoi(arguments[2]);
    }
    if (numberOfArgs == 5)
    {
        excludeProcessId = -atoi(arguments[4]);
    }

    killDefunctProcesses(rootProcessId, excludeProcessId, elapsedTime, numDefuncts, option);

    return 0;
}
