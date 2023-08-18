#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_ARGS 7
#define MAX_CMD 8
#define MAX_BUFFER_SIZE 2048
#define MAX_CHAR_LIMIT 512
#define AND_AND_OPERATOR "&&"
#define OR_OR_OPERATOR "||"
#define SEMICOLON_OPERATOR ";"
#define GREATOR_OPERATOR ">"
#define GREATOR_GREATOR_OPERATOR ">>"
#define LESS_OPERATOR "<"
#define PIPE_OPERATOR "|"
#define AMPERSAND_OPERATOR "&"
#define EMPTY_STRING " "
#define CREATE_FILE_PERMISSION 0644

struct Commands
{
    char ***commands;
    int numberOfCommands;
};

bool executeCommand(char *command[], int inputFileDescriptor, int outputFileDescriptor, bool isBackground)
{
    pid_t processId = fork();
    if (processId == 0)
    {
        // Handle input/output redirection
        if (inputFileDescriptor != STDIN_FILENO)
        {
            dup2(inputFileDescriptor, STDIN_FILENO);
            close(inputFileDescriptor);
        }

        if (outputFileDescriptor != STDOUT_FILENO)
        {
            dup2(outputFileDescriptor, STDOUT_FILENO);
        }
        // command = ["ls", "-u", "-b" NULL]
        execvp(command[0], command);

        perror("execvp error");
        exit(EXIT_FAILURE);
    }
    else if (processId > 0)
    {
        // Parent process
        if (!isBackground)
        {
            int status;

            waitpid(processId, &status, 0);

            if (outputFileDescriptor != STDOUT_FILENO)
            {
                close(outputFileDescriptor);
            }

            if (WIFEXITED(status))
            {
                int exitStatus = WEXITSTATUS(status);

                if (exitStatus == 0)
                {
                    return true; // Child process exited successfully
                }
                else
                {
                    return false; // Child process encountered an error
                }
            }
            else
            {
                // Child process did not exit normally
                perror("Child process did not exit normally.\n");
                return false;
            }
        }
    }
    else
    {
        perror("Error while create fork");
        exit(EXIT_FAILURE);
    }
    return false;
}

int splitString(char *commandString, char *seperator, char *substringArray[])
{
    char *copyOfCommandString = strdup(commandString);
    if (copyOfCommandString == NULL)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int numOfSubstrings = 0;
    char *endOfToken;
    char *token = strtok_r(copyOfCommandString, seperator, &endOfToken);

    while (token != NULL)
    {
        substringArray[numOfSubstrings++] = strdup(token);
        token = strtok_r(NULL, seperator, &endOfToken);
    }

    substringArray[numOfSubstrings] = NULL;

    free(copyOfCommandString);

    return numOfSubstrings;
}

struct Commands getCommands(char *commandString, char *specialChar)
{
    char *substringArray[MAX_ARGS];

    struct Commands commands;

    int numOfSubstrings = splitString(commandString, specialChar, substringArray);
    commands.commands = (char ***)malloc(sizeof(char **) * numOfSubstrings);
    commands.numberOfCommands = numOfSubstrings;

    for (int currentSubString = 0; currentSubString < numOfSubstrings; currentSubString++)
    {

        char *command[MAX_ARGS];

        int numArgStrings = splitString(substringArray[currentSubString], EMPTY_STRING, command);
        commands.commands[currentSubString] = (char **)malloc((numArgStrings + 1) * sizeof(char *));

        for (int currentArgString = 0; currentArgString < numArgStrings; currentArgString++)
        {
            int length = strlen(command[currentArgString]) + 1;

            commands.commands[currentSubString][currentArgString] = (char *)malloc(length * sizeof(char));
            strcpy(commands.commands[currentSubString][currentArgString], command[currentArgString]);
        }

        commands.commands[currentSubString][numArgStrings] = NULL;
    }

    return commands;
}

char *returnCurrentChar(char *commandString)
{
    char *andAndPos = strstr(commandString, AND_AND_OPERATOR), *orOrPos = strstr(commandString, OR_OR_OPERATOR);

    if (andAndPos != NULL && orOrPos != NULL)
    {
        if (strlen(andAndPos) > strlen(orOrPos))
        {
            return AND_AND_OPERATOR;
        }
        else
        {
            return OR_OR_OPERATOR;
        }
    }
    else if (andAndPos != NULL && orOrPos == NULL)
    {
        return AND_AND_OPERATOR;
    }
    else if (andAndPos == NULL && orOrPos != NULL)
    {
        return OR_OR_OPERATOR;
    }

    return NULL;
}

