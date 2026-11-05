#include "Assets.h"
#include "TripFX.h"

#include <SDL_image.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {

std::string findAssetRoot() {
    for (const char* candidate : {"assets", "../assets"}) {
        if (fs::is_directory(candidate))
            return fs::absolute(candidate).string();
    }
    char* base = SDL_GetBasePath();
    if (base) {
        fs::path p = fs::path(base) / "assets";
        SDL_free(base);
        if (fs::is_directory(p))
            return p.string();
    }
    return "";
}

FontPtr openAnyFont(const std::string& root, int ptSize) {
    // Any TTF the user dropped into assets/fonts wins.
    if (!root.empty() && fs::is_directory(fs::path(root) / "fonts")) {
        for (const auto& entry : fs::directory_iterator(fs::path(root) / "fonts")) {
            auto ext = entry.path().extension().string();
            if (ext == ".ttf" || ext == ".TTF" || ext == ".otf") {
                if (TTF_Font* f = TTF_OpenFont(entry.path().string().c_str(), ptSize))
                    return FontPtr(f);
                std::cerr << "[assets] could not open font " << entry.path()
                          << ": " << TTF_GetError() << "\n";
            }
        }
    }
    // System fallbacks: macOS first (the target platform), then common Linux
    // paths (CI containers), then Windows.
    for (const char* path : {
             "/System/Library/Fonts/Supplemental/Andale Mono.ttf",
             "/System/Library/Fonts/Monaco.ttf",
             "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
             "/usr/share/fonts/TTF/DejaVuSansMono.ttf",  // Arch
             "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
             "C:/Windows/Fonts/consola.ttf",
             "C:/Windows/Fonts/lucon.ttf",
             "C:/Windows/Fonts/cour.ttf",
         }) {
        if (TTF_Font* f = TTF_OpenFont(path, ptSize))
            return FontPtr(f);
    }
    return nullptr;
}

// Stable color from a sprite name so placeholders are consistent run-to-run.
SDL_Color colorForName(const std::string& name) {
    uint32_t h = 2166136261u;
    for (char c : name) { h ^= static_cast<uint8_t>(c); h *= 16777619u; }
    auto channel = [&](int shift) {
        return static_cast<Uint8>(96 + ((h >> shift) & 0x7F));  // keep it bright-ish
    };
    return SDL_Color{channel(0), channel(8), channel(16), 255};
}

// Paints a 16x16 character grid at 2x into a 32x32 surface. '.' = clear.
void paintGrid(SDL_Surface* surf, const char* const rows[16],
               const std::unordered_map<char, SDL_Color>& ink) {
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            auto it = ink.find(rows[y][x]);
            if (it == ink.end()) continue;
            SDL_Rect px{x * 2, y * 2, 2, 2};
            SDL_FillRect(surf, &px, SDL_MapRGBA(surf->format, it->second.r, it->second.g,
                                                it->second.b, it->second.a));
        }
    }
}

// The hero: a little green alien. "player_<dir>_<frame>"; anything else
// starting with "player" gets the down-facing idle pose.
void paintAlien(SDL_Surface* surf, const std::string& name, bool shadow = false) {
    const bool up = name.find("_up") != std::string::npos;
    const bool left = name.find("_left") != std::string::npos;
    const bool right = name.find("_right") != std::string::npos;
    const bool step = !name.empty() && name.back() == '1';

    const char* rows[16] = {
        "...p........p...",
        "....a......a....",
        ".....a....a.....",
        "....gggggggg....",
        "...gggggggggg...",
        "..gggggggggggg..",
        "..gKKKggggKKKg..",
        "..gKWKggggKWKg..",
        "...gKKggggKKg...",
        "....gggggggg....",
        "......gggg......",
        "....rrrrrrrr....",
        "...gbbbybbbbg...",
        "....cccccccc....",
        "....gg....gg....",
        "....gg....gg....",
    };
    if (up) {  // back of the head: no face, a stripe of spots instead
        rows[6] = "..gggggggggggg..";
        rows[7] = "..gggyggggyggg..";
        rows[8] = "...gggggggggg...";
    } else if (left) {
        rows[6] = "..KKKgggggggg...";
        rows[7] = "..KWKgggggggg...";
        rows[8] = "...KKggggggg....";
    } else if (right) {
        rows[6] = "...ggggggggKKK..";
        rows[7] = "...ggggggggKWK..";
        rows[8] = "....gggggggKK...";
    }
    if (step) {
        rows[14] = "....gg.....gg...";
        rows[15] = "...gg......gg...";
    }
    if (shadow) {  // the Weight: you, but heavy, dark and staring
        paintGrid(surf, rows, {
            {'g', {58, 30, 92, 255}}, {'K', {255, 240, 255, 255}}, {'W', {255, 80, 200, 255}},
            {'a', {90, 60, 130, 255}}, {'p', {255, 255, 255, 255}}, {'b', {30, 16, 52, 255}},
            {'y', {255, 240, 255, 255}}, {'r', {40, 20, 66, 255}}, {'c', {40, 20, 66, 255}},
        });
        return;
    }
    paintGrid(surf, rows, {
        {'g', {120, 255, 150, 255}}, {'K', {12, 6, 30, 255}}, {'W', {255, 255, 255, 255}},
        {'a', {200, 255, 210, 255}}, {'p', {255, 80, 220, 255}}, {'b', {150, 70, 255, 255}},
        {'y', {255, 240, 90, 255}}, {'r', {255, 70, 160, 255}}, {'c', {70, 230, 255, 255}},
    });
}

