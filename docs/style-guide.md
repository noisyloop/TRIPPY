# TRIPPY style guide

Read this before writing any room, NPC line or boss. The skills in
`.claude/skills/` follow it.

## The feeling

A small green alien visits a universe that is visiting it back. Cosmic,
gentle, funny, a little sad, never cruel. Psychedelic and spiritual without
being preachy: machine-elf joy, UFO bureaucracy, sacred geometry, letting
go. Every line should make the player smile or pause, ideally both.

## Hard rules

- **Original only.** No quotes, song lyrics, catchphrases, names or
  characters from books, films, games, music or real people. No "bat
  country", no famous numbers-as-jokes, no celebrities, no brands. If a line
  feels familiar, rewrite it.
- **No real drugs, no instructions, no medical claims.** The trip is
  visual and emotional, not chemical.
- **Nobody is mocked, nobody dies, nobody loses.** Bosses are won over,
  not beaten. Wrong answers get a gentle joke.
- **Short.** One node is 1–2 sentences, at most ~180 characters (the box
  fits about three lines). An NPC is usually 1–3 nodes. A choice label is
  under ~40 characters.

## Voices

| Who | Voice | Example |
| --- | --- | --- |
| Saucers / drones (`ufo_*`) | ALL CAPS, formal, kind, slightly broken protocol | `WELCOME ABOARD. PLEASE KEEP YOUR SOUL INSIDE THE VEHICLE.` |
| Small things (sparks, seedlings) | all lowercase, breathless | `zzt! i got out. don't tell.` |
| Jeweled / elf beings | delighted, overflowing, finish each other's thoughts with `--` | `--YES! IT FOLDS, IT FOLDS!` |
| Cows, librarians, monks | deadpan cosmic wisdom, one twist per line | `I've been walking down for years. I'm finally at the top.` |
| Earth people (Mom) | warm, plain, a little worried | `You were glowing in your sleep again.` |
| The Weight / inner things | quiet, sentence case, tender | `Nobody tells you that. I'm sorry nobody told you.` |

Speaker names are SHORT CAPS (`MOON COW`, `THE LIBRARIAN`, `DRONE-7`).
Narration uses an empty speaker and parentheses: `: (it hums your name)`.

Good lines turn on **one impossible, specific image** ("North is just the
south of the same sky", "Quiet is just loud that sat down"). Avoid vague
mysticism ("the universe is energy"), sarcasm, and explaining the joke.

## Rooms

- Size: about 16–24 tiles wide and 9–13 tall, so the room fills the
  window at 1.5–2.5x zoom. Tiny rooms get zoomed a lot and scroll.
- Always a wall border; doors (`>`) centered-ish on an edge; keep the tile
  inside each door free so arrivals don't land in a wall or an NPC.
- Give each room one idea: a mechanic (warps, void, mirror, a loop) or a
  set piece (a boss, a crowd, a garden). Name it in the `.meta` comment.
- `trip`: 0.1–0.3 calm Earth, 0.4–0.6 strange, 0.7–0.9 deep. `kaleidoscope`
  only for set pieces (0.15–0.6). Calm mode caps all of it anyway.
- Moods: `calm_pentatonic` (gentle), `playful_major` (bouncy), `dream_lydian`
  (wonder), `tense_chromatic` (busy/odd), `dread_diminished` (eerie),
  `melancholy_a_minor` (tender). Each visit re-rolls the tune.
- Impossible exits are welcome: a transition to the same room (loops), or
  into a room with no door facing back (you materialize).
- At most one floor item per room; put it somewhere that needs a little
  exploring (behind a warp, across a void, at the end of a mirror).

## Bosses

- A boss guards a locked door (`locks = dir:beat_boss_<id>`) and gives a
  reward item.
- 5 rounds is right: 3 `[talk]`, 1 `[echo]`, 1 `[beat]`, alternating.
- Resonance starts at 35 and wins at 100. Best reply +30/+35, okay +5..+20,
  wrong −10..−20. Echo length 4–6, beat length 6–8, gain 25. That means
  about three good answers win, and nobody is stuck for long.
- Every reply gets a response in the boss's voice, wrong ones included,
  and the wrong ones should be the funniest.
- Write after-duel dialogue (`assets/dialogue/<boss_id>.txt`): one or two
  calm nodes pointing the way on.

## Items

Snake_case ids (`moon_milk`), display names in Title Case added to
`Inventory::displayName`. Items are small and poetic: a spark, a page, a
seed, a visa of now. Not weapons, not money.
