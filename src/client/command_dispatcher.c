#include "command_dispatcher.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "logger.h"

// TODO: DRY!!
#define BUFFER_SIZE 1024


static command_t commands[] = {
    [0] = {
        .aliases = {"help", NULL},
        .usage = "/help",
        .description = "Show this help message",
        .function = help_command,
        .arg1 = NULL,
        .arg2 = NULL
    },
    [1] = {
        .aliases = {"whisper", "w", "pm", "msg", NULL},
        .usage = "/whisper <player_id> <message>",
        .description = "Send a private message to a player",
        .function = whisper_command,
        .arg1 = NULL,
        .arg2 = NULL
    },
    [2] = {
        .aliases = {"ww", "werewolf", NULL},
        .usage = "/ww <player_id>",
        .description = "Vote for a player to be a werewolf",
        .function = ww_command,
        .arg1 = NULL,
        .arg2 = NULL
    }
};

void
dispatch_command(const char *command, int sockfd, void *arg1, void *arg2)
{
    if (!command) {
        printf("Usage: /help\n");
        return;
    }

    int command_count = sizeof(commands) / sizeof(commands[0]);
    for (int i = 0; i < command_count; i++) {
        for (int j = 0; j < COMMAND_ALIAS_MAX && commands[i].aliases[j] != NULL; j++) {
            if (strcmp(command, commands[i].aliases[j]) == 0) {
                commands[i].function(sockfd, arg1, arg2);
                return;
            }
        }
    }
    printf("Unknown command: %s\n", command);
}

void
whisper_command(int sockfd, void *player_id_str, void *message)
{
    if (!player_id_str || !message) {
        printf("Usage: %s\n", commands[1].usage);
        return;
    }

    int player_id = atoi(player_id_str);
    if (player_id <= 0) {
        printf("Invalid player ID\n");
        return;
    }

    char whisper_cmd[BUFFER_SIZE];
    snprintf(whisper_cmd, BUFFER_SIZE, "/whisper %d %s", player_id, (char *)message);
    if (send(sockfd, whisper_cmd, strlen(whisper_cmd), 0) < 0) {
        log(ERROR, "Failed to send whisper message");
    }

}

void
help_command(int sockfd, void *arg1, void *arg2)
{
    printf("Available commands:\n");
    int command_count = sizeof(commands) / sizeof(commands[0]);
    for (int i = 0; i < command_count; i++) {
        printf("  %s - %s\n", commands[i].usage, commands[i].description);
        printf("    Aliases: ");
        for (int j = 0; j < COMMAND_ALIAS_MAX & commands[i].aliases[j] != NULL; j++) {
            printf("%s%s", j > 0 ? ", " : "", commands[i].aliases[j]);
        }
        printf("\n");
    }
}

void
ww_command(int sockfd, void *arg1, void *arg2)
{
    // TODO: Implement ww chat
}