// Saucer NPCs ("ufo_*"): hull color hashed from the name.
void paintSaucer(SDL_Surface* surf, SDL_Color hull) {
    const char* rows[16] = {
        "................",
        "................",
        "................",
        "......cccc......",
        ".....cWcccc.....",
        ".....cccccc.....",
        "...mmmmmmmmmm...",
        ".mmmmmmmmmmmmmm.",
        "mmYmmYmmYmmYmmYm",
        ".mmmmmmmmmmmmmm.",
        "...dddddddddd...",
        "......tttt......",
        ".....tttttt.....",
        "....tttttttt....",
        "...tttttttttt...",
        "................",
    };
    SDL_Color dark{static_cast<Uint8>(hull.r / 2), static_cast<Uint8>(hull.g / 2),
                   static_cast<Uint8>(hull.b / 2), 255};
    paintGrid(surf, rows, {
        {'m', hull}, {'d', dark}, {'c', {140, 255, 255, 220}}, {'W', {255, 255, 255, 255}},
        {'Y', {255, 250, 120, 255}}, {'t', {200, 255, 120, 70}},
    });
}

constexpr double kPi = 3.14159265358979323846;

void putPixel(SDL_Surface* surf, int x, int y, SDL_Color c) {
    SDL_Rect px{x, y, 1, 1};
    SDL_FillRect(surf, &px, SDL_MapRGBA(surf->format, c.r, c.g, c.b, c.a));
}

// Distance from p to segment ab.
double segDist(double px, double py, double ax, double ay, double bx, double by) {
    const double dx = bx - ax, dy = by - ay;
    double t = ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy);
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(px - (ax + t * dx), py - (ay + t * dy));
}

