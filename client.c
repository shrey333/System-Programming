#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MIRROR_PORT 8081
#define BUFFER_SIZE 2048
#define ACK_STR "Recieved"
#define FILE_NAME "temp.tar.gz"
#define MAX_ARGS 6
#define SPACE_STRING " "
#define FGETS "fgets"
#define TARFGETZ "tarfgetz"
#define FILESRCH "filesrch"
#define TARGZF "targzf"
#define GETDIRF "getdirf"
#define UNZIP_FLAG "-u"
#define EXIT_FLAG "quit"
#define FILE_FLAG "file"
#define MSG_FLAG "msg"
#define DONE_FLAG "done"
#define UNZIP_FILE_FLAG "unzip"
#define ZIP_FILE_FLAG "zip"
#define FILE_NOT_FOUND_FLAG "No file found"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define MOVED_FLAG "HTTP/1.1 301 Moved Permanently"

bool isValidDate(char *dateString)
{
    int year, month, day;
    if (sscanf(dateString, "%d-%d-%d", &year, &month, &day) == 3)
    {
        if (year >= 1000 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= 31)
        {
            return true;
        }
    }
    return false;
}

int splitArguments(char *commandString, char *seperator, char *commands[])
{
    char *copyOfCommandString = strdup(commandString);
    if (copyOfCommandString == NULL)
    {
        perror("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    int numOfCommands = 0;
    char *endOfToken;
    char *token = strtok_r(copyOfCommandString, seperator, &endOfToken);

    while (token != NULL)
    {
        commands[numOfCommands++] = strdup(token);
        token = strtok_r(NULL, seperator, &endOfToken);
    }

    commands[numOfCommands] = NULL;

    free(copyOfCommandString);

    return numOfCommands;
}

bool validateCommandString(char *commandString)
{
    char *commands[MAX_ARGS];
    int numOfCommands = splitArguments(commandString, SPACE_STRING, commands);

    if (strcmp(commands[0], FGETS) == 0)
    {
        if (numOfCommands > 5 || numOfCommands < 2)
        {
            printf("Usage: fgets file1 file2 file3 file4\n");
            return false;
        }
        return true;
    }
    else if (strcmp(commands[0], TARFGETZ) == 0)
    {
        if (numOfCommands > 4 || (numOfCommands < 4 && strcmp(commands[numOfCommands - 1], UNZIP_FLAG) == 0))
        {
            printf("Usage: tarfgetz size1 size2 <-u>\n");
            return false;
        }
        else if (atoi(commands[0]) && atoi(commands[1]))
        {
            printf("Usage: tarfgetz size1 size2 <-u> [size1 and size2 must be number]\n");
            return false;
        }
        return true;
    }
    else if (strcmp(commands[0], FILESRCH) == 0)
    {
        if (numOfCommands != 2)
        {
            printf("Usage: filesrch filename\n");
            return false;
        }
        return true;
    }
    else if (strcmp(commands[0], TARGZF) == 0)
    {
        if (numOfCommands > 6 || numOfCommands < 2 || (numOfCommands == 2 && strcmp(commands[numOfCommands - 1], UNZIP_FLAG) == 0))
        {
            printf("Usage: targzf <extension list [up to 4 extensions]> <-u>\n");
            return false;
        }

        return true;
    }
    else if (strcmp(commands[0], GETDIRF) == 0)
    {
        if (numOfCommands > 4 || (numOfCommands < 4 && strcmp(commands[numOfCommands - 1], UNZIP_FLAG) == 0))
        {
            printf("Usage: getdirf date1 date2 <-u>\n");
            return false;
        }
        else if (isValidDate(commands[0]) && isValidDate(commands[1]))
        {
            printf("Usage: getdirf date1 date2 <-u> [date1 and date2 must be valid date]\n");
            return false;
        }
        return true;
    }
    printf("Invalid Command\n");
    return false;
}

void receiveFile(int clientSocket)
{
    FILE *receivedFile = fopen(FILE_NAME, "wb");
    if (!receivedFile)
    {
        perror("Error opening file\n");
    }
    char buffer[BUFFER_SIZE];

    ssize_t bytesReceived;
    char tempStr[BUFFER_SIZE];
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, bytesReceived, receivedFile);

        if (strcmp(buffer, "done") == 0)
        {
            send(clientSocket, ACK_STR, strlen(ACK_STR), 0);
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        send(clientSocket, ACK_STR, strlen(ACK_STR), 0);
    }

    fclose(receivedFile);

    memset(buffer, 0, sizeof(buffer));
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    send(clientSocket, ACK_STR, strlen(ACK_STR), 0);

    if (strcmp(buffer, UNZIP_FILE_FLAG) == 0)
    {
        // tar -xvzf temp.tar.gz &>/dev/null
        char unZipCommand[BUFFER_SIZE];
        snprintf(unZipCommand, sizeof(unZipCommand), "tar -xvzf %s &>/dev/null", FILE_NAME);
        int status = system(unZipCommand);
        if (status != 0)
        {
            perror("Error while unzipping tar.gz\n");
        }
    }
}

void receiveReponse(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    send(clientSocket, ACK_STR, strlen(ACK_STR), 0);
    if (strcmp(buffer, MSG_FLAG) == 0)
    {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("%s\n", buffer);
        send(clientSocket, ACK_STR, strlen(ACK_STR), 0);
    }
    else if (strcmp(buffer, FILE_FLAG) == 0)
    {
        memset(buffer, 0, sizeof(buffer));
        receiveFile(clientSocket);
    }
}

int main()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        perror("Error creating socket\n");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Error connecting to server\n");
        close(clientSocket);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (strstr(buffer, MOVED_FLAG) != NULL)
    {
        printf("Redirecting to mirror\n");
        close(clientSocket);
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1)
        {
            perror("Error creating socket\n");
            return 1;
        }
        struct sockaddr_in mirrorAddress;
        mirrorAddress.sin_family = AF_INET;
        mirrorAddress.sin_port = htons(MIRROR_PORT);
        mirrorAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(clientSocket, (struct sockaddr *)&mirrorAddress, sizeof(mirrorAddress)) == -1)
        {
            perror("Error connecting to mirror");
            close(clientSocket);
            return 1;
        }
    }

    while (1)
    {
        char commandString[BUFFER_SIZE];
        printf(COLOR_GREEN "C$ " COLOR_RESET);
        fgets(commandString, sizeof(commandString), stdin);
        commandString[strlen(commandString) - 1] = '\0'; // Remove newline

        if (strcmp(commandString, EXIT_FLAG) == 0)
        {
            printf("Exiting...\n");
            send(clientSocket, commandString, strlen(commandString), 0);
            break;
        }

        if (!validateCommandString(commandString))
        {
            continue;
        }

        // Send commandString to server
        send(clientSocket, commandString, strlen(commandString), 0);

        receiveReponse(clientSocket);
    }

    close(clientSocket);
    return 0;
}
