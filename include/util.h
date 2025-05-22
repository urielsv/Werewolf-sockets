#ifndef __util_h__
#define __util_h__

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

/* Constants */
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
#define MAX_ADDR_LEN INET6_ADDRSTRLEN
#define MAX_TIMEOUT 30

/* Function declarations */
const char *print_family(struct addrinfo *aip);
const char *print_type(struct addrinfo *aip);
const char *print_flags(struct addrinfo *aip);
char *print_address_port(const struct addrinfo *aip, char addr[]);
int print_socket_address(const struct sockaddr *sa, char addr[]);
int socket_addrs_equal(const struct sockaddr *sa1, const struct sockaddr *sa2);
int set_socket_timeout(int sockfd, int timeout_sec);
int set_nonblocking(int sockfd);

#endif // __util_h__
