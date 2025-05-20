#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "game_manager.h"
#include "game_config.h"
typedef struct player_t {
    int socket_id;
    game_role_t role;
    bool is_alive;
    bool is_protected;  
    bool has_used_ability;  
} player_t;

typedef struct role_state_t {
    int total_count;
    int alive_count;
    int protected_count;  
} role_state_t;

typedef struct game_state_data_t {
    bool is_night;
    bool is_game_started;
    game_state_t current_phase;  
    int day_count;
    int night_count;
} game_state_data_t;

typedef struct game_manager_cdt {
    player_t *players;
    int max_players;
    int player_count;
    int alive_count;
    role_state_t roles[GAME_ROLE_COUNT];
    game_state_data_t state;
    int *votes;  // Array of socket_ids for votes
    int vote_count;
} game_manager_cdt;

static void
shuffle_array(int *array, int size)
{
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

game_manager_t
game_manager_create(int max_players)
{
    game_manager_t game_manager = malloc(sizeof(game_manager_cdt));
    if (!game_manager) {
        log(ERROR, "Failed to allocate memory for game manager");
        return NULL;
    }

    
    game_manager->players = malloc(sizeof(player_t) * max_players);
    if (!game_manager->players) {
        log(ERROR, "Failed to allocate memory for players");
        free(game_manager);
        return NULL;
    }

    
    game_manager->votes = malloc(sizeof(int) * max_players);
    if (!game_manager->votes) {
        log(ERROR, "Failed to allocate memory for votes");
        free(game_manager->players);
        free(game_manager);
        return NULL;
    }

    
    game_manager->max_players = max_players;
    game_manager->player_count = 0;
    game_manager->alive_count = 0;
    game_manager->vote_count = 0;

    memset(game_manager->roles, 0, sizeof(game_manager->roles));

    game_manager->state.is_night = false;
    game_manager->state.is_game_started = false;
    game_manager->state.current_phase = GAME_STATE_LOBBY;
    game_manager->state.day_count = 0;
    game_manager->state.night_count = 0;

    srand(time(NULL));

    return game_manager;
}

void
game_manager_destroy(game_manager_t game_manager)
{
    if (game_manager) {
        free(game_manager->votes);
        free(game_manager->players);
        free(game_manager);
    }
}

int
game_manager_add_player(game_manager_t game_manager, int socket_id)
{
    if (!game_manager || socket_id < 0) {
        return -1;
    }

    for (int i = 0; i < game_manager->player_count; i++) {
        if (game_manager->players[i].socket_id == socket_id) {
            log(WARN, "Player %d already exists", socket_id);
            return -1;
        }
    }

    if (game_manager->player_count >= game_manager->max_players) {
        log(WARN, "Maximum players reached");
        return -1;
    }

    player_t *player = &game_manager->players[game_manager->player_count];
    player->socket_id = socket_id;
    player->is_alive = true;
    player->is_protected = false;
    player->has_used_ability = false;
    player->role = ROLE_UNASSIGNED;  // Role will be assigned later

    game_manager->player_count++;
    game_manager->alive_count++;

    return 0;
}

int
game_manager_remove_player(game_manager_t game_manager, int socket_id)
{
    if (!game_manager || socket_id < 0) {
        return -1;
    }

    int player_index = -1;
    for (int i = 0; i < game_manager->player_count; i++) {
        if (game_manager->players[i].socket_id == socket_id) {
            player_index = i;
            break;
        }
    }

    if (player_index == -1) {
        log(WARN, "Player %d not found", socket_id);
        return -1;
    }

    if (game_manager->players[player_index].is_alive) {
        game_manager->alive_count--;
        game_role_t role = game_manager->players[player_index].role;
        game_manager->roles[role].alive_count--;
        
        if (game_manager->players[player_index].is_protected) {
            game_manager->roles[role].protected_count--;
        }
    }

    if (player_index < game_manager->player_count - 1) {
        game_manager->players[player_index] = game_manager->players[game_manager->player_count - 1];
    }
    game_manager->player_count--;

    return 0;
}

int
game_manager_get_player_count(game_manager_t game_manager)
{
    return game_manager ? game_manager->player_count : 0;
}

int
game_manager_get_alive_count(game_manager_t game_manager)
{
    return game_manager ? game_manager->alive_count : 0;
}

game_role_t
game_manager_get_player_role(game_manager_t game_manager, int socket_id)
{
    if (!game_manager || socket_id < 0) {
        return ROLE_UNASSIGNED;
    }

    for (int i = 0; i < game_manager->player_count; i++) {
        if (game_manager->players[i].socket_id == socket_id) {
            return game_manager->players[i].role;
        }
    }

    return ROLE_UNASSIGNED;
}

bool
game_manager_is_player_alive(game_manager_t game_manager, int socket_id)
{
    if (!game_manager || socket_id < 0) {
        return false;
    }

    for (int i = 0; i < game_manager->player_count; i++) {
        if (game_manager->players[i].socket_id == socket_id) {
            return game_manager->players[i].is_alive;
        }
    }

    return false;
}

void
game_manager_start_game(game_manager_t game_manager)
{
    if (!game_manager || game_manager->state.is_game_started) {
        return;
    }

    const game_config_t *config = get_game_config(game_manager->player_count);
    if (!config) {
        log(ERROR, "Invalid player count for game start");
        return;
    }

    int *player_indices = malloc(sizeof(int) * game_manager->player_count);
    if (!player_indices) {
        log(ERROR, "Failed to allocate memory for role assignment");
        return;
    }
    for (int i = 0; i < game_manager->player_count; i++) {
        player_indices[i] = i;
    }

    shuffle_array(player_indices, game_manager->player_count);

    for (int i = 0; i < config->num_werewolves; i++) {
        game_manager->players[player_indices[i]].role = ROLE_WEREWOLF;
        game_manager->roles[ROLE_WEREWOLF].total_count++;
        game_manager->roles[ROLE_WEREWOLF].alive_count++;
    }

    for (int i = config->num_werewolves; i < config->num_werewolves + config->num_villagers; i++) {
        game_manager->players[player_indices[i]].role = ROLE_VILLAGER;
        game_manager->roles[ROLE_VILLAGER].total_count++;
        game_manager->roles[ROLE_VILLAGER].alive_count++;
    }

    int special_role_index = config->num_werewolves + config->num_villagers;
    for (int i = 0; i < config->num_special_roles; i++) {
        if (special_role_index < game_manager->player_count) {
            game_role_t role = config->special_roles[i];
            game_manager->players[player_indices[special_role_index]].role = role;
            game_manager->roles[role].total_count++;
            game_manager->roles[role].alive_count++;
            special_role_index++;
        }
    }

    for (int i = special_role_index; i < game_manager->player_count; i++) {
        game_manager->players[player_indices[i]].role = ROLE_VILLAGER;
        game_manager->roles[ROLE_VILLAGER].total_count++;
        game_manager->roles[ROLE_VILLAGER].alive_count++;
    }

    free(player_indices);

    game_manager->state.is_game_started = true;
    game_manager->state.current_phase = GAME_STATE_NIGHT;  
    game_manager->state.is_night = true;
    game_manager->state.night_count++;
    game_manager->state.day_count = 0;

    log(INFO, "Game started with %d players", game_manager->player_count);
}
