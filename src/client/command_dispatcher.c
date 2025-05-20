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
        .name = "help",
        .function = help_command,
        .arg1 = NULL,
        .arg2 = NULL
    },
    [1] = {
        .name = "whisper",
        .function = whisper_command,
        .arg1 = NULL,
        .arg2 = NULL
    },
    [2] = {
        .name = "ww",
        .function = NULL,//ww_command,
        .arg1 = NULL,
        .arg2 = NULL
    }
};

void dispatch_command(const char *command, int sockfd, void *arg1, void *arg2) {
    int command_count = sizeof(commands) / sizeof(commands[0]);
    for (int i = 0; i < command_count; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            commands[i].function(sockfd, arg1, arg2);
            return;
        }
    }
    printf("Unknown command: %s\n", command);
}

void whisper_command(int sockfd, void *player_id_str, void *message) {
    
    if (!player_id_str || !message) {
        printf("Usage: /whisper <player_id> <message>\n");
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

void help_command(int sockfd, void *arg1, void *arg2) {
    printf("Available commands:\n");
    printf("  /whisper <player_id> <message> - Send private message\n");
    printf("  /help - Show this help message\n");
}
