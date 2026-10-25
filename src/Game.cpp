#include "Game.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

struct MoodPalette {
    SDL_Color floor, wall, grass, door, background;
};

// Room tint follows the music: Vegas neon over desert dusk.
MoodPalette paletteFor(const std::string& mood) {
    if (mood == "melancholy_a_minor")
        return {{58, 52, 82, 255}, {28, 22, 46, 255}, {52, 74, 66, 255}, {214, 62, 106, 255}, {20, 14, 36, 255}};
    if (mood == "tense_chromatic")
        return {{70, 46, 46, 255}, {34, 16, 20, 255}, {84, 62, 40, 255}, {255, 179, 71, 255}, {28, 12, 22, 255}};
    if (mood == "dread_diminished")
        return {{40, 40, 48, 255}, {14, 12, 20, 255}, {36, 48, 44, 255}, {120, 220, 232, 255}, {16, 6, 12, 255}};
    if (mood == "playful_major")
        return {{88, 70, 100, 255}, {40, 26, 56, 255}, {66, 96, 70, 255}, {255, 214, 92, 255}, {40, 22, 44, 255}};
    if (mood == "dream_lydian")
        return {{62, 58, 96, 255}, {26, 24, 52, 255}, {58, 66, 96, 255}, {170, 120, 232, 255}, {26, 14, 44, 255}};
    // calm_pentatonic + default
    return {{66, 60, 88, 255}, {30, 24, 50, 255}, {56, 80, 70, 255}, {214, 62, 106, 255}, {24, 16, 40, 255}};
}

constexpr int kTile = Room::kTile;

// Saturated, hue-cycled version of a palette color. The hue drifts with time
// and ripples across the grid; `trip` scales how far it wanders.
SDL_Color trippyTint(SDL_Color base, double val, double trip, double t, int tx, int ty) {
    double shift = trip * (t * 35.0 + (tx + ty) * 9.0 + std::sin(tx * 0.6 + t * 1.3) * 50.0
                           + std::cos(ty * 0.5 - t * 0.9) * 40.0);
    return hsvColor(hueOf(base) + shift, 0.45 + 0.45 * trip, val);
}

}  // namespace

Game::Game()
    : renderer_(),
      assets_(renderer_),
      audio_(assets_),
      roomLoader_(assets_.rootDir()),
      tripFx_(renderer_) {
    if (auto data = saveGame_.read()) {
        std::cout << "[save] resuming from " << saveGame_.path() << "\n";
        inventory_.setItems(data->inventory);
        flags_ = data->flags;
        enterRoom(data->room, nullptr, /*autosave=*/false);
        if (!room_)  // saved room's map file was deleted; fall back
            enterRoom("bedroom", nullptr, false);
        else
            player_.setPixelPosition(data->playerX, data->playerY);
    } else {
        enterRoom("bedroom", nullptr, /*autosave=*/false);
    }
    if (!room_)
        throw std::runtime_error("Could not load starting room 'bedroom' "
                                 "(missing assets/maps/bedroom.txt?)");

    // The title screen gets its own dreamy theme; the room's mood crossfades
    // in once the player holds a key to start.
    audio_.playMood("dream_lydian");
}

void Game::enterRoom(const std::string& name, const Direction* fromDir, bool autosave) {
    auto next = roomLoader_.load(name);
    if (!next) {
        std::cerr << "[game] staying put: room '" << name << "' failed to load\n";
        return;
    }
    room_ = std::move(next);
    trail_.clear();

    // Collected items don't respawn.
    for (const auto& id : inventory_.items())
        room_->removeEntity(id);

    if (fromDir) {
        // Entering heading north => appear at the room's south door.
        if (const auto* door = room_->doorFacing(oppositeOf(*fromDir))) {
            int tx = door->tx, ty = door->ty;
            switch (door->dir) {  // one tile inward so we don't re-trigger
            case Direction::North: ++ty; break;
            case Direction::South: --ty; break;
            case Direction::West:  ++tx; break;
            case Direction::East:  --tx; break;
            }
            player_.setTilePosition(tx, ty);
        } else {
            int tx, ty;
            room_->defaultSpawn(tx, ty);
            player_.setTilePosition(tx, ty);
        }
    } else {
        int tx, ty;
        room_->defaultSpawn(tx, ty);
        player_.setTilePosition(tx, ty);
    }

    audio_.playMood(room_->mood());
    if (!room_->onEnterFlag().empty())
        setFlag(room_->onEnterFlag());
    if (autosave)
        save();
}

