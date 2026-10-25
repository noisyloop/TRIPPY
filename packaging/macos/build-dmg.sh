#!/usr/bin/env bash
# Package a built trippy binary as TRIPPY.app inside a .dmg. Runs on macOS (needs hdiutil).
#   packaging/macos/build-dmg.sh <version> <path/to/trippy> <output.dmg>
set -euo pipefail

VERSION="$1"
BIN="$2"
OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
STAGE="$(mktemp -d)"
APP="$STAGE/dmg/TRIPPY.app"

mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN" "$APP/Contents/MacOS/TRIPPY"
chmod +x "$APP/Contents/MacOS/TRIPPY"
# The game finds assets via SDL_GetBasePath(), which is Contents/Resources/ in a bundle.
cp -R "$ROOT/assets" "$APP/Contents/Resources/assets"
rm -rf "$APP/Contents/Resources/assets/sprites/legacy"
sed "s/VERSION/$VERSION/g" "$HERE/Info.plist" > "$APP/Contents/Info.plist"

# App icon (Flower of Life) from packaging/icon/icon.png. Cosmetic, so a
# failure here does not stop the build.
(
  ICONSET="$STAGE/icon.iconset"
  mkdir -p "$ICONSET"
  for s in 16 32 128 256 512; do
    sips -z $s $s "$ROOT/packaging/icon/icon.png" --out "$ICONSET/icon_${s}x${s}.png" >/dev/null
    sips -z $((s * 2)) $((s * 2)) "$ROOT/packaging/icon/icon.png" --out "$ICONSET/icon_${s}x${s}@2x.png" >/dev/null
  done
  iconutil -c icns -o "$APP/Contents/Resources/icon.icns" "$ICONSET"
) || echo "warning: app icon not generated"

cp "$HERE/README.txt" "$STAGE/dmg/README.txt"
ln -s /Applications "$STAGE/dmg/Applications"

# Ad-hoc signature only (no Apple identity): Apple Silicon refuses to run
# completely unsigned code, and this keeps the bundle's signature consistent.
codesign --force --deep --sign - "$APP"

hdiutil create -volname "TRIPPY" -srcfolder "$STAGE/dmg" -ov -format UDZO "$OUT"
rm -rf "$STAGE"
echo "built $OUT"
