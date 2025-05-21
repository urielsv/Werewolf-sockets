#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "logger.h"
#include "game_util.h"
#include "game_manager.h"
#include "defs.h"


char *
role_by_name(game_role_t role)
{
    switch (role) {
        case ROLE_VILLAGER:
            return "Villager";
        case ROLE_WEREWOLF:
            return "Werewolf";
        case ROLE_SEER:
            return "Seer";
        case ROLE_DOCTOR:
            return "Doctor";
        case ROLE_BODYGUARD:
            return "Bodyguard";
        default:
            return "Unknown";
    }
}