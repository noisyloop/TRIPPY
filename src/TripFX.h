#pragma once

#include <SDL.h>

#include "Renderer.h"

// Color helpers shared by the procedural sprites and the hue cycling.
SDL_Color hsvColor(double hueDeg, double sat, double val, Uint8 alpha = 255);
double hueOf(SDL_Color c);
SDL_Color hueRotate(SDL_Color c, double degrees);
SDL_Color lerpColor(SDL_Color a, SDL_Color b, double t);

// Screen-space psychedelia, SDL2 render API only (no shaders):
//   begin() redirects world drawing into an offscreen playfield texture;
//   end() composites it back with an optional kaleidoscope (textured
//   triangle wedges via SDL_RenderGeometry) and a sine-wave warp (the
//   playfield re-blitted as thin horizontal strips, each shifted).
// If the driver has no render targets, begin() returns false and the
// world just draws straight to the screen.
class TripFX {
public:
    explicit TripFX(Renderer& renderer);

    // zoom scales world drawing (half steps keep 2px art pixels whole).
    bool begin(SDL_Color clear, float zoom = 1.0f);
    bool available() const { return scene_ != nullptr; }
    // warp, kaleido in 0..1. focus = player's screen position (kaleido center).
    void end(double time, double warp, double kaleido, int focusX, int focusY,
             SDL_Color clear);

private:
    void drawKaleidoscope(double time, double amount, int focusX, int focusY);
    void ensureTargets();

    Renderer& renderer_;
    int w_ = 0;
    TexturePtr scene_;
    TexturePtr post_;
    bool active_ = false;
};
