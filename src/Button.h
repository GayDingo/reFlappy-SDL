#pragma once
#include "Gfx.h"

// The original touchX/touchY. (-100, -100) means no pointer is down.
struct Pointer {
    int x = -100;
    int y = -100;
};

// Port of com.dotgears.Button. A click is a press that starts inside the button and
// is then released (the original never tracks dragging).
class Button {
public:
    const AtlasSprite* sprite = nullptr;
    int posX = 0, posY = 0, width = 0, height = 0;
    bool isActive = false;
    bool isPressed = false;
    bool isJustPressed = false;
    bool isJustReleased = false;
    bool isTouched = false;

    void setSprite(const AtlasSprite* s) {
        sprite = s;
        width = s->width;
        height = s->height;
    }

    void setPosition(int x, int y) {
        posX = x;
        posY = y;
        isActive = true;
        isTouched = false;
        isJustPressed = false;
        isJustReleased = false;
        isPressed = false;
    }

    void update(const Pointer& p) {
        isTouched = p.x > posX && p.x < posX + width && p.y > posY && p.y < posY + height;
        isJustPressed = false;
        isJustReleased = false;
        if (isTouched != isPressed) {
            if (isPressed) {
                isJustReleased = true;
                isPressed = false;
            } else {
                isJustPressed = true;
                isPressed = true;
            }
        }
    }

    void draw(const Gfx& gfx) const {
        if (sprite) gfx.draw(*sprite, posX, posY + (isPressed ? 2 : 0));
    }
};
