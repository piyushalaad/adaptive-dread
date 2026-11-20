#include <math.h>
#include "enemy.h"

// ---------- Tuning ----------
#define SPEED_PATROL        70.0f
#define SPEED_INVESTIGATE  110.0f
#define SPEED_CHASE        150.0f   // faster than walking, slower than sprinting

#define VISION_RANGE       (8 * TILE_SIZE)
#define VISION_HALF_ANGLE  50.0f    // degrees either side of facing (100 degree cone)
#define CLOSE_SENSE_RANGE  (1.5f * TILE_SIZE)   // notices you this close, even from behind

#define REPATH_TIME        0.25f
#define LOSE_TRACK_TIME    2.5f     // CHASE -> INVESTIGATE after this long without sensing the player
#define LOOK_AROUND_TIME   2.5f     // INVESTIGATE: how long to search once at the spot
#define INVESTIGATE_GIVEUP 10.0f    // INVESTIGATE -> PATROL if it can't get there in time

#define ATTACK_RANGE       34.0f    // distance between centers to start an attack
#define ATTACK_WINDUP      0.35f    // telegraph before the hit lands (gives the player a chance to dodge)
#define ATTACK_COOLDOWN    1.0f

// ---------- Small helpers ----------

static float Distance(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

static Vector2 Normalize(Vector2 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len < 0.0001f) return (Vector2){ 1.0f, 0.0f };
    return (Vector2){ v.x / len, v.y / len };
}

Vector2 EnemyCenter(const Enemy *enemy)
{
    return (Vector2){ enemy->position.x + ENEMY_SIZE / 2.0f, enemy->position.y + ENEMY_SIZE / 2.0f };
}

Rectangle EnemyRect(const Enemy *enemy)
{
    return (Rectangle){ enemy->position.x, enemy->position.y, ENEMY_SIZE, ENEMY_SIZE };
}

const char *EnemyStateName(EnemyState state)
{
    switch (state) {
        case ENEMY_PATROL:      return "PATROL";
        case ENEMY_INVESTIGATE: return "INVESTIGATE";
        case ENEMY_CHASE:       return "CHASE";
        case ENEMY_ATTACK:      return "ATTACK";
    }
    return "?";
}

// Every state change goes through here, so per-state timers always start fresh.
static void SetState(Enemy *enemy, EnemyState state)
{
    enemy->state = state;
    enemy->stateTimer = 0.0f;
    enemy->repathTimer = 0.0f;   // plan a new route right away
    enemy->arrived = false;
}

// ---------- Perception ----------

static bool CanSee(const Enemy *enemy, Vector2 target)
{
    Vector2 eye = EnemyCenter(enemy);
    float dist = Distance(eye, target);

    if (dist > VISION_RANGE) return false;

    // Very close: the enemy senses you even if you're behind it
    if (dist > CLOSE_SENSE_RANGE) {
        // Inside the cone? Compare the angle using a dot product instead of atan2:
        // dot(facing, dirToTarget) = cos(angle between them)
        Vector2 dir = Normalize((Vector2){ target.x - eye.x, target.y - eye.y });
        float dot = enemy->facing.x * dir.x + enemy->facing.y * dir.y;
        if (dot < cosf(VISION_HALF_ANGLE * DEG2RAD)) return false;
    }

    return HasLineOfSight(eye, target);
}

static bool CanHear(const Enemy *enemy, PlayerSense player)
{
    return player.noiseRadius > 0.0f && Distance(EnemyCenter(enemy), player.center) <= player.noiseRadius;
}

// ---------- Movement ----------

static void MoveToward(Enemy *enemy, Vector2 target, float speed, float dt)
{
    Vector2 center = EnemyCenter(enemy);
    float dx = target.x - center.x;
    float dy = target.y - center.y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.001f) return;

    enemy->facing = (Vector2){ dx / dist, dy / dist };

    float step = speed * dt;
    if (step > dist) step = dist;   // don't overshoot

    // Axis-by-axis, same as the player, so enemies slide along walls
    Rectangle next = EnemyRect(enemy);

    next.x += dx / dist * step;
    if (!RectHitsWall(next)) enemy->position.x = next.x;
    else next.x = enemy->position.x;

    next.y += dy / dist * step;
    if (!RectHitsWall(next)) enemy->position.y = next.y;
}

