#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftw.h>
#include <sys/stat.h>

char *srcDir, *destDir;
char extensionList[4096] = "";
bool isMove = false;

void removeSubstring(const char *str, char *substr)
{
    char *pos = strstr(str, substr);

    if (pos != NULL)
    {
        size_t len = strlen(substr);
        memmove(pos, pos + len, strlen(pos + len) + 1);
    }
}

bool checkDirectoryExists(char *pathname)
{
    struct stat sb;

    if (stat(pathname, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void removeLastToken(char *path)
{
    // Find the last occurrence of "/"
    char *lastSlash = strrchr(path, '/');

    if (lastSlash != NULL)
    {
        // Set the position after the last "/"
        char *endOfString = lastSlash + 1;

        // Replace the last token with a null-terminating character
        *endOfString = '\0';
    }
}

void createDirectories(const char *path)
{
    char *tempPath = strdup(path);
    char *token = strtok(tempPath, "/");
    char dirPath[512] = "";

    if (path[0] == '/')
    {
        strcat(dirPath, "/");
    }

    strcat(dirPath, token);

    while (token != NULL)
    {
        if (!checkDirectoryExists(dirPath) && mkdir(dirPath, 0777) != 0)
        {
            fprintf(stderr, "Failed to create directory: %s\n", dirPath);
            exit(1);
        }

        token = strtok(NULL, "/");

        if (token != NULL)
        {
            strcat(dirPath, "/");
            strcat(dirPath, token);
        }
    }

    free(tempPath);
}

int copyOrMoveFile(const char *sourcePath, char *destPath, mode_t permissions)
{
    int src_fd, dst_fd, n, err;
    unsigned char buffer[4096];
    src_fd = open(sourcePath, O_RDONLY);
    dst_fd = open(destPath, O_CREAT | O_WRONLY, permissions);

    while (1)
    {
        err = read(src_fd, buffer, 4096);
        if (err == -1)
        {
            printf("Error reading file.\n");
            return -1;
        }
        n = err;

        if (n == 0)
            break;

        err = write(dst_fd, buffer, n);
        if (err == -1)
        {
            printf("Error writing to file.\n");
            return -1;
        }
    }

    close(src_fd);
    close(dst_fd);
    return 0;
}

static int moveOrCopyCallback(const char *pathname, const struct stat *statptr, int fileflags, struct FTW *pfwt)
{
    char *dest = (char *)malloc(sizeof(char) * (strlen(pathname) + 1));
    strcpy(dest, pathname);
    removeSubstring(dest, srcDir);

    char *tempDestDir = (char *)malloc(sizeof(char) * (strlen(destDir) + strlen(dest) + 2));
    strcpy(tempDestDir, destDir);
    strcat(strcat(tempDestDir, "/"), dest);

    mode_t permissions = statptr->st_mode;

    if (fileflags == FTW_D)
    {
        struct stat sb;
        if (!checkDirectoryExists(tempDestDir) && mkdir(tempDestDir, permissions) == -1)
        {
            perror("Error creating destination directory");
            return -1;
        }
    }
    else if (fileflags == FTW_F)
    {
        char *extension = strrchr(pathname, '.');
        if (extension && strstr(extensionList, extension) != NULL)
        {
            return 0;
        }
        return copyOrMoveFile(pathname, tempDestDir, permissions);
    }

    free(dest);
    free(tempDestDir);
    return 0;
}

int deleteSource(const char *filename, const struct stat *statptr, int fileflags, struct FTW *pfwt)
{
    int rv = remove(filename);

    if (rv)
        perror(filename);

    return rv;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s [source_dir] [destination_dir] [options] <extension list>", argv[0]);
        exit(1);
    }

    srcDir = argv[1];
    destDir = argv[2];

    if(!strcmp(srcDir, destDir)){
        fprintf(stderr, "[source_dir] and [destination_dir] cannnot be same");
        exit(1);
    }

    if (!checkDirectoryExists(srcDir))
    {
        fprintf(stderr, "Source directory does not exist");
        exit(1);
    }

    char *tempSrcDir = strdup(srcDir);
    removeLastToken(srcDir);

    if (!checkDirectoryExists(destDir))
    {
        createDirectories(destDir);
    }

    if (strcmp(argv[3], "-mv") == 0)
    {
        isMove = true;
    }
    else if (strcmp(argv[3], "-cp") != 0)
    {
        fprintf(stderr, "Provide valid options");
        exit(1);
    }

    if (argc > 4)
    {
        for (int i = 4; i < argc; i++)
        {
            strcat(strcat(strcat(extensionList, "."), argv[i]), " ");
        }
    }

    if (nftw(tempSrcDir, moveOrCopyCallback, 64, FTW_MOUNT | FTW_PHYS) < 0)
    {
        perror("ERROR: ntfw");
        exit(1);
    }

    if (isMove)
    {
        return nftw(tempSrcDir, deleteSource, 64, FTW_DEPTH | FTW_PHYS);
    }

    return 0;
}
