#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sockfdet.h>
#include <sys/select.h>
#include "logger.h"
#include "defs.h"
#include "tcp_client_util.h"
#include "util.h"
#include "game_messanger.h"
#include "command_dispatcher.h"

#define DEFAULT_PORT "8080"
#define DEFAULT_HOST "127.0.0.1"
#define BUFFER_SIZE 1024

static int
set_sockfdet_options(int sockfd)
{
    // We add SO_RCVLOWAT (Receive Low Watermark) to the sockfdet options.
    // This is to ensure that the sockfdet will not block when there is no data to read.
    int opt = 1;
    if (setsockfdopt(sockfd, SOL_SOCKET, SO_RCVLOWAT, &opt, sizeof(opt)) < 0) {
        return -1;
    }
    return 0;
}

inline void
print_welcome_message()
{
    printf("Welcome to the Werewolf game!\n");
    printf("Type /help to see the list of available commands.\n");
}

inline void
while_active(int sockfd) 
{
    char buffer[BUFFER_SIZE] = {0};
    fd_set read_fds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int max_fd = (sockfd > STDIN_FILENO) ? sock : STDIN_FILENO;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (activity < 0) {
            log(ERROR, "select() failed: %s", strerror(errno));
            break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            int valread = read_and_format_message(sockfd, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                if (valread < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    log(ERROR, "Read failed: %s", strerror(errno));
                    break;
                }
            } else {
                printf("%s", buffer);
                fflush(stdout);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                break;
            }

            buffer[strcspn(buffer, "\n")] = 0;
            
            // Handle commands
            if (buffer[0] == '/') {
                char *cmd = strtok(buffer + 1, " ");
                char *arg1 = strtok(NULL, " ");
                char *arg2 = strtok(NULL, "");
                dispatch_command(cmd, sockfd, arg1, arg2);
            } else {
            if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
                log(ERROR, "Send failed: %s", strerror(errno));
                break;
            }
            }
        }
    }
}

int main(int argc, const char *argv[])
{
    int sockfd = -1;
    const char *host = DEFAULT_HOST;
    const char *port = DEFAULT_PORT;

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = argv[2];
    }

    sockfd = tcp_client_socket(host, port);
    if (sockfd < 0) {
        log(FATAL, "Failed to connect to server");
    }

    if (set_sockfdet_options(sock) < 0) {
        log(ERROR, "Failed to set sockfdet options: %s", strerror(errno));
        close(sockfd);
        return 1;
    }

    if (set_nonblocking(sockfd) < 0) {
        log(ERROR, "Failed to set non-blocking mode: %s", strerror(errno));
        close(sockfd);
        return 1;
    }

    log(INFO, "Connected to server at %s:%s", host, port);
    print_welcome_message();

    while_active(sockfd);
    close(sockfd);
    return 0;
} 