// Terrain is drawn in grayscale sacred geometry; the game tints it with a
// hue-cycled color every frame (SDL_SetTextureColorMod), so white = tint.
void paintTile(SDL_Surface* surf, const std::string& name) {
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            const double fx = x + 0.5, fy = y + 0.5;
            const double cx = fx - 16, cy = fy - 16, r = std::hypot(cx, cy);
            double v = 0.45;
            if (name == "tile_floor") {
                // Seed-of-life lattice: circles on corners + center, seamless.
                double ring = 99;
                static const int centers[][2] = {{0, 0}, {32, 0}, {0, 32}, {32, 32}, {16, 16},
                                                 {16, -16}, {16, 48}, {-16, 16}, {48, 16}};
                for (const auto& o : centers)
                    ring = std::min(ring, std::fabs(std::hypot(fx - o[0], fy - o[1]) - 16));
                v = 0.42 + 0.58 * std::max(0.0, 1 - ring / 1.2);
                if (r < 1.6) v = 1;
            } else if (name == "tile_wall") {
                // Hexagram inside a square frame.
                double d = 99;
                const double up[3][2] = {{16, 3}, {28.5, 24.5}, {3.5, 24.5}};
                const double dn[3][2] = {{16, 29}, {3.5, 7.5}, {28.5, 7.5}};
                for (int i = 0; i < 3; ++i) {
                    d = std::min(d, segDist(fx, fy, up[i][0], up[i][1], up[(i + 1) % 3][0], up[(i + 1) % 3][1]));
                    d = std::min(d, segDist(fx, fy, dn[i][0], dn[i][1], dn[(i + 1) % 3][0], dn[(i + 1) % 3][1]));
                }
                v = 0.25 + 0.6 * std::max(0.0, 1 - d / 1.1);
                if (x < 1 || y < 1 || x > 30 || y > 30) v = 0.7;
            } else if (name == "tile_grass") {
                // Golden-ish spiral arms.
                double theta = std::atan2(cy, cx);
                double arm = std::sin(3 * theta + 1.7 * std::log(r + 1) * 3);
                v = 0.4 + 0.45 * std::max(0.0, arm);
            } else if (name == "tile_void") {
                // Deep space: faint rings falling inward plus a few fixed stars.
                v = 0.04 + 0.2 * std::max(0.0, std::sin(r * 1.1)) * std::max(0.0, 1 - r / 18);
                uint32_t hsh = static_cast<uint32_t>(x * 73856093 ^ y * 19349663);
                if (hsh % 61 == 0) v = 1.0;
            } else if (name == "tile_mirror") {
                // Polished glass: bright with diagonal shine streaks.
                double streak = std::fmod(fx + fy, 16.0);
                v = 0.72 + (streak < 2.5 ? 0.28 : 0.0) + (std::fabs(fx - fy) < 0.8 ? 0.15 : 0.0);
                if (x < 1 || y < 1) v = 1.0;
            } else if (name == "tile_warp") {
                // Concentric diamonds converging on the center.
                double m = std::fabs(cx) + std::fabs(cy);
                v = std::fmod(m, 6.0) < 2.0 ? 1.0 : 0.3;
                if (m < 3) v = 1.0;
            } else if (name == "tile_stairs") {
                // Steps: a lit edge every 8px, shading toward the back of each tread.
                int tread = y % 8;
                v = tread < 2 ? 0.95 : 0.45 + 0.05 * tread;
                if (x < 1 || x > 30) v = 0.3;
            } else {  // tile_door: vesica piscis eye with a pupil and rays
                double a = std::hypot(fx - 10, fy - 16), b = std::hypot(fx - 22, fy - 16);
                bool lens = a < 12 && b < 12;
                v = lens ? 0.95 : 0.35 + 0.3 * std::max(0.0, std::sin(std::atan2(cy, cx) * 8));
                if (std::fabs(a - 12) < 0.8 || std::fabs(b - 12) < 0.8) v = 1;
                if (r < 4) v = 0.1;
                if (r < 1.5) v = 1;
            }
            putPixel(surf, x, y, hsvColor(0, 0, v));
        }
    }
}

// An eye of radius r at (ex, ey): white, a colored iris, a dark pupil and a
// glint. Sub-pixel centers are fine; everything is drawn per pixel.
void paintEye(SDL_Surface* surf, double ex, double ey, double r, double irisHue) {
    for (int y = static_cast<int>(ey - r - 1); y <= static_cast<int>(ey + r + 1); ++y)
        for (int x = static_cast<int>(ex - r - 1); x <= static_cast<int>(ex + r + 1); ++x) {
            if (x < 0 || y < 0 || x > 31 || y > 31) continue;
            const double d = std::hypot(x + 0.5 - ex, y + 0.5 - ey);
            if (d > r) continue;
            SDL_Color c{255, 255, 255, 255};
            if (d < r * 0.62) c = hsvColor(irisHue + d * 40, 0.9, 1.0);
            if (d < r * 0.3) c = SDL_Color{10, 4, 24, 255};
            if (r >= 2 && std::hypot(x + 0.5 - (ex - r * 0.35), y + 0.5 - (ey - r * 0.35)) < 0.8)
                c = SDL_Color{255, 255, 255, 255};
            putPixel(surf, x, y, c);
        }
}

uint32_t nameHash(const std::string& name) {
    uint32_t h = 2166136261u;
    for (char c : name) { h ^= static_cast<uint8_t>(c); h *= 16777619u; }
    return h;
}

// Which kind of being an NPC id becomes. Keywords in the id pick a shape so
// cows read as cows and books as books; everyone else is hashed.
enum class Being { Mandala, EyeCluster, Jelly, Melting, Prism, Spiral, Cow, Screen, Mushroom, Book };

