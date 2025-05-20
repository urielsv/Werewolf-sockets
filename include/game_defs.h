#ifndef __game_defs_h__
#define __game_defs_h__

typedef enum {
    MSG_TYPE_CHAT = 1,
    MSG_TYPE_GAME_STATE = 2,
    MSG_TYPE_ACTION = 3,
    MSG_TYPE_ERROR = 4
} message_type_t;

typedef struct {
    message_type_t type;
    char data[1024];
} message_t;

enum game_state {
    GAME_STATE_LOBBY,
    GAME_STATE_NIGHT,
    GAME_STATE_DAY,
    GAME_STATE_VOTING,
    GAME_STATE_ENDED
};

#endif // __game_defs_h__ 