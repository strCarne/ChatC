#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define CHATTING 1
#define LISTENING_FOR_MESSAGES 1

int createTCPIPv4Socket();
struct sockaddr_in *createIPv4Address(char *ip, unsigned short port);

void startListeningForMsgs(int clientSocketFD);

void listenAndPrint(int clientSocketFD);

int main() {

    int socketFD = createTCPIPv4Socket();

    struct sockaddr_in *address = createIPv4Address("127.0.0.1", 2000);

    int connectionFailed = connect(socketFD, (const struct sockaddr *)address, sizeof *address);

    if (connectionFailed) {
        printf("Connection failed.\n");
        return 1;
    } else {
        printf("Connected successfully!\n");
    }

    startListeningForMsgs(socketFD);

    char *clientName = NULL;
    size_t nameSize = 0;
    printf("Enter your name: ");
    ssize_t nameCharCount = getline(&clientName, &nameSize, stdin);
    clientName[nameCharCount - 1] = 0;

    char buffer[2048 + nameSize];

    char *line = NULL;
    size_t lineSize = 0;
    printf("Enter your msg: ");
    for (; CHATTING; ) {
        ssize_t charCount = getline(&line, &lineSize, stdin);
//        line[charCount-1] = 0;

        if (charCount > 0) {
            if (strcmp(line, "exit\n") == 0) {
                break;
            }

            sprintf(buffer, "\n%s: %s", clientName, line);
            ssize_t amountWasSend = send(socketFD, buffer, strlen(buffer), 0);
            if (amountWasSend < charCount) {
                printf("Server side error: server isn't working.\n");
                break;
            }
        }
    }

    close(socketFD);

    free(address);
    return 0;
}

void startListeningForMsgs(int clientSocketFD) {
    pthread_t id;
    pthread_create(&id, NULL, (void *(*)(void *)) listenAndPrint, (void *) clientSocketFD);
}

void listenAndPrint(int clientSocketFD) {
    char buffer[2048];
    for (; LISTENING_FOR_MESSAGES; ) {
        ssize_t amountReceived = recv(clientSocketFD, buffer, 2048, 0);
        if (amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s", buffer);
        } else if (amountReceived == 0)  {
            printf("Connection closed.\n");
            break;
        } else {
            printf("ERR\n");
            break;
        }
    }
}

int createTCPIPv4Socket() { return socket(AF_INET, SOCK_STREAM, 0); }

struct sockaddr_in *createIPv4Address(char *ip, unsigned short port) {

    struct sockaddr_in *address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if (!strlen(ip)) {
        address->sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, ip, &address->sin_addr.s_addr);
    }

    return address;
}