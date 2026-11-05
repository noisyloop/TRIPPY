#include "TripFX.h"

#include <algorithm>
#include <cmath>
#include <vector>

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

void TripFX::end(double time, const TripParams& p, int focusX, int focusY, SDL_Color clear) {
    if (!active_) return;
    SDL_Renderer* r = renderer_.sdl();
    SDL_RenderSetScale(r, 1.0f, 1.0f);
    SDL_Texture* src = scene_.get();
    SDL_Texture* spare = post_.get();
    const int kW = w_;

    if (p.kaleido > 0.01) {
        SDL_SetRenderTarget(r, post_.get());
        SDL_RenderCopy(r, scene_.get(), nullptr, nullptr);
        drawKaleidoscope(time, p.kaleido, focusX, focusY);
        src = post_.get();
        spare = scene_.get();
    }

    // Melt: thin columns slide down and recover, like paint that is not dry.
    if (p.melt > 0.01) {
        SDL_SetRenderTarget(r, spare);
        renderer_.clear(clear);
        constexpr int kCol = 4;
        for (int x = 0; x < kW; x += kCol) {
            const double drip = std::max(0.0, std::sin(x * 0.047 + time * 0.7))
                                * std::max(0.0, std::sin(x * 0.013 - time * 0.43));
            const int dy = static_cast<int>(p.melt * 26.0 * drip);
            SDL_Rect s{x, 0, kCol, kH}, d{x, dy, kCol, kH};
            SDL_RenderCopy(r, src, &s, &d);
        }
        src = spare;
    }

    SDL_SetRenderTarget(r, nullptr);

    // Final composite: warp strips, scaled around the center by the beat
    // swell, optionally three times (R, G, B) with offsets for the split.
    const double scale = 1.0 + 0.035 * p.breathe;
    const double cx = kW / 2.0, cy = kH / 2.0;
    const int strip = p.warp < 0.01 ? kH : 4;
    const double amp = p.warp * 14.0;
    auto blit = [&](double extraX, double extraY) {
        for (int y = 0; y < kH; y += strip) {
            double off = strip == kH ? 0.0
                : amp * (std::sin(y * 0.031 + time * 2.3) * 0.7 + std::sin(y * 0.087 - time * 1.1) * 0.3);
            // Snap rows to whole pixels so strips never overlap (overlaps
            // would double up in the additive RGB pass and show as seams).
            const int y0 = static_cast<int>(std::floor(cy + (y + extraY - cy) * scale));
            const int y1 = static_cast<int>(std::floor(cy + (y + strip + extraY - cy) * scale));
            SDL_Rect s{0, y, kW, strip};
            SDL_FRect d{static_cast<float>(cx + (off + extraX - cx) * scale), static_cast<float>(y0),
                        static_cast<float>(kW * scale), static_cast<float>(y1 - y0)};
            SDL_RenderCopyF(r, src, &s, &d);
        }
    };

    if (p.chroma < 0.02) {
        renderer_.clear(clear);
        blit(0, 0);
        return;
    }
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);
    const double off = p.chroma * (3.0 + 3.0 * std::sin(time * 1.7));
    const double wob = p.chroma * 2.0 * std::sin(time * 2.9);
    SDL_SetTextureBlendMode(src, SDL_BLENDMODE_ADD);
    SDL_SetTextureColorMod(src, 255, 0, 0);
    blit(off, wob);
    SDL_SetTextureColorMod(src, 0, 255, 0);
    blit(0, 0);
    SDL_SetTextureColorMod(src, 0, 0, 255);
    blit(-off, -wob);
    SDL_SetTextureColorMod(src, 255, 255, 255);
    SDL_SetTextureBlendMode(src, SDL_BLENDMODE_BLEND);
}

void drawRing(SDL_Renderer* r, float cx, float cy, float r0, float r1, SDL_Color inner,
              SDL_Color outer, int segments) {
    std::vector<SDL_Vertex> v;
    v.reserve(static_cast<size_t>(segments) * 6);
    for (int i = 0; i < segments; ++i) {
        const double a0 = i * 2 * kPi / segments, a1 = (i + 1) * 2 * kPi / segments;
        auto at = [&](double rad, double a, SDL_Color c) {
            return SDL_Vertex{{cx + static_cast<float>(rad * std::cos(a)), cy + static_cast<float>(rad * std::sin(a))}, c, {0, 0}};
        };
        v.push_back(at(r0, a0, inner)); v.push_back(at(r1, a0, outer)); v.push_back(at(r1, a1, outer));
        v.push_back(at(r0, a0, inner)); v.push_back(at(r1, a1, outer)); v.push_back(at(r0, a1, inner));
    }
    SDL_RenderGeometry(r, nullptr, v.data(), static_cast<int>(v.size()), nullptr, 0);
}

