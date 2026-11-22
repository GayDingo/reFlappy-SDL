#pragma once
#include <functional>
#include <vector>

#include "Bird.h"
#include "Button.h"
#include "FontRenderer.h"
#include "Gfx.h"
#include "Hud.h"
#include "Transition.h"

// Port of com.dotgears.flappy.GameScene (plus the parts of GameManager it relies on):
// scrolling land, pipes, collision, score, the ready screen, the white flash, the game
// over screen with score panel and medals, the Play button and the title menu.
// Not in yet: pause.
class GameScene {
public:
    GameScene(Gfx& gfx, int highScore);
    GameScene(const GameScene&) = delete;
    GameScene& operator=(const GameScene&) = delete;

    bool ok() const { return valid_; }

    // Input. Pointer coordinates are in the 288x512 game space.
    void onPointerDown(int x, int y);
    void onPointerUp();
    // Keyboard or gamepad: acts like a tap, or presses Play on the game over screen.
    void onAction();

    void update();  // one logic tick, 60 per second
    void draw() const;

    // Hooks for the platform layer.
    std::function<void(Sfx)> onSfx;
    std::function<void(int)> onSaveScore;

    int score() const { return score_; }
    int bestScore() const { return bestScore_; }
    int gameSpeed() const { return gameSpeed_; }
    const Bird& bird() const { return bird_; }

    static bool checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

private:
    const AtlasSprite* require(const char* name);

    void handleTouch(int x, int y);
    void hit(bool killBird, bool pipeHit);
    void resetGameState();
    void beginRun();
    void pressPlay();
    void addScore(int points);
    void spawnSparkle(int x, int y);
    void startFade(bool fadeIn, int callbackId, float speed);
    void startScreenShake(float intensity);
    void scheduleCallback(int id, int delayFrames);
    void runCallbacks();
    void onCallback(int id);

    static constexpr int kMaxCallbacks = 50;
    static constexpr int kSparklePoolSize = 30;

    Gfx& gfx_;
    bool valid_ = true;
    SceneHooks hooks_;
    Bird bird_;
    FontRenderer scoreFont_;
    ScoreRenderer scoreRenderer_;
    ReadyScreen readyScreen_;
    GameOverScreen gameOverScreen_;
    ScorePanel scorePanel_;
    Button playButton_;
    Button scoreButton_;
    Button rateButton_;
    bool isInMenu_ = false;
    std::vector<Sparkle> sparkles_;
    int nextSparkle_ = 0;

    const AtlasSprite* bgDay_ = nullptr;
    const AtlasSprite* bgNight_ = nullptr;
    const AtlasSprite* background_ = nullptr;
    const AtlasSprite* land_ = nullptr;
    const AtlasSprite* pipeUp_ = nullptr;
    const AtlasSprite* pipeDown_ = nullptr;
    const AtlasSprite* black_ = nullptr;
    const AtlasSprite* title_ = nullptr;
    const AtlasSprite* copyright_ = nullptr;
    const AtlasSprite* white_ = nullptr;

    Transition fade_;       // black fade between runs
    Transition whiteFade_;  // white flash on impact
    int fadeCallbackId_ = 0;

    int callbackIds_[kMaxCallbacks] = {};
    int callbackDelays_[kMaxCallbacks] = {};
    int callbackIndex_ = 0;

    Pointer pointer_;
    bool tapPending_ = false;
    int tapX_ = 0, tapY_ = 0;
    int pauseW_ = 0, pauseH_ = 0;

    int landOffset_ = 0;
    int pipe1X_ = 0, pipe2X_ = 0, pipe3X_ = 0;
    int pipe1Height_ = 274, pipe2Height_ = 274, pipe3Height_ = 274;
    int pipeSpacing_ = 0;
    int pipeCountdown_ = 1;
    int gameSpeed_ = 2;
    int score_ = 0;
    int bestScore_ = 0;
};