Being beingFor(const std::string& name, uint32_t h) {
    auto has = [&](const char* k) { return name.find(k) != std::string::npos; };
    if (has("cow")) return Being::Cow;
    if (has("tv") || has("screen") || has("channel") || has("ice_machine")) return Being::Screen;
    if (has("spore") || has("mushroom") || has("myco") || has("fung")) return Being::Mushroom;
    if (has("book") || has("librar") || has("word")) return Being::Book;
    if (has("jelly") || has("drown") || has("sax")) return Being::Jelly;
    if (has("eye") || has("blink")) return Being::EyeCluster;
    if (has("clock") || has("melt") || has("wallpaper")) return Being::Melting;
    if (has("prism") || has("tessel") || has("gear") || has("angel")) return Being::Prism;
    if (has("spiral") || has("sock") || has("monk") || has("stair")) return Being::Spiral;
    static const Being pool[] = {Being::Mandala, Being::EyeCluster, Being::Jelly,
                                 Being::Melting, Being::Prism, Being::Spiral};
    return pool[(h >> 3) % 6];
}

// NPC placeholder: one of ten strange beings, hues and eye counts from the
// name hash, so each one is distinct but stable run to run. They are drawn
// fully saturated; the game wobbles, hue-shifts and haloes them at draw time.
void paintBeing(SDL_Surface* surf, const std::string& name) {
    const uint32_t h = nameHash(name);
    const double hue = (h >> 8) % 360;
    const double hue2 = hue + 120 + (h >> 20) % 90;
    auto px = [&](int x, int y, SDL_Color c) { if (x >= 0 && y >= 0 && x < 32 && y < 32) putPixel(surf, x, y, c); };

    switch (beingFor(name, h)) {
    case Being::Mandala: {
        // Radial petals, a ring of inner petals and 3-7 eyes in a halo.
        const int petals = 5 + static_cast<int>(h % 5);
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                const double cx = x + 0.5 - 16, cy = y + 0.5 - 16;
                const double r = std::hypot(cx, cy), th = std::atan2(cy, cx);
                const double edge = 10.5 + 4.5 * std::cos(petals * th);
                if (r > edge) continue;
                const double inner = 5.5 + 2.0 * std::cos(petals * 2 * th + 1.0);
                SDL_Color c = r < inner ? hsvColor(hue2 + r * 30, 0.9, 1.0)
                                        : hsvColor(hue + r * 14, 0.85, edge - r < 1.2 ? 1.0 : 0.75 + 0.25 * std::cos(r));
                px(x, y, c);
            }
        const int eyes = 3 + static_cast<int>((h >> 5) % 5);
        for (int i = 0; i < eyes; ++i) {
            const double a = -kPi / 2 + i * 2 * kPi / eyes;
            paintEye(surf, 16 + 6 * std::cos(a), 16 + 6 * std::sin(a), 2.1, hue2 + i * 50);
        }
        paintEye(surf, 16, 16, 2.6, hue + 180);
        break;
    }
    case Being::EyeCluster: {
        // A lumpy blob covered in eyes of every size, none looking the same way.
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                const double cx = x + 0.5 - 16, cy = y + 0.5 - 17;
                const double th = std::atan2(cy, cx);
                const double edge = 12 + 1.8 * std::sin(th * 5 + (h % 7)) + 1.2 * std::sin(th * 9);
                const double r = std::hypot(cx, cy);
                if (r < edge) px(x, y, hsvColor(hue + r * 8, 0.8, 0.55 + 0.35 * (1 - r / edge)));
            }
        uint32_t s = h | 1u;
        for (int i = 0; i < 9; ++i) {
            s = s * 1664525u + 1013904223u;
            const double a = (s >> 8) % 628 / 100.0, rr = (s >> 4) % 9;
            const double size = 1.4 + (s >> 20) % 3;
            paintEye(surf, 16 + rr * std::cos(a), 17 + rr * std::sin(a), size, hue2 + i * 37);
        }
        break;
    }
    case Being::Jelly: {
        // Translucent dome, glowing rim, tentacles that wave (at draw time
        // the whole sprite also wobbles, which sells it).
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                const double cx = x + 0.5 - 16, cy = y + 0.5 - 14;
                const double r = std::hypot(cx, cy * 1.3);
                if (cy < 2 && r < 12) {
                    const bool rim = r > 10.5 || std::fabs(cy - 1.5) < 0.6;
                    px(x, y, hsvColor(hue + r * 6, rim ? 0.4 : 0.7, 1.0, rim ? 255 : 150));
                }
            }
        for (int t = 0; t < 6; ++t) {
            const double baseX = 6.5 + t * 3.8;
            for (int y = 16; y < 32; ++y) {
                const double x = baseX + 1.8 * std::sin(y * 0.55 + t * 1.3);
                px(static_cast<int>(x), y, hsvColor(hue2 + y * 9, 0.75, 1.0, static_cast<Uint8>(255 - (y - 16) * 10)));
            }
        }
        paintEye(surf, 12, 9, 2.0, hue2);
        paintEye(surf, 20, 9, 2.0, hue2 + 60);
        paintEye(surf, 16, 5, 1.5, hue2 + 120);
        break;
    }
    case Being::Melting: {
        // A beaming face that is sliding down off its own head.
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                const double cx = x + 0.5 - 16, cy = y + 0.5 - 12;
                const double r = std::hypot(cx, cy);
                const double drip = 12 + 9 * std::max(0.0, std::sin(x * 1.1 + (h % 5))) * std::max(0.0, std::sin(x * 0.37));
                const bool inFace = r < 10 || (std::fabs(cx) < 9.5 && cy > 0 && cy < drip);
                if (!inFace) continue;
                px(x, y, hsvColor(hue + y * 6, 0.85, 0.95 - std::max(0.0, cy) * 0.012));
            }
        paintEye(surf, 11.5, 10, 2.4, hue2);
        paintEye(surf, 20.5, 11, 2.0, hue2 + 90);
        for (int x = 9; x <= 23; ++x) {  // a smile that has gone a bit wavy
            const int y = static_cast<int>(16 + 2.5 * std::sin((x - 9) / 14.0 * kPi) + std::sin(x * 1.3));
            px(x, y, SDL_Color{20, 6, 30, 255});
            px(x, y + 1, SDL_Color{255, 90, 180, 255});
        }
        break;
    }
    case Being::Prism: {
        // A rotating-looking diamond of nested triangles with a crown of eyes.
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                const double cx = x + 0.5 - 16, cy = y + 0.5 - 18;
                const double m = std::fabs(cx) * 1.2 + std::fabs(cy);
                if (m > 13) continue;
                const int band = static_cast<int>(m / 2.2);
                px(x, y, hsvColor(hue + band * 47 + (cx > 0 ? 20 : 0), 0.85, band % 2 ? 1.0 : 0.65));
            }
        for (int i = 0; i < 5; ++i)
            paintEye(surf, 6 + i * 5, 3.5 + (i % 2) * 1.5, 1.7, hue2 + i * 72);
        paintEye(surf, 16, 18, 3.0, hue + 180);
        break;
    }
    case Being::Spiral: {
        // A snail-shell galaxy with one enormous eye at its center.
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                const double cx = x + 0.5 - 16, cy = y + 0.5 - 16;
                const double r = std::hypot(cx, cy), th = std::atan2(cy, cx);
                if (r > 14) continue;
                const double arm = std::sin(th * 2 - r * 0.9 + (h % 6));
                px(x, y, hsvColor(hue + r * 18 + arm * 30, 0.85, arm > 0 ? 1.0 : 0.45));
            }
        paintEye(surf, 16, 16, 4.5, hue2);
        break;
    }
    case Being::Cow: {
        // A cow with a third eye and spots that are tiny galaxies.
        auto fill = [&](int x, int y, int w, int hh, SDL_Color c) {
            SDL_Rect rr{x, y, w, hh};
            SDL_FillRect(surf, &rr, SDL_MapRGBA(surf->format, c.r, c.g, c.b, c.a));
        };
        fill(5, 10, 22, 14, {245, 240, 250, 255});        // body
        fill(7, 24, 3, 6, {230, 225, 240, 255});
        fill(22, 24, 3, 6, {230, 225, 240, 255});
        fill(9, 3, 14, 10, {245, 240, 250, 255});         // head
        fill(7, 2, 3, 3, hsvColor(hue2, 0.8, 1.0));       // horns
        fill(22, 2, 3, 3, hsvColor(hue2, 0.8, 1.0));
        fill(11, 10, 10, 4, hsvColor(hue + 330, 0.45, 1.0));  // snout
        uint32_t s = h | 1u;
        for (int i = 0; i < 5; ++i) {
            s = s * 1664525u + 1013904223u;
            const int sx = 7 + static_cast<int>((s >> 8) % 17), sy = 14 + static_cast<int>((s >> 16) % 8);
            for (int y = -2; y <= 2; ++y)
                for (int x = -2; x <= 2; ++x)
                    if (x * x + y * y <= 5) px(sx + x, sy + y, hsvColor(hue + i * 60 + (x + y) * 20, 0.9, 0.9));
        }
        paintEye(surf, 12.5, 7, 1.6, hue2);
        paintEye(surf, 19.5, 7, 1.6, hue2);
        paintEye(surf, 16, 4.5, 1.4, hue + 180);
        break;
    }
    case Being::Screen: {
        // An old TV for a head showing a color-bar sunrise and one eye.
        auto fill = [&](int x, int y, int w, int hh, SDL_Color c) {
            SDL_Rect rr{x, y, w, hh};
            SDL_FillRect(surf, &rr, SDL_MapRGBA(surf->format, c.r, c.g, c.b, c.a));
        };
        fill(4, 6, 24, 18, {60, 40, 80, 255});
        for (int i = 0; i < 7; ++i) fill(6 + i * 3, 8, 3, 14, hsvColor(i * 51 + hue, 0.85, 1.0));
        fill(9, 1, 2, 6, {200, 200, 220, 255});           // antennae
        fill(21, 1, 2, 6, {200, 200, 220, 255});
        fill(12, 24, 8, 4, {60, 40, 80, 255});             // neck
        fill(8, 27, 16, 5, hsvColor(hue2, 0.6, 0.8));      // shoulders
        paintEye(surf, 16, 15, 4.0, hue2);
        break;
    }
    case Being::Mushroom: {
        // A spotted cap the size of a cathedral bell over a pale stem with eyes.
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x) {
                const double cx = x + 0.5 - 16, cy = y + 0.5 - 13;
                const double r = std::hypot(cx, cy * 1.6);
                if (cy < 1 && r < 15) {
                    const bool spot = std::sin(x * 1.3) * std::sin(y * 1.7 + 0.5) > 0.55;
                    px(x, y, spot ? SDL_Color{255, 250, 235, 255} : hsvColor(hue + r * 5, 0.9, 0.95));
                } else if (cy >= 1 && std::fabs(cx) < 5 + cy * 0.1 && y < 31) {
                    px(x, y, hsvColor(hue2, 0.15, 0.95));
                }
            }
        paintEye(surf, 13.5, 19, 1.7, hue2);
        paintEye(surf, 18.5, 19, 1.7, hue2 + 40);
        break;
    }
    case Being::Book: {
        // An open book with an eye on each page and a tongue for a bookmark.
        for (int y = 6; y < 27; ++y)
            for (int x = 2; x < 30; ++x) {
                const double sag = std::fabs(x + 0.5 - 16) * 0.25;
                if (y < 6 + 4 - sag || y > 26 - sag) continue;
                const bool line = (y % 3 == 0) && x % 16 > 2 && x % 16 < 13;
                px(x, y, line ? hsvColor(hue + x * 10, 0.6, 0.6) : SDL_Color{250, 245, 235, 255});
            }
        for (int y = 6; y < 27; ++y) px(15, y, hsvColor(hue, 0.8, 0.5)), px(16, y, hsvColor(hue, 0.8, 0.5));
        for (int y = 24; y < 31; ++y) px(16, y, SDL_Color{255, 80, 140, 255}), px(17, y, SDL_Color{255, 80, 140, 255});
        paintEye(surf, 9, 15, 3.2, hue2);
        paintEye(surf, 23, 15, 3.2, hue2 + 120);
        break;
    }
    }
}

