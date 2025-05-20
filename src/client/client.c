#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "logger.h"
#include "defs.h"
#include "tcp_client_util.h"
#include "util.h"

#define DEFAULT_PORT "8080"
#define DEFAULT_HOST "127.0.0.1"
#define BUFFER_SIZE 1024

static int set_socket_options(int sock) {
    // We add SO_RCVLOWAT (Receive Low Watermark) to the socket options.
    // This is to ensure that the socket will not block when there is no data to read.
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVLOWAT, &opt, sizeof(opt)) < 0) {
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int sock = 0;
    char buffer[BUFFER_SIZE] = {0};
    const char *host = DEFAULT_HOST;
    const char *port = DEFAULT_PORT;

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = argv[2];
    }

    sock = tcp_client_socket(host, port);
    if (sock < 0) {
        log(FATAL, "Failed to connect to server");
    }

    if (set_socket_options(sock) < 0) {
        log(ERROR, "Failed to set socket options: %s", strerror(errno));
        close(sock);
        return 1;
    }

    if (set_nonblocking(sock) < 0) {
        log(ERROR, "Failed to set non-blocking mode: %s", strerror(errno));
        close(sock);
        return 1;
    }

    log(INFO, "Connected to server at %s:%s", host, port);

    fd_set read_fds;
    struct timeval tv;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (activity < 0) {
            log(ERROR, "select() failed: %s", strerror(errno));
            break;
        }

        if (FD_ISSET(sock, &read_fds)) {
            int valread = read(sock, buffer, BUFFER_SIZE - 1);
            if (valread <= 0) {
                if (valread < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    log(ERROR, "Read failed: %s", strerror(errno));
                    break;
                }
            } else {
                buffer[valread] = '\0';
                printf("%s", buffer);
                fflush(stdout);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                break;
            }

            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                log(ERROR, "Send failed: %s", strerror(errno));
                break;
            }
        }
    }

    close(sock);
    return 0;
} 