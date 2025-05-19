# Werewolf Socket Game

## Game Overview

Werewolf (also known as Mafia) is a social deduction game where players are secretly assigned roles as either villagers or werewolves. The game alternates between day and night phases, with different actions available based on player roles. The village team wins by eliminating all werewolves, while the werewolf team wins by reaching number parity with villagers (equal numbers of werewolves and villagers remaining).

This implementation uses socket programming to create a networked version of the game, allowing players to connect remotely and participate in a moderated game session.

## Game Flow

### 1. Game Setup

- Players connect to the game server
- The server waits in the lobby until enough players have joined
- The host configures game settings (roles, timers, etc.)
- When ready, the host starts the game
- The server randomly assigns roles to all players
- Each player receives a private message with their role

### 2. Game Cycle

The game follows a cycle of night and day phases:

#### Night Phase

- All players close their eyes (UI shows night-time)
- The server activates players with night abilities in a specific order
- Players with night abilities perform their actions through private UI
- Night actions are processed by the server
- Night results are determined (deaths, information gathering, etc.)

#### Day Phase

The day phase consists of three sub-phases:

1. **Dawn Announcement**
   - Results of the night are announced (who died)
   - Any special events are announced (Hunter activation, etc.)

2. **Discussion Period**
   - All living players can freely discuss
   - Players exchange information and suspicions
   - Typical discussion period lasts 3-5 minutes

3. **Voting Period**
   - All living players vote for someone to eliminate
   - Player with the most votes is eliminated (with tie-breaking rules)
   - Eliminated player's role is revealed
   - Special abilities that trigger on elimination are activated

### 3. Game End

The game continues cycling through night and day phases until one of these conditions is met:

- All werewolves are eliminated (Village Team wins)
- Werewolves equal or outnumber villagers (Werewolf Team wins)
- Special victory condition is met (e.g., Tanner is lynched)

When the game ends, all player roles are revealed and statistics are displayed.

## Roles

### Village Team

#### Basic Villager
- No special abilities
- Can only participate in discussions and voting
- Wins when all werewolves are eliminated

#### Seer
- Each night, can investigate one player to learn if they are a werewolf or villager
- Results are only revealed to the Seer
- Should use this information to guide the village, without immediately revealing themselves

#### Doctor
- Each night, can protect one player from being killed
- Cannot protect the same player two nights in a row
- If they choose the same player targeted by werewolves, that player survives

#### Bodyguard
- Each night, can choose to guard one player
- If that player would be killed, the bodyguard dies instead
- Cannot guard the same player two nights in a row

#### Hunter
- When eliminated (either by lynch or night kill), can immediately shoot another player
- The shot player is also eliminated
- Must decide quickly when eliminated

#### Mayor
- Vote counts as 2 votes during day phase lynching
- Can reveal role at any time for added voting power
- Once revealed, becomes a prime target for werewolves

#### Witch
- Has two one-time-use potions: a healing potion and a poison potion
- Can use the healing potion to save someone from a werewolf attack
- Can use the poison potion to kill someone during the night
- Knows who the werewolves targeted before choosing to heal
- Cannot heal themselves

### Werewolf Team

#### Werewolf
- Each night, werewolves vote together to kill one player
- Know the identity of other werewolves
- Must remain hidden during day phase discussions
- Win when werewolves equal or outnumber villagers

#### Alpha Werewolf
- Leader of the werewolves
- Breaks ties in werewolf kill votes
- May have additional minor abilities (optional)

#### Wolf Cub
- If killed, the werewolves get two kills the following night
- Otherwise functions as a normal werewolf

### Independent Roles

#### Tanner
- Objective is to get lynched by the village
- Wins immediately if lynched (game continues for others)
- Appears as a villager to the Seer

#### Jester
- Similar to Tanner, objective is to get lynched
- If lynched, all players who voted for the Jester skip the next day's vote
- Wins immediately if lynched

