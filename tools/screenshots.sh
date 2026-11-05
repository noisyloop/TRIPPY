#!/usr/bin/env bash
# Regenerates the README screenshots headlessly (no window, no sound, and the
# save file is never read or written). Run from the repo root after building:
#   cmake -B build && cmake --build build && tools/screenshots.sh
set -euo pipefail
bin=${TRIPPY_BIN:-./build/trippy}
out=docs/screenshots
mkdir -p "$out"
shot() {  # name room [npc] [seconds]
  SDL_VIDEODRIVER=offscreen "$bin" --shot "$2" "$out/$1.png" ${3:+--talk "$3"} --seconds "${4:-2.6}"
}
shot title              title               ""                  3
shot tessellation_temple tessellation_temple npc_tessellator
shot static_motel       static_motel        npc_tv_head
shot melting_ballroom   melting_ballroom    npc_melting_clock
shot milk_ocean         milk_ocean          npc_jelly_oracle
shot mushroom_cathedral mushroom_cathedral  npc_spore_bishop
shot eyeball_orchard    eyeball_orchard     npc_eye_tree
shot mothership_corridor mothership_corridor ""
echo "wrote $(ls "$out" | wc -l) screenshots to $out/"
