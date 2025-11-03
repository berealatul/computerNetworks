#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 8080
#define BUF_SIZE 1024

// structure to represent the server
typedef struct {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    char buffer[BUF_SIZE];
    int port;
} Server;

// function declarations
int getPort(int argc, char *argv[], int defaultPort);
void initServer(Server *s, int port);
void receiveData(Server *s);
void sendData(Server *s);
void handleFileReception(Server *s, const char *filename);

// get port from command line or use default
int getPort(int argc, char *argv[]) {
    if (argc >= 2) {
        return atoi(argv[1]);
    }
    return DEFAULT_PORT;
}

// create and bind udp socket
void initServer(Server *s, int port) {
    s->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    s->serverAddr.sin_family = AF_INET;
    s->serverAddr.sin_port = htons(port);
    s->serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s->sockfd, (struct sockaddr*)&s->serverAddr, sizeof(s->serverAddr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    s->addr_size = sizeof(s->clientAddr);
    s->port = port;

    printf("server started on port %d\n", port);
}

// handle file reception from client
void handleFileReception(Server *s, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("file open failed");
        return;
    }

    printf("receiving file: %s\n", filename);

    while (1) {
        memset(s->buffer, 0, BUF_SIZE);
        recvfrom(s->sockfd, s->buffer, BUF_SIZE, 0, (struct sockaddr*)&s->clientAddr, &s->addr_size);
        if (strcmp(s->buffer, "EOF") == 0)
            break;
        fwrite(s->buffer, 1, strlen(s->buffer), fp);
    }

    fclose(fp);
    printf("file received successfully\n");
}

// receive message or file from client
void receiveData(Server *s) {
    memset(s->buffer, 0, BUF_SIZE);
    recvfrom(s->sockfd, s->buffer, BUF_SIZE, 0, (struct sockaddr*)&s->clientAddr, &s->addr_size);

    if (strncmp(s->buffer, "FILE:", 5) == 0) {
        handleFileReception(s, s->buffer + 5);
    } else if (strcmp(s->buffer, "exit") == 0) {
        printf("client ended chat\n");
        exit(0);
    } else {
        printf("Client: %s\n", s->buffer);
    }
}

// send message to client
void sendData(Server *s) {
    char msg[BUF_SIZE];
    printf("You: ");
    fgets(msg, BUF_SIZE, stdin);
    msg[strcspn(msg, "\n")] = 0;

    sendto(s->sockfd, msg, strlen(msg), 0, (struct sockaddr*)&s->clientAddr, s->addr_size);

    if (strcmp(msg, "exit") == 0) {
        printf("server closed\n");
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    Server s;
    initServer(&s, getPort(argc, argv));

    while (1) {
        receiveData(&s);
        sendData(&s);
    }

    close(s.sockfd);
    return 0;
}
