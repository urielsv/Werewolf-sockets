#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>

#include "tcp_server_util.h"
#include "logger.h"
#include "game_manager.h"
#include "game_messanger.h"
#include "game_util.h"
#include "command_handler.h"

#define DEFAULT_PORT "8080"
#define DEFAULT_MAX_PLAYERS 16
#define BUFFER_SIZE 1024

static channel_subscription_t chat_channel = {CHANNEL_CHAT, {0}, 0};
static channel_subscription_t werewolf_channel = {CHANNEL_WEREWOLF, {0}, 0};
static channel_subscription_t announcement_channel = {CHANNEL_ANNOUNCEMENT, {0}, 0};
static channel_subscription_t server_channel = {CHANNEL_SERVER, {0}, 0};

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

        unsubscribe_from_channel(&chat_channel, client_socket);
        unsubscribe_from_channel(&werewolf_channel, client_socket);
        unsubscribe_from_channel(&announcement_channel, client_socket);
        unsubscribe_from_channel(&server_channel, client_socket);
        return;
    }

    buffer[valread] = '\0';

    // Ignore empty messages
    char *trimmed = buffer;
   while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r') trimmed++;
   if (*trimmed == '\0') {
       return;
   }

    int sender_number = game_manager_get_player_number(game_manager, client_socket);
    log(INFO, "Received from client %d (player %d): %s", client_socket, sender_number, buffer);

    if (handle_if_command(buffer, client_socket, game_manager) > 0) {
        return;
    }

    if (sender_number <= 0) {
        log(ERROR, "Invalid player number for client %d", client_socket);
        return;
    }

    message_t msg = {
        .channel = CHANNEL_CHAT,
        .sender_id = sender_number,
        .receiver_id = -1,  // broadcast
        .content = buffer,
    };

    switch (game_manager_get_phase(game_manager)) {
        case GAME_STATE_DAY:
            forward_message(&chat_channel, format_message(&msg));
            break;

        case GAME_STATE_NIGHT:
            if (game_manager_is_player_werewolf(game_manager, client_socket)) {
                msg.channel = CHANNEL_WEREWOLF;
                send2client(&msg, client_socket);
            } else {
                message_t error_msg = {
                    .channel = CHANNEL_SERVER,
                    .content = "It is night time, you are not allowed to talk!",
                };
                send2client(&error_msg, client_socket);
            }
            break;
        // TODO: Add voting and ended phases
        case GAME_STATE_VOTING:
        case GAME_STATE_ENDED:
        default:
            break;
    }

}

static void
print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [port] [max_players]\n", program_name);
    fprintf(stderr, "  port: Port number to listen on (default: %s)\n", DEFAULT_PORT);
    fprintf(stderr, "  max_players: Maximum number of players (default: %d)\n", DEFAULT_MAX_PLAYERS);
    fprintf(stderr, "  Note: max_players must be between 6 and 16\n");
}

