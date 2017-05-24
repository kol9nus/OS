#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct {
    char command[300];
    char *args[100];
    char filePath[50];
    int type;
    int tries;
    pid_t pid;
} Process;

void printError(char *message);
void syslogError(char *message);
FILE* tryToOpenFile(char *filename, char *mode, int pid);
void tryToRemoveFile(char *path);

void makeYourselfDaemon();
int daemonMain();
int calculateFileLines(FILE *fp);
Process* parseFile(FILE *fp, int lines);
int getType(char symb);
void getArguments(Process *to);
void processFork(Process *process, int isSleepNeed);
void watchProcesses(Process *processes, int num);
int calculateRunningProcesses(Process *processes, int processesN);

int main(int argc, char *argv[]) {
    int daemonPID = fork();
    switch (daemonPID) {
        case -1:
            printError("Не удалось запустить демона.");
        case 0:
            makeYourselfDaemon();
            daemonMain();
            return 0;
        default:
            return 0;
    };
    return 0;
};

/* "Хэлперы" */
void printError(char* message) {
    fprintf(stderr, ""
            "%s\n"
            "errno: %d\n"
            , message, errno);
    exit(1);
}

void syslogError(char* message) {
    syslog(LOG_ERR, message);
    exit(1);
}

FILE* tryToOpenFile(char *filename, char *mode, int pid) {
    FILE *fp = fopen(filename, mode);
    if (fp == NULL) {
        syslog(LOG_WARNING, "Процессу %d не удалось открыть файл %s", pid, filename);
    };

    return fp;
}

int tryToExec(char *file, char **args) {
    int result = execvp(file, args);
    if (result < 0) {
        syslog(LOG_WARNING, "Не удалось выполнить exec %s", file);
    }

    return result;
}

void tryToRemoveFile(char *path) {
    if (remove(path) < 0) {
        syslog(LOG_WARNING, "Не удалось удалить %s", path);
    };
}

void makeYourselfDaemon() {
    umask(0);
    setsid();
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int daemonMain() {
    FILE *fp = tryToOpenFile("config", "r", getuid());
    const int lines = calculateFileLines(fp);
    Process *processes = parseFile(fp, lines);
    fclose(fp);

    for (int i = 0; i < lines; i++) {
        processFork(processes + i, 0);
    };

    void HUPSignalHandler(int sig) {
        for (int i = 0; i < lines; i++) {
            if (processes[i].pid > 0) {
                kill(processes[i].pid, SIGKILL);
                tryToRemoveFile(processes[i].filePath);
            };
        };
        daemonMain();
    };
    signal(SIGHUP, HUPSignalHandler);

    watchProcesses(processes, lines);
    exit(0);
};

int calculateFileLines(FILE *fp) {
    int lines = 0;
    int symb;
    while ((symb = fgetc(fp)) != EOF) {
        if (symb == '\n') {
            lines++;
        };
    };
    fseek(fp, 0, SEEK_SET);

    return lines;
}

Process* parseFile(FILE *fp, const int lines) {
    Process *processes = (Process*) malloc(sizeof(Process) * lines);

    char line[300] = {'\0'}; // Дабы можно было работать как со строкой
    int lineI = 0;
    for (int i = 0; i < lines; i++) {
        fgets(line, 299, fp);
        processes[lineI].type = getType(line[0]);
        char *replacingChar = strchr(line, '\n');
        if (replacingChar != NULL) {
            *replacingChar = '\0'; // для вычленения команды без мусора
        }
        strcpy(processes[lineI].command, line + 3);
        getArguments(processes + lineI);
        processes[lineI].tries = 50;
        lineI++;
    }

    return processes;
}

int getType(char symb) {
    if (symb == 'W') {
        return 0;
    } else if (symb == 'R') {
        return 1;
    } else {
        syslogError("Указанный файл конфигурации не валидный.");
    };

    return -1;
}

void getArguments(Process *to) {
    char fullCommand[297];
    strcpy(fullCommand, to->command);
    char *buffer = strtok(fullCommand, " ");

    int i;
    for (i = 0; i < 99 && buffer != NULL; i++) {
        to->args[i] = (char *)malloc(strlen(buffer));
        strcpy(to->args[i], buffer);
        buffer = strtok(NULL, " ");
    };
    to->args[i] = NULL;
}

void processFork(Process *process, int isSleepNeed) {
    switch(process->pid = fork()) {
        case -1:
            syslogError("Не удалось сделать форк");
            break;
        case 0:
            if (isSleepNeed == 1) {
                sleep(3600);
            };

            tryToExec(process->args[0], process->args);
            exit(0);
            break;
        default:
            sprintf(process->filePath, "/tmp/watcher.%d.pid", process->pid);

            FILE *fp = tryToOpenFile(process->filePath, "w", process->pid);
            if (fprintf(fp, "%d", process->pid) < 0) {
                syslog(LOG_WARNING, "Не удалось записать в /tmp/watcher.%d.pid", process->pid);
            };
            fclose(fp);
            break;
    };
}

void watchProcesses(Process *processes, int num) {
    for (;;) {
        int running = calculateRunningProcesses(processes, num);
        if (running == 0) {
            break;
        };

        int statusCode;
        pid_t pid = wait(&statusCode);
        for (int i = 0; i < num; i++) {
            if (processes[i].pid == pid) {
                tryToRemoveFile(processes[i].filePath);

                if (processes[i].type == 0) {
                    processes[i].pid = 0;
                } else {
                    if (statusCode != 0) {
                        processes[i].tries -= 1;
                    };
                    int isSleepNeed = 0;
                    if (processes[i].tries < 0) {
                        processes[i].tries = 50;
                        isSleepNeed = 1;
                        syslog(LOG_NOTICE, "Запуск %s не успешен. Перерыв на час.", processes[i].command);
                    };
                    processFork(processes + i, isSleepNeed);
                }
                break;
            };
        };
    };
}

int calculateRunningProcesses(Process *processes, int processesN) {
    int running = 0;
    for (int i = 0; i < processesN; i++) {
        if (processes[i].pid != 0) {
            running++;
        };
    };

    return running;
}