// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 8080
#define DEFAULT_IP "127.0.0.1"
#define BUF_SIZE 1024

// structure to represent the client
typedef struct {
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    char buffer[BUF_SIZE];
    int port;
    char ip[50];
} Client;

// function declarations
int getPort(int argc, char *argv[], int defaultPort);
char* getIP(int argc, char *argv[], const char *defaultIP);
void initClient(Client *c, const char *serverIP, int port);
void sendData(Client *c);
void receiveData(Client *c);
void sendFile(Client *c, const char *filepath);

// get port from command line or use default
int getPort(int argc, char *argv[], int defaultPort) {
    if (argc >= 3) {
        return atoi(argv[2]);
    }
    return defaultPort;
}

// get ip from command line or use default
char* getIP(int argc, char *argv[], const char *defaultIP) {
    if (argc >= 2) {
        return argv[1];
    }
    return (char*)defaultIP;
}

// initialize udp client
void initClient(Client *c, const char *serverIP, int port) {
    c->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (c->sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    c->serverAddr.sin_family = AF_INET;
    c->serverAddr.sin_port = htons(port);
    c->serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    c->addr_size = sizeof(c->serverAddr);

    strcpy(c->ip, serverIP);
    c->port = port;

    printf("connected to server %s:%d\n", serverIP, port);
}

// send file to server
void sendFile(Client *c, const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("file open failed");
        return;
    }

    char filename[100];
    strcpy(filename, strrchr(filepath, '/') ? strrchr(filepath, '/') + 1 : filepath);

    char header[120];
    sprintf(header, "FILE:%s", filename);
    sendto(c->sockfd, header, strlen(header), 0, (struct sockaddr*)&c->serverAddr, c->addr_size);

    printf("sending file: %s\n", filename);

    char filebuf[BUF_SIZE];
    while (!feof(fp)) {
        int n = fread(filebuf, 1, BUF_SIZE - 1, fp);
        filebuf[n] = '\0';
        sendto(c->sockfd, filebuf, n, 0, (struct sockaddr*)&c->serverAddr, c->addr_size);
        usleep(1000);
    }

    sendto(c->sockfd, "EOF", 3, 0, (struct sockaddr*)&c->serverAddr, c->addr_size);
    fclose(fp);
    printf("file sent successfully\n");
}

// send message or file
void sendData(Client *c) {
    char msg[BUF_SIZE];
    printf("you: ");
    fgets(msg, BUF_SIZE, stdin);
    msg[strcspn(msg, "\n")] = 0;

    if (strncmp(msg, "sendfile ", 9) == 0) {
        sendFile(c, msg + 9);
    } else {
        sendto(c->sockfd, msg, strlen(msg), 0, (struct sockaddr*)&c->serverAddr, c->addr_size);
        if (strcmp(msg, "exit") == 0) {
            printf("client closed\n");
            exit(0);
        }
    }
}

// receive reply from server
void receiveData(Client *c) {
    memset(c->buffer, 0, BUF_SIZE);
    recvfrom(c->sockfd, c->buffer, BUF_SIZE, 0, (struct sockaddr*)&c->serverAddr, &c->addr_size);
    printf("server: %s\n", c->buffer);
}

int main(int argc, char *argv[]) {
    Client c;
    char *ip = getIP(argc, argv, DEFAULT_IP);
    int port = getPort(argc, argv, DEFAULT_PORT);
    initClient(&c, ip, port);

    while (1) {
        sendData(&c);
        receiveData(&c);
    }

    close(c.sockfd);
    return 0;
}
