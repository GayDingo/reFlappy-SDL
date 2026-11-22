#pragma once
#include <SDL_mixer.h>

#include <string>

enum class Sfx;  // defined in Hud.h

// Plays the five sound effects with SDL_mixer. If no audio device can be opened the
// game keeps running silently.
class Audio {
public:
    Audio() = default;
    ~Audio() { shutdown(); }
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    // soundDir is the folder holding the sfx_*.ogg files, with a trailing slash.
    bool init(const std::string& soundDir);
    void play(Sfx sfx);
    void shutdown();

private:
    static constexpr int kCount = 5;
    Mix_Chunk* chunks_[kCount] = {};
    bool open_ = false;
};
