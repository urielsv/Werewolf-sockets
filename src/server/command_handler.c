#include "command_handler.h"
#include "defs.h"
#include "logger.h"
#include "game_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "game_messanger.h"
#include "defs.h"


static void handle_whisper_command(char *buffer, int client_socket, game_manager_t game_manager);

int handle_if_command(char *buffer, int client_socket, game_manager_t game_manager) {
    // TODO: Check if there is a more elegant way to do this
    if (strncmp(buffer, "/whisper ", strlen("/whisper")) == 0) {
        handle_whisper_command(buffer, client_socket, game_manager);
        return RET_SUCCESS;
    }
    if (strncmp(buffer, "/ww ", strlen("/ww")) == 0) {
        // handle_ww_command(buffer, client_socket, game_manager);
        return RET_SUCCESS;
    }
    // return RET_ERROR;
    return RET_ERROR;
}

static void handle_whisper_command(char *buffer, int client_socket, game_manager_t game_manager) {
    char *rest = buffer + strlen("/whisper");
    char *player_id_str = strtok(rest, " ");
    char *message = strtok(NULL, "");
    if (player_id_str && message) {
        int target_player_number = atoi(player_id_str);
        int to_socketfd = game_manager_get_socket_by_player_number(game_manager, target_player_number);
        int from_player_number = game_manager_get_player_number(game_manager, client_socket);
        if (to_socketfd > 0 && client_socket > 0) {
            send_whisper(client_socket, to_socketfd, from_player_number, target_player_number, message);
        }
    }
}