---
name: playtest
description: Build and playtest TRIPPY headlessly - validate content, screenshot rooms, and drive scripted input (walk, talk, duel, doors) to see what actually happens. Use after content changes, when asked to test, check, screenshot or verify the game, or to reproduce a bug.
argument-hint: "[room | route | 'all'] [what to check]"
---

# Playtest

Tools (all from the repo root):

- `python3 scripts/check_content.py [--graph]` — static checks, instant.
- `cmake -B build && cmake --build build` — local build (system SDL2).
- `scripts/shot.sh [rooms...]` — one PNG per room in `screenshots/`
  (headless, `SIZE=1280x720`, `EXTRA="--give x --flag y"` for state).
- `./build/trippy --room R --pos x,y --give item --flag f --skip-title` —
  start anywhere with any state; never touches the real save.
- `scripts/drive.sh "<trippy args>" step...` — scripted input under Xvfb
  (Linux; needs xvfb, xdotool, imagemagick). Steps: `hold:Up:0.5`,
  `tap:e`, `wait:1`, `shot:/tmp/x.png`. Taps last ~80ms.

Procedure:

1. Always start with `check_content.py`; fix errors before anything else.
2. Build. A compile error means C++ changed — fix it first.
3. Screenshot the rooms in question (or `all`) and **look at every image**:
   blocked doors, NPCs on top of doors, empty or cramped layouts, unreadable
   text, art missing.
4. For interactions, drive them. Tile math: tiles are 32px; `--pos x,y` is a
   tile; the player faces down at start, so tap the direction first. NPCs
   need to be in the faced tile (bosses: up to two tiles away). Examples:
   - talk: `"--room hallway --pos 13,3" tap:Up tap:e wait:2 shot:/tmp/talk.png`
   - duel: `"--room observation_deck --pos 7,5" tap:Up tap:e wait:2.5 tap:e wait:1.5 shot:/tmp/duel.png`
   - locked door: `"--room waiting_room --pos 10,1" hold:Up:0.5 shot:/tmp/lock.png` (expect the "not yet" toast);
     add `--flag beat_boss_customs` and expect to arrive in the next room.
   - warps/void/beam: walk onto the tile with `hold:` and screenshot after.
5. Check `drive.log` / the shot logs for `error`, `unknown`, `missing`.
6. Report what you saw with the screenshots (attach the important ones),
   and what you fixed. Don't claim something works without an image or log
   showing it.