#### Serial Killer
- Each night, can kill one player
- Acts independently from werewolves
- Wins if last person standing
- Appears as a villager to the Seer

## Game Variants

### Role Distribution

Recommended role distribution based on player count:

| Player Count | Werewolves | Special Village Roles | Basic Villagers |
|--------------|------------|------------------------|-----------------|
| 6            | 1          | 2                      | 3               |
| 7-8          | 2          | 2-3                    | 3               |
| 9-11         | 2-3        | 3-4                    | 4               |
| 12+          | 3-4        | 4-5                    | 5+              |

### Game Modes

#### Standard Mode
- All rules as described above
- Roles are hidden until elimination
- Day discussion period is timed

#### Reveal Mode
- Eliminated players' roles are publicly revealed
- Provides more information but less mystery

#### Speed Werewolf
- Shortened discussion time (1-2 minutes)
- Quick voting period
- Ideal for multiple games in a session

#### Custom Mode
- Host configures which roles are included
- Custom timers for each phase
- Special rules can be enabled/disabled

## User Interface

### Lobby UI
- Player list with ready status
- Game configuration options
- Chat functionality
- Start game button (host only)

### Game UI
- Day/night cycle visual indicators
- Chat window (active during day phase)
- Player list showing alive/dead status
- Action buttons based on role and phase
- Timer display for current phase
- Role and team information

### Night Phase UI
- Role-specific action interface
- Werewolves see a private chat and vote interface
- Special roles see their action options
- Regular villagers see "waiting for night to end" message

### Voting UI
- Display of all living players
- Vote selection mechanism
- Vote tally (may be hidden until complete)
- Timer for voting period

## Implementation Considerations

### Networking Architecture
- Client-server model
- Server as the authoritative game state controller
- Secure communication to prevent cheating
- Handling for disconnections

### Moderation Features
- Ability to pause game
- Vote kick functionality
- Report system for inappropriate behavior
- Observer mode for spectators

### Quality of Life Features
- Game history recording
- Statistics tracking
- Custom role sets
- User profiles and achievements

## Game Etiquette

1. **No Private Communication**
   - Players should not communicate outside the game
   - No sharing screenshots or direct messages

2. **No Quitting**
   - Players commit to finishing the game
   - Disconnected players may be replaced by AI

3. **Sportsmanship**
   - Accept elimination gracefully
   - No revealing roles when eliminated (unless required)
   - Keep discussions constructive and friendly

4. **Honesty About Technical Issues**
   - Report any bugs or connection issues promptly
   - Don't exploit technical glitches

## Building and Running

### Option 1: Native Build

#### Prerequisites
- GCC compiler
- Make
- POSIX-compliant operating system

#### Building
```bash
# Build both server and client
make all

# Build only server
make server

# Build only client
make client
```

The executables will be created in:
- Server: `build/server/werewolf_server`
- Client: `build/client/werewolf_client`

#### Running
```bash
# Run the server
./build/server/werewolf_server

# Run the client
./build/client/werewolf_client
```

### Option 2: Using Docker

#### Building Docker Images
```bash
# Build all images
docker build --target builder -t werewolf-builder .
docker build --target client-runtime -t werewolf-client .
docker build --target server-runtime -t werewolf-server .
```

#### Running with Docker
```bash
# Run the server
docker run -p 8080:8080 werewolf-server

# Run the client
docker run -it --rm werewolf-client
```

### Option 3: Standalone Client Distribution

For Linux users who don't want to build from source, you can distribute the client executable directly. The client is statically linked with minimal dependencies:

1. Download the client executable
2. Make it executable:
   ```bash
   chmod +x werewolf_client
   ```
3. Run it:
   ```bash
   ./werewolf_client
   ```

Note: The client requires only libc6 to run, which is available on all modern Linux distributions.

---

This Werewolf implementation aims to capture the essence of the traditional party game while leveraging networking technology to enable remote play. The social deduction, strategy, and deception elements remain central to the experience, with the added convenience of online connectivity.
