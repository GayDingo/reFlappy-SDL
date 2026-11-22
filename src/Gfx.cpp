#include "Gfx.h"

#include <SDL_image.h>

#include <cmath>
#include <fstream>

bool Atlas::load(SDL_Renderer* renderer, const char* pngPath, const char* txtPath) {
    tex = IMG_LoadTexture(renderer, pngPath);
    if (!tex) {
        SDL_Log("atlas png: %s", IMG_GetError());
        return false;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    int tw = 0, th = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);

    std::ifstream f(txtPath);
    if (!f) {
        SDL_Log("cannot open %s", txtPath);
        return false;
    }

    // Line format: name w h u v uw vh (UVs normalized 0..1)
    std::string name;
    int w, h;
    float u, v, uw, vh;
    while (f >> name >> w >> h >> u >> v >> uw >> vh) {
        AtlasSprite s;
        s.name = name;
        s.width = w;
        s.height = h;
        s.src = {static_cast<int>(std::lround(u * tw)), static_cast<int>(std::lround(v * th)),
                 static_cast<int>(std::lround(uw * tw)), static_cast<int>(std::lround(vh * th))};
        sprites.push_back(std::move(s));
    }
    SDL_Log("atlas %dx%d, %zu sprites", tw, th, sprites.size());
    return true;
}

void Atlas::destroy() {
    if (tex) {
        SDL_DestroyTexture(tex);
        tex = nullptr;
    }
}

std::vector<const AtlasSprite*> Atlas::byPrefix(const std::string& prefix) const {
    std::vector<const AtlasSprite*> out;
    for (const AtlasSprite& s : sprites) {
        if (s.name.compare(0, prefix.size(), prefix) == 0) out.push_back(&s);
    }
    return out;
}

const AtlasSprite* Atlas::find(const std::string& name) const {
    for (const AtlasSprite& s : sprites) {
        if (s.name.compare(0, name.size(), name) == 0) return &s;
    }
    return nullptr;
}

static Uint8 alphaByte(float a) {
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return static_cast<Uint8>(a * 255.0f + 0.5f);
}

void Gfx::draw(const AtlasSprite& s, int x, int y, float alpha) const {
    SDL_Rect dst{x, y, s.width, s.height};
    SDL_SetTextureAlphaMod(atlas.tex, alphaByte(alpha));
    SDL_RenderCopy(ren, atlas.tex, &s.src, &dst);
}

void Gfx::drawScaled(const AtlasSprite& s, int x, int y, int w, int h, float alpha) const {
    SDL_Rect dst{x, y, w, h};
    SDL_SetTextureAlphaMod(atlas.tex, alphaByte(alpha));
    SDL_RenderCopy(ren, atlas.tex, &s.src, &dst);
}

void Gfx::drawRotated(const AtlasSprite& s, int x, int y, float alpha, int angleDeg) const {
    SDL_Rect dst{x, y, s.width, s.height};
    SDL_SetTextureAlphaMod(atlas.tex, alphaByte(alpha));
    SDL_RenderCopyEx(ren, atlas.tex, &s.src, &dst, static_cast<double>(angleDeg), nullptr,
                     SDL_FLIP_NONE);
}
