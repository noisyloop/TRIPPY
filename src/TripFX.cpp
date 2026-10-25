#include "TripFX.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr int kH = Renderer::kViewH;
constexpr double kPi = 3.14159265358979323846;

Uint8 to8(double v) { return static_cast<Uint8>(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5); }
}  // namespace

SDL_Color hsvColor(double h, double s, double v, Uint8 alpha) {
    h = std::fmod(h, 360.0);
    if (h < 0) h += 360.0;
    const double c = v * s;
    const double x = c * (1 - std::fabs(std::fmod(h / 60.0, 2.0) - 1));
    const double m = v - c;
    double r = 0, g = 0, b = 0;
    switch (static_cast<int>(h / 60.0)) {
    case 0: r = c; g = x; break;
    case 1: r = x; g = c; break;
    case 2: g = c; b = x; break;
    case 3: g = x; b = c; break;
    case 4: r = x; b = c; break;
    default: r = c; b = x; break;
    }
    return SDL_Color{to8(r + m), to8(g + m), to8(b + m), alpha};
}

namespace {
void toHsv(SDL_Color c, double& h, double& s, double& v) {
    const double r = c.r / 255.0, g = c.g / 255.0, b = c.b / 255.0;
    const double mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn;
    v = mx;
    s = mx > 0 ? d / mx : 0;
    if (d <= 0) { h = 0; return; }
    if (mx == r) h = 60 * std::fmod((g - b) / d, 6.0);
    else if (mx == g) h = 60 * ((b - r) / d + 2);
    else h = 60 * ((r - g) / d + 4);
    if (h < 0) h += 360;
}
}  // namespace

double hueOf(SDL_Color c) {
    double h, s, v;
    toHsv(c, h, s, v);
    return h;
}

SDL_Color hueRotate(SDL_Color c, double degrees) {
    double h, s, v;
    toHsv(c, h, s, v);
    return hsvColor(h + degrees, s, v, c.a);
}

SDL_Color lerpColor(SDL_Color a, SDL_Color b, double t) {
    auto mix = [&](Uint8 x, Uint8 y) { return static_cast<Uint8>(x + (y - x) * std::clamp(t, 0.0, 1.0)); };
    return SDL_Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}

TripFX::TripFX(Renderer& renderer) : renderer_(renderer) {
    ensureTargets();
}

// (Re)creates the offscreen textures when the logical width changes.
void TripFX::ensureTargets() {
    if (w_ == renderer_.width() && scene_) return;
    SDL_Renderer* r = renderer_.sdl();
    w_ = renderer_.width();
    scene_.reset();
    post_.reset();
    if (!SDL_RenderTargetSupported(r)) return;
    for (TexturePtr* t : {&scene_, &post_}) {
        t->reset(SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w_, kH));
        if (*t) SDL_SetTextureBlendMode(t->get(), SDL_BLENDMODE_BLEND);
    }
    if (!scene_ || !post_) { scene_.reset(); post_.reset(); }
}

bool TripFX::begin(SDL_Color clear, float zoom) {
    ensureTargets();
    active_ = scene_ && SDL_SetRenderTarget(renderer_.sdl(), scene_.get()) == 0;
    if (active_) {
        renderer_.clear(clear);
        SDL_RenderSetScale(renderer_.sdl(), zoom, zoom);
    }
    return active_;
}

void TripFX::drawKaleidoscope(double time, double amount, int focusX, int focusY) {
    const int kW = w_;
    // 12 mirrored wedges around the playfield center, each sampling the same
    // wedge of the scene around the player; odd wedges are flipped.
    constexpr int kWedges = 12;
    constexpr double kStep = 2 * kPi / kWedges;
    constexpr double kSrcR = 250;
    const double kDstR = std::hypot(kW / 2.0, kH / 2.0) + 40;
    const double px = std::clamp<double>(focusX, kSrcR, kW - kSrcR);
    const double py = std::clamp<double>(focusY, kH / 2.0 - 30, kH / 2.0 + 30);
    const double rot = time * 0.15, srcRot = -time * 0.1;
    const SDL_Color col{255, 255, 255, to8(amount)};

    SDL_Vertex v[kWedges * 3];
    for (int k = 0; k < kWedges; ++k) {
        double a0 = rot + k * kStep, a1 = a0 + kStep;
        double s0 = srcRot, s1 = srcRot + kStep;
        if (k % 2) std::swap(s0, s1);
        auto vert = [&](double sx, double sy, double tx, double ty) {
            return SDL_Vertex{{static_cast<float>(sx), static_cast<float>(sy)}, col,
                              {static_cast<float>(tx / kW), static_cast<float>(ty / kH)}};
        };
        v[k * 3 + 0] = vert(kW / 2.0, kH / 2.0, px, py);
        v[k * 3 + 1] = vert(kW / 2.0 + kDstR * std::cos(a0), kH / 2.0 + kDstR * std::sin(a0),
                            px + kSrcR * std::cos(s0), py + kSrcR * std::sin(s0));
        v[k * 3 + 2] = vert(kW / 2.0 + kDstR * std::cos(a1), kH / 2.0 + kDstR * std::sin(a1),
                            px + kSrcR * std::cos(s1), py + kSrcR * std::sin(s1));
    }
    SDL_RenderGeometry(renderer_.sdl(), scene_.get(), v, kWedges * 3, nullptr, 0);
}

void TripFX::end(double time, double warp, double kaleido, int focusX, int focusY,
                 SDL_Color clear) {
    if (!active_) return;
    SDL_Renderer* r = renderer_.sdl();
    SDL_RenderSetScale(r, 1.0f, 1.0f);
    SDL_Texture* src = scene_.get();
    const int kW = w_;

    if (kaleido > 0.01) {
        SDL_SetRenderTarget(r, post_.get());
        SDL_RenderCopy(r, scene_.get(), nullptr, nullptr);
        drawKaleidoscope(time, kaleido, focusX, focusY);
        src = post_.get();
    }

    SDL_SetRenderTarget(r, nullptr);
    renderer_.clear(clear);
    if (warp < 0.01) {
        SDL_Rect dst{0, 0, kW, kH};
        SDL_RenderCopy(r, src, nullptr, &dst);
        return;
    }
    // Two sines of different speed so the ripple never quite repeats.
    constexpr int kStrip = 4;
    const double amp = warp * 14.0;
    for (int y = 0; y < kH; y += kStrip) {
        double off = amp * (std::sin(y * 0.031 + time * 2.3) * 0.7
                            + std::sin(y * 0.087 - time * 1.1) * 0.3);
        // Strips overlap by a pixel so non-integer window scales never
        // leave hairline gaps between them.
        SDL_Rect s{0, y, kW, std::min(kStrip + 1, kH - y)};
        SDL_Rect d{static_cast<int>(std::lround(off)), y, kW, s.h};
        SDL_RenderCopy(r, src, &s, &d);
    }
}
