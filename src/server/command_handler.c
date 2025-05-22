#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_handler.h"
#include "defs.h"
#include "game_manager.h"
#include "game_messanger.h"
#include "logger.h"


// Function declarations
static void handle_whisper_command(const char *buffer, int client_socket,
                                 game_manager_t game_manager);
static void handle_werewolf_command(const char *buffer, int client_socket,
                                  game_manager_t game_manager);

int handle_if_command(const char *buffer, int client_socket, game_manager_t game_manager)
{
    if (!buffer || !game_manager) {
        log(ERROR, "Invalid parameters in handle_if_command");
        return RET_ERROR;
    }

    static const char *WHISPER_CMD = "/whisper ";
    static const char *WEREWOLF_CMD = "/ww ";

    if (strncmp(buffer, WHISPER_CMD, strlen(WHISPER_CMD)) == 0) {
        handle_whisper_command(buffer, client_socket, game_manager);
        return RET_SUCCESS;
    }

    if (strncmp(buffer, WEREWOLF_CMD, strlen(WEREWOLF_CMD)) == 0) {
        handle_werewolf_command(buffer, client_socket, game_manager);
        return RET_SUCCESS;
    }

    return RET_ERROR;
}

static void handle_whisper_command(const char *buffer, int client_socket,
                                 game_manager_t game_manager)
{
    if (!buffer || !game_manager) {
        log(ERROR, "Invalid parameters in handle_whisper_command");
        return;
    }

    char *command_copy = (char *)malloc(strlen(buffer) + 1);
    if (!command_copy) {
        log(ERROR, "Memory allocation failed in handle_whisper_command");
        return;
    }

    strcpy(command_copy, buffer);

    char *rest = command_copy + strlen("/whisper");
    char *player_id_str = strtok(rest, " ");
    char *message = strtok(NULL, "");

    if (player_id_str && message) {
        int target_player_number = atoi(player_id_str);
        int to_socketfd = game_manager_get_socket_by_player_number(game_manager,
                                                                 target_player_number);
        int from_player_number = game_manager_get_player_number(game_manager, client_socket);

        if (to_socketfd > 0 && client_socket > 0) {
            message_t whisper_msg = {
                .channel = CHANNEL_WHISPER,
                .content = message,
                .sender_id = from_player_number,
                .receiver_id = target_player_number
            };
            send2client(&whisper_msg, to_socketfd);
        } else {
            log(ERROR, "Invalid socket or player numbers in whisper command");
        }
    } else {
        log(ERROR, "Invalid whisper command format");
    }

    free(command_copy);
}

static void handle_werewolf_command(const char *buffer, int client_socket,
                                  game_manager_t game_manager)
{
    // TODO: Implement werewolf command handling
    log(INFO, "Werewolf command not yet implemented");
}
