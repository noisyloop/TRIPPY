#include "Content.h"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace {

std::string trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool startsWith(const std::string& s, const char* prefix) {
    return s.rfind(prefix, 0) == 0;
}

// "key: value" -> true with key/value split at the first colon.
bool keyValue(const std::string& line, std::string& key, std::string& value) {
    auto colon = line.find(':');
    if (colon == std::string::npos) return false;
    key = trim(line.substr(0, colon));
    value = trim(line.substr(colon + 1));
    return true;
}

std::string at(int lineNo, const std::string& msg) {
    return "line " + std::to_string(lineNo) + ": " + msg;
}

}  // namespace

// --- Dialogue ---------------------------------------------------------------
//
//   alt: <flag> -> <label>      (header) start here instead once <flag> is set
//   [label]                     a node; the first node is the start
//   SPEAKER: text               first line of a node (": text" = no speaker)
//   more text                   continuation lines are joined with a space
//   set: <flag>   give: <item>  side effects when the node is shown
//   -> <label>                  next node ("end" or nothing = conversation ends)
//   * Choice text -> <label>    a choice (shown once the text finishes)

bool parseDialogue(const std::string& text, DialogueTree& out, std::string& err) {
    out = DialogueTree{};
    std::unordered_map<std::string, int> labels;
    struct Ref { int node; int choice; std::string label; int lineNo; };  // choice -1 = next
    std::vector<Ref> refs;
    std::string altLabel;
    int altLine = 0;
    bool haveSpeakerLine = false;

    std::istringstream in(text);
    std::string raw;
    int lineNo = 0;
    while (std::getline(in, raw)) {
        ++lineNo;
        const std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            const std::string label = trim(line.substr(1, line.size() - 2));
            if (label.empty() || labels.contains(label)) {
                err = at(lineNo, "empty or duplicate label [" + label + "]");
                return false;
            }
            labels[label] = static_cast<int>(out.nodes.size());
            out.nodes.emplace_back();
            haveSpeakerLine = false;
            continue;
        }

        if (out.nodes.empty()) {  // header
            std::string key, value;
            if (keyValue(line, key, value) && key == "alt") {
                auto arrow = value.find("->");
                if (arrow == std::string::npos) { err = at(lineNo, "alt needs 'flag -> label'"); return false; }
                out.altFlag = trim(value.substr(0, arrow));
                altLabel = trim(value.substr(arrow + 2));
                altLine = lineNo;
                continue;
            }
            err = at(lineNo, "expected 'alt: flag -> label' or a [label] before text");
            return false;
        }

        DialogueNode& node = out.nodes.back();
        const int nodeIdx = static_cast<int>(out.nodes.size()) - 1;
        if (startsWith(line, "->")) {
            refs.push_back({nodeIdx, -1, trim(line.substr(2)), lineNo});
        } else if (line[0] == '*') {
            auto arrow = line.rfind("->");
            if (arrow == std::string::npos) { err = at(lineNo, "choice needs '-> label'"); return false; }
            node.choices.push_back({trim(line.substr(1, arrow - 1)), -1});
            refs.push_back({nodeIdx, static_cast<int>(node.choices.size()) - 1,
                            trim(line.substr(arrow + 2)), lineNo});
        } else if (startsWith(line, "set:")) {
            node.setFlag = trim(line.substr(4));
        } else if (startsWith(line, "give:")) {
            node.giveItem = trim(line.substr(5));
        } else if (!haveSpeakerLine) {
            std::string speaker, rest;
            if (keyValue(line, speaker, rest)) {
                node.speaker = speaker;
                node.text = rest;
            } else {
                node.text = line;
            }
            haveSpeakerLine = true;
        } else {
            node.text += (node.text.empty() ? "" : " ") + line;
        }
    }

    if (out.nodes.empty()) { err = "no nodes"; return false; }
    auto resolve = [&](const std::string& label, int lineNo, int& target) {
        if (label == "end") { target = -1; return true; }
        auto it = labels.find(label);
        if (it == labels.end()) { err = at(lineNo, "unknown label '" + label + "'"); return false; }
        target = it->second;
        return true;
    };
    for (const auto& r : refs) {
        int& target = r.choice < 0 ? out.nodes[r.node].next : out.nodes[r.node].choices[r.choice].next;
        if (!resolve(r.label, r.lineNo, target)) return false;
    }
    if (!altLabel.empty() && !resolve(altLabel, altLine, out.altStart)) return false;
    return true;
}

