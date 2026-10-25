#include "Renderer.h"
#include "Assets.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

Renderer::Renderer() {
    // Open big: 85% of the usable desktop (any aspect; the logical width
    // follows it). Falls back to 800x600 if the display can't be queried.
    int winW = kWindowW, winH = kWindowH;
    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(0, &usable) == 0 && usable.w > 0 && usable.h > 0) {
        winW = std::max(kWindowW, usable.w * 85 / 100);
        winH = std::max(kWindowH, usable.h * 85 / 100);
    }
    window_.reset(SDL_CreateWindow("TRIPPY",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW, winH, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE));
    if (!window_)
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    renderer_.reset(SDL_CreateRenderer(window_.get(), -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!renderer_)  // headless / odd drivers: fall back to software
        renderer_.reset(SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_SOFTWARE));
    if (!renderer_)
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

    SDL_SetWindowMinimumSize(window_.get(), 640, 480);
    updateLogicalSize();
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
}

void Renderer::updateLogicalSize() {
    int w = kWindowW, h = kWindowH;
    SDL_GetWindowSize(window_.get(), &w, &h);
    if (h <= 0) h = kWindowH;
    width_ = std::clamp(static_cast<int>(std::lround(double(kWindowH) * w / h)), 600, 1600);
    SDL_RenderSetLogicalSize(renderer_.get(), width_, kWindowH);
}

void Renderer::handleWindowEvent(const SDL_Event& e) {
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        updateLogicalSize();
}

void Renderer::toggleFullscreen() {
    bool fs = SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(window_.get(), fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
    updateLogicalSize();
}

void Renderer::clear(SDL_Color c) {
    SDL_SetRenderDrawColor(renderer_.get(), c.r, c.g, c.b, c.a);
    SDL_RenderClear(renderer_.get());
}

void Renderer::present() {
    SDL_RenderPresent(renderer_.get());
}

void Renderer::drawSprite(SDL_Texture* tex, int worldX, int worldY, int w, int h,
                          const SDL_Rect* src) {
    SDL_Rect dst{worldX - camX_, worldY - camY_, w, h};
    SDL_RenderCopy(renderer_.get(), tex, src, &dst);
}

void Renderer::fillRectWorld(int worldX, int worldY, int w, int h, SDL_Color c) {
    fillRect(SDL_Rect{worldX - camX_, worldY - camY_, w, h}, c);
}

void Renderer::fillRect(const SDL_Rect& r, SDL_Color c) {
    SDL_SetRenderDrawColor(renderer_.get(), c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(renderer_.get(), &r);
}

void Renderer::outlineRect(const SDL_Rect& r, SDL_Color c) {
    SDL_SetRenderDrawColor(renderer_.get(), c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(renderer_.get(), &r);
}

void Renderer::drawText(Assets& assets, const std::string& text, int x, int y, SDL_Color c) {
    if (text.empty()) return;
    SDL_Texture* tex = assets.textTexture(text, c);
    if (!tex) return;
    int w = 0, h = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst{x, y, w, h};
    SDL_RenderCopy(renderer_.get(), tex, nullptr, &dst);
}

void Renderer::drawTextCentered(Assets& assets, const std::string& text, int cx, int cy,
                                SDL_Color c, int scale) {
    if (text.empty() || scale < 1) return;
    SDL_Texture* tex = assets.textTexture(text, c);
    if (!tex) return;
    int w = 0, h = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst{cx - w * scale / 2, cy - h * scale / 2, w * scale, h * scale};
    SDL_RenderCopy(renderer_.get(), tex, nullptr, &dst);
}

int Renderer::textWidth(Assets& assets, const std::string& text) {
    return assets.measureText(text);
}
