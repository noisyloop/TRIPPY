#include "Duel.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "Assets.h"
#include "Dialogue.h"
#include "Input.h"
#include "Renderer.h"
#include "TripFX.h"

// --- Boss scripts (extract to files later, like dialogue) ---------------------

namespace {

using K = DuelRound::Kind;

const std::unordered_map<std::string, DuelScript>& duelRegistry() {
    static const std::unordered_map<std::string, DuelScript> duels = [] {
        std::unordered_map<std::string, DuelScript> d;

        d["boss_mother_of_saucers"] = DuelScript{
            "MOTHER-OF-SAUCERS", "playful_major",
            "LITTLE ONE. YOU ARE VERY FAR FROM YOUR LIGHTS. SPEAK, AND I WILL "
            "DECIDE IF YOU ARE SIGNAL OR NOISE.",
            {
                {K::Talk, "WHY DO YOU WEAR A BODY?",
                 {{"To be visited.", 30, "YES. A BODY IS A GUEST ROOM. I LIKE YOU ALREADY."},
                  {"It came with the game.", 10, "HONEST. HONEST IS A COLOR TOO."},
                  {"Get out of my head.", -20, "I AM NOT IN YOUR HEAD. YOU ARE IN MINE. IT IS ROOMY."}}, 0, 0, "", ""},
                {K::Echo, "SING MY LIGHTS BACK TO ME.", {}, 4, 25,
                 "MY DRONES ARE WEEPING SMALL HAPPY SPARKS.",
                 "OFF-KEY. LIKE A COMET WITH HICCUPS."},
                {K::Talk, "WHAT IS BIGGER: THE SKY, OR THE WINDOW?",
                 {{"The window. It holds the sky.", 30, "OH. OH, THAT IS VERY GOOD."},
                  {"The sky.", 5, "CORRECT, BUT BORING. I HAVE BEEN CORRECT FOR NINE THOUSAND YEARS."},
                  {"I'm scared of heights.", -10, "THEN LOOK DOWN. DOWN IS ALSO SKY, FROM HERE."}}, 0, 0, "", ""},
                {K::Beat, "DANCE WITH MY ENGINE. STEP ON THE PULSE.", {}, 6, 25,
                 "THE SHIP PURRS. YOU HAVE A HULL-SHAPED HEART.",
                 "YOU STEPPED ON THE SILENCE. IT SQUEAKED."},
                {K::Talk, "WILL YOU STAY WITH US FOREVER?",
                 {{"I'll visit.", 35, "VISITING IS HOW FOREVER STAYS FRESH."},
                  {"Forever is a long time.", 15, "ONLY FROM THE INSIDE."},
                  {"No.", -15, "NO IS A DOOR. I RESPECT DOORS. BUT I AM SAD IN TWELVE COLORS."}}, 0, 0, "", ""},
            },
            "YOU ARE SIGNAL. RIDE THE BEAM UP, LITTLE ONE. THE JEWELED ONES ARE "
            "EXPECTING YOU. OR WILL HAVE BEEN.",
            "saucer_song", "",
        };

        d["boss_choir"] = DuelScript{
            "THE CHOIR OF TESSELLATORS", "dream_lydian",
            "WE ARE-- --ARE MANY. WE FINISH-- --EACH OTHER. FINISH US.",
            {
                {K::Talk, "THE OPPOSITE OF A DOOR IS--",
                 {{"--another door.", 30, "--YES! IT FOLDS, IT FOLDS!"},
                  {"--you.", 20, "--FLATTERING. --WE BLUSH IN HEXAGONS."},
                  {"--a wall.", -10, "--NO, NO. --WALLS ARE JUST SHY DOORS."}}, 0, 0, "", ""},
                {K::Echo, "SING THE SHAPE-- --OF THE SHAPE-- --OF THE SHAPE.", {}, 5, 25,
                 "--PERFECT. --PERFECTLY PERFECT. --STOP, WE'LL CRY GEMS.",
                 "--A WRONG NOTE! --DELICIOUS. --BUT WRONG."},
                {K::Talk, "WHEN YOU STOP NAMING THINGS, THEY--",
                 {{"--start naming you.", 30, "--AND WHAT DO THEY CALL YOU? --SHHH, LISTEN."},
                  {"--get quiet.", 15, "--QUIET IS JUST LOUD THAT SAT DOWN."},
                  {"--disappear.", -15, "--NO! --THEY GET BIGGER. --WATCH."}}, 0, 0, "", ""},
                {K::Beat, "CLAP WITH US-- --WITH ALL OUR HANDS-- --ON THE PULSE.", {}, 8, 25,
                 "--ONE OF US! --ONE OF US IS MANY OF US!",
                 "--YOU CLAPPED IN A GAP. --THE GAP IS UPSET."},
                {K::Talk, "THE SECRET IS--",
                 {{"--there's no secret, only more choir.", 35, "--HE KNOWS. --SHE KNOWS. --THEY KNOW. --WE KNOW."},
                  {"--I'm not telling.", 20, "--OH! --YOU'RE ONE OF US ALREADY."},
                  {"--behind the couch.", -10, "--WE CHECKED. --ONLY COINS AND TIME."}}, 0, 0, "", ""},
            },
            "--YOU FINISHED US. --NOW WE CAN START. --GO ON, VISITOR. --THE WAITING "
            "ROOM IS WAITING.",
            "choir_echo", "",
        };

        d["boss_customs"] = DuelScript{
            "CUSTOMS OFFICER OF THE FOURTH DIMENSION", "tense_chromatic",
            "STOP. PAPERS. ALL BEINGS PASSING INTO THE NEXT MOMENT MUST DECLARE "
            "THEIR PAST.",
            {
                {K::Talk, "PURPOSE OF YOUR VISIT?",
                 {{"Visiting.", 30, "...THAT IS CIRCULAR. APPROVED."},
                  {"I'm lost.", 15, "LOST IS A VALID DESTINATION. STAMP."},
                  {"Business.", -10, "WHAT BUSINESS? THE BUSINESS OF BEING? FILL OUT FORM INFINITY-B."}}, 0, 0, "", ""},
                {K::Talk, "DO YOU HAVE ANYTHING TO DECLARE?",
                 {{"A feeling with no name.", 30, "UNNAMED FEELINGS ARE DUTY-FREE. GO ON."},
                  {"Snacks.", 10, "ALL SNACKS MUST BE SHARED WITH THE OFFICER. ...THANK YOU."},
                  {"Nothing.", -15, "NOTHING IS HEAVILY TAXED HERE."}}, 0, 0, "", ""},
                {K::Echo, "REPEAT THE STAMP SEQUENCE EXACTLY. I AM WATCHING WITH ALL NINE EYES.", {}, 6, 25,
                 "IMPECCABLE PAPERWORK. I MAY FRAME IT.",
                 "SMUDGED. AGAIN. I HAVE ALL THE TIME. LITERALLY ALL OF IT."},
                {K::Talk, "HOW LONG DO YOU INTEND TO STAY IN THE PRESENT?",
                 {{"Just this once.", 30, "A RETURNING CUSTOMER. EXCELLENT."},
                  {"Forever.", 5, "NOBODY STAYS FOREVER. IT GETS CROWDED."},
                  {"What present?", -10, "THIS ONE. NO, THIS ONE. NO-- THERE IT GOES."}}, 0, 0, "", ""},
                {K::Beat, "STAMP ON THE BEAT. BUREAUCRACY IS A DANCE.", {}, 6, 25,
                 "STAMP. STAMP. STAMP. ...I FEEL SOMETHING. IS THIS JOY? FORM 3.",
                 "OFF THE BEAT. THE INK HAS FEELINGS, YOU KNOW."},
            },
            "...FINE. FINE! YOU ARE CLEARED. NOW IS RIGHT THROUGH THERE. MIND THE WEIGHT.",
            "visa_of_now", "",
        };

        d["boss_weight"] = DuelScript{
            "THE WEIGHT", "melancholy_a_minor",
            "Hello. I'm everything you carried in here. I'm very heavy. I'm very you.",
            {
                {K::Talk, "Why did you bring me?",
                 {{"I didn't know I could put you down.", 30, "...Nobody tells you that. I'm sorry nobody told you."},
                  {"To get stronger.", 10, "You did. Look at your arms. Tiny, but heroic."},
                  {"I didn't bring you.", -10, "Then who's been holding me all this time?"}}, 0, 0, "", ""},
                {K::Echo, "Remember how you walked in. Every step.", {}, 5, 25,
                 "...You remember. I thought only I remembered.",
                 "That's not how it went. It's okay. Nobody remembers it right."},
                {K::Talk, "If you put me down, who will you be?",
                 {{"Lighter.", 30, "Lighter. Yes. Light-er. Made of more light."},
                  {"I don't know.", 25, "That's the most honest thing anyone has said in here."},
                  {"Nobody.", -10, "Nobody is just somebody with the volume down."}}, 0, 0, "", ""},
                {K::Beat, "Walk with me. Slowly. Like a heartbeat.", {}, 8, 25,
                 "Thump. Thump. There you are.",
                 "Too fast. You were always too fast. It's okay."},
                {K::Talk, "Will you miss me?",
                 {{"A little.", 35, "Good. Missing is how you'll know you're free."},
                  {"Come with me, but lighter.", 20, "I'll come as a story, not a weight."},
                  {"No.", 10, "Ha. Liar. Go on anyway."}}, 0, 0, "", ""},
            },
            "Okay. I'm putting myself down now. ...Oh. Oh, that's nice. Wake up, "
            "visitor. Wake up all the way.",
            "feather_of_letting_go", "bedroom_awake",
        };

        return d;
    }();
    return duels;
}

uint32_t gRng = 0x2545F491u;
int rollArrow() {
    gRng = gRng * 1664525u + 1013904223u;
    return static_cast<int>((gRng >> 16) % 4);
}

// Arrow glyph as a filled triangle + stem (no font glyphs needed).
void drawArrow(Renderer& r, int dir, int cx, int cy, int size, SDL_Color c) {
    // Points for "up", rotated by dir * 90 degrees.
    const float pts[7][2] = {{0, -1}, {0.8f, -0.1f}, {-0.8f, -0.1f},
                             {-0.3f, -0.1f}, {0.3f, -0.1f}, {0.3f, 0.9f}, {-0.3f, 0.9f}};
    SDL_Vertex v[7];
    for (int i = 0; i < 7; ++i) {
        float x = pts[i][0], y = pts[i][1];
        for (int k = 0; k < dir; ++k) { float t = x; x = -y; y = t; }
        v[i] = SDL_Vertex{{cx + x * size, cy + y * size}, c, {0, 0}};
    }
    const int idx[] = {0, 1, 2, 3, 4, 5, 3, 5, 6};
    SDL_RenderGeometry(r.sdl(), nullptr, v, 7, idx, 9);
}

int arrowPressed(const Input& in) {
    if (in.wasPressed(SDL_SCANCODE_UP) || in.wasPressed(SDL_SCANCODE_W)) return 0;
    if (in.wasPressed(SDL_SCANCODE_RIGHT) || in.wasPressed(SDL_SCANCODE_D)) return 1;
    if (in.wasPressed(SDL_SCANCODE_DOWN) || in.wasPressed(SDL_SCANCODE_S)) return 2;
    if (in.wasPressed(SDL_SCANCODE_LEFT) || in.wasPressed(SDL_SCANCODE_A)) return 3;
    return -1;
}

constexpr double kCharsPerSec = 45.0;
constexpr double kShowStep = 0.6;  // seconds per arrow in the Echo demo

}  // namespace

