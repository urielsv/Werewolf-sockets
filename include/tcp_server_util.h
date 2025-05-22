#ifndef __tcp_server_util_h_
#define __tcp_server_util_h_

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

/* Constants */
#define BUF_SIZE (2<<8)
#define MAX_ADDR_BUFF (2<<5)
#define SOL_IPV6 41


/* Client list management */
typedef struct client_fd_list_t {
    int fd;
    struct client_fd_list_t *next;
} client_fd_list_t;

/* Function declarations */
int setup_tcp_server(const char *host, const char *service, int max_backlog);
int accept_tcp_connection(int server_socket);
int set_server_socket_options(int sockfd, int family);
int validate_server_input(const char *host, const char *service);

/* Client list management functions */
client_fd_list_t *add_client_fd(client_fd_list_t *client_fd_list, int client_socket);
client_fd_list_t *remove_client_fd(client_fd_list_t *client_fd_list, int client_socket);
bool is_socket_connected(int sockfd);
#endif // __tcp_server_util_h__
