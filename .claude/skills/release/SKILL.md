---
name: release
description: Ship TRIPPY - validate, push the branch, open a PR to main, watch CI (content, macOS, Windows), and after merge confirm the GitHub release and README download links. Use when asked to release, ship, publish, or make a new version.
argument-hint: "[short summary of what's in it]"
---

# Release

Every push to `main` builds and publishes `v0.<run number>` with
`TRIPPY-macos.dmg` and `TRIPPY-windows.zip` (`.github/workflows/build.yml`).
A `v*` tag releases under its own name.

1. `python3 scripts/check_content.py` → 0 errors. Build locally and run
   `scripts/shot.sh` for any rooms touched since the last release; look at
   them.
2. Commit everything and push the working branch (never push to `main`
   directly). If the previous PR from this branch was already merged,
   restart the branch from `origin/main` first.
3. Open a PR to `main` (GitHub MCP `create_pull_request`): summary grouped
   by area (rooms, NPCs, bosses, engine), what was tested and how, anything
   not tested. Only open it when the user asked for a release/PR.
4. Watch CI on the PR head: the `content` job (checker + room screenshots
   artifact), `macOS app (.dmg)`, `Windows (.exe zip)`. On red, read the
   failed job log, reproduce locally (CMake 4 is on the runners: test
   vendored builds with a CMake 4 binary if CMake itself complains), fix,
   push. A failure before checkout/build (runner network) is not the code;
   say so.
5. After the user merges: confirm the `main` run's `release` job passed and
   the release has both files, e.g.
   `https://github.com/noisyloop/TRIPPY/releases/latest/download/TRIPPY-macos.dmg`.
6. Tell the user the version, the release link, and the one thing worth
   trying first.
