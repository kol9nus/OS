#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <memory.h>

int FIELD_ROWS = 5;
int FAKE_FIELD_ROWS;
int FIELD_COLS = 5;
int FAKE_FIELD_COLS;
char GOL_INIT_FILE[] = "random";
int NEIGHBOURS[8][2] = {
        {-1, -1},
        {-1, 0},
        {-1, 1},
        {0, 1},
        {1, 1},
        {1, 0},
        {1, -1},
        {0, -1},
};

typedef struct {
    int isFilled;
    int filledNeighboursCount;
} Cell;


void printField(Cell field[][FAKE_FIELD_COLS]);

int tryToExecuteCommand(int result, char *msg);
FILE* tryToOpenFile(char *filename, char *mode);

void initFieldFromFile(Cell field[][FAKE_FIELD_COLS], char *filename);
void printError(char* message);
void setCell(Cell field[][FAKE_FIELD_COLS], int rowI, int colI, int value);
int runServer();
void processClient(int sock, Cell field[][FAKE_FIELD_COLS]);

void writeToSocket(int sock, char *msg);
void writeCharToFile(FILE *fp, char c);

void makeGameIteration(Cell field[][FAKE_FIELD_COLS]);
void copyField(Cell source[][FAKE_FIELD_COLS], Cell dest[][FAKE_FIELD_COLS]);

int main(int argc, char *argv[]) {
    FAKE_FIELD_COLS = FIELD_COLS + 2;
    FAKE_FIELD_ROWS = FIELD_ROWS + 2;

    Cell field[FAKE_FIELD_ROWS][FAKE_FIELD_COLS];
    memset(field, 0, FAKE_FIELD_ROWS * FAKE_FIELD_COLS * sizeof(Cell));
    initFieldFromFile(field, GOL_INIT_FILE);

    void writeToFile(int sig) {
        alarm(1);
        FILE *fp = tryToOpenFile("/tmp/Bravilov_game-of-life.mm", "w");

        for (int i = 1; i <= FIELD_ROWS; i++) {
            for (int j = 1; j <= FIELD_COLS; j++) {
                writeCharToFile(fp, (char) (field[i][j].isFilled + 48));
            };
            writeCharToFile(fp, '\n');
        };
        fclose(fp);

        makeGameIteration(field);
        return;
    };

    int sock;
    switch(fork()) {
        case -1:
            printError("Не удалось отфоркаться");
            break;
        case 0:
            sock = runServer();

            processClient(sock, field);

            close(sock);
            break;
        default:
            signal(SIGALRM, writeToFile);
            alarm(1);
            wait(NULL);
            break;
    }

    return 0;
};

/* Хэлперы, дабы не писать каждый раз по 3 строки
 * Если выполняется ок, возвращает result иначе выдаёт сообщение msg
*/
int tryToExecuteCommand(int result, char *msg) {
    if (result < 0) {
        printError(msg);
    }

    return result;
}

FILE* tryToOpenFile(char *filename, char *mode) {
    FILE *fp = fopen(filename, mode);
    if (fp == NULL) {
        printError("Не удалось открыть файл");
    };

    return fp;
}

void printError(char* message) {
    fprintf(stderr, "%s\nerr: %s\n", message, strerror(errno));
    exit(1);
}

void printField(Cell field[][FAKE_FIELD_COLS]) {
    for (int i = 1; i <= FIELD_ROWS; i++) {
        for (int j = 1; j <= FIELD_COLS; j++) {
            printf("%d", field[i][j].isFilled);
        };
        printf("\n");
    };
    printf("---------------------------------------\n");
}