// Item placeholder: glowing hexagon with an inscribed star (Metatron-ish).
void paintGem(SDL_Surface* surf, const std::string& name) {
    const double hue = colorForName(name).r * 1.4;
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            const double fx = x + 0.5, fy = y + 0.5;
            const double r = std::hypot(fx - 16, fy - 16);
            double hex[6][2];
            for (int i = 0; i < 6; ++i) {
                hex[i][0] = 16 + 14 * std::cos(i * kPi / 3 + kPi / 6);
                hex[i][1] = 16 + 14 * std::sin(i * kPi / 3 + kPi / 6);
            }
            double d = 99;
            for (int i = 0; i < 6; ++i) {
                d = std::min(d, segDist(fx, fy, hex[i][0], hex[i][1], hex[(i + 1) % 6][0], hex[(i + 1) % 6][1]));
                d = std::min(d, segDist(fx, fy, hex[i][0], hex[i][1], hex[(i + 2) % 6][0], hex[(i + 2) % 6][1]));
            }
            if (d < 1.0)
                putPixel(surf, x, y, hsvColor(hue + r * 10, 0.6, 1.0));
            else if (r < 6)
                putPixel(surf, x, y, hsvColor(hue + 180, 0.7, 1.0, static_cast<Uint8>(255 - r * 30)));
        }
    }
}

