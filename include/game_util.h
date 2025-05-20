#ifndef __game_util_h__
#define __game_util_h__

#include "game_manager.h"

#define BUFFER_SIZE 1024

void send_message_to_all_players(int *player_sockets, int player_count, const char *message);
void send_message_role_assignment(int *player_sockets, int player_count, game_role_t *player_roles);

char *role_by_name(game_role_t role);

#endif // __game_util_h__