void Game::save() {
    SaveData data;
    data.room = room_->name();
    data.playerX = player_.x();
    data.playerY = player_.y();
    data.inventory = inventory_.items();
    data.flags = flags_;
    if (!saveGame_.write(data))
        std::cerr << "[save] autosave failed\n";
}

void Game::run() {
    Uint64 prev = SDL_GetPerformanceCounter();
    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running_) {
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = std::min((now - prev) / freq, 0.05);  // clamp hitches
        prev = now;
        worldTime_ += dt;

        input_.beginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            input_.handleEvent(e);
            renderer_.handleWindowEvent(e);
        }
        if (input_.quitRequested())
            running_ = false;

        update(dt);
        render();
    }
    save();  // be kind on quit too
}

void Game::toggleCalm() {
    if (calm()) flags_.erase("opt_calm");
    else setFlag("opt_calm");
    audio_.playSfx("confirm");
}

void Game::handleGlobalKeys() {
    // F / F11 / Cmd+F / Alt+Enter: fullscreen. C: calm mode.
    const bool cmd = input_.isDown(SDL_SCANCODE_LGUI) || input_.isDown(SDL_SCANCODE_RGUI);
    const bool alt = input_.isDown(SDL_SCANCODE_LALT) || input_.isDown(SDL_SCANCODE_RALT);
    if (input_.wasPressed(SDL_SCANCODE_F11) || input_.wasPressed(SDL_SCANCODE_F)
        || (alt && input_.wasPressed(SDL_SCANCODE_RETURN)) || (cmd && input_.wasPressed(SDL_SCANCODE_F)))
        renderer_.toggleFullscreen();
    if (input_.wasPressed(SDL_SCANCODE_C) && state_ != State::InDialogue)
        toggleCalm();
}

void Game::update(double dt) {
    if (input_.escapePressed())
        running_ = false;
    handleGlobalKeys();

    // Energy builds over ~2s of walking, drains over ~1.5s of standing.
    const bool walking = state_ == State::Exploring && player_.moving();
    musicEnergy_ = std::clamp(musicEnergy_ + (walking ? dt / 2.0 : -dt / 1.5), 0.0, 1.0);
    audio_.setMotion(static_cast<float>(musicEnergy_), static_cast<int>(player_.facing()), turns_);
    if (uint32_t b = audio_.beatCount(); b != lastBeat_) {
        lastBeat_ = b;
        beatPulse_ = 1.0;
    }
    beatPulse_ = std::max(0.0, beatPulse_ - dt * 4.0);

    const double ease = std::min(1.0, dt * 1.5);
    trip_ += (room_->trip() - trip_) * ease;
    kaleido_ += (room_->kaleidoscope() - kaleido_) * ease;

    // Afterimages: sample the player every 40ms while moving, decay when still.
    trailClock_ += dt;
    if (trailClock_ >= 0.04) {
        trailClock_ = 0;
        const size_t maxGhosts = calm() ? 2 : static_cast<size_t>(2 + trip_ * 10);
        if (player_.moving())
            trail_.push_front({player_.x(), player_.y(), player_.spriteName()});
        else if (!trail_.empty())
            trail_.pop_back();
        while (trail_.size() > maxGhosts) trail_.pop_back();
    }

    switch (state_) {
    case State::Title:
        updateTitle(dt);
        break;
    case State::Exploring:
        if (input_.inventoryPressed()) {
            audio_.playSfx("confirm");
            state_ = State::InventoryOpen;
            return;
        }
        updateExploring(dt);
        break;
    case State::InDialogue:
        dialogue_.update(dt, input_);
        room_->update(dt);
        if (!dialogue_.active())
            state_ = State::Exploring;
        break;
    case State::InventoryOpen:
        if (input_.inventoryPressed() || input_.interactPressed())
            state_ = State::Exploring;
        break;
    }
}

