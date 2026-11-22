#include "Bird.h"

#include "MathHelper.h"

Bird::Bird(const Atlas& atlas) {
    loadFrames(atlas, "bird", 20, 20, 14, 14);
    static const int seq[12] = {0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1};
    defineAnimation(0, seq, 12, 30, false);  // "flap"
    defineAnimation(1, seq, 12, 10, true);   // "auto"
    isReady = true;
    birdColor = MathHelper::nextRandom() % 3;
}

void Bird::init() {
    initialize(80, 246);
    angle = 0.0f;
    verticalVelocity = 0.0f;
    gravity = 1.0f;
    angleAcceleration = 0.4f;
    isDead = false;
    isReady = true;
    flapAngle = 0;
    playAnimation(1, true);
    birdColor = MathHelper::nextRandom() % 3;
}

void Bird::update() {
    AnimatedSprite::update();

    if (isReady) {
        flapAngle += 8;
        if (flapAngle == 360) flapAngle = 0;
        flapOffset = MathHelper::getSin(static_cast<float>(flapAngle)) * 4.0f;
        return;
    }

    flapOffset = 0.0f;
    verticalVelocity += gravity;
    if (verticalVelocity > 8.0f) verticalVelocity = 8.0f;
    y = static_cast<int>(y + verticalVelocity);
    if (y > 400 - height) {
        y = 400 - height;
        gravity = 0.0f;
        verticalVelocity = 0.0f;
    }

    angle += angleVelocity;
    angleVelocity += angleAcceleration;
    if (angle < -20.0f) angle = -20.0f;
    if (angle > 90.0f) angle = 90.0f;

    // The original also ticks a particle timer and emitter here. Not ported yet.
}

void Bird::draw(const Gfx& gfx) const {
    if (!isVisible || frames.empty()) return;

    size_t base = static_cast<size_t>(birdColor) * 3;
    size_t idx = base + 1;  // resting frame
    if (currentAnimation >= 0 && !animations[currentAnimation].isComplete) {
        idx = static_cast<size_t>(animations[currentAnimation].currentFrame) + base;
    }
    if (idx >= frames.size()) return;

    gfx.drawRotated(*frames[idx], x - offsetX, static_cast<int>(flapOffset) + (y - offsetY), 1.0f,
                    static_cast<int>(angle));
}

void Bird::flap() {
    if (isReady) isReady = false;
    if (y >= 0 && !isDead) {
        playAnimation(0, true);
        verticalVelocity = -5.0f;
        gravity = 0.3f;
        angleVelocity = -10.0f;
        angleAcceleration = 0.4f;
        if (onFlap) onFlap();
    }
}
