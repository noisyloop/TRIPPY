#include "UFO.h"

#include <cmath>
#include <cstdint>

namespace {
constexpr double kTile = 32.0;
constexpr double kRangeX = 1.6;  // tiles
constexpr double kRangeY = 1.1;
}

UFO::UFO(std::string id, int tileX, int tileY)
    : NPC(id, tileX, tileY), homeX_(tileX), homeY_(tileY) {
    // Phase from the id so a meadow of saucers never drifts in lockstep.
    uint32_t h = 2166136261u;
    for (char c : id) { h ^= static_cast<uint8_t>(c); h *= 16777619u; }
    phase_ = (h % 1000) / 1000.0 * 6.2831853;
    px_ = homeX_;
    py_ = homeY_;
}

void UFO::update(double dt) {
    NPC::update(dt);
    const double t = idleTime();
    px_ = homeX_ + std::sin(t * 0.47 + phase_) * kRangeX;
    py_ = homeY_ + std::sin(t * 0.31 + phase_ * 2.0) * kRangeY;
    setTile(static_cast<int>(std::lround(px_)), static_cast<int>(std::lround(py_)));
}

void UFO::drawOffset(double& dx, double& dy) const {
    dx = (px_ - tileX()) * kTile;
    dy = (py_ - tileY()) * kTile;
}
