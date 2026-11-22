# reFlappy-SDL

A native C++ and SDL port of **Flappy Bird**, rewritten from the Android Java/AndEngine decompilation project [THEN00P/reFlappy](https://github.com/THEN00P/reFlappy)[cite: 1, 2].

This port replaces the Android SDK, Gradle, and Java AndEngine stack with a C++ codebase built using CMake and SDL[cite: 1, 2].

## Project Structure

```text
reFlappy-SDL/
├── CMakeLists.txt          # CMake build configuration
├── assets/
│   ├── gfx/
│   │   ├── atlas.png       # Sprite texture atlas
│   │   └── atlas.txt       # Texture atlas region coordinates
│   └── sounds/
│       ├── sfx_die.ogg
│       ├── sfx_hit.ogg
│       ├── sfx_point.ogg
│       ├── sfx_swooshing.ogg
│       └── sfx_wing.ogg
└── src/
    ├── main.cpp            # Entry point and main game loop
    ├── GameScene.cpp/.h    # Core gameplay state, pipes, and collision logic
    ├── Bird.cpp/.h         # Player physics, flapping, and animation
    ├── Gfx.cpp/.h          # SDL texture atlas loading and sprite rendering
    ├── Audio.cpp/.h        # Sound effect playback
    ├── AnimatedSprite.h    # Frame-based sprite animation helper
    ├── Button.h            # UI button interaction handling
    ├── FontRenderer.h      # Bitmap/sprite font rendering
    ├── FramePacer.h        # Fixed-timestep / frame pacing loop control
    ├── Hud.h               # Scoreboard, medals, and UI overlays
    ├── MathHelper.h        # Math and geometry utilities
    └── Transition.h        # Screen fade transitions

```

## Prerequisites

* A C++ compiler with C++17 support (`g++`, `clang++`, or MSVC)
* **CMake** (3.15 or newer)
* **SDL2** development libraries (`SDL2`, `SDL2_image`, and `SDL2_mixer` for PNG textures and OGG audio)



### Installing Dependencies

**Ubuntu / Debian:**

```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev

```

**macOS (Homebrew):**

```bash
brew install cmake sdl2 sdl2_image sdl2_mixer

```

**Arch Linux:**

```bash
sudo pacman -S base-devel cmake sdl2 sdl2_image sdl2_mixer

```

## Building and Running

1. Clone the repository:
```bash
git clone [https://github.com/GayDingo/reFlappy-SDL.git](https://github.com/GayDingo/reFlappy-SDL.git)
cd reFlappy-SDL

```


2. Configure and build with CMake:


```bash
cmake -B build
cmake --build build

```


3. Run the executable from the project root so the `assets/` folder is found:


```bash
./build/reFlappy-SDL

```



## Credits

* **Original Decompilation:** [THEN00P/reFlappy](https://github.com/THEN00P/reFlappy) (Java / Android / AndEngine)


* **Original Game:** *Flappy Bird* by Dong Nguyen (.GEARS)
* **Porting & Assistance:** C++ / SDL port created with assistance from Claude (Anthropic)

```

```
