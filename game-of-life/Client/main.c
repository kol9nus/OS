#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void printError(char* message);
int tryToExecute(int result, char *msg);
int tryToConnect(char *host);
struct hostent* tryToResolveHost(char *host);
struct sockaddr_in getServerInfo(struct hostent *server);
void printFieldFromSock(int sock);

int main(int argc, char *argv[]) {
    int sock = tryToConnect("localhost");
    printFieldFromSock(sock);
    close(sock);

    return 0;

};

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

int tryToConnect(char *host) {
    int sock = tryToExecute(socket(AF_INET, SOCK_STREAM, 0), "Не удалось создать сокет");
    struct hostent *server = tryToResolveHost(host);

    struct sockaddr_in serverAddr = getServerInfo(server);
    tryToExecute(
            connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)),
            "Не удалось присоединиться"
    );

    return sock;
}

struct hostent* tryToResolveHost(char *host) {
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        printError("Не удалось зарезолвить хоста");
    };

    return server;
}

struct sockaddr_in getServerInfo(struct hostent *server) {
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    bcopy(server->h_addr, (char *)&servAddr.sin_addr.s_addr, (size_t) server->h_length);
    servAddr.sin_port = htons(9999);

    return servAddr;
}

void printFieldFromSock(int sock) {
    char symb[2];
    while (read(sock, symb, 1) > 0) {
        symb[1] = '\0';
        if (symb[0] == 1 || symb[0] == 0) {
            printf("%d", symb[0]);
        } else {
            printf("%s", symb);
        };
    };
}