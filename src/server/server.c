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
#include "defs.h"
#include "game_manager.h"
#include "game_config.h"
#include "game_util.h"
#define DEFAULT_PORT "8080"
#define DEFAULT_MAX_PLAYERS 16
#define BUFFER_SIZE 1024

void
handle_client_data(int client_socket, game_manager_t game_manager, client_fd_list_t **client_fd_list)
{
    char buffer[BUFFER_SIZE] = {0};
    int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
    
    if (valread <= 0) {
        if (valread < 0) {
            log(ERROR, "Read failed from client %d: %s", client_socket, strerror(errno));
        }
        // Client disconnected
        log(INFO, "Client %d disconnected", client_socket);
        close(client_socket);
        *client_fd_list = remove_client_fd(*client_fd_list, client_socket);
        game_manager_remove_player(game_manager, client_socket);
        return;
    }

    buffer[valread] = '\0';

    // Ignore empty messages
    if (strcmp(buffer, "") == 0 || strcmp(buffer, "\n") == 0) {
        return;
    }
    
    log(INFO, "Received from client %d: %s", client_socket, buffer);
    
    // Echo back to client
    if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
        log(ERROR, "Send failed to client %d: %s", client_socket, strerror(errno));
        close(client_socket);
        log(INFO, "Client %d disconnected", client_socket);
        *client_fd_list = remove_client_fd(*client_fd_list, client_socket);
        game_manager_remove_player(game_manager, client_socket);
    }
}

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [port] [max_players]\n", program_name);
    fprintf(stderr, "  port: Port number to listen on (default: %s)\n", DEFAULT_PORT);
    fprintf(stderr, "  max_players: Maximum number of players (default: %d)\n", DEFAULT_MAX_PLAYERS);
    fprintf(stderr, "  Note: max_players must be between 6 and 16\n");
}

static void handle_new_connection(int client_socket, game_manager_t game_manager, 
                                client_fd_list_t **client_fd_list, int max_players) {
    if (game_manager_get_player_count(game_manager) >= max_players) {
        log(WARN, "Maximum players reached, rejecting connection");
        close(client_socket);
        return;
    }

    if (game_manager_get_phase(game_manager) != GAME_STATE_LOBBY) {
        log(INFO, "Game is not in lobby phase, rejecting connection");
        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "The game has already started, come back later!\n");
        send(client_socket, message, strlen(message), 0);
        close(client_socket);
        return;
    }

    if (game_manager_add_player(game_manager, client_socket) == 0) {
        *client_fd_list = add_client_fd(*client_fd_list, client_socket);
        log(INFO, "New client added to list, total clients: %d", 
            game_manager_get_player_count(game_manager));
        
        int player_count = game_manager_get_player_count(game_manager);
        if (is_valid_player_count(player_count)) {
            log(INFO, "Enough players to start game (%d players)", player_count);
            game_manager_start_game(game_manager);
            
            // Get player sockets and roles for notification
            int *player_sockets = game_manager_get_players_sockets(game_manager);
            game_role_t *player_roles = malloc(sizeof(game_role_t) * player_count);
            
            for (int i = 0; i < player_count; i++) {
                player_roles[i] = game_manager_get_player_role(game_manager, player_sockets[i]);
            }
            
            send_message_to_all_players(player_sockets, player_count, "The game has started!\n");
            send_message_role_assignment(player_sockets, player_count, player_roles, game_manager_get_werewolf_count(game_manager));
            
            free(player_sockets);
            free(player_roles);
        }
    } else {
        log(ERROR, "Failed to add new client");
        close(client_socket);
    }
}

static void setup_fd_sets(int server_socket, client_fd_list_t *client_fd_list, 
                         fd_set *read_fds, int *max_fd, game_manager_t game_manager) {
    FD_ZERO(read_fds);
    FD_SET(server_socket, read_fds);
    *max_fd = server_socket;

    client_fd_list_t *current = client_fd_list;
    while (current) {
        if (!is_socket_connected(current->fd)) {
            log(INFO, "Client %d disconnected", current->fd);
            close(current->fd);
            game_manager_remove_player(game_manager, current->fd);
            client_fd_list = remove_client_fd(client_fd_list, current->fd);
            current = client_fd_list;
            continue;
        }
        FD_SET(current->fd, read_fds);
        if (current->fd > *max_fd) {
            *max_fd = current->fd;
        }
        current = current->next;
    }
}

int main(int argc, char *argv[]) {
    const char *port = DEFAULT_PORT;
    int max_players = DEFAULT_MAX_PLAYERS;

    if (argc > 1) {
        port = argv[1];
    }
    if (argc > 2) {
        max_players = atoi(argv[2]);
        if (!is_valid_player_count(max_players)) {
            fprintf(stderr, "Error: Invalid max_players value. Must be between 6 and 16.\n");
            print_usage(argv[0]);
            return 1;
        }
    }

    game_manager_t game_manager = game_manager_create(max_players);
    if (!game_manager) {
        log(ERROR, "Failed to create game manager");
        return 1;
    }

    int server_socket = setup_tcp_server("0.0.0.0", port);
    if (server_socket < 0) {
        log(ERROR, "Failed to setup server");
        game_manager_destroy(game_manager);
        return 1;
    }

    log(INFO, "Server started successfully, waiting for connections...");
    log(INFO, "Maximum players: %d", max_players);

    client_fd_list_t *client_fd_list = NULL;
    fd_set read_fds;
    int max_fd = server_socket;

    while (1) {
        setup_fd_sets(server_socket, client_fd_list, &read_fds, &max_fd, game_manager);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            log(ERROR, "select() failed: %s", strerror(errno));
            continue;
        }

        // Check for new connection
        if (FD_ISSET(server_socket, &read_fds)) {
            int client_socket = accept_tcp_connection(server_socket);
            if (client_socket > 0) {
                handle_new_connection(client_socket, game_manager, &client_fd_list, max_players);
            }
        }

        // Check for data from existing clients
        client_fd_list_t *current = client_fd_list;
        while (current) {
            int client_socket = current->fd;
            client_fd_list_t *next = current->next;  // Save next before potential removal

            if (FD_ISSET(client_socket, &read_fds)) {
                handle_client_data(client_socket, game_manager, &client_fd_list);
            }
            current = next;
        }
    }

    // Cleanup
    while (client_fd_list) {
        client_fd_list_t *next = client_fd_list->next;
        close(client_fd_list->fd);
        free(client_fd_list);
        client_fd_list = next;
    }
    game_manager_destroy(game_manager);
    close(server_socket);
    return 0;
}

