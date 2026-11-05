#pragma once

#include <functional>
#include <string>
#include <vector>

class Renderer;
class Assets;
class Input;

// --- Data model -------------------------------------------------------------

struct DialogueChoice {
    std::string label;
    int next = -1;  // node index, -1 ends the conversation
};

struct DialogueNode {
    std::string speaker;
    std::string text;
    int next = -1;                        // ignored when choices are present
    std::vector<DialogueChoice> choices;  // shown once the text finishes
    std::string setFlag;                  // story flag set when node is shown
    std::string giveItem;                 // item id granted when node is shown
};

struct DialogueTree {
    std::vector<DialogueNode> nodes;
    int start = 0;
    // If this flag is already set, start at altStart instead (repeat visits).
    std::string altFlag;
    int altStart = -1;
};

// Inline text effects in dialogue: ~wavy~, *rainbow*, ^shaky^. The markers
// are not shown. stripMarkup returns the visible text only.
std::string stripMarkup(const std::string& text);

// Greedy word wrap against a pixel width (shared with the boss duels).
std::vector<std::string> wrapText(Assets& assets, const std::string& text, int maxWidth);

// Trees live in code for now; looked up by NPC id. Null if unknown.
const DialogueTree* dialogueForNpc(const std::string& npcId);

// --- Runtime box ------------------------------------------------------------

// Typewriter dialogue box at the bottom of the screen.
// Interact advances (or skips the typewriter); up/down pick a choice.
class DialogueBox {
public:
    struct Effects {
        std::function<bool(const std::string&)> hasFlag;
        std::function<void(const std::string&)> setFlag;
        std::function<void(const std::string&)> giveItem;
        std::function<void()> onCharTyped;  // e.g. play a blip every few chars
    };

    // portrait = sprite name drawn, alive, beside the text (may be empty).
    void start(const DialogueTree& tree, Effects effects, std::string portrait = "");
    bool active() const { return tree_ != nullptr; }

    void update(double dt, const Input& input);
    // intensity 0..1 scales the wobbling text and portrait (0 in calm mode).
    void render(Renderer& r, Assets& assets, double time, double intensity, double pulse) const;

private:
    void enterNode(int index);
    const DialogueNode& node() const { return tree_->nodes[nodeIndex_]; }

    const DialogueTree* tree_ = nullptr;
    std::string portrait_;
    double age_ = 0;  // seconds since the node appeared (for entry effects)
    Effects effects_;
    int nodeIndex_ = 0;
    double shownChars_ = 0;
    int lastBlipAt_ = 0;
    int choiceIndex_ = 0;

    static constexpr double kCharsPerSec = 40.0;
};
