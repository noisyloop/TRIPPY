#include "NPC.h"

#include <algorithm>
#include <cstdlib>

#include "Room.h"

NPC::NPC(std::string id, int tileX, int tileY)
    : Entity(id, EntityKind::Npc, tileX, tileY, id),
      homeX_(tileX), homeY_(tileY), fromX_(tileX), fromY_(tileY) {
    uint32_t h = 2166136261u;
    for (char c : id) { h ^= static_cast<uint8_t>(c); h *= 16777619u; }
    rng_ = h | 1u;
}

void NPC::update(double dt) {
    idleTime_ += dt;
}

void NPC::wanderStep(double dt, const Room& room, double playerX, double playerY) {
    slide_ = std::min(1.0, slide_ + dt * 3.0);
    wanderTimer_ -= dt;
    if (wanderTimer_ > 0 || slide_ < 1.0) return;
    rng_ = rng_ * 1664525u + 1013904223u;
    wanderTimer_ = 1.2 + (rng_ >> 24) / 255.0 * 2.0;

    static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    const auto& d = dirs[(rng_ >> 12) % 4];
    const int nx = tileX() + d[0], ny = tileY() + d[1];
    if (std::abs(nx - homeX_) > 3 || std::abs(ny - homeY_) > 3) return;
    const Tile t = room.tileAt(nx, ny);
    if (t != Tile::Floor && t != Tile::Grass && t != Tile::Mirror && t != Tile::Stairs) return;
    if (room.entityAt(nx, ny)) return;
    // Keep clear of every tile the player's 32px box touches.
    const int k = Room::kTile;
    if (nx >= static_cast<int>(playerX) / k && nx <= static_cast<int>(playerX + k - 1) / k
        && ny >= static_cast<int>(playerY) / k && ny <= static_cast<int>(playerY + k - 1) / k)
        return;
    fromX_ = tileX();
    fromY_ = tileY();
    setTile(nx, ny);
    slide_ = 0;
}

void NPC::drawOffset(double& dx, double& dy) const {
    dx = (fromX_ - tileX()) * Room::kTile * (1.0 - slide_);
    dy = (fromY_ - tileY()) * Room::kTile * (1.0 - slide_);
}