const DuelScript* duelForBoss(const std::string& bossId) {
    const auto& reg = duelRegistry();
    auto it = reg.find(bossId);
    return it != reg.end() ? &it->second : nullptr;
}

// --- Duel --------------------------------------------------------------------

void Duel::start(const std::string& bossId, const DuelScript& script, Effects fx) {
    script_ = &script;
    bossId_ = bossId;
    fx_ = std::move(fx);
    meter_ = 35;
    displayMeter_ = 35;
    round_ = 0;
    won_ = false;
    gRng ^= SDL_GetTicks();
    phase_ = Phase::Intro;
    say(script.intro);
}

bool Duel::takeVictory() {
    bool w = won_;
    won_ = false;
    return w;
}

void Duel::say(const std::string& text) {
    text_ = text;
    shown_ = 0;
    lastBlip_ = 0;
    choice_ = 0;
}

void Duel::beginRound() {
    const DuelRound& rd = script_->rounds[round_];
    phase_ = Phase::Prompt;
    say(rd.line);
    hits_ = misses_ = 0;
    patternPos_ = 0;
    pattern_.clear();
    if (rd.kind == K::Echo)
        for (int i = 0; i < rd.length; ++i) pattern_.push_back(rollArrow());
}

void Duel::resolve(int delta, const std::string& response) {
    meter_ = std::clamp(meter_ + delta, 0, 100);
    if (delta < 0) { shake_ = 0.45; if (fx_.sfx) fx_.sfx("bump"); }
    else { sparkle_ = 0.8; if (fx_.sfx) fx_.sfx("confirm"); }
    phase_ = Phase::Response;
    say(response);
}