void Game::updateTitle(double dt) {
    // Hold any key to start; releasing resets the fill. C and F are settings,
    // not "any key".
    const bool settingKey = input_.isDown(SDL_SCANCODE_C) || input_.isDown(SDL_SCANCODE_F)
                            || input_.isDown(SDL_SCANCODE_F11);
    if (input_.anyKeyDown() && !settingKey) {
        titleHold_ += dt;
        if (titleHold_ >= kTitleHoldSeconds) {
            state_ = State::Exploring;
            audio_.playMood(room_->mood());  // crossfade out of the title theme
            audio_.playSfx("confirm");
        }
    } else {
        titleHold_ = 0;
    }
}

void Game::updateExploring(double dt) {
    player_.update(dt, input_, *room_);
    if (player_.moving() && player_.facing() != lastFacing_) {
        lastFacing_ = player_.facing();
        ++turns_;
    }
    room_->update(dt);
    pickupItemsUnderPlayer();

    // Standing on a door tile fires its transition.
    if (const auto* door = room_->doorAt(player_.tileX(), player_.tileY())) {
        std::string target = room_->transitionTo(door->dir);
        if (!target.empty()) {
            Direction travel = door->dir;
            enterRoom(target, &travel, /*autosave=*/true);
            return;
        }
    }

    if (input_.interactPressed())
        tryInteract();
}

void Game::tryInteract() {
    int tx, ty;
    player_.facingTile(tx, ty);
    Entity* e = room_->entityAt(tx, ty);
    if (!e)
        e = room_->entityAt(player_.tileX(), player_.tileY());
    if (!e) {
        audio_.playSfx("bump");
        return;
    }

    if (e->kind() == EntityKind::Npc) {
        const DialogueTree* tree = dialogueForNpc(e->id());
        if (!tree) {
            std::cerr << "[dialogue] no tree registered for npc '" << e->id() << "'\n";
            return;
        }
        DialogueBox::Effects fx;
        fx.hasFlag = [this](const std::string& f) { return hasFlag(f); };
        fx.setFlag = [this](const std::string& f) { setFlag(f); };
        fx.giveItem = [this](const std::string& id) {
            inventory_.add(id);
            setFlag("got_" + id);
            audio_.playSfx("pickup");
        };
        fx.onCharTyped = [this] { audio_.playSfx("blip"); };
        dialogue_.start(*tree, std::move(fx));
        state_ = State::InDialogue;
    }
}

void Game::pickupItemsUnderPlayer() {
    Entity* e = room_->entityAt(player_.tileX(), player_.tileY());
    if (e && e->kind() == EntityKind::Item) {
        std::string id = e->id();
        inventory_.add(id);
        setFlag("got_" + id);
        room_->removeEntity(id);
        audio_.playSfx("pickup");
        std::cout << "[game] picked up " << Inventory::displayName(id) << "\n";
    }
}

