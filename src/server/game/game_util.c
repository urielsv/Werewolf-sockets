#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "logger.h"
#include "game_util.h"
#include "game_manager.h"
#include "defs.h"

const char *role_by_name(game_role_t role)
{
    static const char *ROLE_NAMES[] = {
        [ROLE_VILLAGER] = "Villager",
        [ROLE_WEREWOLF] = "Werewolf",
        [ROLE_SEER] = "Seer",
        [ROLE_DOCTOR] = "Doctor",
        [ROLE_BODYGUARD] = "Bodyguard"
    };

    if (role >= 0 && role < sizeof(ROLE_NAMES)/sizeof(ROLE_NAMES[0])) {
        return ROLE_NAMES[role];
    }
    
    return "Unknown";
}