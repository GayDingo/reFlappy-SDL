#pragma once
#include <functional>

#include "AnimatedSprite.h"

// Port of com.dotgears.flappy.Bird (the decompiled class "a").
class Bird : public AnimatedSprite {
public:
    float angle = 0.0f;
    float angleVelocity = 0.0f;
    float angleAcceleration = 0.0f;
    float verticalVelocity = 0.0f;
    float gravity = 0.0f;
    bool isDead = false;
    bool isReady = true;
    int flapAngle = 0;
    float flapOffset = 0.0f;
    int birdColor = 0;

    // Called on every accepted flap. The scene uses it to schedule the wing sound.
    std::function<void()> onFlap;

    explicit Bird(const Atlas& atlas);

    void init();
    void update() override;
    void draw(const Gfx& gfx) const override;
    void flap();  // original Bird.b()
};
