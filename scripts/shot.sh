#!/usr/bin/env bash
# Headless screenshots of rooms (no display or audio device needed).
#   scripts/shot.sh                      every room -> screenshots/<room>.png
#   scripts/shot.sh cow_lounge engine_room
#   OUT=/tmp/shots SIZE=1280x720 BIN=build/trippy scripts/shot.sh garden
#   EXTRA="--pos 7,6 --give third_eye_marble" scripts/shot.sh observation_deck
set -euo pipefail
cd "$(dirname "$0")/.."
BIN="${BIN:-build/trippy}"
OUT="${OUT:-screenshots}"
SIZE="${SIZE:-1280x720}"
FRAMES="${FRAMES:-60}"
[ -x "$BIN" ] || { echo "no $BIN; build first: cmake -B build && cmake --build build"; exit 1; }
mkdir -p "$OUT"
rooms=("$@")
[ ${#rooms[@]} -gt 0 ] || rooms=($(ls assets/maps/*.txt | xargs -n1 basename | sed 's/\.txt$//'))
fail=0
for r in "${rooms[@]}"; do
  # shellcheck disable=SC2086
  if SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$BIN" --room "$r" --screenshot "$OUT/$r.png" \
       --frames "$FRAMES" --size "$SIZE" ${EXTRA:-} > "$OUT/$r.log" 2>&1; then
    echo "ok   $OUT/$r.png"
  else
    echo "FAIL $r (see $OUT/$r.log)"; fail=1
  fi
done
exit $fail
