#ifndef ENEMY_H
#define ENEMY_H

#include <stdbool.h>
#include "raylib.h"
#include "pathfinding.h"

#define ENEMY_SIZE     24.0f
#define MAX_WAYPOINTS  8

// The four behaviours an enemy can be in. Exactly one is active at a time.
//
//   PATROL ──(hears noise)──> INVESTIGATE ──(gives up)──> PATROL
//     │                          │    ▲
//  (sees player)          (sees player)   (loses track)
//     ▼                          ▼    │
//   CHASE <───────────────────── CHASE
//     │  ▲
//  (in range) (attack done)
//     ▼  │
//   ATTACK
typedef enum {
    ENEMY_PATROL,
    ENEMY_INVESTIGATE,
    ENEMY_CHASE,
    ENEMY_ATTACK
} EnemyState;

// What an enemy is allowed to know about the player each frame.
typedef struct {
    Vector2 center;      // player position (only "seen" if in the vision cone with line of sight)
    float noiseRadius;   // how far away the player's footsteps can be heard (0 = silent)
} PlayerSense;

typedef struct {
    Vector2 position;          // top-left corner, in pixels
    Vector2 facing;            // unit vector: the direction the enemy is looking
    EnemyState state;
    float stateTimer;          // seconds spent in the current state

    // Navigation
    Path path;
    int pathIndex;
    float repathTimer;
    double lastSearchMs;

    // Patrol route
    Tile waypoints[MAX_WAYPOINTS];
    int waypointCount;
    int currentWaypoint;

    // Memory of the player
    Vector2 lastKnownPlayerPos;
    float timeSinceSensed;     // seconds since the enemy last saw or heard the player
    bool arrived;              // INVESTIGATE: reached the spot and is looking around

    // Combat
    float attackCooldown;
} Enemy;

void InitEnemy(Enemy *enemy, const Tile *waypoints, int waypointCount);

// Runs one frame of the state machine. Returns the damage dealt to the player this frame.
int UpdateEnemy(Enemy *enemy, PlayerSense player, float dt);

void DrawEnemy(const Enemy *enemy, bool debug);
Rectangle EnemyRect(const Enemy *enemy);
Vector2 EnemyCenter(const Enemy *enemy);
const char *EnemyStateName(EnemyState state);

#endif
