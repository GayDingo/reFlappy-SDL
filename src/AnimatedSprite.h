#pragma once
#include <string>
#include <vector>

#include "Gfx.h"

// Port of com.dotgears.Animation. Timing is per logic tick: the original adds
// 15 "ms" to a timer every update and compares it against 1000 / fps.
struct Animation {
    bool isComplete = false;
    int frameCount = 0;
    std::vector<int> frameSequence;
    int frameTime = 0;
    bool isLooping = false;
    int timer = 0;
    int currentFrameIndex = 0;
    int currentFrame = 0;

    Animation() = default;
    Animation(const int* frames, int count, int fps, bool looping)
        : frameCount(count),
          frameSequence(frames, frames + count),
          frameTime(1000 / fps),
          isLooping(looping) {}

    void reset() {
        timer = 0;
        currentFrameIndex = 0;
        currentFrame = frameSequence[0];
    }

    void update() {
        if (isComplete) return;
        timer += 15;
        if (timer >= frameTime) {
            timer = 0;
            currentFrameIndex++;
            if (currentFrameIndex >= frameCount) {
                if (!isLooping) isComplete = true;
                currentFrameIndex = 0;
            }
            currentFrame = frameSequence[currentFrameIndex];
        }
    }

    void start() {
        if (!isLooping && isComplete) reset();
        isComplete = false;
    }
};

// Port of com.dotgears.AnimatedSprite. (x, y) is the hitbox corner; the sprite
// is drawn at (x - offsetX, y - offsetY).
class AnimatedSprite {
public:
    int x = 0, y = 0;
    int width = 0, height = 0;
    int offsetX = 0, offsetY = 0;
    float alpha = 1.0f;
    bool isActive = false;
    bool isVisible = false;
    std::vector<const AtlasSprite*> frames;
    Animation animations[10];
    int currentAnimation = -1;  // index into animations, -1 for none

    virtual ~AnimatedSprite() = default;

    virtual void update() {
        if (isActive && currentAnimation >= 0) animations[currentAnimation].update();
    }

    virtual void draw(const Gfx& gfx) const {
        if (!isVisible || frames.empty()) return;
        const AtlasSprite* s = frames[0];
        if (currentAnimation >= 0) {
            size_t i = static_cast<size_t>(animations[currentAnimation].currentFrame);
            if (i < frames.size()) s = frames[i];
        }
        gfx.draw(*s, x - offsetX, y - offsetY, alpha);
    }

    void initialize(int px, int py) {
        x = px;
        y = py;
        isActive = true;
        isVisible = true;
        alpha = 1.0f;
    }

    bool loadFrames(const Atlas& atlas, const std::string& prefix, int w, int h, int ox, int oy) {
        frames = atlas.byPrefix(prefix);
        if (frames.empty()) {
            SDL_Log("no sprites with prefix '%s'", prefix.c_str());
            return false;
        }
        if (w == 0 || h == 0) {
            width = frames[0]->width;
            height = frames[0]->height;
        } else {
            width = w;
            height = h;
        }
        if (ox == 0 || oy == 0) {
            offsetX = (frames[0]->width - width) >> 1;
            offsetY = (frames[0]->height - height) >> 1;
        } else {
            offsetX = ox;
            offsetY = oy;
        }
        return true;
    }

    void defineAnimation(int id, const int* sequence, int count, int fps, bool loop) {
        animations[id] = Animation(sequence, count, fps, loop);
    }

    void playAnimation(int id, bool restart) {
        if (restart) animations[id].reset();
        animations[id].start();
        currentAnimation = id;
    }
};
