#include "Audio.h"

#include <SDL.h>

#include "Hud.h"

// Same files and same order as the Sfx enum: Point, Swoosh, Die, Hit, Wing.
static const char* const kFiles[] = {
    "sfx_point.ogg", "sfx_swooshing.ogg", "sfx_die.ogg", "sfx_hit.ogg", "sfx_wing.ogg",
};

bool Audio::init(const std::string& soundDir) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SDL_Log("audio init failed, running silent: %s", SDL_GetError());
        return false;
    }
    if ((Mix_Init(MIX_INIT_OGG) & MIX_INIT_OGG) == 0) {
        SDL_Log("Mix_Init(OGG): %s (trying to load anyway)", Mix_GetError());
    }
    // A small buffer keeps the wing sound close to the tap.
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512) != 0) {
        SDL_Log("Mix_OpenAudio failed, running silent: %s", Mix_GetError());
        return false;
    }
    open_ = true;
    Mix_AllocateChannels(16);  // the swoosh is 2 s long and plays twice, so sounds overlap

    int loaded = 0;
    for (int i = 0; i < kCount; i++) {
        std::string path = soundDir + kFiles[i];
        chunks_[i] = Mix_LoadWAV(path.c_str());
        if (chunks_[i]) {
            loaded++;
        } else {
            SDL_Log("cannot load %s: %s", path.c_str(), Mix_GetError());
        }
    }
    SDL_Log("audio ready, %d of %d sounds loaded", loaded, kCount);
    return loaded > 0;
}

void Audio::play(Sfx sfx) {
    if (!open_) return;
    Mix_Chunk* chunk = chunks_[static_cast<int>(sfx)];
    if (chunk) Mix_PlayChannel(-1, chunk, 0);
}

void Audio::shutdown() {
    if (!open_) return;
    Mix_HaltChannel(-1);
    for (Mix_Chunk*& c : chunks_) {
        if (c) {
            Mix_FreeChunk(c);
            c = nullptr;
        }
    }
    Mix_CloseAudio();
    Mix_Quit();
    open_ = false;
}