void Game::render() {
    if (state_ == State::Title) {
        renderTitle();
        return;
    }

    const MoodPalette pal = paletteFor(room_->mood());
    const double fxTrip = calm() ? std::min(trip_, 0.12) : trip_;
    const double pulse = calm() ? 0.0 : beatPulse_;
    const SDL_Color bg = hsvColor(hueOf(pal.background) + worldTime_ * 20.0 * fxTrip, 0.6, 0.12);
    renderer_.clear(bg);

    // Zoom so the room fills the playfield (half steps keep the 2px art
    // pixels whole), never below 1.5x; bigger rooms scroll.
    const int viewW = renderer_.width(), viewH = Renderer::kViewH;
    float zoom = 1.0f;
    if (tripFx_.available()) {
        double fit = std::min(double(viewW) / room_->pixelWidth(), double(viewH) / room_->pixelHeight());
        zoom = static_cast<float>(std::clamp(std::floor(fit * 2.0) / 2.0, 1.5, 3.0));
    }
    const int visW = static_cast<int>(viewW / zoom), visH = static_cast<int>(viewH / zoom);

    // Camera centered on the player, clamped to the room.
    int camX = static_cast<int>(player_.x()) + kTile / 2 - visW / 2;
    int camY = static_cast<int>(player_.y()) + kTile / 2 - visH / 2;
    camX = std::clamp(camX, 0, std::max(0, room_->pixelWidth() - visW));
    camY = std::clamp(camY, 0, std::max(0, room_->pixelHeight() - visH));
    if (room_->pixelWidth() < visW)
        camX = (room_->pixelWidth() - visW) / 2;
    if (room_->pixelHeight() < visH)
        camY = (room_->pixelHeight() - visH) / 2;
    renderer_.setCamera(camX, camY);

    tripFx_.begin(bg, zoom);
    renderWorld(fxTrip, pulse);
    tripFx_.end(worldTime_, calm() ? 0.0 : fxTrip * (0.75 + 0.5 * pulse), calm() ? 0.0 : kaleido_,
                static_cast<int>((player_.x() + kTile / 2 - camX) * zoom),
                static_cast<int>((player_.y() + kTile / 2 - camY) * zoom), bg);

    if (state_ == State::InventoryOpen)
        inventory_.render(renderer_, assets_);
    if (dialogue_.active())
        dialogue_.render(renderer_, assets_);
    renderStatusBar();

    renderer_.present();
}

void Game::renderTitle() {
    renderer_.clear(SDL_Color{8, 5, 16, 255});

    // Neon-sign flicker: two incommensurate sines so it never quite repeats.
    float flicker = 0.78f + 0.22f * static_cast<float>(
        std::sin(worldTime_ * 3.1) * std::sin(worldTime_ * 7.3));
    auto dim = [&](SDL_Color c) {
        return SDL_Color{static_cast<Uint8>(c.r * flicker), static_cast<Uint8>(c.g * flicker),
                         static_cast<Uint8>(c.b * flicker), c.a};
    };

    const int cx = renderer_.width() / 2;
    renderer_.drawTextCentered(assets_, "TRIPPY", cx + 5, 215, SDL_Color{40, 12, 24, 255}, 5);
    renderer_.drawTextCentered(assets_, "TRIPPY", cx, 210, dim(SDL_Color{120, 255, 150, 255}), 5);
    renderer_.drawTextCentered(assets_, calm() ? "calm mode ON (C)" : "flashing colors & motion  -  press C for calm mode",
                               cx, 500, calm() ? SDL_Color{120, 255, 150, 255} : SDL_Color{255, 179, 71, 255});
    renderer_.drawTextCentered(assets_, "F: fullscreen", cx, 530, SDL_Color{110, 95, 140, 255});
    renderer_.drawTextCentered(assets_, TRIPPY_VERSION, renderer_.width() - 40,
                               Renderer::kWindowH - 20, SDL_Color{90, 70, 120, 255});
    renderer_.drawTextCentered(assets_, "you are the visitor. everything else is visiting you.",
                               cx, 290, SDL_Color{140, 120, 170, 255});

    // Hold-to-start prompt with a fill bar; brightens while a key is held.
    bool holding = titleHold_ > 0;
    SDL_Color prompt = holding ? SDL_Color{255, 179, 71, 255} : SDL_Color{200, 190, 215, 255};
    renderer_.drawTextCentered(assets_, "hold any key", cx, 420, prompt);

    const SDL_Rect bar{cx - 120, 448, 240, 10};
    renderer_.outlineRect(bar, SDL_Color{60, 40, 80, 255});
    int fill = static_cast<int>(bar.w * std::min(titleHold_ / kTitleHoldSeconds, 1.0));
    if (fill > 0)
        renderer_.fillRect(SDL_Rect{bar.x, bar.y, fill, bar.h}, SDL_Color{255, 179, 71, 255});

    renderer_.present();
}

