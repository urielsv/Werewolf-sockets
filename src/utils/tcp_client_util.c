#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "logger.h"
#include "defs.h"
#include "util.h"
#include "tcp_client_util.h"

int
validate_client_input(const char *host, const char *port)
{
    if (!host || !port) {
        log(ERROR, "Invalid host or port parameters");
        return RET_ERROR;
    }

    // Check host length
    if (strlen(host) > MAX_HOST_LEN) {
        log(ERROR, "Host name too long");
        return RET_ERROR;
    }

    // Check port length and content
    if (strlen(port) > MAX_PORT_LEN) {
        log(ERROR, "Port number too long");
        return RET_ERROR;
    }

    // Validate port is numeric
    for (const char *p = port; *p; p++) {
        if (*p < '0' || *p > '9') {
            log(ERROR, "Invalid port number");
            return RET_ERROR;
        }
    }

    // Validate port range
    int port_num = atoi(port);
    if (port_num <= 0 || port_num > 65535) {
        log(ERROR, "Port number out of range");
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

int
set_client_socket_options(int sockfd)
{
    // Set socket timeout
    if (set_socket_timeout(sockfd, CONNECT_TIMEOUT_SEC) < 0) {
        return RET_ERROR;
    }

    // Set socket to non-blocking mode
    if (set_nonblocking(sockfd) < 0) {
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

int
tcp_client_socket(const char *host, const char *port)
{
    if (validate_client_input(host, port) < 0) {
        return RET_ERROR;
    }

    char addr_buff[ADDR_BUFF_SIZE];
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *server_info;
    int rv = getaddrinfo(host, port, &hints, &server_info);
    if (rv != 0) {
        log(ERROR, "getaddrinfo() failed: %s", gai_strerror(rv));
        return RET_ERROR;
    }

    int sockfd = -1;
    for (struct addrinfo *paddr = server_info; paddr != NULL && sockfd < 0; paddr = paddr->ai_next) {
        sockfd = socket(paddr->ai_family, paddr->ai_socktype, paddr->ai_protocol);
        if (sockfd < 0) {
            log(WARN, "socket() failed: %s", strerror(errno));
            continue;
        }

        if (set_client_socket_options(sockfd) < 0) {
            close(sockfd);
            sockfd = -1;
            continue;
        }

        if (connect(sockfd, paddr->ai_addr, paddr->ai_addrlen) < 0) {
            if (errno == EINPROGRESS) {
                // Connection in progress, wait for it to complete
                fd_set write_fds;
                struct timeval tv;
                tv.tv_sec = CONNECT_TIMEOUT_SEC;
                tv.tv_usec = 0;

                FD_ZERO(&write_fds);
                FD_SET(sockfd, &write_fds);

                int ret = select(sockfd + 1, NULL, &write_fds, NULL, &tv);
                if (ret <= 0) {
                    log(ERROR, "Connection timeout");
                    close(sockfd);
                    sockfd = -1;
                    continue;
                }

                // Check if connection was successful
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
                    log(ERROR, "Connection failed: %s", strerror(error));
                    close(sockfd);
                    sockfd = -1;
                    continue;
                }
            } else {
                log(ERROR, "connect() failed: %s", strerror(errno));
                close(sockfd);
                sockfd = -1;
                continue;
            }
        }

        if (print_address_port(paddr, addr_buff)) {
            log(INFO, "Connected to %s", addr_buff);
        }
        break;
    }

    freeaddrinfo(server_info);

    if (sockfd < 0) {
        log(ERROR, "Failed to connect to any address");
        return RET_ERROR;
    }

    return sockfd;
}