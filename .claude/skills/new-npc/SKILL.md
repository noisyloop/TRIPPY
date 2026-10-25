---
name: new-npc
description: Add or rewrite a TRIPPY NPC - place it in a room slot and write its dialogue file in the house voice. Use when asked for a new character, NPC, conversation, or to change what someone says.
argument-hint: "<room> <who they are / idea>"
---

# New NPC

1. Read `docs/style-guide.md` (Voices + Hard rules) and the dialogue
   section of `docs/content-format.md`. Skim two existing files in
   `assets/dialogue/` with a similar voice for calibration.
2. **Id**: `npc_<snake>` for beings, `ufo_<snake>` for a drifting saucer
   (walk-under). Not `boss_` (that's **new-boss**). Check it's unused.
3. **Place it** (skip if rewriting): add an `N` to the room map on a floor
   tile that doesn't block a door or the path, then add `<slot>:<id>` to
   `npcs =` in the `.meta`. Slots number in reading order (left→right,
   top→bottom), so inserting an `N` earlier in the map **renumbers the
   later ones**; update all of them. Add the id to `wander =` if it should
   amble.
4. **Write `assets/dialogue/<id>.txt`**:
   - SHORT CAPS speaker name, voice from the table.
   - 1–3 nodes, each ≤ ~180 chars, one impossible specific image per line.
   - Optional: a choice with 2–3 options leading to different short replies;
     `set: met_<id>` on the first node plus `alt: met_<id> -> again` with a
     one-line repeat-visit node; `give: <item>` if it hands something over
     (then add a display name in `src/Inventory.cpp`).
   - Hints are allowed and good: point at the room's trick or the way on.
5. Run `python3 scripts/check_content.py` (0 errors). If the game is running,
   F5 reloads it.
6. Optional check in game: `scripts/drive.sh "--room <room> --pos x,y"
   tap:<dir toward npc> tap:e wait:2 shot:/tmp/npc.png` and view the PNG.
7. Commit.
