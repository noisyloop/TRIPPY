#pragma once

#include <SDL.h>

#include <functional>
#include <string>
#include <vector>

class Renderer;
class Assets;
class Input;

// --- Data model -------------------------------------------------------------

// Boss "fights" are conversations. Each round either asks a question (Talk),
// shows a pattern of arrows to repeat (Echo), or asks the player to press a
// direction on the music's beat (Beat). Answers move a resonance meter; the
// boss is won over when it reaches 100. Hitting 0 just resets it to 35:
// nobody loses, the conversation simply goes around again.
struct DuelReply {
    std::string label;
    int delta = 0;          // resonance change
    std::string response;   // what the boss says back
};

struct DuelRound {
    enum class Kind { Talk, Echo, Beat };
    Kind kind = Kind::Talk;
    std::string line;
    std::vector<DuelReply> replies;  // Talk only
    int length = 4;                  // Echo: arrows to repeat; Beat: hits needed
    int delta = 25;                  // Echo/Beat: gain on success, -delta/2 on failure
    std::string success, failure;    // Echo/Beat responses
};

struct DuelScript {
    std::string name;     // shown above the boss
    std::string mood;     // music mood during the duel
    std::string intro;
    std::vector<DuelRound> rounds;
    std::string victory;
    std::string rewardItem;  // optional
    std::string afterRoom;   // optional: room to wake up in after winning
};

// Registered in code by boss id ("boss_*"). Null if unknown.
const DuelScript* duelForBoss(const std::string& bossId);

// --- Runtime ----------------------------------------------------------------

class Duel {
public:
    struct Effects {
        std::function<void(const std::string&)> sfx;  // "blip", "confirm", "bump", ...
        std::function<uint32_t()> beatCount;          // synth beat counter
    };

    void start(const std::string& bossId, const DuelScript& script, Effects fx);
    bool active() const { return script_ != nullptr; }
    // True once, right after a duel is won (consumed by the caller).
    bool takeVictory();
    const std::string& bossId() const { return bossId_; }
    const DuelScript* script() const { return script_; }

    double harmony() const { return meter_ / 100.0; }  // 0..1, drives visuals

    void update(double dt, const Input& input);
    // sprite: the boss texture, drawn large and alive.
    void render(Renderer& r, Assets& assets, SDL_Texture* sprite, double time) const;

private:
    enum class Phase { Intro, Prompt, Show, Input, Response, Victory };
    void beginRound();
    void say(const std::string& text);
    void resolve(int delta, const std::string& response);
    bool typing() const { return shown_ < static_cast<double>(text_.size()); }

    const DuelScript* script_ = nullptr;
    std::string bossId_;
    Effects fx_;
    Phase phase_ = Phase::Intro;
    int round_ = 0;
    int meter_ = 35;
    double displayMeter_ = 35;
    bool won_ = false;

    std::string text_;
    double shown_ = 0;
    int lastBlip_ = 0;
    int choice_ = 0;

    // Echo
    std::vector<int> pattern_;  // 0 up, 1 right, 2 down, 3 left
    int patternPos_ = 0;
    double showTimer_ = 0;

    // Beat
    int hits_ = 0, misses_ = 0;
    uint32_t lastBeat_ = 0;
    double sinceBeat_ = 0, beatInterval_ = 0.6, metronome_ = 0;
    double flash_ = 0;   // feedback flash on hits

    double shake_ = 0;    // seconds of shake left (bad answers)
    double sparkle_ = 0;  // seconds of sparkle left (good answers)
};
