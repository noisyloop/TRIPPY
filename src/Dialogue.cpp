#include "Dialogue.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>

#include "Assets.h"
#include "Input.h"
#include "Renderer.h"

// --- Dialogue content (extract to files later) -------------------------------

namespace {

const std::unordered_map<std::string, DialogueTree>& dialogueRegistry() {
    static const std::unordered_map<std::string, DialogueTree> trees = [] {
        std::unordered_map<std::string, DialogueTree> t;

        t["npc_mother"] = DialogueTree{
            {
                {"MOM", "You slept through the whole afternoon, sweetheart. "
                        "The Strip is already lighting up out there.", 1, {}, "talked_mother", ""},
                {"MOM", "Your father used to say the neon looks like the city "
                        "apologizing for the desert. Do you miss him too?", -1,
                 {{"Every day.", 2}, {"I don't want to talk about it.", 3}}, "", ""},
                {"MOM", "Me too. Every single day. Go on, get some air -- "
                        "just be back before the lights go out.", -1, {}, "misses_father", ""},
                {"MOM", "That's alright. The feeling waits for you either way. "
                        "Go on, get some air.", -1, {}, "", ""},
            },
            0, "talked_mother", 4,
        };
        t["npc_mother"].nodes.push_back(
            {"MOM", "Still here? The casino floor never closes, and neither "
                    "does a mother's worrying.", -1, {}, "", ""});

        t["npc_dealer"] = DialogueTree{
            {
                {"DEALER", "This hallway is longer than it looks, kid. "
                           "Everything in this town is longer than it looks.", 1, {}, "met_dealer", ""},
                {"DEALER", "House always wins, they say. But the house never "
                           "says sorry. Remember that when you reach the garden.", -1, {}, "", ""},
            },
            0, "met_dealer", 2,
        };
        t["npc_dealer"].nodes.push_back(
            {"DEALER", "Cards down, kid. The garden is waiting east of nowhere.", -1, {}, "", ""});

        t["npc_shadow"] = DialogueTree{
            {
                {"???", "Careful. The grass here remembers footsteps that haven't happened yet.", 1, {}, "met_shadow", ""},
                {"???", "...but you already stopped, didn't you. Something in "
                        "the grass is waiting for you. It has your handwriting.", -1, {}, "", ""},
            },
            0, "got_apology_note", 2,
        };
        t["npc_shadow"].nodes.push_back(
            {"???", "You found it. Some words weigh more than the whole "
                    "desert. Carry it gently.", -1, {}, "", ""});

        // --- TRIPPY: the strange rooms past the garden ---------------------

        t["npc_hallkeeper"] = DialogueTree{
            {
                {"THE COUNTER", "Welcome back. That's visit number... oh. "
                                "It's always visit number one.", 1, {}, "met_counter", ""},
                {"THE COUNTER", "Ask me something. I'm the part of the hallway "
                                "that noticed itself.", -1,
                 {{"How do I leave?", 2}, {"Why does east go east?", 3}}, "", ""},
                {"THE COUNTER", "Leaving is a direction. Try one that isn't east. "
                                "Up is a direction too, if you squint.", -1, {}, "", ""},
                {"THE COUNTER", "East is shy. It only ever meets itself.", -1, {}, "", ""},
                {"THE COUNTER", "Again! Again! The hallway loves you in a circle.", -1, {}, "", ""},
            },
            0, "met_counter", 4,
        };

        t["ufo_pilgrim"] = DialogueTree{
            {
                {"PILGRIM", "bzzt. we have been parked here since tomorrow.", 1, {}, "", ""},
                {"PILGRIM", "the hallway is a ring. the ring is a halo. "
                            "the halo is on you. bzzt.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["ufo_glimmer"] = DialogueTree{
            {
                {"GLIMMER", "Hello, small green certainty. Your planet is very "
                            "loud from up here.", 1, {}, "met_glimmer", ""},
                {"GLIMMER", "Mostly it hums in the key of worry. "
                            "We hum back in the key of 'it's okay'.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["ufo_humm"] = DialogueTree{
            {
                {"HUMM", "mmmmmmmmmmmmmmmmmmmm.", 1, {}, "", ""},
                {"", "(it is humming the exact shape of your name)", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["ufo_wobble"] = DialogueTree{
            {
                {"WOBBLE", "We came for the cows but stayed for the colors. "
                           "Have you seen the color nine?", -1,
                 {{"Yes.", 1}, {"What's nine?", 2}}, "", ""},
                {"WOBBLE", "Then you know why our lights never stop smiling.", -1, {}, "", ""},
                {"WOBBLE", "Close all your eyes. Not those ones. The other ones. "
                           "The sky folds west from here.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_tessellator"] = DialogueTree{
            {
                {"THE TESSELLATOR", "OH! A visitor! A visitor made of visitor! "
                                    "I folded this greeting just for you.", 1, {}, "met_tessellator", ""},
                {"THE TESSELLATOR", "It is a gift made of smaller gifts. Hold it "
                                    "with your inside hands.", -1,
                 {{"It's beautiful.", 2}, {"I don't understand.", 3}}, "", ""},
                {"THE TESSELLATOR", "Everything is, once you stop naming it. "
                                    "North is waking up. Waking up is also a room.", -1, {}, "", ""},
                {"THE TESSELLATOR", "Perfect! Understanding is just a smaller room. "
                                    "You are much too big for it now.", -1, {}, "", ""},
                {"THE TESSELLATOR", "Still unfolding? Good. The temple likes you "
                                    "best half-open.", -1, {}, "", ""},
            },
            0, "met_tessellator", 4,
        };

        t["npc_mirror_you"] = DialogueTree{
            {
                {"YOU (AGAIN)", "Don't panic. I'm the version of you that already "
                                "got home.", 1, {}, "", ""},
                {"YOU (AGAIN)", "Spoiler: home was also a spaceship. "
                                "It was the whole time.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["ufo_greeter"] = DialogueTree{
            {
                {"DRONE-7", "WELCOME ABOARD. PLEASE KEEP YOUR SOUL INSIDE THE VEHICLE AT ALL TIMES.", 1, {}, "", ""},
                {"DRONE-7", "THE WALLS ARE BREATHING. THAT IS NORMAL. IF THEY STOP, PLEASE INFORM A WALL.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_engineer"] = DialogueTree{
            {
                {"THE ENGINEER", "I am the engine and the engineer. It saves on staffing.", 1, {}, "", ""},
                {"THE ENGINEER", "The core runs on gratitude. Step on the diamonds to reach it. Thank them afterwards.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["ufo_spark"] = DialogueTree{
            {
                {"SPARK", "zzt! i'm a spark from the core. i got out. don't tell.", 1, {}, "", ""},
                {"SPARK", "the diamonds go round and round. outside, inside, outside. like breathing.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_cow_moon"] = DialogueTree{
            {
                {"MOON COW", "Moo means 'hello' in eleven dimensions and 'goodbye' in the twelfth.", 1, {}, "", ""},
                {"MOON COW", "We weren't abducted. We applied.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_cow_static"] = DialogueTree{
            {
                {"STATIC COW", "Mmmmmmmzzzzzz. I'm tuned between two stations. Both of them are grass.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_cow_mirror"] = DialogueTree{
            {
                {"MIRROR COW", "Look across the floor. Your reflection is walking the other way.", 1, {}, "", ""},
                {"MIRROR COW", "It's going home without you. Let it. It knows a shortcut.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["ufo_drone_a"] = DialogueTree{
            {
                {"HONOR DRONE", "THE MOTHER LISTENS. SPEAK FROM THE MIDDLE OF YOURSELF.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["ufo_drone_b"] = DialogueTree{
            {
                {"HONOR DRONE", "SHE HAS NINE THOUSAND YEARS OF PATIENCE AND FOUR MINUTES OF ATTENTION. HURRY, LOVINGLY.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_librarian"] = DialogueTree{
            {
                {"THE LIBRARIAN", "Every book here is the same book, read from a different chair.", 1, {}, "", ""},
                {"THE LIBRARIAN", "Mind the gaps between the shelves. That's where unread pages fall. You'll fall too, but somewhere nicer.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_book_that_reads"] = DialogueTree{
            {
                {"A BOOK THAT READS", "I've been reading you. You're at a good part.", 1, {}, "", ""},
                {"A BOOK THAT READS", "Don't skip ahead. The ending is better if you arrive at it.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_gardener"] = DialogueTree{
            {
                {"GARDENER OF NINE", "Most colors grow from light. Nine grows from attention.", 1, {}, "", ""},
                {"GARDENER OF NINE", "Pick the seed in the middle. Don't look at it directly. Look at it the way you'd look at a friend.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_seedling"] = DialogueTree{
            {
                {"SEEDLING", "i'm going to be a color when i grow up.", 1, {}, "", ""},
                {"SEEDLING", "maybe a loud one.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_stair_counter"] = DialogueTree{
            {
                {"STEP COUNTER", "Step four thousand and ninety-six. Or six. I keep losing count of up.", 1, {}, "", ""},
                {"STEP COUNTER", "Up is a habit. Try down.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_descending_monk"] = DialogueTree{
            {
                {"DESCENDING MONK", "I've been walking down for years.", 1, {}, "", ""},
                {"DESCENDING MONK", "I'm finally at the top.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_number_taker"] = DialogueTree{
            {
                {"NUMBER TAKER", "Take a number.", 1, {}, "", ""},
                {"NUMBER TAKER", "Your number is the color of a sneeze. You're next. You've always been next.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_patient_light"] = DialogueTree{
            {
                {"PATIENT LIGHT", "I've been waiting here since before light was fast.", 1, {}, "", ""},
                {"PATIENT LIGHT", "Don't hurry the Officer. Hurrying is a form, and you haven't filled it out.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["boss_mother_of_saucers"] = DialogueTree{
            {
                {"MOTHER-OF-SAUCERS", "GO ON, LITTLE SIGNAL. THE BEAM IS NORTH. I WILL WAVE WITH ALL MY LIGHTS.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["boss_choir"] = DialogueTree{
            {
                {"THE CHOIR", "--GO ON-- --NORTH-- --WE'LL HUM YOU THROUGH THE DOOR.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["boss_customs"] = DialogueTree{
            {
                {"CUSTOMS OFFICER", "CLEARED. MOVE ALONG. HAVE A TIMELESS DAY.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_hull_whisperer"] = DialogueTree{
            {
                {"HULL WHISPERER", "Shhh. The ship is dreaming it's a seashell.", -1,
                 {{"What does it hear?", 1}, {"Can I dream too?", 2}}, "", ""},
                {"HULL WHISPERER", "The sea it came from. The sea is also a ship. "
                                   "Everything is, if you hold it to your ear.", -1, {}, "", ""},
                {"HULL WHISPERER", "Sweet thing, you've been dreaming since the bedroom.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        // The ending. Collecting every floor item opens the second one.
        t["npc_mother_awake"] = DialogueTree{
            {
                {"MOM?", "You're up! You were glowing in your sleep again. All the colors.", 1, {}, "", ""},
                {"MOM?", "Did you have the dream with the saucers? Your father used to "
                         "have that one.", -1,
                 {{"I put something down.", 2}, {"I want to go back.", 3}}, "", ""},
                {"MOM?", "Then you're lighter. Good. Breakfast is whatever color you want.",
                 -1, {}, "ending_light", ""},
                {"MOM?", "You can. The door's right there. It always was.", -1, {}, "ending_door", ""},
                {"MOM?", "...Sweetheart. You brought ALL of it back? The milk from the moon, "
                         "the seed, the page, the spark?", 5, {}, "", ""},
                {"MOM?", "Then you're not dreaming anymore. You're visiting. Welcome home, "
                         "little visitor. Here -- you dropped this on the way in.", -1, {},
                 "true_ending", "the_color_nine"},
            },
            0, "collected_everything", 4,
        };

        return t;
    }();
    return trees;
}

}  // namespace

// Greedy word wrap against a pixel width.
std::vector<std::string> wrapText(Assets& assets, const std::string& text, int maxWidth) {
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string word, current;
    while (ss >> word) {
        std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && assets.measureText(candidate) > maxWidth) {
            lines.push_back(current);
            current = word;
        } else {
            current = std::move(candidate);
        }
    }
    if (!current.empty())
        lines.push_back(current);
    return lines;
}


const DialogueTree* dialogueForNpc(const std::string& npcId) {
    const auto& reg = dialogueRegistry();
    auto it = reg.find(npcId);
    return it != reg.end() ? &it->second : nullptr;
}

// --- DialogueBox -------------------------------------------------------------

void DialogueBox::start(const DialogueTree& tree, Effects effects) {
    tree_ = &tree;
    effects_ = std::move(effects);
    int startNode = tree.start;
    if (!tree.altFlag.empty() && tree.altStart >= 0 && effects_.hasFlag
        && effects_.hasFlag(tree.altFlag))
        startNode = tree.altStart;
    enterNode(startNode);
}

void DialogueBox::enterNode(int index) {
    if (!tree_ || index < 0 || index >= static_cast<int>(tree_->nodes.size())) {
        tree_ = nullptr;
        return;
    }
    nodeIndex_ = index;
    shownChars_ = 0;
    lastBlipAt_ = 0;
    choiceIndex_ = 0;
    const auto& n = tree_->nodes[index];
    if (!n.setFlag.empty() && effects_.setFlag)
        effects_.setFlag(n.setFlag);
    if (!n.giveItem.empty() && effects_.giveItem)
        effects_.giveItem(n.giveItem);
}

void DialogueBox::update(double dt, const Input& input) {
    if (!tree_) return;
    const auto& n = node();
    const int total = static_cast<int>(n.text.size());
    const bool typing = shownChars_ < total;

    if (typing) {
        shownChars_ = std::min<double>(total, shownChars_ + dt * kCharsPerSec);
        int typed = static_cast<int>(shownChars_);
        if (typed / 3 > lastBlipAt_ / 3 && effects_.onCharTyped)
            effects_.onCharTyped();
        lastBlipAt_ = typed;
        if (input.interactPressed())
            shownChars_ = total;  // skip
        return;
    }

    if (!n.choices.empty()) {
        if (input.upPressed())
            choiceIndex_ = (choiceIndex_ + static_cast<int>(n.choices.size()) - 1)
                           % static_cast<int>(n.choices.size());
        if (input.downPressed())
            choiceIndex_ = (choiceIndex_ + 1) % static_cast<int>(n.choices.size());
        if (input.interactPressed())
            enterNode(n.choices[choiceIndex_].next);
    } else if (input.interactPressed()) {
        enterNode(n.next);
    }
}

void DialogueBox::render(Renderer& r, Assets& assets) const {
    if (!tree_) return;
    const auto& n = node();

    const int boxH = 140;
    const SDL_Rect box{12, Renderer::kViewH - boxH - 12, r.width() - 24, boxH};
    r.fillRect(box, SDL_Color{16, 10, 28, 235});
    r.outlineRect(box, SDL_Color{214, 62, 106, 255});  // neon pink trim

    int y = box.y + 10;
    if (!n.speaker.empty()) {
        r.drawText(assets, n.speaker, box.x + 14, y, SDL_Color{255, 179, 71, 255});
        y += assets.fontHeight() + 4;
    }

    std::string visible = n.text.substr(0, static_cast<size_t>(shownChars_));
    for (const auto& line : wrapText(assets, visible, box.w - 28)) {
        r.drawText(assets, line, box.x + 14, y, SDL_Color{235, 231, 240, 255});
        y += assets.fontHeight() + 2;
    }

    const bool done = shownChars_ >= static_cast<double>(n.text.size());
    if (done && !n.choices.empty()) {
        for (size_t i = 0; i < n.choices.size(); ++i) {
            bool sel = static_cast<int>(i) == choiceIndex_;
            std::string label = (sel ? "> " : "  ") + n.choices[i].label;
            r.drawText(assets, label, box.x + 28, y,
                       sel ? SDL_Color{255, 179, 71, 255} : SDL_Color{160, 150, 175, 255});
            y += assets.fontHeight() + 2;
        }
    } else if (done) {
        r.drawText(assets, "[E]", box.x + box.w - 44, box.y + box.h - assets.fontHeight() - 8,
                   SDL_Color{214, 62, 106, 255});
    }
}
