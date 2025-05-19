#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <fcntl.h>

#include "tcp_server_util.h"
#include "logger.h"

#define PROG_ARG_COUNT 2
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct client_fd_list_t {
    int fd;
    struct client_fd_list_t *next;
} client_fd_list_t;

// TODO: Move to utils
client_fd_list_t *
add_client_fd(client_fd_list_t *client_fd_list, int client_socket)
{
    client_fd_list_t *new_node = malloc(sizeof(client_fd_list_t));
    if (!new_node) {
        log(ERROR, "Failed to allocate memory for new client");
        return client_fd_list;
    }
    new_node->fd = client_socket;
    new_node->next = client_fd_list;
    log(INFO, "New client added to list, total clients: %d", 
        client_fd_list ? 2 : 1);  // Simple count, could be improved
    return new_node;
}

client_fd_list_t *
remove_client_fd(client_fd_list_t *client_fd_list, int client_socket)
{
    if (!client_fd_list) {
        return NULL;
    }

    if (client_fd_list->fd == client_socket) {
        client_fd_list_t *next = client_fd_list->next;
        log(INFO, "Removing client %d from list", client_socket);
        free(client_fd_list);
        return next;
    }

    client_fd_list_t *current = client_fd_list;
    while (current->next) {
        if (current->next->fd == client_socket) {
            client_fd_list_t *next = current->next->next;
            log(INFO, "Removing client %d from list", client_socket);
            free(current->next);
            current->next = next;
            break;
        }
        current = current->next;
    }
    return client_fd_list;
}

void
handle_client_data(int client_socket)
{
    char buffer[BUFFER_SIZE] = {0};
    int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
    
    if (valread <= 0) {
        if (valread < 0) {
            log(ERROR, "Read failed from client %d: %s", client_socket, strerror(errno));
        }
        close(client_socket);
        log(INFO, "Client %d disconnected", client_socket);
        return;
    }

    buffer[valread] = '\0';
    log(INFO, "Received from client %d: %s", client_socket, buffer);
    
    // Echo back to client
    if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
        log(ERROR, "Send failed to client %d: %s", client_socket, strerror(errno));
        close(client_socket);
        log(INFO, "Client %d disconnected", client_socket);
    }
}

int
main(int argc, char *argv[])
{
    close(STDIN_FILENO); // Close stdin since we don't need it
    int listenfd = -1;
    client_fd_list_t *client_fd_list = NULL;
    fd_set read_fds;
    int max_fd;

    if (argc != PROG_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listenfd = setup_tcp_server("0.0.0.0", argv[1]);
    if (listenfd < 0) {
        log(FATAL, "Failed to setup server");
        exit(EXIT_FAILURE);
    }

    log(INFO, "Server started successfully, waiting for connections...");

    // Main server loop
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(listenfd, &read_fds);
        max_fd = listenfd;

        // Add all client sockets to the set
        client_fd_list_t *current = client_fd_list;
        while (current) {
            FD_SET(current->fd, &read_fds);
            if (current->fd > max_fd) {
                max_fd = current->fd;
            }
            current = current->next;
        }

        // Wait for activity on any socket
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            log(ERROR, "select() failed: %s", strerror(errno));
            continue;
        }

        // Check for new connection
        if (FD_ISSET(listenfd, &read_fds)) {
            int client_socket = accept_tcp_connection(listenfd);
            if (client_socket >= 0) {
                log(INFO, "New connection accepted, socket: %d", client_socket);
                client_fd_list = add_client_fd(client_fd_list, client_socket);
            }
        }

        // Check for data from existing clients
        current = client_fd_list;
        while (current) {
            int client_socket = current->fd;
            client_fd_list_t *next = current->next;  // Save next before potential removal

            if (FD_ISSET(client_socket, &read_fds)) {
                handle_client_data(client_socket);
                // If the client was disconnected, remove it from the list
                if (fcntl(client_socket, F_GETFD) == -1) {
                    client_fd_list = remove_client_fd(client_fd_list, client_socket);
                }
            }
            current = next;
        }
    }
}

