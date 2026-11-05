#pragma once

#include <SDL.h>

#include "Renderer.h"

// Color helpers shared by the procedural sprites and the hue cycling.
SDL_Color hsvColor(double hueDeg, double sat, double val, Uint8 alpha = 255);
double hueOf(SDL_Color c);
SDL_Color hueRotate(SDL_Color c, double degrees);
SDL_Color lerpColor(SDL_Color a, SDL_Color b, double t);

// Draws a sprite as a living thing: it breathes (squash and stretch), sways,
// sheds two hue-shifted echoes, wears a pulsing halo and is orbited by
// little lights. intensity 0 = plain sprite (calm mode), 1 = full trip.
// seed desynchronizes neighbours. Coordinates are in current render space.
void drawLivingSprite(SDL_Renderer* r, SDL_Texture* tex, float cx, float cy, float size,
                      double time, double intensity, double pulse, unsigned seed);
// A soft ring (triangle strip), used for halos.
void drawRing(SDL_Renderer* r, float cx, float cy, float r0, float r1, SDL_Color inner,
              SDL_Color outer, int segments = 40);

struct TripParams {
    double warp = 0;     // horizontal sine ripple
    double kaleido = 0;  // 12-wedge mirror
    double chroma = 0;   // RGB channel split, in 0..1
    double breathe = 0;  // whole playfield swells on the beat
    double melt = 0;     // columns slide down and back like wet paint
};

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
    void end(double time, const TripParams& p, int focusX, int focusY, SDL_Color clear);

private:
    void drawKaleidoscope(double time, double amount, int focusX, int focusY);
    void ensureTargets();

    Renderer& renderer_;
    int w_ = 0;
    TexturePtr scene_;
    TexturePtr post_;
    bool active_ = false;
};
