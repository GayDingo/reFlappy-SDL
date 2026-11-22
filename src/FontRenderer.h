#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>

#include "Gfx.h"

// Port of com.dotgears.FontRenderer for number sprites named "<prefix>_<charcode>"
// (for example font_048 is the digit '0').
class FontRenderer {
public:
    FontRenderer(const Atlas& atlas, const std::string& prefix, int spacing) : spacing_(spacing) {
        for (const AtlasSprite* s : atlas.byPrefix(prefix)) {
            size_t us = s->name.find('_');
            if (us == std::string::npos) continue;
            int code = std::atoi(s->name.c_str() + us + 1);
            if (code < 0 || code > 255) continue;
            sprites_[code] = s;
            widths_[code] = s->width;
            maxHeight_ = std::max(maxHeight_, s->height);
        }
        widths_[32] = widths_[48];
    }

    // align bits, same as the original alignmentFlags:
    // 1 right-aligned at x, 2 centered on x, 4 centered on y, 8 bottom-aligned at y.
    void drawNumber(const Gfx& gfx, int number, int x, int y, int align, float alpha) const {
        std::string text = std::to_string(number < 0 ? 0 : number);

        int totalWidth = 0;
        for (char c : text) totalWidth += widths_[static_cast<unsigned char>(c)] - spacing_;
        int finalWidth = totalWidth + 2;

        if (align & 2) {
            x -= finalWidth / 2;
        } else if (align & 1) {
            x -= finalWidth;
        }
        if (align & 4) {
            y -= maxHeight_ / 2;
        } else if (align & 8) {
            y -= maxHeight_;
        }

        for (char c : text) {
            unsigned char u = static_cast<unsigned char>(c);
            if (sprites_[u]) gfx.draw(*sprites_[u], x, y, alpha);
            x += widths_[u] - spacing_;
        }
    }

private:
    const AtlasSprite* sprites_[256] = {};
    int widths_[256] = {};
    int maxHeight_ = 0;
    int spacing_ = 0;
};
