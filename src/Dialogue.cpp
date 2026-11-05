#include "Dialogue.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>

#include "Assets.h"
#include "Input.h"
#include "Renderer.h"
#include "TripFX.h"

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
                {"THE COUNTER", "~Again! Again!~ The hallway loves you in a circle.", -1, {}, "", ""},
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
                {"HUMM", "~mmmmmmmmmmmmmmmmmmmm.~", 1, {}, "", ""},
                {"", "(it is humming the exact shape of your name)", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["ufo_wobble"] = DialogueTree{
            {
                {"WOBBLE", "We came for the cows but stayed for the colors. "
                           "Have you seen the *color nine*?", -1,
                 {{"Yes.", 1}, {"What's nine?", 2}}, "", ""},
                {"WOBBLE", "Then you know why our lights never stop smiling.", -1, {}, "", ""},
                {"WOBBLE", "Close all your eyes. Not those ones. The other ones. "
                           "The sky folds west from here.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_tessellator"] = DialogueTree{
            {
                {"THE TESSELLATOR", "OH! A visitor! *A visitor made of visitor!* "
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
                {"YOU (AGAIN)", "Spoiler: home was also a ~spaceship~. "
                                "It was the whole time.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["ufo_greeter"] = DialogueTree{
            {
                {"DRONE-7", "WELCOME ABOARD. PLEASE KEEP YOUR SOUL INSIDE THE VEHICLE AT ALL TIMES.", 1, {}, "", ""},
                {"DRONE-7", "THE WALLS ARE ~BREATHING~. THAT IS NORMAL. IF THEY STOP, PLEASE INFORM A WALL.", -1, {}, "", ""},
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
                {"SPARK", "^zzt!^ i'm a spark from the core. i got out. don't tell.", 1, {}, "", ""},
                {"SPARK", "the diamonds go round and round. outside, inside, outside. like breathing.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_cow_moon"] = DialogueTree{
            {
                {"MOON COW", "Moo means 'hello' in eleven dimensions and 'goodbye' in the *twelfth*.", 1, {}, "", ""},
                {"MOON COW", "We weren't abducted. We applied.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_cow_static"] = DialogueTree{
            {
                {"STATIC COW", "^Mmmmmmmzzzzzz.^ I'm tuned between two stations. Both of them are grass.", -1, {}, "", ""},
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
                {"THE LIBRARIAN", "Every book here is the *same book*, read from a different chair.", 1, {}, "", ""},
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
                {"DESCENDING MONK", "I'm ~finally~ at the top.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_number_taker"] = DialogueTree{
            {
                {"NUMBER TAKER", "Take a number.", 1, {}, "", ""},
                {"NUMBER TAKER", "Your number is *the color of a sneeze*. You're next. You've always been next.", -1, {}, "", ""},
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
                {"HULL WHISPERER", "Shhh. The ship is ~dreaming it's a seashell~.", -1,
                 {{"What does it hear?", 1}, {"Can I dream too?", 2}}, "", ""},
                {"HULL WHISPERER", "The sea it came from. The sea is also a ship. "
                                   "Everything is, if you hold it to your ear.", -1, {}, "", ""},
                {"HULL WHISPERER", "Sweet thing, you've been dreaming since the bedroom.", -1, {}, "", ""},
            },
            0, "", -1,
        };


        // --- The side rooms: deeper in, further out ---------------------------

        t["npc_tv_head"] = DialogueTree{
            {
                {"CHANNEL ZERO", "^bzzhhhk^ -- oh! A viewer! Nobody has watched me since the moon was a rerun.", 1, {}, "met_channel_zero", ""},
                {"CHANNEL ZERO", "Tonight's program: you, standing there, being watched by a television. "
                                 "Riveting. *Five stars.*", -1,
                 {{"Change the channel.", 2}, {"What's on next?", 3}}, "", ""},
                {"CHANNEL ZERO", "^click.^ Now I'm a weather report for a country made of soup. "
                                 "~Light croutons, scattered.~ Back to you.", -1, {}, "", ""},
                {"CHANNEL ZERO", "Next: the pool. It shows every channel at once. "
                                 "That's why the light comes out of it *sideways*.", -1, {}, "", ""},
                {"CHANNEL ZERO", "Still watching? Good. I can see myself in your eyes and I look ~fabulous~.", -1, {}, "", ""},
            },
            0, "met_channel_zero", 4,
        };
        t["npc_ice_machine"] = DialogueTree{
            {
                {"ICE MACHINE", "I make ice. The ice makes cubes. The cubes make *corners*. "
                                "Nobody asked for corners, and yet.", 1, {}, "", ""},
                {"ICE MACHINE", "There's a shard by the pool. Ice that ^remembered it was light^. "
                                "Carry it and the whole world splits into its three favorite colors.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_channel_surfer"] = DialogueTree{
            {
                {"CHANNEL SURFER", "I surf between stations, man. The waves are ~sine waves~.", 1, {}, "", ""},
                {"CHANNEL SURFER", "Hang ten. Hang eleven. Hang a *frequency*. "
                                   "Wipe out into the static and it's all warm noise down there.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_eye_tree"] = DialogueTree{
            {
                {"THE EYE TREE", "Don't mind us. We're ~ripening~.", 1, {}, "", ""},
                {"THE EYE TREE", "Every fruit here is an eye. Every eye is a fruit. You can eat what you see, "
                                 "but afterwards it sees you from the inside.", -1,
                 {{"That's horrible.", 2}, {"Can I have one?", 3}}, "", ""},
                {"THE EYE TREE", "It's *lovely*. You would be amazed how much a fig notices.", -1, {}, "", ""},
                {"THE EYE TREE", "Take the unblinking one by the roots. It has been staring at you since "
                                 "before you arrived. ^It is very fond of you.^", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_eye_tree_sapling"] = DialogueTree{
            {
                {"SAPLING", "i only have one eye so far. i'm saving it for something ~really good~.", 1, {}, "", ""},
                {"SAPLING", "...", 2, {}, "", ""},
                {"SAPLING", "*okay it's you. you're the something.*", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_blink_twins"] = DialogueTree{
            {
                {"THE BLINK TWINS", "We take turns blinking so the world never goes dark.", 1, {}, "", ""},
                {"THE BLINK TWINS", "~Blink.~ That was me. ~Blink.~ That was also me. "
                                    "We're the same twin, twice. *It's a scheduling thing.*", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_spore_bishop"] = DialogueTree{
            {
                {"SPORE BISHOP", "Welcome, little mushroom-to-be. Kneel if you have knees. "
                                 "Kneel ~sideways~ if you don't.", 1, {}, "met_bishop", ""},
                {"SPORE BISHOP", "We are one creature, holding hands underground across the whole planet. "
                                 "Your forest is gossiping about you right now.", -1,
                 {{"What are they saying?", 2}, {"I'm not a mushroom.", 3}}, "", ""},
                {"SPORE BISHOP", "That you are made of the same stardust as a puddle, "
                                 "and that the puddle is *so proud of you*.", -1, {}, "", ""},
                {"SPORE BISHOP", "Not ^yet^. Give it a few thousand rainy afternoons.", -1, {}, "", ""},
                {"SPORE BISHOP", "Take the spore in the moss. It only knows one word, and the word is *yes*.", -1, {}, "", ""},
            },
            0, "met_bishop", 4,
        };
        t["npc_mycelium_choirboy"] = DialogueTree{
            {
                {"MYCELIUM CHOIRBOY", "la la ~laaaaa~. i sing through my feet. "
                                      "the hymn comes out of the soil as a smell.", 1, {}, "", ""},
                {"MYCELIUM CHOIRBOY", "the smell of the hymn is called *petrichor*. "
                                      "you've been hearing it your whole life.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_gear_angel"] = DialogueTree{
            {
                {"GEAR ANGEL", "BE NOT AFRAID. I am only a little bit of clockwork and a great deal of *wings*.", 1, {}, "", ""},
                {"GEAR ANGEL", "Every gear turns the next gear the other way. "
                               "That's how the universe stays polite.", -1,
                 {{"Who turns you?", 2}, {"What's the lens for?", 3}}, "", ""},
                {"GEAR ANGEL", "^You do.^ Every time you change your mind, I spin a little.", -1, {}, "", ""},
                {"GEAR ANGEL", "It makes every room into a flower of itself. Take it. "
                               "Look through it ~gently~. Rooms are shy.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["ufo_loop"] = DialogueTree{
            {
                {"LOOP", "loop loop loop. i fly in a circle so i always arrive.", 1, {}, "", ""},
                {"LOOP", "if you ever feel lost, just keep going. eventually you're ~back~.", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_jelly_oracle"] = DialogueTree{
            {
                {"THE JELLY ORACLE", "~Blub.~ I see your future. It is very wet and very blue.", 1, {}, "", ""},
                {"THE JELLY ORACLE", "This ocean is the milk the moon forgot. Don't fall in. Or do. "
                                     "It only drops you back where you began, but softer.", -1,
                 {{"What's my future, really?", 2}, {"Why are you see-through?", 3}}, "", ""},
                {"THE JELLY ORACLE", "*You will eat breakfast.* It will taste like a color. "
                                     "That's all any oracle really knows.", -1, {}, "", ""},
                {"THE JELLY ORACLE", "So my thoughts can get out. Yours are stuck inside you, poor thing. "
                                     "^They keep knocking.^", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_drowned_saxophone"] = DialogueTree{
            {
                {"DROWNED SAXOPHONE", "~bwaaaaaaa~. I've been playing the same note under milk for a thousand years.", 1, {}, "", ""},
                {"DROWNED SAXOPHONE", "It's an E flat. Flat because the ocean is flat. "
                                      "*Jazz is just geography, baby.*", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_melting_clock"] = DialogueTree{
            {
                {"THE MELTING CLOCK", "Oh, is it ~already~ now? I was sure it would be later for longer.", 1, {}, "", ""},
                {"THE MELTING CLOCK", "Time is warm in here. It drips. Every minute is a little puddle, "
                                      "and we dance in them.", -1,
                 {{"What time is it?", 2}, {"Can time stop?", 3}}, "", ""},
                {"THE MELTING CLOCK", "It's *half past forever*. Quarter to you.", -1, {}, "", ""},
                {"THE MELTING CLOCK", "Only when you're happy. Then it holds its breath to watch. "
                                      "The liquid clock on the floor ^ticks in honey^.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_wallpaper_dancer"] = DialogueTree{
            {
                {"WALLPAPER DANCER", "I peeled off the wall to dance with you. Don't tell the wall. "
                                     "^It gets jealous in patterns.^", 1, {}, "", ""},
                {"WALLPAPER DANCER", "Spin! The room spins the other way, so you stay perfectly still. "
                                     "~That's the whole trick of dancing.~", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_disco_eye"] = DialogueTree{
            {
                {"DISCO EYE", "*I SEE EVERYTHING AT ONCE AND ALL OF IT IS DANCING.*", 1, {}, "", ""},
                {"DISCO EYE", "Even the walls. Especially the walls. ~Look at them go.~", -1, {}, "", ""},
            },
            0, "", -1,
        };

        t["npc_marble_self"] = DialogueTree{
            {
                {"TINY YOU", "Hi. I'm you, but small enough to fit inside the marble you're inside of.", 1, {}, "", ""},
                {"TINY YOU", "Look closely: there's a marble in me. And a you in that. "
                             "It's *you all the way down*.", -1,
                 {{"Who's at the bottom?", 2}, {"Want to come with me?", 3}}, "", ""},
                {"TINY YOU", "Nobody knows. Someone very small and ~very brave~.", -1, {}, "", ""},
                {"TINY YOU", "I'm already coming. I'm in your pocket. Check. ^Check now.^", -1, {},
                 "met_tiny_you", "tiny_you"},
                {"TINY YOU", "(a small voice, from your pocket) ~we're doing great.~", -1, {}, "", ""},
            },
            0, "met_tiny_you", 4,
        };

        t["npc_sock_prophet"] = DialogueTree{
            {
                {"THE SOCK PROPHET", "Every lost sock comes here. We have formed a ~religion~.", 1, {}, "", ""},
                {"THE SOCK PROPHET", "We believe in the Great Pair. Somewhere our other halves are waiting "
                                     "in a dryer the size of the sky.", -1,
                 {{"That's beautiful.", 2}, {"That's ridiculous.", 3}}, "", ""},
                {"THE SOCK PROPHET", "Thank you. Hold onto your socks tonight. "
                                     "*The Rapture is a spin cycle.*", -1, {}, "", ""},
                {"THE SOCK PROPHET", "So is a foot. And yet here you are, with ^two of them^.", -1, {}, "", ""},
            },
            0, "", -1,
        };
        t["npc_forgotten_word"] = DialogueTree{
            {
                {"A FORGOTTEN WORD", "I'm on the tip of your tongue. Don't look! You'll ^forget me again^.", 1, {}, "", ""},
                {"A FORGOTTEN WORD", "I used to mean something between 'goodbye' and 'thank you'. "
                                     "After a while nobody needed me.", 2, {}, "", ""},
                {"A FORGOTTEN WORD", "Take me with you. Say me when nobody's listening. ~I'll know.~", -1, {},
                 "remembered_word", "forgotten_word"},
                {"A FORGOTTEN WORD", "(you can feel it there, warm, ~just behind your teeth~)", -1, {}, "", ""},
            },
            0, "remembered_word", 3,
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

// --- Markup ------------------------------------------------------------------

namespace {

enum GlyphStyle : unsigned { kWave = 1, kRainbow = 2, kShake = 4 };

struct Glyph {
    char c;
    unsigned style;
};

std::vector<Glyph> parseGlyphs(const std::string& text) {
    std::vector<Glyph> out;
    unsigned style = 0;
    for (char c : text) {
        if (c == '~') { style ^= kWave; continue; }
        if (c == '*') { style ^= kRainbow; continue; }
        if (c == '^') { style ^= kShake; continue; }
        out.push_back({c, style});
    }
    return out;
}

}  // namespace

std::string stripMarkup(const std::string& text) {
    std::string out;
    for (const auto& g : parseGlyphs(text)) out.push_back(g.c);
    return out;
}

// --- DialogueBox -------------------------------------------------------------

void DialogueBox::start(const DialogueTree& tree, Effects effects, std::string portrait) {
    tree_ = &tree;
    effects_ = std::move(effects);
    portrait_ = std::move(portrait);
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
    age_ = 0;
    const auto& n = tree_->nodes[index];
    if (!n.setFlag.empty() && effects_.setFlag)
        effects_.setFlag(n.setFlag);
    if (!n.giveItem.empty() && effects_.giveItem)
        effects_.giveItem(n.giveItem);
}

void DialogueBox::update(double dt, const Input& input) {
    if (!tree_) return;
    age_ += dt;
    const auto& n = node();
    const int total = static_cast<int>(stripMarkup(n.text).size());
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

namespace {

// Draws one glyph, white texture tinted per frame (keeps the text cache tiny).
void drawGlyph(Renderer& r, Assets& assets, char c, float x, float y, SDL_Color col, float scale = 1.0f) {
    if (c == ' ') return;
    SDL_Texture* tex = assets.textTexture(std::string(1, c), SDL_Color{255, 255, 255, 255});
    if (!tex) return;
    int w = 0, h = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_SetTextureColorMod(tex, col.r, col.g, col.b);
    SDL_SetTextureAlphaMod(tex, col.a);
    SDL_FRect d{x, y, w * scale, h * scale};
    SDL_RenderCopyF(r.sdl(), tex, nullptr, &d);
    SDL_SetTextureColorMod(tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex, 255);
}

}  // namespace

void DialogueBox::render(Renderer& r, Assets& assets, double time, double k, double pulse) const {
    if (!tree_) return;
    const auto& n = node();
    k = std::clamp(k, 0.0, 1.0);

    const int boxH = 150;
    const SDL_Rect box{12, Renderer::kViewH - boxH - 12, r.width() - 24, boxH};

    // Background: deep violet with slow drifting color bands behind the text.
    r.fillRect(box, SDL_Color{16, 10, 28, 238});
    if (k > 0.01) {
        for (int i = 0; i < box.w; i += 6) {
            const double band = std::sin(i * 0.02 + time * 0.8) * std::sin(i * 0.007 - time * 0.3);
            SDL_Color c = hsvColor(i * 0.4 + time * 30, 0.8, 0.6, static_cast<Uint8>(std::max(0.0, band) * 40 * k));
            r.fillRect(SDL_Rect{box.x + i, box.y, 6, box.h}, c);
        }
    }
    // Border: a rainbow that crawls around the box (two rects, offset).
    for (int ring = 0; ring < 3; ++ring) {
        SDL_Color c = k > 0.01 ? hsvColor(time * 120 + ring * 40, 0.8, 1.0, static_cast<Uint8>(255 - ring * 70))
                               : SDL_Color{214, 62, 106, 255};
        r.outlineRect(SDL_Rect{box.x - ring, box.y - ring, box.w + ring * 2, box.h + ring * 2}, c);
    }

    // Portrait: the speaker, alive, in a little window of its own.
    int textX = box.x + 14;
    if (!portrait_.empty()) {
        const SDL_Rect frame{box.x + 10, box.y + 12, 120, boxH - 24};
        r.fillRect(frame, SDL_Color{6, 3, 14, 255});
        if (k > 0.01)
            drawRing(r.sdl(), frame.x + frame.w / 2.0f, frame.y + frame.h / 2.0f, 0, 70,
                     hsvColor(time * 50, 0.7, 0.5, 200), hsvColor(time * 50 + 180, 0.7, 0.3, 0));
        SDL_RenderSetClipRect(r.sdl(), &frame);
        unsigned seed = 0;
        for (char c : portrait_) seed = seed * 31 + static_cast<unsigned char>(c);
        drawLivingSprite(r.sdl(), assets.sprite(portrait_), frame.x + frame.w / 2.0f,
                         frame.y + frame.h / 2.0f, 92.0f, time, k, pulse, seed);
        SDL_RenderSetClipRect(r.sdl(), nullptr);
        r.outlineRect(frame, hsvColor(time * 80 + 180, 0.7, 1.0));
        textX = frame.x + frame.w + 16;
    }
    const int textW = box.x + box.w - 14 - textX;

    // Speaker: each letter its own color, riding a slow wave.
    int y = box.y + 10;
    if (!n.speaker.empty()) {
        float x = static_cast<float>(textX);
        for (size_t i = 0; i < n.speaker.size(); ++i) {
            const char c = n.speaker[i];
            SDL_Color col = k > 0.01 ? hsvColor(time * 140 + i * 28, 0.75, 1.0) : SDL_Color{255, 179, 71, 255};
            const float wy = static_cast<float>(std::sin(time * 4 + i * 0.6) * 3 * k);
            drawGlyph(r, assets, c, x, y + wy, col);
            x += assets.measureText(std::string(1, c));
        }
        y += assets.fontHeight() + 6;
    }

    // Body: wrap the visible glyphs, then draw them one at a time so each
    // can carry its effect. Every letter shimmers a little at high trip.
    const auto glyphs = parseGlyphs(n.text);
    const int adv = std::max(1, assets.measureText("M"));
    std::vector<std::vector<Glyph>> lines(1);
    {
        size_t i = 0;
        int lineW = 0;
        while (i < glyphs.size()) {
            size_t j = i;
            while (j < glyphs.size() && glyphs[j].c != ' ') ++j;  // word [i, j)
            const int wordW = static_cast<int>(j - i) * adv;
            if (lineW > 0 && lineW + adv + wordW > textW) {
                lines.emplace_back();
                lineW = 0;
            } else if (lineW > 0) {
                lines.back().push_back({' ', 0});
                lineW += adv;
            }
            for (size_t q = i; q < j; ++q) lines.back().push_back(glyphs[q]);
            lineW += wordW;
            i = j;
            while (i < glyphs.size() && glyphs[i].c == ' ') ++i;
        }
    }
    int remaining = static_cast<int>(shownChars_);
    int idx = 0;
    for (const auto& line : lines) {
        float x = static_cast<float>(textX);
        for (const auto& g : line) {
            if (remaining <= 0) break;
            --remaining;
            ++idx;
            SDL_Color col{235, 231, 240, 255};
            float dx = 0, dy = 0;
            if (k > 0.01) {
                dy += static_cast<float>(std::sin(time * 2.2 + idx * 0.35) * 0.8 * k);
                col = lerpColor(col, hsvColor(time * 40 + idx * 6, 0.5, 1.0), 0.25 * k);
            }
            if (g.style & kWave)
                dy += static_cast<float>(std::sin(time * 6 + idx * 0.7) * (2 + 3 * k));
            if (g.style & kRainbow)
                col = hsvColor(time * 200 + idx * 24, 0.85, 1.0);
            if (g.style & kShake) {
                unsigned hsh = static_cast<unsigned>(idx * 2654435761u) ^ static_cast<unsigned>(time * 30);
                hsh *= 2246822519u;
                dx += static_cast<float>(((hsh >> 8) % 5) - 2.0) * (0.5f + static_cast<float>(k));
                dy += static_cast<float>(((hsh >> 16) % 5) - 2.0) * (0.5f + static_cast<float>(k));
                col = lerpColor(col, SDL_Color{255, 90, 200, 255}, 0.5);
            }
            drawGlyph(r, assets, g.c, x + dx, y + dy, col);
            x += adv;
        }
        y += assets.fontHeight() + 2;
    }

    const bool done = shownChars_ >= static_cast<double>(glyphs.size());
    if (done && !n.choices.empty()) {
        for (size_t i = 0; i < n.choices.size(); ++i) {
            bool sel = static_cast<int>(i) == choiceIndex_;
            std::string label = (sel ? "> " : "  ") + stripMarkup(n.choices[i].label);
            SDL_Color col = sel ? (k > 0.01 ? hsvColor(time * 160, 0.6, 1.0) : SDL_Color{255, 179, 71, 255})
                                : SDL_Color{160, 150, 175, 255};
            const int sway = sel ? static_cast<int>(std::sin(time * 5) * 3 * k) : 0;
            r.drawText(assets, label, textX + 14 + sway, y, col);
            y += assets.fontHeight() + 2;
        }
    } else if (done) {
        const int bob = static_cast<int>(std::sin(time * 5) * 2);
        r.drawText(assets, "[E]", box.x + box.w - 44, box.y + box.h - assets.fontHeight() - 8 + bob,
                   k > 0.01 ? hsvColor(time * 90, 0.7, 1.0) : SDL_Color{214, 62, 106, 255});
    }
}
