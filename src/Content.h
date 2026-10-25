#pragma once

#include <string>

#include "Dialogue.h"
#include "Duel.h"

// Text formats for NPC dialogue (assets/dialogue/<npc_id>.txt) and boss
// duels (assets/duels/<boss_id>.txt). Documented in docs/content-format.md.
// Both return false and fill `err` ("line N: ...") on malformed input.

bool parseDialogue(const std::string& text, DialogueTree& out, std::string& err);
bool parseDuel(const std::string& text, DuelScript& out, std::string& err);