void Game::renderWorld(double fxTrip, double pulse) {
    const MoodPalette pal = paletteFor(room_->mood());

    // Tiles: real PNG if provided (tile_wall.png etc.), else generated
    // sacred geometry. Both get a hue-cycled tint; real art only partially,
    // so it stays readable.
    for (int ty = 0; ty < room_->height(); ++ty) {
        for (int tx = 0; tx < room_->width(); ++tx) {
            Tile t = room_->tileAt(tx, ty);
            const char* spriteName = "tile_floor";
            SDL_Color base = pal.floor;
            double val = 0.85;
            switch (t) {
            case Tile::Wall:  spriteName = "tile_wall";  base = pal.wall;  val = 0.7; break;
            case Tile::Floor: spriteName = "tile_floor"; base = pal.floor; val = 0.85; break;
            case Tile::Grass: spriteName = "tile_grass"; base = pal.grass; val = 0.9; break;
            case Tile::Door:  spriteName = "tile_door";  base = pal.door;  val = 1.0; break;
            }
            // Tiles flash brighter on the beat.
            val *= 1.0 - 0.3 * fxTrip * (1.0 - pulse);
            SDL_Color tint = trippyTint(base, val, fxTrip, worldTime_, tx, ty);
            const bool real = !assets_.spriteIsPlaceholder(spriteName);
            if (real)
                tint = lerpColor(SDL_Color{255, 255, 255, 255}, tint, fxTrip);
            SDL_Texture* tex = assets_.sprite(spriteName);
            SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
            renderer_.drawSprite(tex, tx * kTile, ty * kTile, kTile, kTile);

            if (!real && t == Tile::Grass) {
                // Swaying blades in the complementary hue.
                double sway = std::sin(worldTime_ * 2.0 + tx * 1.7 + ty * 0.9) * 2.0;
                SDL_Color blade = hueRotate(tint, 180);
                blade.a = 160;
                for (int b = 0; b < 3; ++b)
                    renderer_.fillRectWorld(tx * kTile + 6 + b * 9 + static_cast<int>(sway),
                                            ty * kTile + 8, 3, 18, blade);
            } else if (!real && t == Tile::Door) {
                double pulse = (std::sin(worldTime_ * 3.0) + 1.0) * 0.5;
                SDL_Color glow = hueRotate(tint, 120);
                glow.a = static_cast<Uint8>(60 + pulse * 90);
                renderer_.fillRectWorld(tx * kTile + 4, ty * kTile + 4,
                                        kTile - 8, kTile - 8, glow);
            }
        }
    }
    for (const char* n : {"tile_floor", "tile_wall", "tile_grass", "tile_door"})
        SDL_SetTextureColorMod(assets_.sprite(n), 255, 255, 255);

    // Entities.
    for (const auto& e : room_->entities()) {
        double ox, oy;
        e->drawOffset(ox, oy);
        int px = e->tileX() * kTile + static_cast<int>(ox);
        int py = e->tileY() * kTile + static_cast<int>(oy);
        if (e->kind() == EntityKind::Npc) {
            double bob = std::sin(worldTime_ * 2.2 + e->tileX()) * 2.0;
            renderer_.drawSprite(assets_.sprite(e->sprite()), px,
                                 py + static_cast<int>(bob), kTile, kTile);
        } else {
            // Items hover and glimmer so they read as collectible.
            double hover = std::sin(worldTime_ * 3.0) * 3.0;
            renderer_.drawSprite(assets_.sprite(e->sprite()), px + 4,
                                 py + 4 + static_cast<int>(hover), kTile - 8, kTile - 8);
        }
    }

    // Afterimages: older ghosts fade out and slide around the color wheel.
    for (size_t i = trail_.size(); i-- > 0;) {
        const Ghost& g = trail_[i];
        SDL_Texture* tex = assets_.sprite(g.frame);
        SDL_Color c = hsvColor(worldTime_ * 90.0 + i * 40.0, 0.8, 1.0);
        SDL_SetTextureColorMod(tex, c.r, c.g, c.b);
        SDL_SetTextureAlphaMod(tex, static_cast<Uint8>(150.0 * (1.0 - (i + 1.0) / (trail_.size() + 1.0))));
        renderer_.drawSprite(tex, static_cast<int>(g.x), static_cast<int>(g.y), kTile, kTile);
        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    // Player: directional frames if real art exists, bobbing fallback if not.
    std::string frame = player_.spriteName();
    int px = static_cast<int>(player_.x());
    int py = static_cast<int>(player_.y());
    if (!assets_.spriteIsPlaceholder(frame)) {
        renderer_.drawSprite(assets_.sprite(frame), px, py, kTile, kTile);
    } else if (assets_.spriteIsPlaceholder("player")) {
        // Generated alien frames are directional already; just add a hop.
        double bob = player_.moving() ? std::sin(player_.animTime() * 18.0) * 2.0 : 0.0;
        renderer_.drawSprite(assets_.sprite(frame), px, py + static_cast<int>(bob), kTile, kTile);
    } else {
        double bob = player_.moving() ? std::sin(player_.animTime() * 18.0) * 2.0 : 0.0;
        renderer_.drawSprite(assets_.sprite("player"), px,
                             py + static_cast<int>(bob), kTile, kTile);
        // Tiny facing tick so direction is readable on the plain square.
        int fx = px + kTile / 2 - 3, fy = py + kTile / 2 - 3;
        switch (player_.facing()) {
        case Facing::Down:  fy = py + kTile - 8; break;
        case Facing::Up:    fy = py + 2; break;
        case Facing::Left:  fx = px + 2; break;
        case Facing::Right: fx = px + kTile - 8; break;
        }
        renderer_.fillRectWorld(fx, fy + static_cast<int>(bob), 6, 6,
                                SDL_Color{255, 255, 255, 230});
    }
}

void Game::renderStatusBar() {
    const SDL_Rect bar{0, Renderer::kViewH, renderer_.width(), Renderer::kStatusBarH};
    renderer_.fillRect(bar, SDL_Color{10, 6, 18, 255});
    renderer_.outlineRect(bar, SDL_Color{60, 40, 80, 255});

    int textY = bar.y + (Renderer::kStatusBarH - assets_.fontHeight()) / 2;
    renderer_.drawText(assets_, room_->name(), 14, textY, SDL_Color{255, 179, 71, 255});
    renderer_.drawText(assets_, "~ " + audio_.currentMood(),
                       14 + renderer_.textWidth(assets_, room_->name()) + 18, textY,
                       SDL_Color{140, 120, 170, 255});

    std::string hint;
    switch (state_) {
    case State::Title:         break;  // no status bar on the title screen
    case State::InDialogue:    hint = "E: continue"; break;
    case State::InventoryOpen: hint = "I: close"; break;
    case State::Exploring: {
        int tx, ty;
        player_.facingTile(tx, ty);
        const Entity* e = room_->entityAt(tx, ty);
        hint = (e && e->kind() == EntityKind::Npc) ? "E: talk" : "I: inventory";
        break;
    }
    }
    std::string items = std::to_string(inventory_.items().size()) + " item"
                        + (inventory_.items().size() == 1 ? "" : "s");
    std::string right = (calm() ? std::string("calm   ") : std::string()) + items + "   " + hint;
    renderer_.drawText(assets_, right,
                       renderer_.width() - renderer_.textWidth(assets_, right) - 14,
                       textY, SDL_Color{200, 190, 215, 255});
}
