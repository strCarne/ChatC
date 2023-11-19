#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define SERVING 1
#define NOT_AN_ERR 1

struct AcceptedSocket {
    int acceptedSocketFD;
    struct sockaddr_in address;
    int error;
    int acceptedSuccessfully;
};

#define MAX_BACKLOG 10

#define MSG_DATA_TYPE 0
#define LEAVE_DATA_TYPE 1

int createTCPIPv4Socket();
struct sockaddr_in *createIPv4Address(char *ip, unsigned short port);
struct AcceptedSocket *acceptIncomingConnection(int serverSocketFD);
void receiveAndPrintIncomingData(int socketFD);
void startAcceptingIncomingConnections(int serverSocketFD);
void receiveAndPrintIncomingDataOnASeparateThread(const struct AcceptedSocket *clientSocket);

void broadcastToOther(char data[2048], int dataType, int fromFD);

void repairUsers(int socketFD);

struct AcceptedSocket users[MAX_BACKLOG];
int numOfUsersOnline = 0;

int main() {
    int serverSocketFD = createTCPIPv4Socket();
    struct sockaddr_in *serverAddress = createIPv4Address("", 2000);

    int error = bind(serverSocketFD, serverAddress, sizeof *serverAddress);

    if (error) {
        printf("LOG: Binding failed!\n");
        return 1;
    }
    printf("LOG: Server's socket was bound successfully.\n");

    error = listen(serverSocketFD, MAX_BACKLOG);
    if (error) {
        printf("LOG: Listening failed!\n");
        return 1;
    }
    printf("LOG: Server's socket started listening.\n");

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RDWR);

    free(serverAddress);
    return 0;
}

void startAcceptingIncomingConnections(int serverSocketFD) {

    for (; SERVING;) {
        struct AcceptedSocket *clientSocket = acceptIncomingConnection(serverSocketFD);

        if (clientSocket->acceptedSuccessfully < 0) {
            printf("LOG: Failed to accept the connection from the client socket");
            return;
        }
        printf("LOG: Accepted client %d\n ", clientSocket->acceptedSocketFD);
        users[numOfUsersOnline] = *clientSocket;
        ++numOfUsersOnline;

        receiveAndPrintIncomingDataOnASeparateThread(clientSocket);
    }
}

void receiveAndPrintIncomingDataOnASeparateThread(const struct AcceptedSocket *clientSocket) {

    // id of the process thread
    pthread_t id;

    // pthread_create creates a new thread in the operating system,
    // asks for the function and runs inside the different thread
    pthread_create(&id, NULL, (void *(*)(void *)) receiveAndPrintIncomingData, (void *) clientSocket->acceptedSocketFD);
}

void receiveAndPrintIncomingData(int socketFD) {
    char buffer[2048];
    for (; SERVING; ) {
        ssize_t amountReceived = recv(socketFD, buffer, 2048, 0);
        if (amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("Client msg: %s", buffer);
            broadcastToOther(buffer, MSG_DATA_TYPE, socketFD);
        } else if (amountReceived == 0)  {
            printf("Connection closed.\n");
            break;
        } else {
            printf("ERR\n");
            break;
        }
    }
    repairUsers(socketFD);
    close(socketFD);
}

void repairUsers(int socketFD) {
    for (int i = 0; i < numOfUsersOnline; ++i) {
        if (users[i].acceptedSocketFD == socketFD) {
            users[i] = users[numOfUsersOnline - 1];
            --numOfUsersOnline;
            break;
        }
    }
}

void broadcastToOther(char data[2048], int dataType, int fromFD) {

    for (int i = 0; i < numOfUsersOnline; ++i) {
        int userSocketFD = users[i].acceptedSocketFD;
        if (userSocketFD == fromFD) {
            continue;
        }

        send(userSocketFD, data, strlen(data), 0);
//        char msg[2100];
//        switch (dataType) {
//            case MSG_DATA_TYPE:
//                snprintf(msg, sizeof msg, "%d: %s", fromFD, data);
//                send(userSocketFD, msg, strlen(msg), 0);
//                send(userSocketFD, data, strlen(data), 0);
//                break;
//            case LEAVE_DATA_TYPE:
//                break;
//        }
    }

}

struct AcceptedSocket *acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in address;
    socklen_t addressSize = sizeof address;
    int socketFD = accept(serverSocketFD, (struct sockaddr *) &address, &addressSize);

    struct AcceptedSocket *acceptedSocket = malloc(sizeof(struct AcceptedSocket));
    acceptedSocket->address = address;
    acceptedSocket->acceptedSocketFD = socketFD;
    acceptedSocket->acceptedSuccessfully = socketFD > 0;
    acceptedSocket->error = socketFD > 0 ? NOT_AN_ERR : socketFD;

    return acceptedSocket;
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