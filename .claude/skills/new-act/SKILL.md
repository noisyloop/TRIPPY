---
name: new-act
description: Plan and build a whole new TRIPPY act - 3 to 5 connected rooms with a theme, NPCs, one item per room, and a boss guarding the way on. Use when asked for a new chapter, act, world, zone or several levels at once.
argument-hint: "<theme> [after <room>]"
---

# New act

1. Read `docs/style-guide.md` and run `python3 scripts/check_content.py --graph`
   to see the current route (bedroom → … → bedroom_awake) and where an act
   can branch in or be inserted (usually off a hub room's free edge, or
   between two acts by re-pointing one transition).
2. **Plan first, in a short message to the user** (then proceed unless they
   object): act theme in one line; 3–5 rooms, each with one idea (mechanic
   or set piece), its mood and trip level; how they connect (draw arrows);
   one item per room; the boss concept, which door it locks and its reward;
   3–6 NPC concepts. Include at least one impossible exit (loop or
   materialize) and one beam or void for flavor.
3. **Build in this order**, using the other skills' steps:
   rooms with **new-room** (wire entrances as you go), then NPCs with
   **new-npc**, then the boss with **new-boss**.
4. Run `python3 scripts/check_content.py` after each room; 0 errors at the
   end and no "unreachable" warnings for the new rooms.
5. Build, `scripts/shot.sh <each new room>`, view every screenshot, fix
   anything cramped or blocked. Playtest the boss duel and the locked door.
6. Update README's "The route" paragraph if the main path changed.
7. One commit per room or one for the act — describe the act in the message.
