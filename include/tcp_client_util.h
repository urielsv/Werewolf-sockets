#ifndef __tcp_client_util_h__
#define __tcp_client_util_h__

#include <sys/socket.h>
#include <netinet/in.h>

/* Constants */
#define ADDR_BUFF_SIZE (2<<6)
#define CONNECT_TIMEOUT_SEC 5
#define MAX_HOST_LEN 256
#define MAX_PORT_LEN 6

/* Function declarations */
int tcp_client_socket(const char *host, const char *port);
int set_client_socket_options(int sockfd);
int validate_client_input(const char *host, const char *port);

#endif // __tcp_client_util_h__
