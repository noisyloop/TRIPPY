#include "ChiptuneSynth.h"

#include <algorithm>
#include <cmath>

namespace {

// Deterministic LCG so patterns are stable across runs and platforms.
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1) {}
    uint32_t next() { s = s * 1664525u + 1013904223u; return s; }
    int range(int n) { return static_cast<int>((next() >> 8) % static_cast<uint32_t>(n)); }
    bool chance(int percent) { return range(100) < percent; }
};

double midiToFreq(int note) {
    return 440.0 * std::pow(2.0, (note - 69) / 12.0);
}

const std::vector<Mood>& moodTable() {
    // Alternate scales keep each mood's character while every visit differs.
    static const std::vector<int> dorian{0, 2, 3, 5, 7, 9, 10}, harmMinor{0, 2, 3, 5, 7, 8, 11},
        minorPent{0, 3, 5, 7, 10}, phrygian{0, 1, 3, 5, 7, 8, 10}, locrian{0, 1, 3, 5, 6, 8, 10},
        phrygDom{0, 1, 4, 5, 7, 8, 10}, susPent{0, 2, 5, 7, 9}, mixolydian{0, 2, 4, 5, 7, 9, 10},
        wholeTone{0, 2, 4, 6, 8, 10}, doubleHarm{0, 1, 4, 5, 7, 8, 11},
        lydDom{0, 2, 4, 6, 7, 9, 10}, lydAug{0, 2, 4, 6, 8, 9, 11}, majPent{0, 2, 4, 7, 9};
    static const std::vector<Mood> moods = {
        // name                scale (semitones)          root  bpm  seed  duty  bright noise  alt scales
        {"melancholy_a_minor", {0, 2, 3, 5, 7, 8, 10},      57,  76, 0xA110, 0.50f, 0.85f, 1, {dorian, harmMinor, minorPent}},
        {"tense_chromatic",    {0, 1, 3, 4, 6, 7, 9, 10},   58, 118, 0xBADD, 0.25f, 0.95f, 3, {phrygian, locrian, phrygDom}},
        {"calm_pentatonic",    {0, 2, 4, 7, 9},             60,  84, 0xCA1B, 0.50f, 0.80f, 1, {susPent, mixolydian, majPent}},
        {"dread_diminished",   {0, 2, 3, 5, 6, 8, 9, 11},   55,  66, 0xD3AD, 0.125f, 0.90f, 2, {wholeTone, doubleHarm, locrian}},
        {"playful_major",      {0, 2, 4, 5, 7, 9, 11},      62, 132, 0x91A9, 0.50f, 1.00f, 3, {lydDom, mixolydian, majPent}},
        {"dream_lydian",       {0, 2, 4, 6, 7, 9, 11},      59,  92, 0xD12A, 0.375f, 0.85f, 2, {wholeTone, lydAug, majPent}},
    };
    return moods;
}

}  // namespace

const Mood& moodByName(const std::string& name) {
    for (const auto& m : moodTable())
        if (m.name == name) return m;
    return moodTable().front();
}

std::vector<std::string> allMoodNames() {
    std::vector<std::string> names;
    for (const auto& m : moodTable()) names.push_back(m.name);
    return names;
}

// ---------------------------------------------------------------- Voice ----

void ChiptuneSynth::Voice::noteOn(double frequency, double gateSeconds) {
    freq = frequency;
    gate = gateSeconds;
    noteAge = 0;
    // Retrigger the phase so attacks click consistently (very chiptune).
    phase = 0;
}

