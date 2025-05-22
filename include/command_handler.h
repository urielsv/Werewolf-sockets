#ifndef __command_handler_h__
#define __command_handler_h__
#include "game_manager.h"

int handle_if_command(const char *buffer, int client_socket, game_manager_t game_manager);

#endif
