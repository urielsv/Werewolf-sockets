#ifndef __game_manager_h__
#define __game_manager_h__

#include <stdbool.h>

// Game roles
typedef enum {
    ROLE_UNASSIGNED = 0,
    ROLE_VILLAGER,
    ROLE_WEREWOLF,
    ROLE_SEER,
    ROLE_DOCTOR,
    ROLE_BODYGUARD,
    GAME_ROLE_COUNT
} game_role_t;

// Game states
typedef enum {
    GAME_STATE_LOBBY = 0,
    GAME_STATE_NIGHT,
    GAME_STATE_DAY,
    GAME_STATE_VOTING,
    GAME_STATE_ENDED
} game_state_t;

typedef struct game_manager_cdt *game_manager_t;

game_manager_t game_manager_create(int max_players);
void game_manager_destroy(game_manager_t game_manager);

int game_manager_add_player(game_manager_t game_manager, int socket_id);
int game_manager_remove_player(game_manager_t game_manager, int socket_id);
int game_manager_get_player_count(game_manager_t game_manager);
int game_manager_get_alive_count(game_manager_t game_manager);

game_role_t game_manager_get_player_role(game_manager_t game_manager, int socket_id);
int game_manager_get_werewolf_count(game_manager_t game_manager);
bool game_manager_is_player_alive(game_manager_t game_manager, int socket_id);
bool game_manager_is_player_protected(game_manager_t game_manager, int socket_id);
bool game_manager_is_player_werewolf(game_manager_t game_manager, int socket_id);
int game_manager_get_werewolf_alive_count(game_manager_t game_manager);
int game_manager_start_game(game_manager_t game_manager);
int *game_manager_get_players_sockets(game_manager_t game_manager);
game_state_t game_manager_get_phase(game_manager_t game_manager);

int game_manager_get_player_number(game_manager_t game_manager, int socket_id);
int game_manager_get_socket_by_player_number(game_manager_t game_manager, int player_number);
#endif // __game_manager_h__
