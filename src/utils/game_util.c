#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "logger.h"
#include "game_util.h"
#include "game_manager.h"
#include "defs.h"

void
send_message_to_all_players(int *player_sockets, int player_count, const char *message)
{
    if (!player_sockets || !message) {
        log(ERROR, "Invalid parameters for send_message_to_all_players");
        return;
    }

    for (int i = 0; i < player_count; i++) {
        if (send(player_sockets[i], message, strlen(message), 0) < 0) {
            log(ERROR, "Failed to send message to player %d", player_sockets[i]);
        }
    }
}

void
send_message_role_assignment(int *player_sockets, int player_count, 
                           game_role_t *player_roles)
{
    if (!player_sockets || !player_roles) {
        log(ERROR, "Invalid parameters for send_message_role_assignment");
        return;
    }

    char message[BUFFER_SIZE];
    for (int i = 0; i < player_count; i++) {
        const char *role_name;
        role_name = role_by_name(player_roles[i]);
        snprintf(message, BUFFER_SIZE, "You are a %s!", role_name);
        if (send(player_sockets[i], message, strlen(message), 0) < 0) {
            log(ERROR, "Failed to send role to player %d", player_sockets[i]);
        }

        // If player is a werewolf, send list of other werewolves
        if (player_roles[i] == ROLE_WEREWOLF) {
            snprintf(message, BUFFER_SIZE, "Your werewolf teammates are: ");
            for (int j = 0; j < player_count; j++) {
                if (i != j && player_roles[j] == ROLE_WEREWOLF) {
                    char temp[32];
                    snprintf(temp, sizeof(temp), "Player %d, ", player_sockets[j]);
                    strncat(message, temp, BUFFER_SIZE - strlen(message) - 1);
                }
            }
            if (send(player_sockets[i], message, strlen(message), 0) < 0) {
                log(ERROR, "Failed to send werewolf team info to player %d", player_sockets[i]);
            }
        }
    }
}

char *
role_by_name(game_role_t role)
{
    switch (role) {
        case ROLE_VILLAGER:
            return "Villager";
        case ROLE_WEREWOLF:
            return "Werewolf";
        case ROLE_SEER:
            return "Seer";
        case ROLE_DOCTOR:
            return "Doctor";
        case ROLE_BODYGUARD:
            return "Bodyguard";
        default:
            return "Unknown";
    }
}