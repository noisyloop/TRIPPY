---
name: new-boss
description: Create a TRIPPY boss - a conversation duel (talk / echo / beat rounds), its placement, the door it unlocks, its reward and after-duel dialogue. Use when asked for a boss, guardian, duel, or a big character encounter.
argument-hint: "<room> <boss concept> [unlocks <direction>]"
---

# New boss

1. Read `docs/style-guide.md` (Bosses + Voices) and the duel section of
   `docs/content-format.md`. Read one existing `assets/duels/*.txt` to match
   the pacing.
2. **Id**: `boss_<snake>`. Concept = one strong, gentle, funny idea (a
   bureaucrat of time, a choir, a giant saucer mother, your own weight).
3. **`assets/duels/<id>.txt`**: `name:` (CAPS, can be long), `mood:` (see
   style guide), `reward:` (new item id), optional `after_room:`, `intro:`,
   `victory:` (points the way on). Then 5 rounds: talk, echo, talk, beat,
   talk. Talk: 3 replies — one wonderful (+30..+35), one okay (+5..+20), one
   wrong (−10..−20) — with the wrong response the funniest. Echo: length
   4–6, gain 25. Beat: length 6–8, gain 25. Success/failure lines in voice.
4. **`assets/dialogue/<id>.txt`**: 1–2 calm after-duel nodes.
5. **Place it**: `N` slot in the room (bosses draw 2x and talk from two
   tiles away — leave clear floor in front, and don't block the door it
   guards), `npcs =` entry, and `locks = <dir>:beat_<id>` on the door it
   guards (optionally `beam = <dir>`).
6. **Reward**: display name in `Inventory::displayName` (`src/Inventory.cpp`).
7. `python3 scripts/check_content.py` → 0 errors; build; `scripts/shot.sh
   <room>`.
8. **Playtest the duel** (see **playtest**): stand two tiles in front of it,
   face it, press E, step through a round, screenshot. Confirm the locked
   door hums before winning, e.g. `--flag beat_<id>` opens it.
9. Commit.
