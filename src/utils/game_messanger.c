#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "logger.h"
#include "game_messanger.h"
#include "defs.h"

const char *
channel_name(message_channel_t channel)
{
    switch (channel) {
        case CHANNEL_ANNOUNCEMENT: return "ANNOUNCEMENT";
        case CHANNEL_CHAT: return "CHAT";
        case CHANNEL_WEREWOLF: return "WEREWOLF";
        case CHANNEL_WHISPER: return "WHISPER";
        case CHANNEL_SERVER: return "SERVER";
        default: return "?";
    }
}

const char *get_channel_color(message_channel_t channel)
{
    switch (channel) {
        case CHANNEL_ANNOUNCEMENT: return "\033[1;33m";  // Yellow
        case CHANNEL_WEREWOLF: return "\033[1;31m";      // Red
        case CHANNEL_SERVER: return "\033[1;33m";          // Yellow
        case CHANNEL_WHISPER: return "\033[1;35m";       // Magenta
        case CHANNEL_CHAT: return "\033[1;32m";          // Green
        default: return "\033[0m";                       // Reset
    }
}

message_channel_t
parse_message_channel(const char *message)
{
    if (!message || message[0] != '[') {
        return RET_ERROR;
    }

    const char *end_bracket = strchr(message, ']');
    if (!end_bracket) {
        return RET_ERROR;
    }

    char channel[CHANNEL_MAX_LEN] = {0};
    strncpy(channel, message + 1, end_bracket - message - 1);

    if (strcmp(channel, "ANNOUNCEMENT") == 0) return CHANNEL_ANNOUNCEMENT;
    if (strcmp(channel, "CHAT") == 0) return CHANNEL_CHAT;
    if (strcmp(channel, "WEREWOLF") == 0) return CHANNEL_WEREWOLF;
    if (strcmp(channel, "SERVER") == 0) return CHANNEL_SERVER;
    if (strcmp(channel, "WHISPER") == 0) return CHANNEL_WHISPER;

    return CHANNEL_COUNT;
}

char *
format_message(const message_t *msg)
{
    static char formatted[BUFFER_SIZE] = {0};
    const char *color = get_channel_color(msg->channel);

    if (msg->channel == CHANNEL_SERVER) {
        snprintf(formatted, BUFFER_SIZE, "%s[!] %s\033[0m\n",
                color, msg->content);
        return formatted;
    }

    if (msg->receiver_id != -1) {  // Whisper/Direct message
        snprintf(formatted, BUFFER_SIZE, "%s[%s] From Player %d to Player %d: %s\033[0m\n",
                color, channel_name(msg->channel),
                msg->sender_id,
                msg->receiver_id,
                msg->content);
    }
    else {  // Normal player message
        snprintf(formatted, BUFFER_SIZE, "%s[%s] Player %d: %s\033[0m\n",
                color, channel_name(msg->channel), msg->sender_id, msg->content);
    }

    return formatted;
}

int
send2client(const message_t *msg, int socket_fd)
{
    if (socket_fd < 0 || !msg || !msg->content) {
        log(ERROR, "Invalid parameters for send_game_message");
        return -1;
    }

    char *formatted = format_message(msg);
    int rv = send(socket_fd, formatted, strlen(formatted), 0);
    if (rv < 0) {
        log(ERROR, "Failed to send message to socket %d", socket_fd);
        return -1;
    }
    return rv;
}

int
read_and_format_message(int socket_fd, char *buffer, size_t buffer_size)
{
    if (socket_fd < 0 || !buffer || buffer_size == 0) {
        log(ERROR, "Invalid parameters for read_and_format_message");
        return -1;
    }

    int valread = read(socket_fd, buffer, buffer_size - 1);
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
forward_message(channel_subscription_t *subscription, const char *message)
{
    if (!subscription || !message) {
        log(ERROR, "Invalid parameters for forward_message");
        return RET_ERROR;
    }

    char formatted[BUFFER_SIZE] = {0};
    snprintf(formatted, BUFFER_SIZE, "%s[%s] %s", get_channel_color(subscription->channel), channel_name(subscription->channel), message);
    for (int i = 1; i < subscription->subscription_count; i++) {
        if (send(subscription->socket_fds[i], formatted, strlen(formatted), 0) < 0) {
            log(ERROR, "Failed to forward message to subscriber %d",
            subscription->socket_fds[i]);
        }
    }
    return RET_SUCCESS;
}

int
subscribe_to_channel(channel_subscription_t *subscription, int socket_fd)
{
    if (!subscription || socket_fd < 0) {
        log(ERROR, "Invalid parameters for subscribe_to_channel");
        return RET_ERROR;
    }

    if (subscription->subscription_count >= MAX_SUBSCRIPTIONS) {
        log(ERROR, "Channel subscription limit reached");
        return RET_ERROR;
    }

    if (is_subscribed(subscription, socket_fd)) {
        log(WARN, "Socket %d already subscribed to channel", socket_fd);
        return RET_SUCCESS;
    }

    subscription->socket_fds[subscription->subscription_count++] = socket_fd;
    return RET_SUCCESS;
}

int
unsubscribe_from_channel(channel_subscription_t *subscription, int socket_fd)
{
    if (!subscription || socket_fd < 0) {
        log(ERROR, "Invalid parameters for unsubscribe_from_channel");
        return -1;
    }

    for (int i = 0; i < subscription->subscription_count; i++) {
        if (subscription->socket_fds[i] == socket_fd) {
            subscription->socket_fds[i] =
                subscription->socket_fds[--subscription->subscription_count];
            return 0;
        }
    }

    log(WARN, "Socket %d not found in channel subscription", socket_fd);
    return -1;
}

bool
is_subscribed(channel_subscription_t *subscription, int socket_fd)
{
    if (!subscription || socket_fd < 0) {
        return false;
    }

    for (int i = 0; i < subscription->subscription_count; i++) {
        if (subscription->socket_fds[i] == socket_fd) {
            return true;
        }
    }
    return false;
}
