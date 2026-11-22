#pragma once
#include <SDL.h>
#include <string>
#include <vector>

// One entry from atlas.txt. src is the pixel rect inside atlas.png.
// width/height is the size it is drawn at, same as the original AtlasSprite.
struct AtlasSprite {
    std::string name;
    int width = 0;
    int height = 0;
    SDL_Rect src{0, 0, 0, 0};
};

class Atlas {
public:
    SDL_Texture* tex = nullptr;
    std::vector<AtlasSprite> sprites;  // file order, like the original sprite ids

    bool load(SDL_Renderer* renderer, const char* pngPath, const char* txtPath);
    void destroy();

    // Original getSpritesByPrefix: all sprites whose name starts with prefix, in file order.
    std::vector<const AtlasSprite*> byPrefix(const std::string& prefix) const;
    // Original findSpriteByName: first sprite whose name starts with name, or nullptr.
    const AtlasSprite* find(const std::string& name) const;
};

// Stands in for GameManager's drawSprite / drawRotatedSprite calls.
// Draw order is call order, same as the original sprite pool.
struct Gfx {
    SDL_Renderer* ren = nullptr;
    Atlas atlas;

    void draw(const AtlasSprite& s, int x, int y, float alpha = 1.0f) const;
    // Stretches the sprite over w x h (used for the full screen fades).
    void drawScaled(const AtlasSprite& s, int x, int y, int w, int h, float alpha) const;
    // angleDeg is clockwise and the pivot is the sprite center, like the original.
    void drawRotated(const AtlasSprite& s, int x, int y, float alpha, int angleDeg) const;
};
