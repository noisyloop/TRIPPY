#pragma once

#include <cstdint>

#include "Entity.h"

class Room;

// An NPC's dialogue tree is looked up by its id in the dialogue registry
// (see Dialogue.h / dialogueForNpc). Sprite name matches the id so art can
// be dropped in as assets/sprites/<id>.png.
//
// NPCs listed in a room's `wander = ...` meta key amble one tile at a time
// near where they started, sliding smoothly between tiles.
class NPC : public Entity {
public:
    NPC(std::string id, int tileX, int tileY);

    void update(double dt) override;
    double idleTime() const { return idleTime_; }  // drives the idle bob

    void setWander(bool w) { wander_ = w; }
    bool wanders() const { return wander_; }
    // Called by Room::update. Never steps onto (or next to) the player's box.
    void wanderStep(double dt, const Room& room, double playerX, double playerY);
    void drawOffset(double& dx, double& dy) const override;

private:
    double idleTime_ = 0;
    bool wander_ = false;
    int homeX_, homeY_, fromX_, fromY_;
    double wanderTimer_ = 1.5;
    double slide_ = 1.0;  // 0 -> 1 while sliding from (fromX_, fromY_)
    uint32_t rng_;
};
