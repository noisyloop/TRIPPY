#!/usr/bin/env bash
# Drive the game with scripted input and capture what happens (Linux: needs
# Xvfb, xdotool and ImageMagick's `import`). For interactions screenshots
# alone can't show: talking, duels, doors, warps, the beam.
#
#   scripts/drive.sh "--room observation_deck --pos 7,6" hold:Up:0.4 tap:e wait:1 shot:deck.png tap:e shot:duel.png
#
# Steps:  hold:<key>:<secs>   tap:<key>   wait:<secs>   shot:<file.png>
# Keys are xdotool names: Up Down Left Right e space i c F5 Return ...
# The first argument is passed to trippy (add --skip-title to skip the title;
# it is added automatically). Window is 1280x720 unless SIZE=WxH is set.
set -euo pipefail
cd "$(dirname "$0")/.."
BIN="${BIN:-build/trippy}"
SIZE="${SIZE:-1280x720}"
ARGS="$1"; shift
for tool in Xvfb xdotool import; do
  command -v $tool >/dev/null || { echo "needs $tool (apt-get install xvfb xdotool imagemagick)"; exit 1; }
done
DISP=":$((90 + RANDOM % 9))"
Xvfb "$DISP" -screen 0 "${SIZE}x24" >/dev/null 2>&1 &
XP=$!
trap 'kill $GP $XP 2>/dev/null || true' EXIT
sleep 1
# shellcheck disable=SC2086
DISPLAY="$DISP" SDL_AUDIODRIVER=dummy "$BIN" $ARGS --skip-title --fresh --size "$SIZE" > drive.log 2>&1 &
GP=$!
sleep 1.5
WID=$(DISPLAY="$DISP" xdotool search --pid $GP | tail -1)
for step in "$@"; do
  IFS=: read -r kind a b <<< "$step"
  case "$kind" in
    hold) DISPLAY="$DISP" xdotool keydown "$a"; sleep "$b"; DISPLAY="$DISP" xdotool keyup "$a" ;;
    # A tap is held ~80ms so the game sees it for at least one frame.
    tap)  DISPLAY="$DISP" xdotool keydown "$a"; sleep 0.08; DISPLAY="$DISP" xdotool keyup "$a"; sleep 0.2 ;;
    wait) sleep "$a" ;;
    shot) DISPLAY="$DISP" import -window "$WID" "$a"; echo "shot $a" ;;
    *) echo "unknown step $step"; exit 1 ;;
  esac
done
kill -0 $GP 2>/dev/null || { echo "game exited early:"; cat drive.log; exit 1; }
grep -iE "error|fatal|unknown|missing" drive.log || true