static void
handle_new_connection(int client_socket, game_manager_t game_manager,
                       client_fd_list_t **client_fd_list, int max_players)
{
    if (game_manager_get_player_count(game_manager) > max_players) {
        log(WARN, "Maximum players reached, rejecting connection");
        close(client_socket);
        return;
    }

    if (game_manager_get_phase(game_manager) != GAME_STATE_LOBBY) {
        log(INFO, "Game is not in lobby phase, rejecting connection");
        message_t msg = {
            .content = "The game has already started, come back later!",
            .channel = CHANNEL_SERVER
        };
        send2client(&msg, client_socket);
        unsubscribe_from_channel(&announcement_channel, client_socket);
        unsubscribe_from_channel(&server_channel, client_socket);
        unsubscribe_from_channel(&chat_channel, client_socket);
        unsubscribe_from_channel(&werewolf_channel, client_socket);
        close(client_socket);
        return;
    }

    if (game_manager_add_player(game_manager, client_socket) == 0) {
        *client_fd_list = add_client_fd(*client_fd_list, client_socket);
        log(INFO, "New client added to list, total clients: %d",
            game_manager_get_player_count(game_manager));

        subscribe_to_channel(&chat_channel, client_socket);
        subscribe_to_channel(&announcement_channel, client_socket);
        subscribe_to_channel(&server_channel, client_socket);
        int player_count = game_manager_get_player_count(game_manager);
        if (player_count == max_players) {
            log(INFO, "Enough players to start game (%d players), starting game...", player_count);
            game_manager_start_game(game_manager);

            // Get player sockets and roles for notification
            int *player_sockets = game_manager_get_players_sockets(game_manager);
            game_role_t *player_roles = malloc(sizeof(game_role_t) * player_count);

            for (int i = 0; i < player_count; i++) {
                player_roles[i] = game_manager_get_player_role(game_manager, player_sockets[i]);
                if (player_roles[i] == ROLE_WEREWOLF) {
                    subscribe_to_channel(&werewolf_channel, player_sockets[i]);
                }
            }

            forward_message(&announcement_channel, "The game has started! It is now night time.");
            int werewolf_count = game_manager_get_werewolf_count(game_manager);
            for (int i = 0; i < player_count; i++) {
                char role_message[BUFFER_SIZE];
                const char *role_name = role_by_name(player_roles[i]);
                snprintf(role_message, BUFFER_SIZE, "You are a %s!", role_name);
                message_t role_msg = {
                    .channel = CHANNEL_SERVER,
                    .content = role_message,
                };
                send2client(&role_msg, player_sockets[i]);

                if (player_roles[i] == ROLE_WEREWOLF && werewolf_count-1 > 0) {
                    char team_message[BUFFER_SIZE] = "Your werewolf teammates are: ";
                    for (int j = 0; j < player_count; j++) {
                        if (i != j && player_roles[j] == ROLE_WEREWOLF) {
                            char temp[32];
                            snprintf(temp, sizeof(temp), "Player %d, ", player_sockets[j]);
                            strncat(team_message, temp, BUFFER_SIZE - strlen(team_message) - 1);
                        }
                    }
                    strncat(team_message, "\n", BUFFER_SIZE - strlen(team_message) - 1);
                    message_t team_msg = {
                        .channel = CHANNEL_ANNOUNCEMENT,
                        .content = team_message,
                    };
                    send2client(&team_msg, player_sockets[i]);
                }
            }

            free(player_sockets);
            free(player_roles);
        }
    } else {
        log(ERROR, "Failed to add new client");
        close(client_socket);
    }
}

static void
setup_fd_sets(int server_socket, client_fd_list_t *client_fd_list,
                         fd_set *read_fds, int *max_fd, game_manager_t game_manager)
{
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

int
main(int argc, const char *argv[])
{
    close(STDIN_FILENO);
    const char *port = DEFAULT_PORT;
    int max_players = DEFAULT_MAX_PLAYERS;
    if (argc > 1) {
        port = argv[1];
    }

    if (argc > 2) {
        max_players = atoi(argv[2]);
        if (max_players < 6 || max_players > 16) {
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

    int server_socket = setup_tcp_server("0.0.0.0", port, max_players);
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
        // TODO: Check if this is a correct way to do this
        log(INFO, "Werewolf alive count: %d", game_manager_get_werewolf_alive_count(game_manager));
        log(INFO, "Phase: %d", game_manager_get_phase(game_manager));
        if (game_manager_get_werewolf_alive_count(game_manager) <= 0 && game_manager_get_phase(game_manager) != GAME_STATE_LOBBY) {
            log(INFO, "No werewolves left, ending game...");
            char msg[BUFFER_SIZE] = "No werewolves left, ending game...";
            forward_message(&announcement_channel, msg);
            break;
        }

        setup_fd_sets(server_socket, client_fd_list, &read_fds, &max_fd, game_manager);

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            log(ERROR, "select() failed: %s", strerror(errno));
            continue;
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            int client_socket = accept_tcp_connection(server_socket);
            if (client_socket > 0) {
                handle_new_connection(client_socket, game_manager, &client_fd_list, max_players);
            }
        }



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
