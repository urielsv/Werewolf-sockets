#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "logger.h"
#include "defs.h"
#include "util.h" 
#include "tcp_server_util.h"

static char addr_buff[MAX_ADDR_BUFF];

int
validate_server_input(const char *host, const char *service)
{
    if (!host || !service) {
        log(ERROR, "Invalid host or service parameters");
        return RET_ERROR;
    }

    // Validate service (port) is numeric
    for (const char *p = service; *p; p++) {
        if (*p < '0' || *p > '9') {
            log(ERROR, "Invalid service number");
            return RET_ERROR;
        }
    }

    // Validate port range
    int port = atoi(service);
    if (port <= 0 || port > 65535) {
        log(ERROR, "Service number out of range");
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

int
set_server_socket_options(int sockfd, int family)
{
    int optval = 1;

    // Set SO_REUSEADDR for both IPv4 and IPv6
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        log(ERROR, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        return RET_ERROR;
    }

    // For IPv6, set IPV6_V6ONLY to true
    if (family == AF_INET6) {
        if (setsockopt(sockfd, SOL_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) < 0) {
            log(ERROR, "setsockopt(IPV6_V6ONLY) failed: %s", strerror(errno));
            return RET_ERROR;
        }
    }

    if (set_nonblocking(sockfd) < 0) {
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

int
accept_tcp_connection(int server_socket)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_socket < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No pending connections
            return RET_ERROR;
        }
        log(ERROR, "accept() failed: %s", strerror(errno));
        return RET_ERROR;
    }

    if (set_nonblocking(client_socket) < 0) {
        close(client_socket);
        return RET_ERROR;
    }

    if (set_socket_timeout(client_socket, MAX_TIMEOUT) < 0) { 
        close(client_socket);
        return RET_ERROR;
    }

    if (print_socket_address((struct sockaddr *) &client_addr, addr_buff) == RET_SUCCESS) {
        log(INFO, "New connection from %s", addr_buff);
    }

    return client_socket;
}

int
setup_tcp_server(const char *host, const char *service)
{
    if (validate_server_input(host, service) < 0) {
        return RET_ERROR;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *ailist;
    int err = getaddrinfo(host, service, &hints, &ailist);
    if (err != 0) {
        log(ERROR, "getaddrinfo() failed: %s", gai_strerror(err));
        return RET_ERROR;
    }

    int sockfd = -1;
    for (struct addrinfo *aip = ailist; aip != NULL; aip = aip->ai_next) {
        sockfd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
        if (sockfd < 0) {
            log(WARN, "socket() failed: %s", strerror(errno));
            continue;
        }

        if (set_server_socket_options(sockfd, aip->ai_family) < 0) {
            close(sockfd);
            continue;
        }

        if (bind(sockfd, aip->ai_addr, aip->ai_addrlen) < 0) {
            log(WARN, "bind() failed: %s", strerror(errno));
            close(sockfd);
            continue;
        }

        if (listen(sockfd, MAX_BACKLOG) < 0) {
            log(WARN, "listen() failed: %s", strerror(errno));
            close(sockfd);
            continue;
        }

        if (print_socket_address(aip->ai_addr, addr_buff) == RET_SUCCESS) {
            log(INFO, "Server listening on %s", addr_buff);
        }
        break;
    }

    freeaddrinfo(ailist);

    if (sockfd < 0) {
        log(ERROR, "Failed to setup server on any address");
        return RET_ERROR;
    }

    return sockfd;
}