float ChiptuneSynth::Voice::sample(double dt) {
    noteAge += dt;

    float env;
    if (noteAge < attack)
        env = static_cast<float>(noteAge / attack);
    else if (noteAge < attack + decay)
        env = 1.0f - (1.0f - sustain) * static_cast<float>((noteAge - attack) / decay);
    else
        env = sustain;
    if (noteAge > gate)
        env *= std::max(0.0f, 1.0f - static_cast<float>((noteAge - gate) / release));
    if (env <= 0.0f)
        return 0.0f;

    float raw;
    if (isNoise) {
        // LFSR white noise clocked by `freq` (higher = brighter hit).
        phase += freq * dt;
        while (phase >= 1.0) {
            phase -= 1.0;
            uint32_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1u;
            lfsr = (lfsr >> 1) | (bit << 15);
        }
        raw = (lfsr & 1u) ? 1.0f : -1.0f;
    } else {
        phase += freq * dt;
        phase -= std::floor(phase);
        raw = (phase < duty) ? 1.0f : -1.0f;
    }
    return raw * env * gain;
}

// ---------------------------------------------------------------- Track ----

void ChiptuneSynth::Track::start(const Mood& m, uint32_t visitSeed, const Control* control) {
    mood = m;
    seed = mood.seed ^ (visitSeed * 2654435761u);
    ctl = control;
    stepTimer = 0;
    stepIndex = 0;
    arpPos = 0;
    active = true;
    // Start wherever the player currently is (no jump in tempo/level).
    energy = ctl ? ctl->energy.load(std::memory_order_relaxed) : 0.0f;
    melodyLevel = std::clamp((energy - 0.05f) / 0.35f, 0.0f, 1.0f);
    droneLevel = 1.0f - std::clamp(energy / 0.35f, 0.0f, 1.0f);

    // Roll this visit's variation of the mood.
    if (visitSeed) {
        Rng rng(seed ^ 0x5EEDu);
        int pick = rng.range(static_cast<int>(mood.altScales.size()) + 1);
        if (pick > 0) mood.scale = mood.altScales[pick - 1];
        mood.rootNote += rng.range(7) - 3;
        mood.bpm *= 0.92 + rng.range(17) / 100.0;
    }

    auto& melody = voices[0];
    melody.duty = mood.melodyDuty;
    melody.gain = 0.22f;
    melody.attack = 0.004f; melody.decay = 0.10f; melody.sustain = 0.55f; melody.release = 0.06f;

    auto& bass = voices[1];
    bass.duty = 0.5f;
    bass.gain = 0.20f;
    bass.attack = 0.006f; bass.decay = 0.05f; bass.sustain = 0.7f; bass.release = 0.04f;

    auto& arp = voices[2];
    arp.duty = 0.25f;
    arp.gain = 0.10f;
    arp.attack = 0.002f; arp.decay = 0.03f; arp.sustain = 0.3f; arp.release = 0.03f;

    auto& noise = voices[3];
    noise.isNoise = true;
    noise.gain = 0.14f;
    noise.attack = 0.001f; noise.decay = 0.04f; noise.sustain = 0.2f; noise.release = 0.05f;

    generatePattern();
}

void ChiptuneSynth::Track::generatePattern() {
    constexpr int kBars = 4;
    constexpr int kStepsPerBar = 16;
    steps.assign(kBars * kStepsPerBar, {});

    Rng rng(seed);
    const int degrees = static_cast<int>(mood.scale.size());

    // A tiny chord progression: one scale degree per bar, always coming home.
    std::array<int, kBars> chordDegree{};
    chordDegree[0] = 0;
    for (int bar = 1; bar < kBars; ++bar)
        chordDegree[bar] = rng.range(degrees);
    chordDegree[kBars - 1] = rng.chance(50) ? 0 : chordDegree[kBars - 1];

    int melodyDegree = degrees;  // start an octave up, on the root
    for (int bar = 0; bar < kBars; ++bar) {
        int chord = chordDegree[bar];
        for (int i = 0; i < kStepsPerBar; ++i) {
            auto& st = steps[bar * kStepsPerBar + i];

            // Melody: a lazy random walk with a candidate note on every
            // step. Each gets a roll; the player's energy decides how many
            // play (on-beat ones first, faster moods a little busier).
            melodyDegree += rng.range(5) - 2;  // -2..+2 steps
            melodyDegree = std::clamp(melodyDegree, degrees - 2, 2 * degrees + 3);
            st.melody = scaleNote(melodyDegree, 1);
            st.accent = (i % 8 == 0);
            float r = rng.range(1000) / 1000.0f;
            st.roll = (i % 2 == 0) ? r * 0.55f : 0.35f + r * 0.65f;
            if (mood.bpm > 110) st.roll *= 0.85f;

            // Bass: root of the bar's chord on strong beats, fifth as a pickup.
            if (i % 4 == 0)
                st.bass = scaleNote(chord, -1);
            else if (i % 4 == 2 && rng.chance(60))
                st.bass = scaleNote(chord + 4, -1);

            // Arpeggio: pattern over this chord is picked live (see advanceStep).
            st.arpChord = chord;

            // Percussion: kick on the downbeats, hats/snare by density.
            bool kick = (i % 8 == 0);
            bool snare = (i % 8 == 4) && mood.noiseDensity >= 2;
            bool hat = (i % 2 == 0) && mood.noiseDensity >= 3 && rng.chance(70);
            st.noise = kick || snare || hat;
        }
    }
}

