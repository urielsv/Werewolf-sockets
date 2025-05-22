#ifndef GAME_MESSANGER_H
#define GAME_MESSANGER_H

#include <stdbool.h>
#include <stdio.h>
#include "defs.h"

#define CHANNEL_MAX_LEN 16
typedef enum {
    CHANNEL_ANNOUNCEMENT,    // Server announcements
    CHANNEL_CHAT,           // General chat
    CHANNEL_WEREWOLF,       // Werewolf team chat
    CHANNEL_WHISPER,        // Private messages
    CHANNEL_SERVER,         // Server messages
    CHANNEL_COUNT
} message_channel_t;

typedef struct {
    int subscription_count;
    int socket_fds[MAX_SUBSCRIPTIONS];
    message_channel_t channel;
} channel_subscription_t;

typedef struct message {
    message_channel_t channel;
    int sender_id;        // -1 for system messages
    int receiver_id;      // -1 for broadcast messages
    const char *content;
} message_t;

// Message formatting and parsing
const char *channel_name(message_channel_t channel);
const char *get_channel_color(message_channel_t channel);
message_channel_t parse_message_channel(const char *message);
char *format_message(const message_t *msg);
int read_and_format_message(int socket_id, char *buffer, size_t buffer_size);

// Message sending functions
int send2client(const message_t *msg, int socket_id);
int forward_message(channel_subscription_t *subscription, const char *message);

// Channel management
int subscribe_to_channel(channel_subscription_t *subscription, int socket_id);
int unsubscribe_from_channel(channel_subscription_t *subscription, int socket_id);
bool is_subscribed(channel_subscription_t *subscription, int socket_id);

#endif // GAME_MESSANGER_H
