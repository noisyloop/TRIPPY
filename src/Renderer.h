#pragma once

#include <SDL.h>
#include <memory>
#include <string>

struct SdlWindowDeleter   { void operator()(SDL_Window* w) const   { if (w) SDL_DestroyWindow(w); } };
struct SdlRendererDeleter { void operator()(SDL_Renderer* r) const { if (r) SDL_DestroyRenderer(r); } };
struct SdlTextureDeleter  { void operator()(SDL_Texture* t) const  { if (t) SDL_DestroyTexture(t); } };
struct SdlSurfaceDeleter  { void operator()(SDL_Surface* s) const  { if (s) SDL_FreeSurface(s); } };

using WindowPtr   = std::unique_ptr<SDL_Window, SdlWindowDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, SdlRendererDeleter>;
using TexturePtr  = std::unique_ptr<SDL_Texture, SdlTextureDeleter>;
using SurfacePtr  = std::unique_ptr<SDL_Surface, SdlSurfaceDeleter>;

class Assets;

// Thin SDL wrapper: owns the window/renderer, offers draw helpers.
// World-space draws take a camera offset; UI draws are in screen space.
//
// Logical height is fixed at kWindowH; logical width follows the window's
// aspect ratio (resizable / fullscreen), so the playfield fills the window
// instead of letterboxing. Query width() rather than assuming 800.
class Renderer {
public:
    static constexpr int kWindowW = 800;   // initial width; see width()
    static constexpr int kWindowH = 600;
    static constexpr int kStatusBarH = 40;
    static constexpr int kViewH = kWindowH - kStatusBarH;  // playfield height

    // width/height 0 = 85% of the desktop. Throws std::runtime_error on failure.
    Renderer(int windowW = 0, int windowH = 0);

    SDL_Renderer* sdl() const { return renderer_.get(); }

    int width() const { return width_; }        // logical width, tracks aspect
    void handleWindowEvent(const SDL_Event& e);  // resize -> new logical width
    void toggleFullscreen();
    void setIcon(SDL_Surface* icon) { SDL_SetWindowIcon(window_.get(), icon); }
    // The next present() first saves the frame as a PNG (dev screenshots).
    void requestCapture(std::string path) { capturePath_ = std::move(path); }
    bool captureFailed() const { return captureFailed_; }

    void clear(SDL_Color c);
    void present();

    void setCamera(int x, int y) { camX_ = x; camY_ = y; }
    int cameraX() const { return camX_; }
    int cameraY() const { return camY_; }

    // World space (camera applied).
    void drawSprite(SDL_Texture* tex, int worldX, int worldY, int w, int h,
                    const SDL_Rect* src = nullptr);
    void fillRectWorld(int worldX, int worldY, int w, int h, SDL_Color c);

    // Screen space.
    void fillRect(const SDL_Rect& r, SDL_Color c);
    void outlineRect(const SDL_Rect& r, SDL_Color c);
    void drawText(Assets& assets, const std::string& text, int x, int y, SDL_Color c);
    // Integer-scaled (chunky) text centered on (cx, cy).
    void drawTextCentered(Assets& assets, const std::string& text, int cx, int cy,
                          SDL_Color c, int scale = 1);
    int textWidth(Assets& assets, const std::string& text);

private:
    WindowPtr window_;
    RendererPtr renderer_;
    int camX_ = 0;
    int camY_ = 0;
    int width_ = kWindowW;
    std::string capturePath_;
    bool captureFailed_ = false;

    void updateLogicalSize();
};
