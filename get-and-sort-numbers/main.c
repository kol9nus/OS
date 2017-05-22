#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <memory.h>

typedef long long ll;

void printError(char* message);
void showHelp();
int* tryToOpenFiles(char** filenames, int count);
int tryToOpenFile(char* name, int flags, mode_t mode);
ll* initNumbersArray() ;
size_t getAllNumbersFromFiles(ll* numbers, int* files, int filesCount);
void appendNumberFromBuffer(char* buffer, int digitN, ll* numbers, size_t* numbersCount) ;
int compare(const void* a, const void* b);


int main(int argc, char* argv[]) {

    if (argc < 3 || argv[1] == "-h" || argv[1] == "--help") {
        showHelp();
    };

    int filesCount = argc - 2;
    char* filenames[filesCount];
    memcpy(filenames, argv + 1, filesCount * sizeof(*argv));
    int* inputFileDescs = tryToOpenFiles(filenames, filesCount);

    int outputFileDesc = tryToOpenFile(argv[argc - 1], O_WRONLY | O_EXCL | O_CREAT, 0666);

    ll* numbers = initNumbersArray();
    size_t count_numbers = getAllNumbersFromFiles(numbers, inputFileDescs, filesCount);

    qsort(numbers, count_numbers, sizeof(ll), compare);

    for (size_t i = 0; i < count_numbers; i++) {
        dprintf(outputFileDesc, "%lld\n", numbers[i]);
    };
    close(outputFileDesc);

    return 0;
}

void printError(char* message) {
    fprintf(stderr, "%s\nerrno: %d\n", message, errno);
    exit(1);
}

void showHelp() {
    printf("Использование: \n"
                   "task3 file_to_sort [[file_to_sort] ...] file_to_output\n"
                   "\tгде file_to_sort - имя выходного файла, из которого будут взяты числа\n"
                   "\t    file_to_output - имя файла результата\n");
    exit(1);
}

int* tryToOpenFiles(char** filenames, int count) {
    int* result = malloc(count * sizeof(int));
    for (int i = 0; i < count; i++) {
        result[i] = tryToOpenFile(filenames[i], O_RDONLY, 0);
    };

    return result;
}

int tryToOpenFile(char* name, int flags, mode_t mode) {
    int fileDesc;
    if (mode == 0)
        fileDesc = open(name, flags);
    else
        fileDesc = open(name, flags, mode);
    if (fileDesc < 0) {
        printError("Файл существует или нет прав на запись.");
    };

    return fileDesc;
}

ll* initNumbersArray() {
    ll* result = (ll*) malloc(0 * sizeof(ll));
    if (result == NULL) {
        printError("Не удалось выделить достаточно памяти.");
    };
    return result;
}

size_t getAllNumbersFromFiles(ll* numbers, int *files, int filesCount) {
    char buffer[19];
    char byte;
    int digitN;
    size_t numbersCount = 0;

    for (int i = 0; i < filesCount; i++) {
        digitN = 0;
        while (read(files[i], &byte, 1) > 0) {
            if ((byte >= '0' && byte <= '9') || (byte == '-' && digitN == 0)) {
                // 19 девяток будет слишком много для long long
                if (digitN >= 18) {
                    printError("Слишком большое число.");
                };
                buffer[digitN] = byte;
                digitN++;
            } else {
                if (digitN > 0 && !(digitN == 1 && buffer[0] == '-')) {
                    appendNumberFromBuffer(buffer, digitN, numbers, &numbersCount);
                };
                digitN = 0;
            };
        };
        close(files[i]);
    };

    return numbersCount;
}

void appendNumberFromBuffer(char* buffer, int digitN, ll* numbers, size_t* numbersCount) {
    (*numbersCount)++;
    buffer[digitN] = 0;
    ll number = atoll(buffer);
    numbers = realloc(numbers, (*numbersCount) * sizeof(ll));
    if (numbers == NULL) {
        printError("Не удалось выделить достаточно памяти.");
    };
    numbers[(*numbersCount) - 1] = number;
}

int compare(const void* a, const void* b) {
    ll first = *(ll*) a;
    ll second = *(ll*) b;

    if (first < second) return -1;
    if (first > second) return 1;
    return 0;
}