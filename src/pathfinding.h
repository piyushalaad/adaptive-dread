#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <stdbool.h>
#include "map.h"

#define MAX_PATH (MAP_COLS * MAP_ROWS)

typedef struct {
    int col;
    int row;
} Tile;

typedef struct {
    Tile tiles[MAX_PATH];  // tiles from start to goal (inclusive)
    int length;            // number of tiles in the path (0 = no path)
    int nodesExplored;     // how many tiles A* expanded (useful for performance stats)
} Path;

// Finds the shortest path from start to goal with A*.
// Moves in 8 directions; diagonals can't cut wall corners.
// Returns true and fills `out` if a path exists.
bool FindPath(Tile start, Tile goal, Path *out);

// Turn the heuristic on (A*) or off (h = 0, which makes the search Dijkstra's algorithm).
void SetUseHeuristic(bool enabled);
bool GetUseHeuristic(void);

// Converts a pixel position to the tile it's in, and a tile to its center pixel.
Tile TileFromPixel(Vector2 pixel);
Vector2 TileCenter(Tile tile);

#endif