int ChiptuneSynth::Track::scaleNote(int degree, int octave) const {
    const int degrees = static_cast<int>(mood.scale.size());
    int wrapped = ((degree % degrees) + degrees) % degrees;
    int octShift = (degree - wrapped) / degrees;
    return mood.rootNote + mood.scale[wrapped] + 12 * (octave + octShift);
}

double ChiptuneSynth::Track::stepSeconds() const {
    // Still = 0.6x tempo, sprinting around = 1.2x.
    return 60.0 / (mood.bpm * (0.6 + 0.6 * energy)) / 4.0;
}

void ChiptuneSynth::Track::advanceStep(std::atomic<uint32_t>* beats) {
    const double stepSec = stepSeconds();
    const auto& st = steps[stepIndex];
    const int degrees = static_cast<int>(mood.scale.size());

    const float density = 0.2f + 0.8f * energy;
    if (st.melody >= 0 && st.roll < density)
        voices[0].noteOn(midiToFreq(st.melody), stepSec * (st.accent ? 1.8 : 0.9));
    if (st.bass >= 0)
        voices[1].noteOn(midiToFreq(st.bass), stepSec * 0.95);
    if (st.arpChord >= 0) {
        // Each turn the player makes switches the arp shape; facing up or
        // right lifts it an octave.
        static const int kPatterns[4][4] = {{0, 2, 4, 2}, {4, 2, 0, 2}, {0, 4, 2, 7}, {0, 2, 4, 6}};
        const int turns = ctl ? ctl->turns.load(std::memory_order_relaxed) : 0;
        const int facing = ctl ? ctl->facing.load(std::memory_order_relaxed) : 0;
        int tone = kPatterns[turns % 4][arpPos % 4];
        if (tone == 7) tone = degrees;  // the octave, in scale degrees
        int octave = (facing == 1 || facing == 3) ? 1 : 0;
        voices[2].noteOn(midiToFreq(scaleNote(st.arpChord + tone, octave)), stepSec * 0.5);
        ++arpPos;
    }
    if (st.noise) {
        bool onBeat = (stepIndex % 8 == 0);
        voices[3].noteOn(onBeat ? 900.0 : 5500.0, stepSec * 0.3);
    }
    if (beats && stepIndex % 4 == 0)
        beats->fetch_add(1, std::memory_order_relaxed);

    stepIndex = (stepIndex + 1) % static_cast<int>(steps.size());
}

