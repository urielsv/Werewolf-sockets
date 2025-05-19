#ifndef __tcp_server_util_h_
#define __tcp_server_util_h_

#include <sys/socket.h>
#include <netinet/in.h>

/* Constants */
#define MAX_BACKLOG 20
#define BUF_SIZE (2<<8)
#define MAX_ADDR_BUFF (2<<5)
#define SOL_IPV6 41

/* Function declarations */
int setup_tcp_server(const char *host, const char *service);
int accept_tcp_connection(int server_socket);
int set_server_socket_options(int sockfd, int family);
int validate_server_input(const char *host, const char *service);

#endif // __tcp_server_util_h__
