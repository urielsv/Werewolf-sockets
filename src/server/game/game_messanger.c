#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "logger.h"
#include "game_messanger.h"

const char *
channel_name(message_channel_t channel)
{
    switch (channel) {
        case CHANNEL_ANNOUNCEMENT: return "ANNOUNCEMENT";
        case CHANNEL_CHAT: return "CHAT";
        case CHANNEL_WEREWOLF: return "WEREWOLF";
        case CHANNEL_DEAD: return "DEAD";
        case CHANNEL_WHISPER: return "WHISPER";
        default: return "UNKNOWN";
    }
}

const char *get_channel_color(message_channel_t channel)
{
    switch (channel) {
        case CHANNEL_ANNOUNCEMENT: return "\033[1;33m";  // Yellow
        case CHANNEL_WEREWOLF: return "\033[1;31m";      // Red
        case CHANNEL_DEAD: return "\033[1;30m";          // Gray
        case CHANNEL_WHISPER: return "\033[1;35m";       // Magenta
        case CHANNEL_CHAT: return "\033[1;32m";          // Green
        default: return "\033[0m";                       // Reset
    }
}

message_channel_t
parse_message_channel(const char *message)
{
    if (!message || message[0] != '[') {
        return CHANNEL_COUNT;  
    }

    const char *end_bracket = strchr(message, ']');
    if (!end_bracket) {
        return CHANNEL_COUNT;
    }

    char channel[32] = {0};
    strncpy(channel, message + 1, end_bracket - message - 1);

    if (strcmp(channel, "ANNOUNCEMENT") == 0) return CHANNEL_ANNOUNCEMENT;
    if (strcmp(channel, "CHAT") == 0) return CHANNEL_CHAT;
    if (strcmp(channel, "WEREWOLF") == 0) return CHANNEL_WEREWOLF;
    if (strcmp(channel, "DEAD") == 0) return CHANNEL_DEAD;
    if (strcmp(channel, "WHISPER") == 0) return CHANNEL_WHISPER;

    return CHANNEL_COUNT;
}

char *
format_message(message_channel_t channel, int player_number, const char *message) 
{
    static char formatted[BUFFER_SIZE];
    snprintf(formatted, BUFFER_SIZE, "[%s] Player %d: %s\n", 
             channel_name(channel), player_number, message);
    return formatted;
}


static char *
format_whisper_message(int from_id, int to_id, const char *message)
{
    static char formatted[BUFFER_SIZE];
    snprintf(formatted, BUFFER_SIZE, "[WHISPER] From Player %d to Player %d: %s\n",
             from_id, to_id, message);
    return formatted;
}

inline int
read_and_format_message(int socket_id, char *buffer, size_t buffer_size)
{
    if (socket_id < 0 || !buffer || buffer_size == 0) {
        log(ERROR, "Invalid parameters for read_and_format_message");
        return -1;
    }

    int valread = read(socket_id, buffer, buffer_size - 1);
    if (valread <= 0) {
        return valread;
    }

    buffer[valread] = '\0';
    message_channel_t channel = parse_message_channel(buffer);
    
    if (channel != CHANNEL_COUNT) {
        // Format the message with color
        char temp[BUFFER_SIZE];
        snprintf(temp, BUFFER_SIZE, "%s%s\033[0m", 
                get_channel_color(channel), buffer);
        strncpy(buffer, temp, buffer_size);
    }

    return valread;
}

int 
send_message(int socket_id, message_channel_t channel, const char *message, int player_number) 
{
    if (socket_id < 0 || !message) {
        log(ERROR, "Invalid parameters for send_message");
        return -1;
    }

    char *formatted = format_message(channel, player_number, message);
    int rv = send(socket_id, formatted, strlen(formatted), 0);
    if (rv < 0) {
        log(ERROR, "Failed to send message to socket %d", socket_id);
        return -1;
    }
    return rv;
}


int 
send_whisper(int from_socket_id, int to_socket_id, int from_player_number, int to_player_number, const char *message) 
{
    if (from_socket_id < 0 || to_socket_id < 0 || !message) {
        log(ERROR, "Invalid parameters for send_whisper");
        return -1;
    }

    char *formatted = format_whisper_message(from_player_number, to_player_number, message);
    log(INFO, "Sending whisper to socket %d: %s", to_socket_id, formatted);
    int rv = send(to_socket_id, formatted, strlen(formatted), 0);
    if (rv < 0) {
        log(ERROR, "Failed to send whisper to socket %d", to_socket_id);
        return -1;
    }

    char *confirmation = format_whisper_message(from_player_number, to_player_number, message);
    rv = send(from_socket_id, confirmation, strlen(confirmation), 0);
    if (rv < 0) {
        log(ERROR, "Failed to send whisper confirmation to socket %d", from_socket_id);
        return -1;
    }

    return rv;
}

int 
broadcast_message(message_channel_t channel, const char *message) 
{
    if (!message) {
        log(ERROR, "Invalid message for broadcast");
        return -1;
    }

    char *formatted = format_message(channel, 0, message); 
    int rv = send(0, formatted, strlen(formatted), 0); 
    if (rv < 0) {
        log(ERROR, "Failed to broadcast message");
        return -1;
    }
    return rv;
}

int 
forward_message(channel_subscription_t *subscription, const char *message) 
{
    if (!subscription || !message) {
        log(ERROR, "Invalid parameters for forward_message");
        return -1;
    }

    for (int i = 1; i < subscription->subscription_count; i++) {
        if (send(subscription->socket_ids[i], message, strlen(message), 0) < 0) {
            log(ERROR, "Failed to forward message to subscriber %d", 
            subscription->socket_ids[i]);
        }
    }
    return 0;
}

// Subscribe a socket to a channel
int 
subscribe_to_channel(channel_subscription_t *subscription, int socket_id) 
{
    if (!subscription || socket_id < 0) {
        log(ERROR, "Invalid parameters for subscribe_to_channel");
        return -1;
    }

    if (subscription->subscription_count >= MAX_SUBSCRIPTIONS) {
        log(ERROR, "Channel subscription limit reached");
        return -1;
    }

    if (is_subscribed(subscription, socket_id)) {
        log(WARN, "Socket %d already subscribed to channel", socket_id);
        return 0;
    }

    subscription->socket_ids[subscription->subscription_count++] = socket_id;
    return 0;
}

int 
unsubscribe_from_channel(channel_subscription_t *subscription, int socket_id) 
{
    if (!subscription || socket_id < 0) {
        log(ERROR, "Invalid parameters for unsubscribe_from_channel");
        return -1;
    }

    for (int i = 0; i < subscription->subscription_count; i++) {
        if (subscription->socket_ids[i] == socket_id) {
            subscription->socket_ids[i] = 
                subscription->socket_ids[--subscription->subscription_count];
            return 0;
        }
    }

    log(WARN, "Socket %d not found in channel subscription", socket_id);
    return -1;
}

bool 
is_subscribed(channel_subscription_t *subscription, int socket_id) 
{
    if (!subscription || socket_id < 0) {
        return false;
    }

    for (int i = 0; i < subscription->subscription_count; i++) {
        if (subscription->socket_ids[i] == socket_id) {
            return true;
        }
    }
    return false;
}