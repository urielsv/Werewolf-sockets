#ifndef __command_dispatcher_h__
#define __command_dispatcher_h__

typedef struct {
    const char *name;
    void (*function)(int sockfd, void *arg1, void *arg2);
    void *arg1;
    void *arg2;
} command_t;

void dispatch_command(const char *command, int sockfd, void *arg1, void *arg2);
void help_command(int sockfd, void *arg1, void *arg2);
void whisper_command(int sockfd, void *arg1, void *arg2);
void ww_command(int sockfd, void *arg1, void *arg2);

#endif