void Duel::update(double dt, const Input& input) {
    if (!script_) return;
    shake_ = std::max(0.0, shake_ - dt);
    sparkle_ = std::max(0.0, sparkle_ - dt);
    flash_ = std::max(0.0, flash_ - dt * 3);
    displayMeter_ += (meter_ - displayMeter_) * std::min(1.0, dt * 4);

    const bool interact = input.interactPressed();
    if (typing()) {
        shown_ = std::min<double>(text_.size(), shown_ + dt * kCharsPerSec);
        if (static_cast<int>(shown_) / 3 > lastBlip_ / 3 && fx_.sfx) fx_.sfx("blip");
        lastBlip_ = static_cast<int>(shown_);
        if (interact) shown_ = static_cast<double>(text_.size());
        return;
    }

    const DuelRound* rd = phase_ == Phase::Intro || phase_ == Phase::Victory
                              ? nullptr : &script_->rounds[round_];
    switch (phase_) {
    case Phase::Intro:
        if (interact) { round_ = 0; beginRound(); }
        break;

    case Phase::Prompt:
        if (rd->kind == K::Talk) {
            const int n = static_cast<int>(rd->replies.size());
            if (input.upPressed()) choice_ = (choice_ + n - 1) % n;
            if (input.downPressed()) choice_ = (choice_ + 1) % n;
            if (interact) resolve(rd->replies[choice_].delta, rd->replies[choice_].response);
        } else if (interact) {
            phase_ = rd->kind == K::Echo ? Phase::Show : Phase::Input;
            showTimer_ = 0;
            patternPos_ = 0;
            sinceBeat_ = 0;
            metronome_ = 0;
            lastBeat_ = fx_.beatCount ? fx_.beatCount() : 0;
        }
        break;

    case Phase::Show: {
        // Demo the pattern, one arrow at a time, then hand over.
        int before = static_cast<int>(showTimer_ / kShowStep);
        showTimer_ += dt;
        int now = static_cast<int>(showTimer_ / kShowStep);
        if (now != before && now < static_cast<int>(pattern_.size()) && fx_.sfx) fx_.sfx("blip");
        if (now >= static_cast<int>(pattern_.size())) { phase_ = Phase::Input; patternPos_ = 0; }
        break;
    }

    case Phase::Input: {
        const int a = arrowPressed(input);
        if (rd->kind == K::Echo) {
            if (a < 0) break;
            if (a == pattern_[patternPos_]) {
                ++patternPos_;
                flash_ = 1;
                if (fx_.sfx) fx_.sfx("blip");
                if (patternPos_ >= static_cast<int>(pattern_.size())) resolve(rd->delta, rd->success);
            } else {
                resolve(-rd->delta / 2, rd->failure);
            }
        } else {  // Beat
            // Follow the synth's beat; if it stops (recorded music), keep time ourselves.
            uint32_t b = fx_.beatCount ? fx_.beatCount() : 0;
            sinceBeat_ += dt;
            if (b != lastBeat_) {
                if (sinceBeat_ > 0.2 && sinceBeat_ < 1.6) beatInterval_ = sinceBeat_;
                lastBeat_ = b;
                sinceBeat_ = 0;
                metronome_ = 0;
            } else if (sinceBeat_ > 2.0) {
                metronome_ += dt;
                if (metronome_ >= 0.6) { metronome_ = 0; sinceBeat_ = 0; beatInterval_ = 0.6; }
            }
            if (a < 0) break;
            const bool onBeat = sinceBeat_ < 0.2 || beatInterval_ - sinceBeat_ < 0.15;
            if (onBeat) {
                ++hits_;
                flash_ = 1;
                if (fx_.sfx) fx_.sfx("blip");
                if (hits_ >= rd->length) resolve(rd->delta, rd->success);
            } else {
                ++misses_;
                shake_ = 0.2;
                if (fx_.sfx) fx_.sfx("bump");
                if (misses_ >= 3) resolve(-rd->delta / 2, rd->failure);
            }
        }
        break;
    }

    case Phase::Response:
        if (!interact) break;
        if (meter_ >= 100) {
            phase_ = Phase::Victory;
            say(script_->victory);
            if (fx_.sfx) fx_.sfx("pickup");
        } else if (meter_ <= 0) {
            meter_ = 35;
            say("(the conversation folds back on itself. it's fine. try again.)");
        } else {
            round_ = (round_ + 1) % static_cast<int>(script_->rounds.size());
            beginRound();
        }
        break;

    case Phase::Victory:
        if (interact) { won_ = true; script_ = nullptr; }
        break;
    }
}

