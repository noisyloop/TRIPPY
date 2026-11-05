# Security review

A review of TRIPPY, a single-player SDL2 game (~5,500 lines of C++). For a game, the attack surface
isn't a network or privileges — it's **memory safety** and **the files the game parses**, since a
player might open a save file or a room pack from someone else. Scope: the save-file parser, the
room/meta loader, the audio callback, and the procedural-asset code.

## Summary

The C++ is clean. It uses `std::string`/`std::vector` throughout, bounds-checks tile access, and
has none of the classic landmines (`strcpy`, `sprintf`, raw `memcpy`, manual `new[]`). Two
low-severity issues with untrusted input are fixed in this change; nothing else needed changing.

| Area | State |
|---|---|
| Save-file JSON parser | Safe — bounded, no nested recursion, `stod` guarded |
| Tile indexing (`Room::tileAt`) | Safe — out-of-range returns `Wall` |
| Audio callback / synth render | Safe — writes exactly `len/2` samples |
| Procedural textures (`putPixel`) | Safe — goes through `SDL_FillRect`, which clips |
| **Room name → file path** | **Fixed: now validated** |
| **Save coordinates → `int` cast** | **Fixed: now validated** |

## What's done well

- **The save parser is defensive.** The hand-written JSON reader is bounded by the input length at
  every step, doesn't recurse into nested objects (so no stack-overflow from deep input), wraps
  `std::stod` in try/catch, and unknown keys are skipped for forward compatibility. A corrupt save
  is reported and the game starts fresh rather than crashing.
- **Tile access can't go out of bounds.** `Room::tileAt` returns `Tile::Wall` for any coordinate
  outside the grid, so even a garbage player position can't cause an out-of-bounds read.
- **The audio callback is correctly sized.** `musicHook` renders exactly `len / 2` samples into the
  mixer's buffer (mono S16), and the synth's render loop writes exactly that many.
- **Procedural art is clip-safe.** `putPixel` draws through `SDL_FillRect`, which clips to the
  surface, so out-of-range pixel coordinates are dropped rather than written past the buffer.

## Findings and fixes

### Fixed — room name was used as a file path unchecked (`src/RoomLoader.cpp`)

`RoomLoader::load(name)` built `assets/maps/<name>.txt` directly from `name`. That name comes from
the save file's `room` field and from `transitions` / `void_to` values in `.meta` files — both of
which could arrive in a save or room pack from elsewhere. A name like `../../../../etc/hostname`
walked out of the maps folder.

Impact was low (the file is opened read-only and parsed as a tile map; its contents aren't shown or
sent anywhere), but it's a clear footgun. *Fix:* room names are now validated against the
characters real rooms use (`[A-Za-z0-9_-]`, up to 64 chars) and anything else is refused before any
file is opened. Every built-in room passes; verified at runtime that a traversal name is rejected.

### Fixed — unvalidated save coordinates reached an `int` cast (`src/Game.cpp`)

`player_x` / `player_y` were read from the save as doubles and applied with no range check. They are
later cast to `int` for the camera (`static_cast<int>(player_.x())`) and tile lookup (`tileX()`).
Casting a non-finite or out-of-`int`-range `double` to `int` is **undefined behaviour** in C++, so a
hand-edited save with, say, `"player_x": 1e18` was UB. *Fix:* on load, a coordinate that isn't
finite or lies outside the room's pixel bounds is dropped in favour of the room's default spawn.

## Residual / out of scope

- **`.meta` numeric fields** (`trip`, `melt`, …) are already clamped to `0..1`; slot lists and
  directions are validated. No action needed.
- **Real asset files** (PNG/OGG/etc.) are decoded by SDL_image / SDL_mixer; their safety is those
  libraries', and CI links pinned versions.
- This is a single-player offline game with no network and no elevated privileges, so there is no
  remote attack surface to review.
