#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "logger.h"
#include "defs.h"
#include "util.h"

#define FLAG_BUFF_SIZE 100
#define ADDR_BUFF_SIZE INET6_ADDRSTRLEN

const char *
print_family(struct addrinfo *aip) 
{
    switch (aip->ai_family) {
        case AF_INET:
            return "inet";
        case AF_INET6:
            return "inet6";
        case AF_UNIX:
            return "unix";
        case AF_UNSPEC:
            return "unspecified";
        default:
            return "unknown";
    }
}

const char *
print_type(struct addrinfo *aip) {
    switch (aip->ai_socktype) {
        case SOCK_STREAM:
            return "stream";
        case SOCK_DGRAM:
            return "datagram";
        case SOCK_RAW:
            return "raw";
        case SOCK_SEQPACKET:
            return "seqpacket";
        default:
            return "unknown";
    }
}

const char *
print_flags(struct addrinfo *aip) {
    if (aip->ai_flags == 0) {
        log(INFO, "flags: 0x%x", aip->ai_flags);
    }
    else {
        log(INFO, "flags: 0x%x", aip->ai_flags);
        if (aip->ai_flags & AI_PASSIVE)
            log(INFO, "  - passive");
        if (aip->ai_flags & AI_CANONNAME)
            log(INFO, "  - canonname");
        if (aip->ai_flags & AI_NUMERICHOST)
            log(INFO, "  - numerichost");
        if (aip->ai_flags & AI_NUMERICSERV)
            log(INFO, "  - numericport");
        if (aip->ai_flags & AI_V4MAPPED)
            log(INFO, "  - v4mapped");
        if (aip->ai_flags & AI_ADDRCONFIG)
            log(INFO, "  - addrconfig");
        if (aip->ai_flags & AI_ALL)
            log(INFO, "  - all");
    }
    return NULL;
}

char *
print_address_port(const struct addrinfo *aip, char addr[]) {
    char abuf[INET6_ADDRSTRLEN];
    const char *addr_aux;
    struct sockaddr_in *sin;
    struct sockaddr_in6 *sin6;

    switch (aip->ai_family) {
        case AF_INET:
            sin = (struct sockaddr_in *) aip->ai_addr;
            addr_aux = inet_ntop(AF_INET, &sin->sin_addr, abuf, sizeof(abuf));
            if (addr_aux == NULL) {
                log(ERROR, "inet_ntop() failed");
                return NULL;
            }
            strncpy(addr, addr_aux, ADDR_BUFF_SIZE);
            if (sin->sin_port != 0) {
                snprintf(addr + strlen(addr), ADDR_BUFF_SIZE - strlen(addr), 
                        ":%d", ntohs(sin->sin_port));
            }
            break;
        case AF_INET6:
            sin6 = (struct sockaddr_in6 *) aip->ai_addr;
            addr_aux = inet_ntop(AF_INET6, &sin6->sin6_addr, abuf, sizeof(abuf));
            if (addr_aux == NULL) {
                log(ERROR, "inet_ntop() failed");
                return NULL;
            }
            strncpy(addr, addr_aux, ADDR_BUFF_SIZE);
            if (sin6->sin6_port != 0) {
                snprintf(addr + strlen(addr), ADDR_BUFF_SIZE - strlen(addr), 
                        ":%d", ntohs(sin6->sin6_port));
            }
            break;
        default:
            log(ERROR, "unknown address family");
            strncpy(addr, "unknown", ADDR_BUFF_SIZE);
    }
    return addr;
}

int
print_socket_address(const struct sockaddr *sa, char addr[]) {
    void *numeric_addr;
    in_port_t port;
    switch (sa->sa_family) {
        case AF_INET:
            numeric_addr = &((struct sockaddr_in *) sa)->sin_addr;
            port = ntohs(((struct sockaddr_in *) sa)->sin_port);
            break;
        case AF_INET6:
            numeric_addr = &((struct sockaddr_in6 *) sa)->sin6_addr;
            port = ntohs(((struct sockaddr_in6 *) sa)->sin6_port);
            break;
        default:
            strncpy(addr, "unknown", ADDR_BUFF_SIZE);
            return RET_SUCCESS;
    }

    if (inet_ntop(sa->sa_family, numeric_addr, addr, ADDR_BUFF_SIZE) == NULL) {
        log(ERROR, "inet_ntop() failed, invalid address family");
        return RET_ERROR;
    }

    if (port != 0) {
        snprintf(addr + strlen(addr), ADDR_BUFF_SIZE - strlen(addr), ":%u", port);
    }
    return RET_SUCCESS;
}

int
socket_addrs_equal(const struct sockaddr *sa1, const struct sockaddr *sa2) {
    if (sa1 == NULL || sa2 == NULL) {
        return RET_ERROR;
    }

    if (sa1->sa_family != sa2->sa_family) {
        return RET_ERROR;
    }
    
    if (sa1->sa_family == AF_INET) {
        struct sockaddr_in *ipv4_1 = (struct sockaddr_in *) sa1;
        struct sockaddr_in *ipv4_2 = (struct sockaddr_in *) sa2;
        return memcmp(&ipv4_1->sin_addr, &ipv4_2->sin_addr, sizeof(ipv4_1->sin_addr)) == 0 && ipv4_1->sin_port == ipv4_2->sin_port;
    }

    if (sa1->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6_1 = (struct sockaddr_in6 *) sa1;
        struct sockaddr_in6 *ipv6_2 = (struct sockaddr_in6 *) sa2;
        return memcmp(&ipv6_1->sin6_addr, &ipv6_2->sin6_addr, sizeof(ipv6_1->sin6_addr)) == 0 && ipv6_1->sin6_port == ipv6_2->sin6_port;
    }

    return RET_SUCCESS;
}

int
set_socket_timeout(int sockfd, int timeout_sec)
{
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        log(ERROR, "Failed to set receive timeout: %s", strerror(errno));
        return RET_ERROR;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        log(ERROR, "Failed to set send timeout: %s", strerror(errno));
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

int
set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        log(ERROR, "Failed to get socket flags: %s", strerror(errno));
        return RET_ERROR;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        log(ERROR, "Failed to set non-blocking mode: %s", strerror(errno));
        return RET_ERROR;
    }
    return RET_SUCCESS;
}