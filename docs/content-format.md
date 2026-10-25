# Content formats

Everything a level needs is a text file under `assets/`. No C++ or rebuild
is needed: edit, then press **F5** in game (or relaunch) to reload. Run
`python3 scripts/check_content.py` before committing; CI runs it too.

## Rooms — `assets/maps/<room>.txt` + `<room>.meta`

One character per 32px tile:

| Char | Tile | Notes |
| --- | --- | --- |
| `#` | wall | also used for padding (spaces count as wall) |
| `.` | floor | |
| `~` | tall grass | walkable |
| `>` | door | direction comes from the edge it sits on (top row = north …) |
| `N` | NPC slot | numbered 0,1,2… in reading order, bound in `npcs =` |
| `I` | item slot | numbered 0,1,2… in reading order, bound in `items =` |
| `V` | void | falling in lands you in `void_to` (or back at the entrance) |
| `M` | mirror floor | your reflection walks the other way across it |
| `^` | warp | jumps to the next `^` in reading order (wraps around) |
| `=` | stairs | walkable |

`.meta` keys (all optional except what the map uses):

```ini
music_mood   = dream_lydian                 # see moodTable() in src/ChiptuneSynth.cpp
transitions  = west:garden north:my_room    # direction:room for each door
npcs         = 0:npc_gardener 1:ufo_humm 2:boss_choir
items        = 0:color_nine_seed
on_enter_flag = reached_my_room
trip         = 0.6        # 0..1 hue cycling, warp, trails
kaleidoscope = 0.3        # 0..1 mirrored wedges around the player
locks        = north:beat_boss_choir        # door opens once the flag is set
beam         = north      # leaving that way plays the tractor beam
void_to      = color_nine_garden
wander       = npc_gardener                 # these NPCs amble around their slot
```

Id prefixes decide behavior: `npc_*` talks, `ufo_*` drifts (walk-under),
`boss_*` starts a duel if `assets/duels/<id>.txt` exists (afterwards its
`assets/dialogue/<id>.txt` is used). Arriving players appear one tile inside
the door facing back the way they came; if there is none they materialize
at the room's first free floor tile ("impossible exits").

## Dialogue — `assets/dialogue/<npc_id>.txt`

```text
# comment
alt: met_gardener -> again        # optional: start at [again] once the flag is set

[hello]                            # first node is the start
GARDENER OF NINE: Most colors grow from light.
Nine grows from attention.         # continuation lines join with a space
set: met_gardener                  # optional side effects when shown
give: color_nine_seed
-> pick                            # next node; omit (or "-> end") to end

[pick]
GARDENER OF NINE: Will you pick the seed?
* Yes. -> yes                      # choices, shown after the text
* Not yet. -> end

[yes]
: (the seed is warm, like a held hand)   # leading ":" = no speaker
```

## Boss duels — `assets/duels/<boss_id>.txt`

```text
name: THE CHOIR OF TESSELLATORS     # shown above the boss
mood: dream_lydian                  # music during the duel
reward: choir_echo                  # item given on victory (optional)
after_room: bedroom_awake           # optional: where you wake up after winning
intro: WE ARE-- --ARE MANY.
victory: --YOU FINISHED US.

[talk]
line: THE OPPOSITE OF A DOOR IS--
* --another door. | +30 | --YES! IT FOLDS!       # reply | resonance change | response
* --a wall. | -10 | --NO, WALLS ARE SHY DOORS.

[echo]                              # repeat an arrow pattern
line: SING THE SHAPE BACK.
length: 5                           # arrows
gain: 25                            # resonance on success (failure costs half)
success: --PERFECT.
failure: --A WRONG NOTE!

[beat]                              # press arrows on the music's beat
line: CLAP WITH US.
length: 8                           # hits needed (3 misses fail)
gain: 25
success: --ONE OF US!
failure: --YOU CLAPPED IN A GAP.
```

Resonance starts at 35. Reaching 100 wins (sets `beat_<boss_id>`, gives the
reward). Hitting 0 just resets to 35 — nobody loses. Rounds loop in order.

## Items

An item is just an id in `items =`. Give it a display name in
`Inventory::displayName` (`src/Inventory.cpp`) or it is shown de-underscored.
Art: `assets/sprites/<id>.png` (32×32), else a generated gem.