// Bosses get their own shapes; the game draws them 2x in rooms and huge in duels.
void paintBoss(SDL_Surface* surf, const std::string& name) {
    if (name.find("saucer") != std::string::npos) {
        // Mother-of-Saucers: a golden saucer wearing a crown of eyes.
        paintSaucer(surf, SDL_Color{255, 190, 80, 255});
        for (int i = 0; i < 5; ++i) {
            SDL_Rect spike{6 + i * 5, 2 + (i % 2) * 2, 2, 5};
            SDL_FillRect(surf, &spike, SDL_MapRGBA(surf->format, 255, 90, 220, 255));
        }
        for (int ex : {12, 16, 20}) {
            SDL_Rect eye{ex - 1, 8, 3, 3}, pupil{ex, 9, 1, 1};
            SDL_FillRect(surf, &eye, SDL_MapRGBA(surf->format, 255, 255, 255, 255));
            SDL_FillRect(surf, &pupil, SDL_MapRGBA(surf->format, 10, 4, 24, 255));
        }
        return;
    }
    if (name.find("weight") != std::string::npos) {
        paintAlien(surf, "player_down_0", /*shadow=*/true);
        return;
    }
    if (name.find("choir") != std::string::npos) {
        // Three overlapping mandala heads, each with a different petal count.
        static const int heads[3][3] = {{10, 13, 5}, {22, 13, 7}, {16, 21, 6}};
        for (const auto& hd : heads)
            for (int y = 0; y < 32; ++y)
                for (int x = 0; x < 32; ++x) {
                    double cx = x + 0.5 - hd[0], cy = y + 0.5 - hd[1];
                    double r = std::hypot(cx, cy), th = std::atan2(cy, cx);
                    double edge = 6.5 + 2.5 * std::cos(hd[2] * th);
                    if (r > edge) continue;
                    putPixel(surf, x, y, hsvColor(hd[2] * 50 + r * 25, 0.8, edge - r < 1 ? 1.0 : 0.8));
                }
        for (const auto& hd : heads) {
            SDL_Rect eye{hd[0] - 1, hd[1] - 1, 3, 3}, pupil{hd[0], hd[1], 1, 1};
            SDL_FillRect(surf, &eye, SDL_MapRGBA(surf->format, 255, 255, 255, 255));
            SDL_FillRect(surf, &pupil, SDL_MapRGBA(surf->format, 10, 4, 24, 255));
        }
        return;
    }
    // Customs Officer: a filing-cabinet head with a grid of nine eyes,
    // a peaked cap and a big rubber stamp.
    auto fill = [&](int x, int y, int w, int h, SDL_Color c) {
        SDL_Rect rr{x, y, w, h};
        SDL_FillRect(surf, &rr, SDL_MapRGBA(surf->format, c.r, c.g, c.b, c.a));
    };
    fill(7, 2, 18, 4, {40, 60, 150, 255});     // cap
    fill(5, 5, 22, 2, {20, 30, 90, 255});      // brim
    fill(7, 7, 18, 16, {150, 160, 175, 255});  // head
    for (int i = 0; i < 9; ++i) {
        int ex = 10 + (i % 3) * 5, ey = 9 + (i / 3) * 4;
        fill(ex, ey, 3, 3, {255, 255, 255, 255});
        fill(ex + 1, ey + 1, 1, 1, {10, 4, 24, 255});
    }
    fill(9, 23, 14, 7, {40, 60, 150, 255});    // uniform
    fill(24, 18, 4, 6, {120, 70, 40, 255});    // stamp handle
    fill(22, 24, 8, 3, {230, 40, 80, 255});    // stamp
}

}  // namespace