void Duel::render(Renderer& r, Assets& assets, SDL_Texture* sprite, double time) const {
    if (!script_) return;
    const int w = r.width();
    const int sx = shake_ > 0 ? static_cast<int>(std::sin(time * 90) * 10 * shake_ / 0.45) : 0;

    r.fillRect(SDL_Rect{0, 0, w, Renderer::kViewH}, SDL_Color{6, 2, 18, 190});

    // Sparkle burst on good answers: a ring of hue-cycling squares.
    if (sparkle_ > 0) {
        const double t = 1.0 - sparkle_ / 0.8;
        for (int i = 0; i < 24; ++i) {
            double a = i * 6.2831853 / 24 + time;
            int px = w / 2 + static_cast<int>(std::cos(a) * (60 + t * 260));
            int py = 128 + static_cast<int>(std::sin(a) * (60 + t * 200));
            SDL_Color c = hsvColor(i * 15 + time * 200, 0.8, 1.0, static_cast<Uint8>(255 * (1 - t)));
            r.fillRect(SDL_Rect{px - 4, py - 4, 8, 8}, c);
        }
    }

    // The boss: big, bobbing, slowly hue-shifting, harmony makes it glow.
    const int size = 176;
    const int bob = static_cast<int>(std::sin(time * 1.7) * 8);
    if (sprite) {
        SDL_Color tint = lerpColor(hsvColor(time * 40, 0.5, 1.0), SDL_Color{255, 255, 255, 255},
                                   0.4 + 0.6 * harmony());
        SDL_SetTextureColorMod(sprite, tint.r, tint.g, tint.b);
        SDL_Rect dst{w / 2 - size / 2 + sx, 34 + bob, size, size};
        SDL_RenderCopy(r.sdl(), sprite, nullptr, &dst);
        SDL_SetTextureColorMod(sprite, 255, 255, 255);
    }
    r.drawTextCentered(assets, script_->name, w / 2 + sx, 20, SDL_Color{255, 179, 71, 255});

    // Resonance meter.
    const SDL_Rect bar{w / 2 - 180, 226, 360, 14};
    r.fillRect(bar, SDL_Color{20, 12, 36, 255});
    const int fill = static_cast<int>(bar.w * displayMeter_ / 100.0);
    for (int x = 0; x < fill; x += 6)
        r.fillRect(SDL_Rect{bar.x + x, bar.y, std::min(6, fill - x), bar.h},
                   hsvColor(x * 1.2 + time * 60, 0.75, 1.0));
    r.outlineRect(bar, SDL_Color{140, 120, 170, 255});
    r.drawTextCentered(assets, "RESONANCE", w / 2, bar.y + bar.h + 12, SDL_Color{140, 120, 170, 255});

    // Challenge visuals.
    const DuelRound* rd = (phase_ == Phase::Intro || phase_ == Phase::Victory) ? nullptr
                                                                              : &script_->rounds[round_];
    const int cy = 300;
    if (rd && rd->kind == K::Echo && (phase_ == Phase::Show || phase_ == Phase::Input)) {
        if (phase_ == Phase::Show) {
            int idx = static_cast<int>(showTimer_ / kShowStep);
            double within = std::fmod(showTimer_, kShowStep) / kShowStep;
            if (idx < static_cast<int>(pattern_.size()) && within < 0.8)
                drawArrow(r, pattern_[idx], w / 2, cy, 34, hsvColor(pattern_[idx] * 90 + 30, 0.8, 1.0));
        }
        const int n = static_cast<int>(pattern_.size());
        for (int i = 0; i < n; ++i) {
            int x = w / 2 + (i - n / 2) * 44 + (n % 2 ? 0 : 22);
            r.outlineRect(SDL_Rect{x - 18, cy + 40, 36, 36}, SDL_Color{90, 70, 120, 255});
            if (phase_ == Phase::Input && i < patternPos_)
                drawArrow(r, pattern_[i], x, cy + 58, 14, hsvColor(pattern_[i] * 90 + 30, 0.8, 1.0));
        }
        if (phase_ == Phase::Input)
            r.drawTextCentered(assets, "repeat it with the arrows", w / 2, cy - 10, SDL_Color{200, 190, 215, 255});
    } else if (rd && rd->kind == K::Beat && phase_ == Phase::Input) {
        // A ring that swells on each beat; hits flash it white.
        double phase = beatInterval_ > 0 ? std::clamp(sinceBeat_ / beatInterval_, 0.0, 1.0) : 0;
        int rad = static_cast<int>(26 + (1 - phase) * 30);
        SDL_Color c = flash_ > 0 ? SDL_Color{255, 255, 255, 255} : hsvColor(time * 120, 0.8, 1.0);
        for (int k = 0; k < 3; ++k)
            r.outlineRect(SDL_Rect{w / 2 - rad - k, cy + 20 - rad - k, 2 * (rad + k), 2 * (rad + k)}, c);
        r.drawTextCentered(assets, "press any arrow on the beat   " + std::to_string(hits_) + "/"
                                       + std::to_string(rd->length),
                           w / 2, cy - 30, SDL_Color{200, 190, 215, 255});
    }

    // Text box.
    const int boxH = 150;
    const SDL_Rect box{12, Renderer::kViewH - boxH - 12, w - 24, boxH};
    r.fillRect(box, SDL_Color{16, 10, 28, 240});
    r.outlineRect(box, hsvColor(time * 50, 0.7, 1.0));
    int y = box.y + 10;
    std::string visible = text_.substr(0, static_cast<size_t>(shown_));
    for (const auto& line : wrapText(assets, visible, box.w - 28)) {
        r.drawText(assets, line, box.x + 14, y, SDL_Color{235, 231, 240, 255});
        y += assets.fontHeight() + 2;
    }
    if (typing()) return;
    if (rd && phase_ == Phase::Prompt && rd->kind == K::Talk) {
        y += 4;
        for (size_t i = 0; i < rd->replies.size(); ++i) {
            bool sel = static_cast<int>(i) == choice_;
            r.drawText(assets, (sel ? "> " : "  ") + rd->replies[i].label, box.x + 28, y,
                       sel ? SDL_Color{255, 179, 71, 255} : SDL_Color{160, 150, 175, 255});
            y += assets.fontHeight() + 2;
        }
    } else if (phase_ != Phase::Show && phase_ != Phase::Input) {
        r.drawText(assets, "[E]", box.x + box.w - 44, box.y + box.h - assets.fontHeight() - 8,
                   SDL_Color{214, 62, 106, 255});
    }
}
