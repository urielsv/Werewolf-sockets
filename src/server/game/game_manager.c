#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "game_manager.h"
#include "game_config.h"

typedef struct player_t {
    int socket_id;
    int player_number;
    game_role_t role;
    bool is_alive;
    bool is_protected;  
    bool has_used_ability;
    struct player_t *next;  // For player list
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
    player_t *players;  // Linked list of players
    int max_players;
    int player_count;
    int alive_count;
    role_state_t roles[GAME_ROLE_COUNT];
    game_state_data_t state;
    int *votes;  // Array of socket_ids for votes
    int vote_count;
} game_manager_cdt;

// Add these validation macros near the top with the other macros:

#define VALIDATE_SOCKET_ID(socket_id) \
    do { \
        if (socket_id < 0) { \
            log(ERROR, "Invalid socket ID"); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_SOCKET_ID_WITH_NULL(socket_id) \
    do { \
        if (socket_id < 0) { \
            log(ERROR, "Invalid socket ID"); \
            return NULL; \
        } \
    } while(0)

// Update the validation macros at the top of the file:

#define VALIDATE_GAME_MANAGER_INT(gm) \
    do { \
        if (!gm) { \
            log(ERROR, "Invalid game manager"); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_GAME_MANAGER_PTR(gm) \
    do { \
        if (!gm) { \
            log(ERROR, "Invalid game manager"); \
            return NULL; \
        } \
    } while(0)

#define VALIDATE_GAME_MANAGER_ROLE(gm) \
    do { \
        if (!gm) { \
            log(ERROR, "Invalid game manager"); \
            return ROLE_UNASSIGNED; \
        } \
    } while(0)

#define VALIDATE_GAME_MANAGER_STATE(gm) \
    do { \
        if (!gm) { \
            log(ERROR, "Invalid game manager"); \
            return GAME_STATE_LOBBY; \
        } \
    } while(0)

// Find player by socket_id
static player_t *find_player_by_socket(game_manager_t game_manager, int socket_id) {
    VALIDATE_GAME_MANAGER_PTR(game_manager);
    VALIDATE_SOCKET_ID_WITH_NULL(socket_id);

    player_t *player = game_manager->players;
    while (player) {
        if (player->socket_id == socket_id) {
            return player;
        }
        player = player->next;
    }
    return NULL;
}

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
    game_manager_cdt *game_manager = malloc(sizeof(game_manager_cdt));
    if (!game_manager) {
        log(ERROR, "Failed to allocate memory for game manager");
        return NULL;
    }

    game_manager->players = NULL;
    game_manager->max_players = max_players;
    game_manager->player_count = 0;
    game_manager->alive_count = 0;
    game_manager->vote_count = 0;

    game_manager->votes = malloc(sizeof(int) * max_players);
    if (!game_manager->votes) {
        log(ERROR, "Failed to allocate memory for votes");
        free(game_manager);
        return NULL;
    }

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
    if (!game_manager) {
        return;
    }

    // Free all players
    player_t *current = game_manager->players;
    while (current) {
        player_t *next = current->next;
        free(current);
        current = next;
    }
    
    free(game_manager->votes);
}

int
game_manager_add_player(game_manager_t game_manager, int socket_id)
{
    VALIDATE_GAME_MANAGER_INT(game_manager);
    VALIDATE_SOCKET_ID(socket_id);
    if (find_player_by_socket(game_manager, socket_id)) {
        log(WARN, "Player %d already exists", socket_id);
        return -1;
    }

    if (game_manager->player_count >= game_manager->max_players) {
        log(WARN, "Maximum players reached");
        return -1;
    }

    player_t *player = malloc(sizeof(player_t));
    if (!player) {
        log(ERROR, "Failed to allocate memory for player");
        return -1;
    }

    player->socket_id = socket_id;
    player->is_alive = true;
    player->is_protected = false;
    player->has_used_ability = false;
    player->role = ROLE_UNASSIGNED;
    player->player_number = game_manager->player_count + 1;
    player->next = game_manager->players;  // Add to front of list
    game_manager->players = player;

    game_manager->player_count++;
    game_manager->alive_count++;

    return 0;
}

int
game_manager_remove_player(game_manager_t game_manager, int socket_id)
{
    VALIDATE_GAME_MANAGER_INT(game_manager);
    VALIDATE_SOCKET_ID(socket_id);
    player_t **pp = &game_manager->players;
    while (*pp) {
        if ((*pp)->socket_id == socket_id) {
            player_t *player = *pp;
            
            if (player->is_alive) {
                game_manager->alive_count--;
                game_manager->roles[player->role].alive_count--;
                
                if (player->is_protected) {
                    game_manager->roles[player->role].protected_count--;
                }
            }

            *pp = player->next;  // Remove from list
            free(player);
            game_manager->player_count--;
            return 0;
        }
        pp = &(*pp)->next;
    }

    log(WARN, "Player %d not found", socket_id);
    return -1;
}

int
game_manager_get_player_count(game_manager_t game_manager)
{
    return game_manager->player_count;
}

int
game_manager_get_alive_count(game_manager_t game_manager)
{
    VALIDATE_GAME_MANAGER_INT(game_manager);
    return game_manager->alive_count;
}

int
game_manager_get_werewolf_count(game_manager_t game_manager)
{
    VALIDATE_GAME_MANAGER_INT(game_manager);
    return game_manager->roles[ROLE_WEREWOLF].total_count;
}

game_role_t
game_manager_get_player_role(game_manager_t game_manager, int socket_id)
{
    VALIDATE_GAME_MANAGER_ROLE(game_manager);
    VALIDATE_SOCKET_ID(socket_id);
    player_t *player = find_player_by_socket(game_manager, socket_id);
    return player ? player->role : ROLE_UNASSIGNED;
}

bool
game_manager_is_player_alive(game_manager_t game_manager, int socket_id)
{
    VALIDATE_GAME_MANAGER_PTR(game_manager);
    VALIDATE_SOCKET_ID(socket_id);
    player_t *player = find_player_by_socket(game_manager, socket_id);
    return player ? player->is_alive : false;
}

bool
game_manager_is_player_werewolf(game_manager_t game_manager, int socket_id)
{
    VALIDATE_GAME_MANAGER_PTR(game_manager);
    VALIDATE_SOCKET_ID(socket_id);
    player_t *player = find_player_by_socket(game_manager, socket_id);
    return player ? player->role == ROLE_WEREWOLF : false;
}


int
game_manager_start_game(game_manager_t game_manager)
{
    if (!game_manager) {
        log(ERROR, "Invalid game manager");
        return -1;
    }

    const game_config_t *config = get_game_config(game_manager->max_players);
    if (!config) {
        log(ERROR, "Invalid player count for game start");
        return -1;
    }

    // Create array of player indices for role assignment
    int *player_indices = malloc(sizeof(int) * game_manager->player_count);
    if (!player_indices) {
        log(ERROR, "Failed to allocate memory for role assignment");
        return -1;
    }

    // Fill array with indices
    int index = 0;
    for (player_t *player = game_manager->players; player; player = player->next) {
        player_indices[index++] = player->socket_id;
    }

    shuffle_array(player_indices, game_manager->player_count);

    // Assign roles
    index = 0;
    for (int i = 0; i < config->num_werewolves; i++) {
        player_t *player = find_player_by_socket(game_manager, player_indices[index++]);
        player->role = ROLE_WEREWOLF;
        game_manager->roles[ROLE_WEREWOLF].total_count++;
        game_manager->roles[ROLE_WEREWOLF].alive_count++;
    }

    for (int i = 0; i < config->num_villagers; i++) {
        player_t *player = find_player_by_socket(game_manager, player_indices[index++]);
        player->role = ROLE_VILLAGER;
        game_manager->roles[ROLE_VILLAGER].total_count++;
        game_manager->roles[ROLE_VILLAGER].alive_count++;
    }

    for (int i = 0; i < config->num_special_roles; i++) {
        if (index < game_manager->player_count) {
            player_t *player = find_player_by_socket(game_manager, player_indices[index++]);
            game_role_t role = config->special_roles[i];
            player->role = role;
            game_manager->roles[role].total_count++;
            game_manager->roles[role].alive_count++;
        }
    }

    // Any remaining players become villagers
    while (index < game_manager->player_count) {
        player_t *player = find_player_by_socket(game_manager, player_indices[index++]);
        player->role = ROLE_VILLAGER;
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
    return 0;
}

int *
game_manager_get_players_sockets(game_manager_t game_manager)
{
    VALIDATE_GAME_MANAGER_PTR(game_manager);

    int *player_sockets = malloc(sizeof(int) * game_manager->player_count);
    if (!player_sockets) {
        log(ERROR, "Failed to allocate memory for player sockets");
        return NULL;
    }
    
    int index = 0;
    
    for (player_t *player = game_manager->players; player; player = player->next) {
        player_sockets[index++] = player->socket_id;
    }
    
    return player_sockets;
}

game_state_t
game_manager_get_phase(game_manager_t game_manager)
{
    VALIDATE_GAME_MANAGER_STATE(game_manager);
    return game_manager->state.current_phase;
}

bool
game_manager_is_player_protected(game_manager_t game_manager, int socket_id)
{
    VALIDATE_GAME_MANAGER_PTR(game_manager);
    VALIDATE_SOCKET_ID(socket_id);
    player_t *player = find_player_by_socket(game_manager, socket_id);
    return player ? player->is_protected : false;
}

int
game_manager_get_player_number(game_manager_t game_manager, int socket_id)
{
    VALIDATE_GAME_MANAGER_INT(game_manager);
    VALIDATE_SOCKET_ID(socket_id);
    player_t *player = find_player_by_socket(game_manager, socket_id);
    return player ? player->player_number : -1;
}

int
game_manager_get_socket_by_player_number(game_manager_t game_manager, int player_number)
{
    VALIDATE_GAME_MANAGER_INT(game_manager);
    player_t *player = game_manager->players;
    while (player) {
        if (player->player_number == player_number) {
            return player->socket_id;
        }
        player = player->next;
    }
    return -1;
}

int
game_manager_cmp_phase(game_manager_t game_manager, game_state_t phase)
{
    VALIDATE_GAME_MANAGER_INT(game_manager);
    return game_manager->state.current_phase == phase;
}