void conditionalExecution(char *commandString)
{
    char *restCommandString;
    char *tempPath = strdup(commandString);
    char *specialChar = returnCurrentChar(tempPath);

    char *token = strtok_r(tempPath, specialChar, &restCommandString);

    while (token != NULL)
    {

        char *currentCommand[MAX_ARGS];
        splitString(token, EMPTY_STRING, currentCommand);

        bool status = executeCommand(currentCommand, STDIN_FILENO, STDOUT_FILENO, false);

        if (strcmp(specialChar, OR_OR_OPERATOR) == 0 && status)
        {
            break;
        }
        else if (strcmp(specialChar, AND_AND_OPERATOR) == 0 && !status)
        {
            break;
        }

        char *tempSpecialChar = returnCurrentChar(restCommandString);
        if (tempSpecialChar != NULL && tempSpecialChar != specialChar)
        {
            restCommandString = restCommandString + 1;
        }
        specialChar = (tempSpecialChar != NULL) ? strdup(tempSpecialChar) : specialChar;

        token = strtok_r(NULL, specialChar, &restCommandString);
    }
}

void executePipeline(char ***commands, int numberOfCommands)
{
    int pipeFileDescriptor[2];
    int inputFileDescriptor = STDIN_FILENO, outputFileDescriptor = STDOUT_FILENO;

    for (int currentCommand = 0; currentCommand < numberOfCommands; currentCommand++)
    {
        if (pipe(pipeFileDescriptor) < 0)
        {
            perror("Pipe initialization failed");
            exit(EXIT_FAILURE);
        }

        outputFileDescriptor = (currentCommand != numberOfCommands - 1) ? pipeFileDescriptor[1] : STDOUT_FILENO;

        executeCommand(commands[currentCommand], inputFileDescriptor, outputFileDescriptor, false);

        inputFileDescriptor = pipeFileDescriptor[0];
    }
}

void sequentialExecution(char ***commands, int numberOfCommands)
{
    for (int currentCommand = 0; currentCommand < numberOfCommands; currentCommand++)
    {
        executeCommand(commands[currentCommand], STDIN_FILENO, STDOUT_FILENO, false);
    }
}

void backgroundExecution(char *commandString)
{
    char *tempPath = strdup(commandString);

    char *token = strtok(tempPath, AMPERSAND_OPERATOR);
    char *command[MAX_ARGS];
    splitString(token, EMPTY_STRING, command);
    executeCommand(command, STDIN_FILENO, STDOUT_FILENO, true);
}

void redirectionExecution(char *commandString)
{
    struct Commands commands;
    if (strstr(commandString, LESS_OPERATOR))
    {
        commands = getCommands(commandString, LESS_OPERATOR);

        int readFileDescriptor = open(commands.commands[1][0], O_RDONLY);

        executeCommand(commands.commands[0], readFileDescriptor, STDOUT_FILENO, false);
    }
    else if (strstr(commandString, GREATOR_GREATOR_OPERATOR))
    {
        commands = getCommands(commandString, GREATOR_GREATOR_OPERATOR);
        int writeFileDescriptor = open(commands.commands[1][0], O_CREAT | O_WRONLY | O_APPEND, CREATE_FILE_PERMISSION);

        executeCommand(commands.commands[0], STDIN_FILENO, writeFileDescriptor, false);
    }
    else if (strstr(commandString, GREATOR_OPERATOR))
    {
        commands = getCommands(commandString, GREATOR_OPERATOR);
        int writeFileDescriptor = open(commands.commands[1][0], O_CREAT | O_WRONLY, CREATE_FILE_PERMISSION);

        executeCommand(commands.commands[0], STDIN_FILENO, writeFileDescriptor, false);
    }
}

void checkAndExecuteRules(char *commandString)
{
    if (strstr(commandString, AND_AND_OPERATOR) || strstr(commandString, OR_OR_OPERATOR))
    {
        conditionalExecution(commandString);
    }
    else if (strstr(commandString, GREATOR_GREATOR_OPERATOR) || strstr(commandString, GREATOR_OPERATOR) || strstr(commandString, LESS_OPERATOR))
    {
        redirectionExecution(commandString);
    }
    else if (strstr(commandString, PIPE_OPERATOR))
    {
        struct Commands commands;
        commands = getCommands(commandString, PIPE_OPERATOR);
        executePipeline(commands.commands, commands.numberOfCommands);
    }
    else if (strstr(commandString, AMPERSAND_OPERATOR))
    {
        backgroundExecution(commandString);
    }
    else if (strstr(commandString, SEMICOLON_OPERATOR))
    {
        struct Commands commands;
        commands = getCommands(commandString, SEMICOLON_OPERATOR);
        sequentialExecution(commands.commands, commands.numberOfCommands);
    }
    else
    {
        char *command[MAX_ARGS];
        splitString(commandString, EMPTY_STRING, command);

        executeCommand(command, STDIN_FILENO, STDOUT_FILENO, false);
    }
}

int main()
{
    char commandString[MAX_BUFFER_SIZE];

    while (true)
    {
        printf("\033[0;32m mshell$ \033[0m");
        fgets(commandString, MAX_BUFFER_SIZE, stdin);
        commandString[strcspn(commandString, "\n")] = 0;

        checkAndExecuteRules(commandString);
    }

    return 0;
}