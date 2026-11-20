# Adaptive Dread

A top-down survival horror game written in C with [raylib](https://www.raylib.com/).

> **In progress**

## Features
- ✅ **A\* pathfinding** on a tile grid (binary-heap priority queue, octile heuristic, no corner-cutting), with a live toggle to compare against Dijkstra's algorithm
- ✅ **Enemy AI state machine:** patrol → investigate → chase → attack
  - **Sight:** vision cones with wall-blocked line of sight
  - **Hearing:** sprinting is loud, walking is quiet, standing still is silent
  - **Attacks** have a wind-up, so you can dodge by moving away
- ⬜ **Combat:** player weapons and ammo
- ⬜ **Dynamic difficulty:** adjusts enemy aggression and item drops based on how the player is doing (accuracy, damage taken, deaths)
- ⬜ **Performance:** steady 60 FPS with many active enemies

## Build & run

**Requirements:** macOS with Xcode Command Line Tools (`xcode-select --install`) and Git.

```bash
make run
```
The first build downloads and compiles [raylib](https://github.com/raysan5/raylib) into `external/` automatically.

## Controls
| Key | Action |
|-----|--------|
| WASD | Move |
| Left Shift | Sprint (faster, but enemies can hear you from further away) |
| TAB | Show/hide debug view (vision cones, A* paths, enemy states, noise radius) |
| H | Toggle A* heuristic (compare with Dijkstra) |
| R | Restart |
| Esc | Quit |
