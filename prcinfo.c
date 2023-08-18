#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>


bool isDefunctProcess(pid_t processId)
{   // checks stst statFile of process
    char statFilePath[100];
    sprintf(statFilePath, "/proc/%d/stat", processId);

    FILE *statFile = fopen(statFilePath, "r");
    if (statFile == NULL)
    {
        printf("Error opening stat statFile for process %d.\n", processId);
        return -1;
    }

    char status;
    // It reads the contents based on the provided format string .
    if (fscanf(statFile, "%*d %*s %c", &status) != 1)
    {
        fclose(statFile);
        printf("Failed to current status for process %d.\n", processId);
        return false;
    }

    fclose(statFile);
    return (status == 'Z');
}

pid_t getParentProcessId(pid_t processId)
{
    char statFilePath[100];
    // Format a string and store it in char array statFilePath
    // format specifier %d is used to insert the value of processId into the path string
    sprintf(statFilePath, "/proc/%d/stat", processId);
    FILE *statFile = fopen(statFilePath, "r");
    if (statFile == NULL)
        return -1;

    pid_t ppid;
    //fscanf to read formatted input from a statFile
    // It reads the contents based on the provided format string . 
    // The asterisk () before each conversion specifier indicates that the corresponding data should be read and discarded (skipped).
    if (fscanf(statFile, "%*d %*s %*c %d", &ppid) != 1)
    {
        fclose(statFile);
        return -1;
    }

    fclose(statFile);
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

void printNoneDirectDescendants(pid_t rootProcessId, pid_t rootParentProcessId, pid_t currentProcessId, pid_t currentParentId, bool checkDefunct)
{   
    // checks for current process' existance in rootprocess tree
    // and root process and current process's parent shouldn't be same
    if (findAndCheckParent(rootProcessId, currentProcessId) && rootProcessId != currentParentId)
    {
        printf("%d %d\n", currentProcessId, currentParentId);
    }
}

void printDirectDescendants(pid_t rootProcessId, pid_t rootParentProcessId, pid_t currentProcessId, pid_t currentParentId, bool checkDefunct)
{
    if (rootProcessId == currentParentId && (!checkDefunct || checkDefunct && isDefunctProcess(currentProcessId)))
    {
        printf("%d %d\n", currentProcessId, currentParentId);
    }
}

void printSibling(pid_t rootProcessId, pid_t rootParentProcessId, pid_t currentProcessId, pid_t currentParentId, bool checkDefunct)
{   
    // if process_1 parent and current process parent are same
    if (rootParentProcessId == currentParentId && (!checkDefunct || checkDefunct && isDefunctProcess(currentProcessId)))
    {
        printf("%d %d\n", currentProcessId, currentParentId);
    }
}

void printGrandChildren(pid_t rootProcessId, pid_t rootParentProcessId, pid_t currentProcessId, pid_t currentParentId, bool checkDefunct)
{
    pid_t currentGrandParentId = getParentProcessId(currentParentId);
    if (currentGrandParentId == -1)
    {
        return;
    }
    // chek for process_1 and current grandparent
    if (rootProcessId == currentGrandParentId)
    {
        printf("%d %d\n", currentProcessId, currentParentId);
    }
}

void findProcess(pid_t rootProcessId, void (*fn)(pid_t, pid_t, pid_t, pid_t, bool), bool checkDefunct)
{
    DIR *dir = opendir("/proc");
    if (dir == NULL)
    {
        printf("Error opening /proc directory.\n");
        return;
    }

    // finds parent processid of process_1
    pid_t rootParentProcessId = getParentProcessId(rootProcessId);
    if (rootParentProcessId == -1)
    {
        return;
    }
    // traverse through each process in /proc directory
    struct dirent *processEntry;
    while ((processEntry = readdir(dir)) != NULL)
    {
        if (processEntry->d_type != DT_DIR)
            continue;
        
        // cheks for whether process id contains any char or not 
        char *processName = processEntry->d_name;
        if (processName[0] < '0' || processName[0] > '9')
            continue;

        pid_t pid = atoi(processName);
        pid_t ppid = getParentProcessId(pid);
        fn(rootProcessId, rootParentProcessId, pid, ppid, checkDefunct);
    }
}

void searchProcesses(pid_t rootProcessId, pid_t processIds[], int numProcessIds, const char *option)
{
    // Looping through each processIds arg given by user
    for (int processId = 0; processId < numProcessIds; processId++)
    {   
        // To check whether processId existes in a rootProcess tree or not
        if (findAndCheckParent(rootProcessId, processIds[processId]))
        {
            printf("%d %d\n", processIds[processId], getParentProcessId(processIds[processId]));
        }
    }

    // Compares two strings option, -nd
    if (strcmp(option, "-nd") == 0)
    {
        findProcess(processIds[0], printNoneDirectDescendants, false);
    }
    else if (strcmp(option, "-dd") == 0)
    {
        findProcess(processIds[0], printDirectDescendants, false);
    }
    else if (strcmp(option, "-zc") == 0)
    {
        findProcess(processIds[0], printDirectDescendants, true);
    }
    else if (strcmp(option, "-sb") == 0)
    {
        findProcess(processIds[0], printSibling, false);
    }
    else if (strcmp(option, "-sz") == 0)
    {
        findProcess(processIds[0], printSibling, true);
    }
    else if (strcmp(option, "-gc") == 0)
    {
        findProcess(processIds[0], printGrandChildren, true);
    }
    else if (strcmp(option, "-zz") == 0)
    {
        if (isDefunctProcess(processIds[0]))
        {
            printf("DEFUNCT\n");
        }
        else
        {
            printf("NOT DEFUNCT\n");
        }
    }
}

int main(int numberOfArgs, char *arguments[])
{
    if (numberOfArgs < 3 || numberOfArgs > 7)
    {
        printf("Error processing arguments.\n");
        return 1;
    }

    // it takes arg 1 rootprocessid and covert it into integer
    pid_t rootProcessId = atoi(arguments[1]);

    int numProcessIds = numberOfArgs - 2;

    const char *option = "";
    // checks last argument and if it's not an integer then take it as option
    if (atoi(arguments[numberOfArgs - 1]) == 0)
    {
        option = arguments[numberOfArgs - 1];
        numProcessIds--;
    }

    // initialization of process IDs
    pid_t processIds[numProcessIds];
    for (int processId = 0; processId < numProcessIds; processId++)
    {
        processIds[processId] = atoi(arguments[processId + 2]);
    }

    searchProcesses(rootProcessId, processIds, numProcessIds, option);

    return 0;
}
