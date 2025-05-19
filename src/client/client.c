#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "logger.h"
#include "defs.h"
#include "tcp_client_util.h"
#include "util.h"

#define DEFAULT_PORT "8080"
#define DEFAULT_HOST "127.0.0.1"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sock = 0;
    char buffer[BUFFER_SIZE] = {0};
    const char *host = DEFAULT_HOST;
    const char *port = DEFAULT_PORT;

    // Parse command line arguments
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = argv[2];
    }

    // Create socket and connect to server
    sock = tcp_client_socket(host, port);
    if (sock < 0) {
        log(FATAL, "Failed to connect to server");
    }

    log(INFO, "Connected to server at %s:%s", host, port);

    // Main client loop
    while (1) {
        // Read from stdin
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        // Send message to server
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            log(ERROR, "Send failed: %s", strerror(errno));
            break;
        }

        // Receive response from server
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            log(ERROR, "Read failed: %s", strerror(errno));
            break;
        }

        buffer[valread] = '\0';
        printf("Server response: %s", buffer);
    }

    close(sock);
    return 0;
} 