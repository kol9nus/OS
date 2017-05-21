#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void printError(char* message);
void showHelp();
int tryToOpenFile(char* name);
char writeNotEmptyBytesToFile(int fileDesc);
void moveCursor(int fileDesc, __off_t offset);
void writeToFile(int fileDesc, char* byte);

int main(int argc, char* argv[]) {

    if (argc != 2 || argv[1] == "-h" || argv[1] == "--help") {
        showHelp();
    };

    int fileDesc = tryToOpenFile(argv[1]);

    char lastByte = writeNotEmptyBytesToFile(fileDesc);

    if (lastByte == 0) {
        moveCursor(fileDesc, -1);
        writeToFile(fileDesc, &lastByte);
    };

    close(fileDesc);
    return 0;
};

void printError(char* message) {
    fprintf(stderr, "%s\nerrno: %d\n", message, errno);
    exit(1);
}

void showHelp() {
    printf("Использование: \n"
                   "sparser filename\n"
                   "\tгде filename - имя выходного файла, который будет создан");
    exit(1);
}

int tryToOpenFile(char* name) {
    int fileDesc = open(name, O_WRONLY|O_EXCL|O_CREAT, 0666);
    if (fileDesc < 0) {
        printError("Файл существует или нет прав на запись.");
    };

    return fileDesc;
}

char writeNotEmptyBytesToFile(int fileDesc) {
    char byte;
    while (read(STDIN_FILENO, &byte, 1) > 0) {
        if (byte == 0) {
            moveCursor(fileDesc, 1);
        } else {
            writeToFile(fileDesc, &byte);
        };
    };

    return byte;
}

void moveCursor(int fileDesc, __off_t offset) {
    if (lseek(fileDesc, offset, SEEK_CUR) <= 0) {
        printError("Произошла ошибка при записи файла.");
    };
}

void writeToFile(int fileDesc, char* byte) {
    if (write(fileDesc, &byte, 1) <= 0) {
        printError("Произошла ошибка при записи файла.");
    };
}