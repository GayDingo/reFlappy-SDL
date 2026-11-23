#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <string>

#include "Audio.h"
#include "FramePacer.h"
#include "GameScene.h"
#include "Gfx.h"
#include "MathHelper.h"

static std::string g_scorePath;

static int loadHighScore() {
    std::ifstream f(g_scorePath);
    int v = 0;
    if (f >> v && v > 0) return v;
    return 0;
}

static void saveHighScore(int v) {
    std::ofstream f(g_scorePath, std::ios::trunc);
    f << v << "\n";
}

static double displayHz(SDL_Window* win) {
    int idx = SDL_GetWindowDisplayIndex(win);
    SDL_DisplayMode mode;
    if (idx >= 0 && SDL_GetCurrentDisplayMode(idx, &mode) == 0) return mode.refresh_rate;
    return 0.0;
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    IMG_Init(IMG_INIT_PNG);
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("gamepad support unavailable: %s", SDL_GetError());
    }

    SDL_Window* win = SDL_CreateWindow("reFlappy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       576, 1024, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren =
        SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!win || !ren) {
        SDL_Log("window/renderer: %s", SDL_GetError());
        return 1;
    }
    SDL_RenderSetLogicalSize(ren, 288, 512);

    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(ren, &info) == 0) {
        SDL_Log("renderer: %s, vsync %s, video driver: %s", info.name,
                (info.flags & SDL_RENDERER_PRESENTVSYNC) ? "on" : "OFF", SDL_GetCurrentVideoDriver());
    }

    Gfx gfx;
    gfx.ren = ren;
    if (!gfx.atlas.load(ren, "assets/gfx/atlas.png", "assets/gfx/atlas.txt")) return 1;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();
    MathHelper::setSeed(static_cast<int32_t>(ms));

    // High score lives in the per-user data folder (~/.local/share/reflappy/reflappy on Linux).
    if (char* pref = SDL_GetPrefPath("reflappy", "reflappy")) {
        g_scorePath = std::string(pref) + "highscore.txt";
        SDL_free(pref);
    }
    int best = g_scorePath.empty() ? 0 : loadHighScore();

    GameScene scene(gfx, best);
    if (!scene.ok()) return 1;
    scene.onSaveScore = [&](int s) {
        if (s > best) {
            best = s;
            if (!g_scorePath.empty()) saveHighScore(s);
        }
    };

    Audio audio;
    if (std::getenv("REFLAPPY_NOSOUND")) {
        SDL_Log("sound disabled by REFLAPPY_NOSOUND");
    } else {
        audio.init("assets/sounds/");  // keeps going silently if audio is unavailable
    }
    scene.onSfx = [&](Sfx s) { audio.play(s); };

    // Frame pacing. REFLAPPY_STATS=1 prints a summary every 5 s and reports slow frames.
    const bool stats = std::getenv("REFLAPPY_STATS") != nullptr;
    FramePacer pacer;
    double hz = displayHz(win);
    pacer.setDisplayHz(hz);
    SDL_Log("display refresh: %.0f Hz", hz);
    int lastMode = -1;  // -1 not logged yet, 0 time based, 1 locked
    int frameNo = 0;
    int sFrames = 0, sTicks[4] = {0, 0, 0, 0};
    double sMaxDt = 0.0, sElapsed = 0.0;

    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    Uint64 prev = SDL_GetPerformanceCounter();

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                scene.onPointerDown(e.button.x, e.button.y);  // already in 288x512 game space
            }
            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                scene.onPointerUp();
            }
            // Gamepads. SDL reports controllers that are already plugged in at startup too.
            if (e.type == SDL_CONTROLLERDEVICEADDED) {
                if (SDL_GameController* pad = SDL_GameControllerOpen(e.cdevice.which)) {
                    SDL_Log("gamepad connected: %s", SDL_GameControllerName(pad));
                } else {
                    SDL_Log("cannot open gamepad: %s", SDL_GetError());
                }
            }
            if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                if (SDL_GameController* pad = SDL_GameControllerFromInstanceID(e.cdevice.which)) {
                    SDL_Log("gamepad disconnected: %s", SDL_GameControllerName(pad));
                    SDL_GameControllerClose(pad);
                }
            }
            if (e.type == SDL_JOYDEVICEADDED && !SDL_IsGameController(e.jdevice.which)) {
                SDL_Log("joystick '%s' has no gamepad mapping, ignoring it",
                        SDL_JoystickNameForIndex(e.jdevice.which));
            }
            if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                switch (e.cbutton.button) {
                    // Like a tap anywhere: flap, or press Play on the menu and game over screens.
                    case SDL_CONTROLLER_BUTTON_A:
                    case SDL_CONTROLLER_BUTTON_B:
                    case SDL_CONTROLLER_BUTTON_X:
                    case SDL_CONTROLLER_BUTTON_Y:
                    case SDL_CONTROLLER_BUTTON_START:
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: scene.onAction(); break;
                    default: break;
                }
            }
            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_SPACE:
                    case SDLK_UP:
                    case SDLK_RETURN: scene.onAction(); break;
                    case SDLK_F11: {
                        bool fs = (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                        SDL_SetWindowFullscreen(win, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                        break;
                    }
                    default: break;
                }
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double dt = static_cast<double>(now - prev) / freq;
        prev = now;

        frameNo++;
        if (frameNo % 60 == 0) pacer.setDisplayHz(displayHz(win));  // follows the window between displays
        int ticks = pacer.onFrame(dt);
        for (int i = 0; i < ticks; i++) scene.update();  // one tick = one original frame at 60 fps

        int mode = pacer.locked() ? 1 : 0;
        if (frameNo >= 31 && mode != lastMode) {  // the first lock check happens at frame 30
            lastMode = mode;
            if (mode == 1) {
                SDL_Log("frame pacing: locked to the display, %d frame(s) per tick", pacer.framesPerTick());
            } else {
                SDL_Log("frame pacing: time based (display is not a multiple of 60 Hz, or vsync is not pacing)");
            }
        }

        if (stats) {
            sFrames++;
            sTicks[std::min(ticks, 3)]++;
            sMaxDt = std::max(sMaxDt, dt);
            sElapsed += dt;
            double expected = 1.0 / (hz > 0.0 ? hz : 60.0);
            if (frameNo > 60 && dt > expected * 1.5) {
                SDL_Log("slow frame: %.1f ms (score %d, game speed %d)", dt * 1000.0, scene.score(), scene.gameSpeed());
            }
            if (sElapsed >= 5.0) {
                SDL_Log("stats: %d frames, ticks per frame 0x=%d 1x=%d 2x=%d 3x+=%d, slowest %.1f ms%s", sFrames,
                        sTicks[0], sTicks[1], sTicks[2], sTicks[3], sMaxDt * 1000.0, pacer.locked() ? " (locked)" : "");
                sFrames = 0;
                sTicks[0] = sTicks[1] = sTicks[2] = sTicks[3] = 0;
                sMaxDt = 0.0;
                sElapsed = 0.0;
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        scene.draw();
        SDL_RenderPresent(ren);
    }

    audio.shutdown();
    gfx.atlas.destroy();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
