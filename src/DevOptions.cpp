#include "DevOptions.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {
void usage() {
    std::cout <<
        "usage: trippy [options]\n"
        "  --room <name>         start in assets/maps/<name>.txt\n"
        "  --pos <x>,<y>         start on this tile\n"
        "  --give <item>         add an item (repeatable)\n"
        "  --flag <flag>         set a story flag (repeatable), e.g. beat_boss_choir\n"
        "  --fresh               ignore the save file and don't write it\n"
        "  --skip-title          go straight into the room\n"
        "  --calm                start in calm mode\n"
        "  --screenshot <png>    render, save a PNG after --frames frames, quit\n"
        "  --frames <n>          frames before the screenshot (default 45)\n"
        "  --size <w>x<h>        window size\n";
}
}  // namespace

bool DevOptions::parse(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto value = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
        const char* v = nullptr;
        if (a == "--help" || a == "-h") { usage(); return false; }
        else if (a == "--fresh") fresh = true;
        else if (a == "--skip-title") skipTitle = true;
        else if (a == "--calm") calm = true;
        else if (a == "--room" && (v = value())) { room = v; fresh = true; }
        else if (a == "--give" && (v = value())) { give.push_back(v); fresh = true; }
        else if (a == "--flag" && (v = value())) { flags.push_back(v); fresh = true; }
        else if (a == "--pos" && (v = value()) && std::sscanf(v, "%d,%d", &tileX, &tileY) == 2) fresh = true;
        else if (a == "--screenshot" && (v = value())) { screenshot = v; skipTitle = true; fresh = true; }
        else if (a == "--frames" && (v = value())) frames = std::max(1, std::atoi(v));
        else if (a == "--size" && (v = value()) && std::sscanf(v, "%dx%d", &windowW, &windowH) == 2) {}
        else if (a.rfind("-psn_", 0) == 0) {}  // macOS Finder process serial number; ignore
        else {
            std::cerr << "unknown or incomplete option: " << a << "\n";
            usage();
            return false;
        }
    }
    return true;
}