void initFieldFromFile(Cell field[][FAKE_FIELD_COLS], char *filename) {
    FILE *fp = tryToOpenFile(filename, "r");
    int symb = fgetc(fp);
    for (int i = 1; i <= FIELD_ROWS; i++) {
        for (int j = 1; j <= FIELD_COLS; j++) {
            switch(symb) {
                case '1':
                    setCell(field, i, j, 1);
                    break;
                case '0':
                    setCell(field, i, j, 0);
                    break;
                default:
                    printError("Непредвиденный символ. Ожидалось увидеть 0 или 1.");
                    break;
            };
            symb = fgetc(fp);
        };
        while (symb != '\n' && symb != EOF) {
            symb = fgetc(fp);
        }
        if (symb == '\n') {
            symb = fgetc(fp);
        }
    };
    fclose(fp);
}

void setCell(Cell field[][FAKE_FIELD_COLS], int rowI, int colI, int value) {
    // TODO: Выпилить перед релизом
    if (value != 0 && value != 1) {
        printError("Разработчик дурак. Значение value может быть 0 или 1.");
    }

    int diff = value - field[rowI][colI].isFilled;
    if (diff != 0) {
        field[rowI][colI].isFilled = value;
        for (int i = 0; i < 8; i++) {
            field[rowI + NEIGHBOURS[i][0]][colI + NEIGHBOURS[i][1]].filledNeighboursCount += diff;
        }
    }
}

int runServer() {
    struct sockaddr_in serverAddr;

    int sock = tryToExecuteCommand(socket(AF_INET, SOCK_STREAM, 0), "Не удалось создать сокет");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(9999);

    tryToExecuteCommand(
            bind(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)),
            "Не удалось забиндиться"
    );

    listen(sock, 5);

    return sock;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void processClient(int sock, Cell field[][FAKE_FIELD_COLS]) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    printf("вечный цикл");
    for (;;) {
        int newsock = tryToExecuteCommand(accept(sock, (struct sockaddr *) &clientAddr, &clientAddrLen),
                                          "Не удалось принять соединение");
        printf("принял коннекшн");
        initFieldFromFile(field, "/tmp/Bravilov_game-of-life.mm");

        for (int i = 1; i <= FIELD_ROWS; i++) {
            for (int j = 1; j <= FIELD_COLS; j++) {
                writeToSocket(newsock, (void *) &field[i][j].isFilled);
            };
            writeToSocket(newsock, "\n");
        };
        close(newsock);
    };
}
#pragma clang diagnostic pop

void writeToSocket(int sock, char *msg) {
    tryToExecuteCommand((int) write(sock, msg, 1), "Не удалось записать в сокет");
}

void writeCharToFile(FILE *fp, char c) {
    tryToExecuteCommand(fputc(c, fp), "Не удалось записать в сокет");
}

void makeGameIteration(Cell field[][FAKE_FIELD_COLS]) {
    time_t startTime = time(NULL);
    Cell tempField[FAKE_FIELD_ROWS][FAKE_FIELD_COLS];
    memset(tempField, 0, FAKE_FIELD_ROWS * FAKE_FIELD_COLS * sizeof(Cell));
    for (int i = 1; i <= FIELD_ROWS; i++) {
        for (int j = 1; j <= FIELD_COLS; j++) {
            if (field[i][j].filledNeighboursCount == 3 || field[i][j].filledNeighboursCount == 2 && field[i][j].isFilled == 1) {
                setCell(tempField, i, j, 1);
            } else {
                setCell(tempField, i, j, 0);
            }
        };
    };
    copyField(tempField, field);
    // printField(field);
    time_t endTime = time(NULL);
    double iterationTime = difftime(endTime, startTime);
    if (iterationTime >= 1.0) {
        fprintf(stderr, "Не успели посчитать в одну секунду.");
        return;
    };
}

void copyField(Cell source[][FAKE_FIELD_COLS], Cell dest[][FAKE_FIELD_COLS]) {
    for (int i = 1; i <= FIELD_ROWS; i++) {
        for (int j = 0; j <= FIELD_COLS; j++) {
            dest[i][j].isFilled = source[i][j].isFilled;
            dest[i][j].filledNeighboursCount = source[i][j].filledNeighboursCount;
        };
    };
}