float ChiptuneSynth::Track::tick(double dt, std::atomic<uint32_t>* beats) {
    // Ease toward the player's live energy (~0.5s), then the layer levels.
    const float target = ctl ? ctl->energy.load(std::memory_order_relaxed) : 0.0f;
    energy += (target - energy) * static_cast<float>(dt * 2.0);
    const float melodyTarget = std::clamp((energy - 0.05f) / 0.35f, 0.0f, 1.0f);
    const float droneTarget = 1.0f - std::clamp(energy / 0.35f, 0.0f, 1.0f);
    melodyLevel += (melodyTarget - melodyLevel) * static_cast<float>(dt * 1.5);
    droneLevel += (droneTarget - droneLevel) * static_cast<float>(dt * 1.0);

    stepTimer += dt;
    const double stepSec = stepSeconds();
    if (stepTimer >= stepSec) {
        stepTimer -= stepSec;
        advanceStep(beats);
    }
    float mix = voices[0].sample(dt) * melodyLevel
              + voices[1].sample(dt)
              + voices[2].sample(dt) * (0.5f + 0.5f * energy)
              + voices[3].sample(dt) * (0.3f + 0.7f * energy);

    // Drone: root and fifth an octave down as soft triangles, slowly
    // breathing, slightly detuned so it shimmers.
    droneTime += dt;
    if (droneLevel > 0.001f) {
        const double base = midiToFreq(mood.rootNote - 12);
        const double freqs[2] = {base, base * 1.4983 * 1.002};
        float drone = 0;
        for (int i = 0; i < 2; ++i) {
            dronePhase[i] += freqs[i] * dt;
            dronePhase[i] -= std::floor(dronePhase[i]);
            drone += static_cast<float>(4.0 * std::fabs(dronePhase[i] - 0.5) - 1.0);
        }
        const float breath = 0.7f + 0.3f * static_cast<float>(std::sin(droneTime * 2 * M_PI * 0.12));
        mix += drone * 0.11f * breath * droneLevel;
    }
    // Gentle soft clip keeps chord stacks from cracking.
    return std::tanh(mix * 1.2f) * mood.brightness;
}

// ---------------------------------------------------------------- Synth ----

void ChiptuneSynth::setMood(const std::string& name, uint32_t visitSeed) {
    std::lock_guard lock(pendingMutex_);
    pendingMood_ = name;
    pendingSeed_ = visitSeed;
    hasPending_ = true;
}

void ChiptuneSynth::setMotion(float energy, int facing, int turns) {
    control_.energy.store(std::clamp(energy, 0.0f, 1.0f), std::memory_order_relaxed);
    control_.facing.store(facing, std::memory_order_relaxed);
    control_.turns.store(turns, std::memory_order_relaxed);
}

void ChiptuneSynth::render(int16_t* out, int frames) {
    {
        std::lock_guard lock(pendingMutex_);
        if (hasPending_) {
            const Mood& m = moodByName(pendingMood_);
            hasPending_ = false;
            bool alreadyPlaying = current_->active && current_->mood.name == m.name
                                  && current_->seed == (m.seed ^ (pendingSeed_ * 2654435761u)) && !next_;
            if (!alreadyPlaying) {
                if (!current_->active) {
                    current_->start(m, pendingSeed_, &control_);  // first mood: no fade
                } else {
                    Track* other = (current_ == &a_) ? &b_ : &a_;
                    other->start(m, pendingSeed_, &control_);
                    next_ = other;
                    fade_ = 0;
                }
            }
        }
    }

    const double dt = 1.0 / kSampleRate;
    for (int i = 0; i < frames; ++i) {
        // Beats come from whichever track is more audible.
        const bool nextLeads = next_ && fade_ > 0.5;
        float s = current_->active ? current_->tick(dt, nextLeads ? nullptr : &beats_) : 0.0f;
        if (next_) {
            fade_ = std::min(1.0, fade_ + dt / kCrossfadeSeconds);
            float t = static_cast<float>(fade_);
            // Equal-power crossfade.
            s = s * std::cos(t * static_cast<float>(M_PI_2))
              + next_->tick(dt, nextLeads ? &beats_ : nullptr) * std::sin(t * static_cast<float>(M_PI_2));
            if (fade_ >= 1.0) {
                current_->active = false;
                current_ = next_;
                next_ = nullptr;
            }
        }
        int v = static_cast<int>(s * 22000.0f);
        out[i] = static_cast<int16_t>(std::clamp(v, -32768, 32767));
    }
}
