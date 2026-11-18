# Adaptive Dread

A top-down survival horror game written in C with [raylib](https://www.raylib.com/).

> 🚧 **In progress**

## Planned features
- **Enemy AI:** A* pathfinding and a finite state machine (patrol → investigate → chase → attack)
- **Dynamic difficulty:** adjusts enemy aggression and item drops based on how the player is doing (accuracy, damage taken, deaths)
- **Performance:** steady 60 FPS with many active enemies

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
| Esc | Quit |
