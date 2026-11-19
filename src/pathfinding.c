#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include "pathfinding.h"

#define NODE_COUNT    (MAP_COLS * MAP_ROWS)
#define HEAP_CAPACITY (NODE_COUNT * 8)   // each tile can be pushed once per neighbor

#define COST_STRAIGHT 10   // moving up/down/left/right
#define COST_DIAGONAL 14   // moving diagonally (~10 * sqrt(2), kept as an int)

// ---------- Min-heap (priority queue) ordered by f-score ----------
// A* always expands the open tile with the lowest f = g + h. A binary heap
// gives us that tile in O(log n) instead of scanning every open tile.

typedef struct {
    int node;   // tile index: row * MAP_COLS + col
    int f;      // priority (lower = expanded first)
} HeapItem;

static HeapItem heap[HEAP_CAPACITY];
static int heapSize;

static void HeapPush(int node, int f)
{
    if (heapSize >= HEAP_CAPACITY) return;   // can't happen with our bound, but stay safe

    int i = heapSize++;
    heap[i] = (HeapItem){ node, f };

    // Sift up: swap with parent while we're smaller than it
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap[parent].f <= heap[i].f) break;
        HeapItem tmp = heap[parent]; heap[parent] = heap[i]; heap[i] = tmp;
        i = parent;
    }
}

static int HeapPop(void)
{
    int top = heap[0].node;
    heap[0] = heap[--heapSize];

    // Sift down: swap with the smaller child while a child is smaller than us
    int i = 0;
    for (;;) {
        int left = 2 * i + 1, right = left + 1, smallest = i;
        if (left  < heapSize && heap[left].f  < heap[smallest].f) smallest = left;
        if (right < heapSize && heap[right].f < heap[smallest].f) smallest = right;
        if (smallest == i) break;
        HeapItem tmp = heap[smallest]; heap[smallest] = heap[i]; heap[i] = tmp;
        i = smallest;
    }
    return top;
}

// ---------- A* ----------

static bool useHeuristic = true;

void SetUseHeuristic(bool enabled) { useHeuristic = enabled; }
bool GetUseHeuristic(void)         { return useHeuristic; }

// Octile distance: the exact cost between two tiles on an empty 8-direction grid.
// It never overestimates, so A* is guaranteed to find the shortest path.
static int Heuristic(int col, int row, int goalCol, int goalRow)
{
    if (!useHeuristic) return 0;   // h = 0 turns A* into Dijkstra's algorithm

    int dx = abs(col - goalCol);
    int dy = abs(row - goalRow);
    int diag = dx < dy ? dx : dy;
    return COST_STRAIGHT * (dx + dy) + (COST_DIAGONAL - 2 * COST_STRAIGHT) * diag;
}

bool FindPath(Tile start, Tile goal, Path *out)
{
    static int  gScore[NODE_COUNT];    // cheapest known cost from start to each tile
    static int  cameFrom[NODE_COUNT];  // previous tile on that cheapest route
    static bool closed[NODE_COUNT];    // tiles already fully expanded

    out->length = 0;
    out->nodesExplored = 0;

    if (IsWallAt(start.col, start.row) || IsWallAt(goal.col, goal.row)) return false;

    for (int i = 0; i < NODE_COUNT; i++) {
        gScore[i] = INT_MAX;
        cameFrom[i] = -1;
        closed[i] = false;
    }
    heapSize = 0;

    int startIdx = start.row * MAP_COLS + start.col;
    int goalIdx  = goal.row  * MAP_COLS + goal.col;

    gScore[startIdx] = 0;
    HeapPush(startIdx, Heuristic(start.col, start.row, goal.col, goal.row));

    // The 8 neighbor directions
    static const int DC[8] = { 1, -1, 0,  0, 1,  1, -1, -1 };
    static const int DR[8] = { 0,  0, 1, -1, 1, -1,  1, -1 };

    while (heapSize > 0) {
        int current = HeapPop();

        // The heap can hold old, worse copies of a tile; skip them
        if (closed[current]) continue;
        closed[current] = true;
        out->nodesExplored++;

        if (current == goalIdx) {
            // Walk backwards through cameFrom to count the path...
            int length = 0;
            for (int n = goalIdx; n != -1; n = cameFrom[n]) length++;

            // ...then fill it in start-to-goal order
            out->length = length;
            int i = length - 1;
            for (int n = goalIdx; n != -1; n = cameFrom[n]) {
                out->tiles[i--] = (Tile){ n % MAP_COLS, n / MAP_COLS };
            }
            return true;
        }

        int col = current % MAP_COLS;
        int row = current / MAP_COLS;

        for (int d = 0; d < 8; d++) {
            int nc = col + DC[d];
            int nr = row + DR[d];
            if (IsWallAt(nc, nr)) continue;

            bool diagonal = (DC[d] != 0 && DR[d] != 0);
            // No corner-cutting: a diagonal step needs both side tiles to be open
            if (diagonal && (IsWallAt(col + DC[d], row) || IsWallAt(col, row + DR[d]))) continue;

            int neighbor = nr * MAP_COLS + nc;
            if (closed[neighbor]) continue;

            int tentative = gScore[current] + (diagonal ? COST_DIAGONAL : COST_STRAIGHT);
            if (tentative < gScore[neighbor]) {
                gScore[neighbor] = tentative;
                cameFrom[neighbor] = current;
                HeapPush(neighbor, tentative + Heuristic(nc, nr, goal.col, goal.row));
            }
        }
    }

    return false;   // open set ran out: goal is unreachable
}

Tile TileFromPixel(Vector2 pixel)
{
    return (Tile){ (int)floorf(pixel.x / TILE_SIZE), (int)floorf(pixel.y / TILE_SIZE) };
}

Vector2 TileCenter(Tile tile)
{
    return (Vector2){ tile.col * TILE_SIZE + TILE_SIZE / 2.0f, tile.row * TILE_SIZE + TILE_SIZE / 2.0f };
}