void drawLivingSprite(SDL_Renderer* r, SDL_Texture* tex, float cx, float cy, float size,
                      double time, double k, double pulse, unsigned seed) {
    const double ph = (seed % 1000) / 1000.0 * 2 * kPi;
    const double t = time + ph;
    k = std::clamp(k, 0.0, 1.0);

    if (k > 0.01) {
        // Halo: two soft rings breathing out of phase, hue drifting.
        const double hr = size * (0.55 + 0.08 * std::sin(t * 2.1) + 0.12 * pulse);
        SDL_Color in = hsvColor(t * 60 + seed % 360, 0.8, 1.0, to8(0.35 * k));
        SDL_Color out = hsvColor(t * 60 + seed % 360 + 90, 0.8, 1.0, 0);
        drawRing(r, cx, cy, static_cast<float>(hr * 0.55), static_cast<float>(hr), in, out);
        SDL_Color in2 = hsvColor(t * 60 + seed % 360 + 180, 0.8, 1.0, to8(0.25 * k));
        drawRing(r, cx, cy, static_cast<float>(hr * 1.05), static_cast<float>(hr * 1.25 + size * 0.1 * std::sin(t * 1.3)),
                 out, in2);
        drawRing(r, cx, cy, static_cast<float>(hr * 1.25 + size * 0.1 * std::sin(t * 1.3)),
                 static_cast<float>(hr * 1.45), in2, SDL_Color{in2.r, in2.g, in2.b, 0});
    }

    // Breathing: squash one way while stretching the other.
    const double br = std::sin(t * 2.4) * 0.07 * k + 0.08 * pulse * k;
    const float w = static_cast<float>(size * (1 + br)), h = static_cast<float>(size * (1 - br * 0.8));
    const double angle = std::sin(t * 1.6) * 9.0 * k;

    if (k > 0.01) {
        // Echoes: the same being half a beat out of step, in other colors.
        for (int e = 0; e < 2; ++e) {
            const double ea = t * 1.3 + e * kPi;
            const float ex = static_cast<float>(std::cos(ea) * size * 0.12 * k);
            const float ey = static_cast<float>(std::sin(ea * 0.7) * size * 0.08 * k);
            SDL_Color c = hsvColor(t * 90 + e * 140 + seed % 360, 0.9, 1.0);
            SDL_SetTextureColorMod(tex, c.r, c.g, c.b);
            SDL_SetTextureAlphaMod(tex, to8(0.38 * k));
            SDL_FRect d{cx - w / 2 + ex, cy - h / 2 + ey, w, h};
            SDL_RenderCopyExF(r, tex, nullptr, &d, -angle * 1.5, nullptr, SDL_FLIP_NONE);
        }
    }
    // The being itself, faintly hue-tinted.
    SDL_Color tint = lerpColor(SDL_Color{255, 255, 255, 255}, hsvColor(t * 40 + seed % 360, 0.5, 1.0), 0.45 * k);
    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, 255);
    SDL_FRect d{cx - w / 2, cy - h / 2, w, h};
    SDL_RenderCopyExF(r, tex, nullptr, &d, angle, nullptr, SDL_FLIP_NONE);
    SDL_SetTextureColorMod(tex, 255, 255, 255);

    if (k > 0.01) {
        // Little lights in orbit, each on its own tilted ellipse.
        for (int i = 0; i < 4; ++i) {
            const double a = t * (1.1 + i * 0.37) + i * kPi / 2;
            const float ox = cx + static_cast<float>(std::cos(a) * size * (0.55 + 0.1 * i));
            const float oy = cy + static_cast<float>(std::sin(a) * size * (0.25 + 0.06 * i));
            const float s = std::max(1.5f, size / 14.0f);
            SDL_Color c = hsvColor(t * 120 + i * 90, 0.6, 1.0, to8(0.9 * k));
            SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
            SDL_FRect dot{ox - s / 2, oy - s / 2, s, s};
            SDL_RenderFillRectF(r, &dot);
        }
    }
}
