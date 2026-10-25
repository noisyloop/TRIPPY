#include "Input.h"

void Input::beginFrame() {
    pressed_.fill(false);
}

Input::~Input() {
    for (auto* p : pads_) SDL_GameControllerClose(p);
}

bool Input::anyKeyDown() const {
    for (size_t i = 0; i < down_.size(); ++i)
        if (down_[i] || padDown_[i]) return true;
    return false;
}

void Input::setPad(SDL_Scancode sc, bool down) {
    if (down && !padDown_[sc]) pressed_[sc] = true;
    padDown_[sc] = down;
}

namespace {
SDL_Scancode padButtonKey(Uint8 button) {
    switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP:    return SDL_SCANCODE_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  return SDL_SCANCODE_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  return SDL_SCANCODE_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return SDL_SCANCODE_RIGHT;
    case SDL_CONTROLLER_BUTTON_A:          return SDL_SCANCODE_E;
    case SDL_CONTROLLER_BUTTON_B:          return SDL_SCANCODE_SPACE;
    case SDL_CONTROLLER_BUTTON_X:
    case SDL_CONTROLLER_BUTTON_Y:          return SDL_SCANCODE_I;
    case SDL_CONTROLLER_BUTTON_BACK:       return SDL_SCANCODE_C;
    case SDL_CONTROLLER_BUTTON_START:      return SDL_SCANCODE_F;
    default:                               return SDL_SCANCODE_UNKNOWN;
    }
}
}  // namespace

void Input::handleEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_QUIT:
        quit_ = true;
        break;
    case SDL_KEYDOWN:
        if (!e.key.repeat) {
            down_[e.key.keysym.scancode] = true;
            pressed_[e.key.keysym.scancode] = true;
        }
        break;
    case SDL_KEYUP:
        down_[e.key.keysym.scancode] = false;
        break;
    case SDL_CONTROLLERDEVICEADDED:
        if (SDL_GameController* p = SDL_GameControllerOpen(e.cdevice.which))
            pads_.push_back(p);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        if (SDL_Scancode sc = padButtonKey(e.cbutton.button); sc != SDL_SCANCODE_UNKNOWN)
            setPad(sc, e.type == SDL_CONTROLLERBUTTONDOWN);
        break;
    case SDL_CONTROLLERAXISMOTION: {
        constexpr Sint16 kDead = 12000;
        const Sint16 v = e.caxis.value;
        if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
            setPad(SDL_SCANCODE_LEFT, v < -kDead);
            setPad(SDL_SCANCODE_RIGHT, v > kDead);
        } else if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            setPad(SDL_SCANCODE_UP, v < -kDead);
            setPad(SDL_SCANCODE_DOWN, v > kDead);
        }
        break;
    }
    default:
        break;
    }
}
