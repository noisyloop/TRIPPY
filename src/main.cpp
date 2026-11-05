#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <iostream>

#include "Game.h"

namespace {

// RAII for the SDL global init/quit pairs so early throws still clean up.
struct SdlRuntime {
    SdlRuntime() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        if (TTF_Init() != 0) {
            SDL_Quit();
            throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
        }
        IMG_Init(IMG_INIT_PNG);  // optional; missing PNGs fall back to rects
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);  // optional too: no pads, no problem
    }
    ~SdlRuntime() {
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
    }
};

}  // namespace

// SDL wants this exact signature (SDL2main supplies WinMain on Windows).
int main(int argc, char* argv[]) {
    // trippy --shot <room|title> <out.png> [--talk <npc_id>] [--seconds S] [--size WxH]
    ShotRequest shot;
    bool shooting = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--shot" && i + 2 < argc) {
            shooting = true;
            shot.room = argv[++i];
            shot.out = argv[++i];
        } else if (a == "--talk" && i + 1 < argc) {
            shot.talk = argv[++i];
        } else if (a == "--seconds" && i + 1 < argc) {
            shot.seconds = std::atof(argv[++i]);
        } else if (a == "--size" && i + 1 < argc) {
            std::sscanf(argv[++i], "%dx%d", &shot.width, &shot.height);
        }
    }
    if (shooting) {
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);  // screenshots are silent
        Renderer::forcedW = shot.width;
        Renderer::forcedH = shot.height;
    }
    try {
        SdlRuntime sdl;
        Game game(shooting ? &shot : nullptr);
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