// --- Duels ------------------------------------------------------------------
//
//   name: / mood: / intro: / victory: / reward: / after_room:   (header)
//   [talk]  line: ...   then  * Reply | +30 | What the boss says back
//   [echo]  line:  length:  gain:  success:  failure:
//   [beat]  line:  length:  gain:  success:  failure:

bool parseDuel(const std::string& text, DuelScript& out, std::string& err) {
    out = DuelScript{};
    std::istringstream in(text);
    std::string raw;
    int lineNo = 0;
    DuelRound* round = nullptr;
    while (std::getline(in, raw)) {
        ++lineNo;
        const std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            const std::string kind = trim(line.substr(1, line.size() - 2));
            DuelRound r;
            if (kind == "talk") r.kind = DuelRound::Kind::Talk;
            else if (kind == "echo") r.kind = DuelRound::Kind::Echo;
            else if (kind == "beat") r.kind = DuelRound::Kind::Beat;
            else { err = at(lineNo, "round must be [talk], [echo] or [beat]"); return false; }
            if (r.kind != DuelRound::Kind::Talk) { r.length = 4; r.delta = 25; }
            else { r.length = 0; r.delta = 0; }
            out.rounds.push_back(r);
            round = &out.rounds.back();
            continue;
        }

        if (line[0] == '*') {
            if (!round || round->kind != DuelRound::Kind::Talk) {
                err = at(lineNo, "replies (*) belong in a [talk] round");
                return false;
            }
            std::vector<std::string> parts;
            std::string rest = line.substr(1);
            for (size_t bar; (bar = rest.find('|')) != std::string::npos; rest = rest.substr(bar + 1))
                parts.push_back(trim(rest.substr(0, bar)));
            parts.push_back(trim(rest));
            if (parts.size() != 3) { err = at(lineNo, "reply needs 'text | delta | response'"); return false; }
            try {
                round->replies.push_back({parts[0], std::stoi(parts[1]), parts[2]});
            } catch (const std::exception&) {
                err = at(lineNo, "bad delta '" + parts[1] + "'");
                return false;
            }
            continue;
        }

        std::string key, value;
        if (!keyValue(line, key, value)) { err = at(lineNo, "expected 'key: value'"); return false; }
        if (!round) {
            if (key == "name") out.name = value;
            else if (key == "mood") out.mood = value;
            else if (key == "intro") out.intro = value;
            else if (key == "victory") out.victory = value;
            else if (key == "reward") out.rewardItem = value;
            else if (key == "after_room") out.afterRoom = value;
            else { err = at(lineNo, "unknown key '" + key + "'"); return false; }
            continue;
        }
        try {
            if (key == "line") round->line = value;
            else if (key == "length") round->length = std::stoi(value);
            else if (key == "gain") round->delta = std::stoi(value);
            else if (key == "success") round->success = value;
            else if (key == "failure") round->failure = value;
            else { err = at(lineNo, "unknown round key '" + key + "'"); return false; }
        } catch (const std::exception&) {
            err = at(lineNo, "bad number '" + value + "'");
            return false;
        }
    }
    if (out.rounds.empty()) { err = "a duel needs at least one round"; return false; }
    for (const auto& r : out.rounds)
        if (r.kind == DuelRound::Kind::Talk && r.replies.empty()) {
            err = "a [talk] round has no replies";
            return false;
        }
    return true;
}
