#pragma once

#include <string>
#include <vector>

// Command-line options for building and testing content quickly:
//
//   trippy --room observation_deck --pos 7,6 --give third_eye_marble
//          --flag beat_boss_choir --skip-title --calm
//   trippy --room engine_room --screenshot shot.png --frames 45 --size 1280x720
//
// Any of --room/--pos/--give/--flag/--fresh starts from a fresh state (the
// save file is neither read nor written), so testing never clobbers a real
// playthrough. --screenshot implies --skip-title and quits after saving the
// PNG; with SDL_VIDEODRIVER=dummy it needs no display at all.
struct DevOptions {
    std::string room;
    int tileX = -1, tileY = -1;
    std::vector<std::string> give, flags;
    bool fresh = false;      // ignore + don't write the save file
    bool skipTitle = false;
    bool calm = false;
    std::string screenshot;  // write a PNG after `frames` frames, then quit
    int frames = 45;
    int windowW = 0, windowH = 0;  // 0 = default (85% of the desktop)

    // Returns false (and prints usage) on bad input or --help.
    bool parse(int argc, char** argv);
};
