#ifndef __game_defs_h__
#define __game_defs_h__

// Common message types
typedef enum {
    MSG_TYPE_CHAT = 1,
    MSG_TYPE_GAME_STATE = 2,
    MSG_TYPE_ACTION = 3,
    MSG_TYPE_ERROR = 4
} message_type_t;

// Common message structure
typedef struct {
    message_type_t type;
    char data[1024];
} message_t;

#endif // __game_defs_h__ 