#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// A mood is everything needed to deterministically generate a looping track:
// same seed + scale + tempo => same music every run.
struct Mood {
    std::string name;
    std::vector<int> scale;   // semitone offsets from the root, e.g. {0,2,3,5,7,8,10}
    int rootNote = 57;        // MIDI note number (57 = A3)
    double bpm = 100.0;
    uint32_t seed = 1;
    float melodyDuty = 0.5f;  // square-wave duty cycle for the melody voice
    float brightness = 1.0f;  // scales overall level a touch per mood
    int noiseDensity = 2;     // 0..4, how busy the percussion is
    // Other scales this mood may pick on a visit (the main `scale` is always
    // a candidate too). Root also wanders +-3 semitones, tempo +-8%.
    std::vector<std::vector<int>> altScales;
};

// Registry of the built-in moods. Unknown names fall back to the first entry.
const Mood& moodByName(const std::string& name);
std::vector<std::string> allMoodNames();

// Procedural chiptune generator: melody, bass, arpeggio, noise + a drone.
// render() is called from the audio callback thread; setMood()/setMotion()
// from the main thread. Crossfades between tracks over kCrossfadeSeconds.
//
// Every setMood() call with a new visit seed rolls a fresh tune within the
// mood (scale, root, tempo, pattern). setMotion() makes it play along with
// the player: energy drives tempo and melody density, turns change the arp
// pattern/octave, and standing still fades the melody into a slow drone.
class ChiptuneSynth {
public:
    static constexpr int kSampleRate = 44100;
    static constexpr double kCrossfadeSeconds = 1.5;

    void setMood(const std::string& name, uint32_t visitSeed = 0);

    // energy: 0 = standing still for a while .. 1 = moving for a while.
    // facing: 0 down, 1 up, 2 left, 3 right. turns: running count of
    // direction changes.
    void setMotion(float energy, int facing, int turns);
    // Adds a voice that shadows the melody a third above (the Humming Pebble).
    void setHarmony(bool on) { control_.harmony.store(on, std::memory_order_relaxed); }

    // Counts beats (quarter notes) of the audible track, for visual pulses.
    uint32_t beatCount() const { return beats_.load(std::memory_order_relaxed); }

    // Fills `frames` mono int16 samples. Audio-thread only.
    void render(int16_t* out, int frames);

private:
    struct Control {
        std::atomic<float> energy{0};
        std::atomic<int> facing{0};
        std::atomic<int> turns{0};
        std::atomic<bool> harmony{false};
    };

    // One playing pattern: 4 voices stepping through a 64-step (4 bar) loop.
    struct Voice {
        double phase = 0;
        double freq = 0;
        double noteAge = 1e9;   // seconds since note-on
        double gate = 0;        // seconds the note stays held
        bool isNoise = false;
        uint32_t lfsr = 0xACE1u;
        float duty = 0.5f;
        // ADSR (seconds, sustain is a level)
        float attack = 0.005f, decay = 0.08f, sustain = 0.5f, release = 0.05f;
        float gain = 0.25f;

        float sample(double dt);
        void noteOn(double frequency, double gateSeconds);
    };

    struct Track {
        Mood mood;
        std::array<Voice, 4> voices;  // melody, bass, arp, noise
        Voice harmonyVoice;           // optional third above the melody
        // Pattern data, one entry per 16th-note step over 4 bars.
        // melody/bass/arpChord hold precomputed MIDI notes, -1 = rest.
        // arpChord is the bar's chord as a scale degree; the arp pattern
        // over it is chosen live from the player's turns. roll gates the
        // melody note: it plays when roll < density (from energy).
        struct Step { int melody = -1; int melodyDegree = 0; int bass = -1; int arpChord = -1; bool noise = false; bool accent = false; float roll = 0; };
        std::vector<Step> steps;
        double stepTimer = 0;
        int stepIndex = 0;
        bool active = false;
        uint32_t seed = 0;
        const Control* ctl = nullptr;

        // Smoothed live controls (audio thread).
        float energy = 0, melodyLevel = 0, droneLevel = 1;
        double dronePhase[2] = {0, 0};
        double droneTime = 0;
        int arpPos = 0;

        void start(const Mood& m, uint32_t visitSeed, const Control* control);
        // Advance one sample, return mixed sample [-1,1]. `beats` is bumped
        // on each quarter note when non-null.
        float tick(double dt, std::atomic<uint32_t>* beats);
    private:
        void generatePattern();
        void advanceStep(std::atomic<uint32_t>* beats);
        int scaleNote(int degree, int octave) const;
        double stepSeconds() const;
    };

    Track a_, b_;                 // crossfade pair; one fades out, the other in
    Track* current_ = &a_;
    Track* next_ = nullptr;
    double fade_ = 0;             // 0..1 progress of crossfade

    std::mutex pendingMutex_;
    std::string pendingMood_;
    uint32_t pendingSeed_ = 0;
    bool hasPending_ = false;

    Control control_;
    std::atomic<uint32_t> beats_{0};
};
