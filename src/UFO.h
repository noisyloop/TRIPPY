#pragma once

#include "NPC.h"

// A drifting saucer NPC. Any npc id starting with "ufo_" in a .meta file
// becomes one. It floats on a slow Lissajous loop around its home tile,
// is non-solid (it hovers, you walk under it) and talks like any NPC.
class UFO : public NPC {
public:
    UFO(std::string id, int tileX, int tileY);

    bool solid() const override { return false; }
    void update(double dt) override;
    void drawOffset(double& dx, double& dy) const override;

private:
    int homeX_, homeY_;
    double phase_;
    double px_ = 0, py_ = 0;  // current position in tiles
};
