---
name: new-room
description: Create a new TRIPPY room (map + .meta), wire its doors both ways into existing rooms, stub dialogue for its NPCs, validate it and screenshot it. Use when asked to add a room, level, area or map.
argument-hint: "<theme / idea> [connect to <room> <direction>] [size WxH]"
---

# New room

Read `docs/content-format.md` and `docs/style-guide.md` first, then follow
these steps in order.

1. **See the world.** Run `python3 scripts/check_content.py --graph`. Pick
   where the room attaches: the user's choice, or an existing room with a
   free edge that fits the theme. Note its free edges.
2. **Name it.** Use a snake_case name that isn't taken (`ls assets/maps`).
3. **Draw `assets/maps/<name>.txt`.**
   - Size it 16–24 wide and 9–13 tall. Every row must be the same width.
   - Wall border, with doors `>` on the edges it connects through. Keep the tile inside each door free.
   - Build in one idea from the style guide. That can be a mechanic (`^` warps, at least 2; `V` void plus `void_to`; `M` mirror half; `=` stairs; a loop) or a set piece.
   - Add `N` slots for 1–3 NPCs and at most one `I` slot.
4. **Write `assets/maps/<name>.meta`.**
   - A one-line `#` comment naming the room's idea.
   - `music_mood`, `transitions` (one per door), `npcs`, `items`, `trip`.
   - Optionally `kaleidoscope`, `locks`, `beam`, `void_to`, `wander`, `on_enter_flag`.
5. **Wire the neighbour.**
   - In the room you attach to, turn a wall tile on the matching edge into `>`. Use the opposite edge: going east from A arrives at B's west door.
   - Add `dir:<name>` to its `transitions`.
   - Make sure that new door's inner tile is floor.
6. **NPCs and items.**
   - For each new `npc_*`/`ufo_*` id, write `assets/dialogue/<id>.txt` (use the **new-npc** skill's rules).
   - For a `boss_*`, use the **new-boss** skill.
   - For a new item id, add a display name to `Inventory::displayName` in `src/Inventory.cpp` (the only C++ edit).
7. **Validate.** `python3 scripts/check_content.py` must report 0 errors. Fix warnings unless intentional (an intentional loop or materialize is fine).
8. **Look at it.**
   - `cmake --build build` (configure first if `build/` is missing).
   - `scripts/shot.sh <name>`, then view `screenshots/<name>.png` and the neighbour's screenshot.
   - Adjust the layout if it's cramped, empty or the door is blocked.
   - Use **playtest** to walk the doors if anything is unusual.
9. **Commit** with a message describing the room's idea and connections.
