#ifndef __game_config_h__
#define __game_config_h__

#include <stdbool.h>
#include "game_manager.h"
#include "logger.h"

typedef struct {
    int min_players;
    int max_players;
    int num_werewolves;
    int num_villagers;
    int num_special_roles;
    game_role_t special_roles[GAME_ROLE_COUNT];
} game_config_t;

static const game_config_t GAME_CONFIGS[] = {
    // 6 players
    {
        .min_players = 6,
        .max_players = 6,
        .num_werewolves = 1,
        .num_villagers = 5,
        .num_special_roles = 0,
        .special_roles = {ROLE_UNASSIGNED}  // No special roles
    },
    // 7-8 players
    {
        .min_players = 7,
        .max_players = 8,
        .num_werewolves = 1,
        .num_villagers = 5,
        .num_special_roles = 1,
        .special_roles = {ROLE_SEER, ROLE_UNASSIGNED}  // Add Seer
    },
    // 9-11 players
    {
        .min_players = 9,
        .max_players = 11,
        .num_werewolves = 2,
        .num_villagers = 5,
        .num_special_roles = 2,
        .special_roles = {ROLE_SEER, ROLE_DOCTOR, ROLE_UNASSIGNED}  // Add Seer and Doctor
    },
    // 12+ players
    {
        .min_players = 12,
        .max_players = 16,
        .num_werewolves = 3,
        .num_villagers = 6,
        .num_special_roles = 3,
        .special_roles = {ROLE_SEER, ROLE_DOCTOR, ROLE_BODYGUARD, ROLE_UNASSIGNED}  // Add Seer, Doctor, and Bodyguard
    }
};

#define NUM_CONFIGS (sizeof(GAME_CONFIGS) / sizeof(GAME_CONFIGS[0]))

static inline const game_config_t*
get_game_config(int player_count)
{
    for (size_t i = 0; i < NUM_CONFIGS; i++) {
        if (player_count >= GAME_CONFIGS[i].min_players &&
            player_count <= GAME_CONFIGS[i].max_players) {
            log(INFO, "Game config found: %ld", i);
            return &GAME_CONFIGS[i];
        }
    }
    return NULL;
}


#endif // __game_config_h__