Assets::Assets(Renderer& renderer) : renderer_(renderer), root_(findAssetRoot()) {
    if (root_.empty())
        std::cerr << "[assets] WARNING: no assets/ directory found; everything will be a placeholder\n";
    font_ = openAnyFont(root_, 18);
    if (!font_)
        throw std::runtime_error(
            "No usable font. Drop a .ttf into assets/fonts/ "
            "(fallback /System/Library/Fonts/Supplemental/Andale Mono.ttf was also unavailable).");
}

SDL_Texture* Assets::sprite(const std::string& name) {
    if (auto it = sprites_.find(name); it != sprites_.end())
        return it->second.get();

    SDL_Texture* tex = nullptr;
    if (!root_.empty()) {
        fs::path png = fs::path(root_) / "sprites" / (name + ".png");
        if (fs::exists(png)) {
            tex = IMG_LoadTexture(renderer_.sdl(), png.string().c_str());
            if (!tex)
                std::cerr << "[assets] failed to load " << png << ": " << IMG_GetError() << "\n";
        }
    }
    bool isPlaceholder = (tex == nullptr);
    if (!tex)
        tex = makePlaceholder(name);

    placeholder_[name] = isPlaceholder;
    sprites_[name] = TexturePtr(tex);
    return tex;
}

bool Assets::spriteIsPlaceholder(const std::string& name) {
    sprite(name);  // ensure loaded
    return placeholder_[name];
}