// Follows an A* path toward `goal`. Returns true once the enemy has reached it.
static bool NavigateTo(Enemy *enemy, Vector2 goal, float speed, float dt)
{
    Vector2 center = EnemyCenter(enemy);
    if (Distance(center, goal) < 4.0f) return true;

    // Re-plan every so often (the goal may be moving)
    enemy->repathTimer -= dt;
    if (enemy->repathTimer <= 0.0f) {
        enemy->repathTimer = REPATH_TIME;

        double t0 = GetTime();
        bool found = FindPath(TileFromPixel(center), TileFromPixel(goal), &enemy->path);
        enemy->lastSearchMs = (GetTime() - t0) * 1000.0;

        enemy->pathIndex = 1;   // index 0 is the tile we're standing on
        if (!found) enemy->path.length = 0;
    }

    if (enemy->path.length == 0) return false;   // unreachable

    if (enemy->pathIndex < enemy->path.length) {
        Vector2 next = TileCenter(enemy->path.tiles[enemy->pathIndex]);
        if (Distance(center, next) < 2.0f) enemy->pathIndex++;
        else MoveToward(enemy, next, speed, dt);
    } else {
        MoveToward(enemy, goal, speed, dt);   // same tile as the goal: close the last gap
    }
    return false;
}

// ---------- States ----------

static void UpdatePatrol(Enemy *enemy, PlayerSense player, float dt)
{
    if (CanSee(enemy, player.center)) {
        enemy->lastKnownPlayerPos = player.center;
        SetState(enemy, ENEMY_CHASE);
        return;
    }
    if (CanHear(enemy, player)) {
        enemy->lastKnownPlayerPos = player.center;
        SetState(enemy, ENEMY_INVESTIGATE);
        return;
    }

    if (enemy->waypointCount == 0) return;

    Vector2 waypoint = TileCenter(enemy->waypoints[enemy->currentWaypoint]);
    if (NavigateTo(enemy, waypoint, SPEED_PATROL, dt)) {
        enemy->currentWaypoint = (enemy->currentWaypoint + 1) % enemy->waypointCount;
        enemy->repathTimer = 0.0f;
    }
}

static void UpdateInvestigate(Enemy *enemy, PlayerSense player, float dt)
{
    if (CanSee(enemy, player.center)) {
        enemy->lastKnownPlayerPos = player.center;
        SetState(enemy, ENEMY_CHASE);
        return;
    }
    if (CanHear(enemy, player)) {
        // Heard it again: head for the newer sound, and reset the search
        enemy->lastKnownPlayerPos = player.center;
        enemy->stateTimer = 0.0f;
        enemy->arrived = false;
    }

    if (!enemy->arrived) {
        if (NavigateTo(enemy, enemy->lastKnownPlayerPos, SPEED_INVESTIGATE, dt)) {
            enemy->arrived = true;
            enemy->stateTimer = 0.0f;   // now time the look-around
        } else if (enemy->stateTimer > INVESTIGATE_GIVEUP) {
            SetState(enemy, ENEMY_PATROL);
        }
        return;
    }

    // At the spot: slowly turn to scan the room
    float angle = 2.0f * dt;   // radians per frame
    Vector2 f = enemy->facing;
    enemy->facing = (Vector2){ f.x * cosf(angle) - f.y * sinf(angle),
                               f.x * sinf(angle) + f.y * cosf(angle) };

    if (enemy->stateTimer > LOOK_AROUND_TIME) SetState(enemy, ENEMY_PATROL);
}

static void UpdateChase(Enemy *enemy, PlayerSense player, float dt)
{
    // Keep tracking the player as long as we can see or hear them
    if (CanSee(enemy, player.center) || CanHear(enemy, player)) {
        enemy->lastKnownPlayerPos = player.center;
        enemy->timeSinceSensed = 0.0f;
    } else {
        enemy->timeSinceSensed += dt;
    }

    if (enemy->timeSinceSensed > LOSE_TRACK_TIME) {
        SetState(enemy, ENEMY_INVESTIGATE);   // search where we last had them
        return;
    }

    float dist = Distance(EnemyCenter(enemy), player.center);
    if (dist < ATTACK_RANGE && enemy->attackCooldown <= 0.0f && enemy->timeSinceSensed == 0.0f) {
        SetState(enemy, ENEMY_ATTACK);
        return;
    }

    bool reached = NavigateTo(enemy, enemy->lastKnownPlayerPos, SPEED_CHASE, dt);
    if (reached && enemy->timeSinceSensed > 0.0f) {
        SetState(enemy, ENEMY_INVESTIGATE);   // got to the last known spot and they're gone
    }
}

