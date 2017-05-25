#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

int isnumber(char *str);
void showHelp();
void printError(char* message);
int tryToExecute(int result, char *msg);
FILE* tryToOpen(char *file);
char* getLockFilePath(char *file);
void lockFile(char *path);

int main(int argc, char *argv[]) {

    if (argc != 3 && !(argc == 4 && isnumber(argv[3]) >= 0)) {
        showHelp();
    };
    char *lockFilePath = getLockFilePath(argv[1]);
    lockFile(lockFilePath);
    FILE *fp = tryToOpen(argv[1]);
    if (argc == 4) {
        sleep(atoi(argv[3]));
    }
    tryToExecute(fprintf(fp, "%s\n", argv[2]), "Не удалось записать сообщение в файл.");
    fclose(fp);

    tryToExecute(remove(lockFilePath), "Не удалось удалить файл блокировки.");

    return 0;

};

int isnumber(char *str) {
    int i = 0;
    while (str[i] != '\0') {
        if (!isdigit(str[i])) {
            return -1;
        };
        i++;
    }
    return atoi(str);
}

void showHelp() {
    printf("Программа просто дописывает в конец файла (этакий логер)\n"
                   "Использование: \n"
                   "locker <path> <message>[ <time>]\n"
                   "\tгде path - имя файла, в который будет записан message\n"
                   "\t    message - сообщение, которое будет записано в файл.\n"
                   "\t    time - сколько секунд длится запись, дефаут - 0\n");
    exit(1);
}

void printError(char* message) {
    fprintf(stderr, "%s\nerr: %s\n", message, strerror(errno));
    exit(1);
}

int tryToExecute(int result, char *msg) {
    if (result < 0) {
        printError(msg);
    };

    return result;
}

FILE* tryToOpen(char *file) {
    FILE *fp = fopen(file, "a");
    if (fp == NULL) {
        printError("Не удалось открыть файл.");
    };

    return fp;
}

char* getLockFilePath(char *file) {
    static char lockFile[100];
    strcpy(lockFile, file);
    strcat(lockFile, ".lck");

    return lockFile;
}

void lockFile(char *path) {
    // ждём пока освободится и лочим
    for (;;) {
        int fd = open(path, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
        if (fd >= 0) {
            pid_t pid = getpid();
            tryToExecute(dprintf(fd, "W%d\n", pid), "Не удалось записать в файл блокировки.");
            close(fd);
            break;
        };
    };
}