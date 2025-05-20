#ifndef __game_messanger_h__
#define __game_messanger_h__

#include <stdbool.h>
#include "game_manager.h"

#define BUFFER_SIZE 1024
#define MAX_SUBSCRIPTIONS 16

typedef enum {
    CHANNEL_ANNOUNCEMENT,    // Server announcements
    CHANNEL_CHAT,           // General chat
    CHANNEL_WEREWOLF,       // Werewolf team chat
    CHANNEL_DEAD,           // Dead players chat
    CHANNEL_WHISPER,        // Private messages
    CHANNEL_COUNT
} message_channel_t;

typedef struct {
    message_channel_t channel;
    int socket_ids[MAX_SUBSCRIPTIONS];
    int subscription_count;
} channel_subscription_t;

// Message formatting and parsing
const char *get_channel_color(message_channel_t channel);
message_channel_t parse_message_channel(const char *message);
char *format_message(message_channel_t channel, int player_number, const char *message);
int read_and_format_message(int socket_id, char *buffer, size_t buffer_size);

// Message sending functions
int send_message(int socket_id, message_channel_t channel, const char *message, int player_number);
int send_whisper(int from_socket_id, int to_socket_id, int from_player_number, int to_player_number, const char *message);
int broadcast_message(message_channel_t channel, const char *message);
int forward_message(channel_subscription_t *subscription, const char *message);

// Channel management
int subscribe_to_channel(channel_subscription_t *subscription, int socket_id);
int unsubscribe_from_channel(channel_subscription_t *subscription, int socket_id);
bool is_subscribed(channel_subscription_t *subscription, int socket_id);

// Message formatting
const char *channel_name(message_channel_t channel);

#endif // __game_messanger_h__ 