static int UpdateAttack(Enemy *enemy, PlayerSense player)
{
    // Wind up (stand still, face the player), then strike
    Vector2 c = EnemyCenter(enemy);
    enemy->facing = Normalize((Vector2){ player.center.x - c.x, player.center.y - c.y });

    if (enemy->stateTimer < ATTACK_WINDUP) return 0;

    int damage = 0;
    if (Distance(c, player.center) < ATTACK_RANGE + 8.0f) damage = 1;   // dodged if you moved away in time

    enemy->attackCooldown = ATTACK_COOLDOWN;
    SetState(enemy, ENEMY_CHASE);
    return damage;
}

// ---------- Public ----------

void InitEnemy(Enemy *enemy, const Tile *waypoints, int waypointCount)
{
    if (waypointCount > MAX_WAYPOINTS) waypointCount = MAX_WAYPOINTS;

    Vector2 start = TileCenter(waypoints[0]);
    enemy->position = (Vector2){ start.x - ENEMY_SIZE / 2.0f, start.y - ENEMY_SIZE / 2.0f };
    enemy->facing = (Vector2){ 1.0f, 0.0f };

    for (int i = 0; i < waypointCount; i++) enemy->waypoints[i] = waypoints[i];
    enemy->waypointCount = waypointCount;
    enemy->currentWaypoint = waypointCount > 1 ? 1 : 0;

    enemy->path.length = 0;
    enemy->path.nodesExplored = 0;
    enemy->pathIndex = 0;
    enemy->lastSearchMs = 0.0;
    enemy->lastKnownPlayerPos = start;
    enemy->timeSinceSensed = 0.0f;
    enemy->attackCooldown = 0.0f;

    SetState(enemy, ENEMY_PATROL);
}

int UpdateEnemy(Enemy *enemy, PlayerSense player, float dt)
{
    enemy->stateTimer += dt;
    if (enemy->attackCooldown > 0.0f) enemy->attackCooldown -= dt;

    switch (enemy->state) {
        case ENEMY_PATROL:      UpdatePatrol(enemy, player, dt);      break;
        case ENEMY_INVESTIGATE: UpdateInvestigate(enemy, player, dt); break;
        case ENEMY_CHASE:       UpdateChase(enemy, player, dt);       break;
        case ENEMY_ATTACK:      return UpdateAttack(enemy, player);
    }
    return 0;
}

static Color StateColor(EnemyState state)
{
    switch (state) {
        case ENEMY_PATROL:      return (Color){ 110, 160,  90, 255 };   // green
        case ENEMY_INVESTIGATE: return (Color){ 220, 190,  60, 255 };   // yellow
        case ENEMY_CHASE:       return (Color){ 230, 120,  40, 255 };   // orange
        case ENEMY_ATTACK:      return (Color){ 255, 255, 255, 255 };   // white flash
    }
    return GRAY;
}

void DrawEnemy(const Enemy *enemy, bool debug)
{
    Vector2 c = EnemyCenter(enemy);
    Color color = StateColor(enemy->state);

    if (debug) {
        // Vision cone, clipped by walls: cast rays across the cone and fill between them
        const int RAYS = 24;
        float baseAngle = atan2f(enemy->facing.y, enemy->facing.x);
        float half = VISION_HALF_ANGLE * DEG2RAD;
        Color coneColor = (Color){ color.r, color.g, color.b, 40 };

        Vector2 prev = { 0 };
        for (int i = 0; i <= RAYS; i++) {
            float a = baseAngle - half + (2.0f * half) * i / RAYS;
            Vector2 end = { c.x + cosf(a) * VISION_RANGE, c.y + sinf(a) * VISION_RANGE };
            Vector2 hit = CastRay(c, end);
            if (i > 0) DrawTriangle(c, hit, prev, coneColor);   // counter-clockwise on screen
            prev = hit;
        }

        // Current A* path
        if (enemy->path.length > 0) {
            Vector2 p0 = c;
            for (int i = enemy->pathIndex; i < enemy->path.length; i++) {
                Vector2 p = TileCenter(enemy->path.tiles[i]);
                DrawLineV(p0, p, (Color){ color.r, color.g, color.b, 140 });
                p0 = p;
            }
        }

        DrawText(EnemyStateName(enemy->state), (int)enemy->position.x - 8, (int)enemy->position.y - 16, 10, color);
    }

    DrawRectangleRec(EnemyRect(enemy), color);

    // Small "eye" dot showing which way it's facing
    DrawCircleV((Vector2){ c.x + enemy->facing.x * 8, c.y + enemy->facing.y * 8 }, 3.0f, BLACK);
}