SDL_Texture* Assets::makePlaceholder(const std::string& name) {
    constexpr int kSize = 32;
    SurfacePtr surf(SDL_CreateRGBSurfaceWithFormat(0, kSize, kSize, 32, SDL_PIXELFORMAT_RGBA32));
    const SDL_Color body = colorForName(name);

    // Pixel-art placeholders for the cast that needs to read as a shape.
    if (name.rfind("player", 0) == 0 || name.rfind("ufo_", 0) == 0 || name.rfind("boss_", 0) == 0) {
        SDL_FillRect(surf.get(), nullptr, SDL_MapRGBA(surf->format, 0, 0, 0, 0));
        if (name[0] == 'p') paintAlien(surf.get(), name);
        else if (name[0] == 'b') paintBoss(surf.get(), name);
        else paintSaucer(surf.get(), body);
        return SDL_CreateTextureFromSurface(renderer_.sdl(), surf.get());
    }
    if (name.rfind("tile_", 0) == 0) {
        paintTile(surf.get(), name);
        return SDL_CreateTextureFromSurface(renderer_.sdl(), surf.get());
    }
    // NPCs become radial beings; everything else (items) a sacred-geometry gem.
    SDL_FillRect(surf.get(), nullptr, SDL_MapRGBA(surf->format, 0, 0, 0, 0));
    if (name.rfind("npc_", 0) == 0) paintBeing(surf.get(), name);
    else paintGem(surf.get(), name);
    return SDL_CreateTextureFromSurface(renderer_.sdl(), surf.get());
}

SDL_Texture* Assets::textTexture(const std::string& text, SDL_Color c) {
    std::string key = text + "\x1f" + std::to_string(c.r) + "," + std::to_string(c.g) + ","
                      + std::to_string(c.b) + "," + std::to_string(c.a);
    if (auto it = textCache_.find(key); it != textCache_.end())
        return it->second.get();

    // Unbounded caches leak in long sessions; UI text churns (typewriter).
    if (textCache_.size() > 512)
        textCache_.clear();

    SurfacePtr surf(TTF_RenderUTF8_Blended(font_.get(), text.c_str(), c));
    if (!surf) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_.sdl(), surf.get());
    textCache_[key] = TexturePtr(tex);
    return tex;
}

int Assets::measureText(const std::string& text) {
    int w = 0, h = 0;
    TTF_SizeUTF8(font_.get(), text.c_str(), &w, &h);
    return w;
}

Mix_Chunk* Assets::sfx(const std::string& name) {
    if (auto it = sfx_.find(name); it != sfx_.end())
        return it->second.get();
    Mix_Chunk* chunk = nullptr;
    if (!root_.empty()) {
        fs::path wav = fs::path(root_) / "sfx" / (name + ".wav");
        if (fs::exists(wav))
            chunk = Mix_LoadWAV(wav.string().c_str());
    }
    sfx_[name] = ChunkPtr(chunk);
    return chunk;
}
