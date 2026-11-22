#include "GameScene.h"

#include "MathHelper.h"

static constexpr int kPipeGap = 96;
static constexpr int kGroundY = 400;

GameScene::GameScene(Gfx& gfx, int highScore)
    : gfx_(gfx),
      bird_(gfx.atlas),
      scoreFont_(gfx.atlas, "font", 4),
      scoreRenderer_(gfx.atlas, "number_score"),
      readyScreen_(gfx.atlas),
      gameOverScreen_(gfx.atlas, hooks_),
      scorePanel_(gfx.atlas, scoreRenderer_, hooks_),
      bestScore_(highScore) {
    bgDay_ = require("bg_day");
    bgNight_ = require("bg_night");
    land_ = require("land");
    pipeUp_ = require("pipe_up");
    pipeDown_ = require("pipe_down");
    black_ = require("black");
    white_ = require("white");
    const AtlasSprite* playSprite = require("button_play");
    const AtlasSprite* scoreSprite = require("button_score");
    const AtlasSprite* rateSprite = require("button_rate");
    title_ = require("title");
    copyright_ = require("brand_copyright");
    const AtlasSprite* pauseSprite = require("button_pause");
    require("text_ready");
    require("tutorial");
    require("text_game_over");
    require("score_panel");
    require("new");
    if (!scoreRenderer_.ok()) {
        SDL_Log("number_score sprites missing from atlas");
        valid_ = false;
    }
    if (!valid_) return;

    hooks_.sfx = [this](Sfx s) {
        if (onSfx) onSfx(s);
    };
    hooks_.addScore = [this](int points) { addScore(points); };
    hooks_.spawnSparkle = [this](int x, int y) { spawnSparkle(x, y); };

    bird_.onFlap = [this]() { scheduleCallback(4, 5); };  // wing sound, next tick

    playButton_.setSprite(playSprite);
    scoreButton_.setSprite(scoreSprite);
    rateButton_.setSprite(rateSprite);
    pauseW_ = pauseSprite->width;
    pauseH_ = pauseSprite->height;

    sparkles_.reserve(kSparklePoolSize);
    for (int i = 0; i < kSparklePoolSize; i++) sparkles_.emplace_back(gfx.atlas);

    // Same layout as the original startGame().
    pipeSpacing_ = (288 - ((pipeUp_->width * 3) / 2)) / 2;
    pipe1X_ = pipeSpacing_ - (pipeUp_->width >> 1);
    pipe1Height_ = 274;
    pipe2X_ = pipe1X_ + pipeSpacing_ + pipeUp_->width;
    pipe2Height_ = 274;
    pipe3X_ = pipe2X_ + pipeSpacing_ + pipeUp_->width;
    pipe3Height_ = 274;

    // Like the original startGame(): open on the title menu and fade in from black.
    resetGameState();
    startFade(false, 0, 0.5f);
}

const AtlasSprite* GameScene::require(const char* name) {
    const AtlasSprite* s = gfx_.atlas.find(name);
    if (!s) {
        SDL_Log("sprite '%s' not found in atlas", name);
        valid_ = false;
    }
    return s;
}

bool GameScene::checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return x1 + w1 >= x2 && x1 <= x2 + w2 && y1 + h1 >= y2 && y1 <= y2 + h2;
}

// ---- input -------------------------------------------------------------------------

void GameScene::onPointerDown(int x, int y) {
    pointer_ = {x, y};
    tapPending_ = true;
    tapX_ = x;
    tapY_ = y;
}

void GameScene::onPointerUp() {
    pointer_ = {-100, -100};
}

void GameScene::onAction() {
    if (playButton_.isActive) {
        pressPlay();
        return;
    }
    tapPending_ = true;
    tapX_ = 144;
    tapY_ = 256;
}

void GameScene::handleTouch(int x, int y) {
    if (isInMenu_) return;  // only the buttons react in the menu
    // The original ignores taps near the (hidden) pause button in the top left corner.
    if (x < 0 - 20 || x > pauseW_ + 20 || y < 0 - 20 || y > pauseH_ + 20) {
        if (!bird_.isReady) {
            if (gameSpeed_ > 0) bird_.flap();
        } else if (readyScreen_.isActive && readyScreen_.state == 1) {
            readyScreen_.state = 2;
            readyScreen_.transition.start(1.0f, 0.0f, 0, 0.5f);
            bird_.isReady = false;
            bird_.flap();
        }
    }
}

void GameScene::pressPlay() {
    startFade(true, 5, 0.5f);
    hooks_.sfx(Sfx::Swoosh);
}

// ---- run lifecycle -------------------------------------------------------------------

void GameScene::resetGameState() {
    background_ = (MathHelper::nextRandom() % 10 > 3) ? bgDay_ : bgNight_;
    for (Sparkle& s : sparkles_) {
        s.isActive = false;
        s.isVisible = false;
    }
    nextSparkle_ = 0;
    playButton_.setPosition((288 - ((playButton_.width + scoreButton_.width) + 16)) >> 1, 340);
    scoreButton_.setPosition(playButton_.posX + playButton_.width + 16, 340);
    rateButton_.setPosition((288 - rateButton_.width) >> 1, 270);
    readyScreen_.isActive = false;
    gameOverScreen_.isActive = false;
    scorePanel_.isActive = false;
    scorePanel_.isVisible = false;
    isInMenu_ = true;
    landOffset_ = 0;
    bird_.init();
    gameSpeed_ = 2;
    pipeCountdown_ = 1;
    score_ = 0;
    // Like the original, pipe positions are not reset here, so the distance to the
    // first pipe depends on where the previous run ended.
}

// Original handleCallbackCompletion case 5: what happens after the Play fade.
void GameScene::beginRun() {
    resetGameState();
    playButton_.isActive = false;
    scoreButton_.isActive = false;
    rateButton_.isActive = false;
    isInMenu_ = false;
    startFade(false, 0, 0.5f);
    landOffset_ = 0;
    bird_.init();
    gameSpeed_ = 2;
    pipeCountdown_ = 1;
    score_ = 0;
    readyScreen_.run();
}

void GameScene::hit(bool killBird, bool pipeHit) {
    startScreenShake(1.0f);  // despite the name, the original only flashes white
    if (killBird) bird_.isDead = true;
    gameSpeed_ = 0;
    hooks_.sfx(Sfx::Hit);
    if (pipeHit) scheduleCallback(3, 500);  // die sound, only for pipes
    scheduleCallback(2, 1000);              // game over screen
}

void GameScene::addScore(int points) {
    if (onSaveScore) onSaveScore(points);
    if (points > bestScore_) bestScore_ = points;
}

void GameScene::spawnSparkle(int x, int y) {
    sparkles_[static_cast<size_t>(nextSparkle_)].spawn(x, y);
    nextSparkle_ = (nextSparkle_ + 1) % kSparklePoolSize;
}

// ---- fades and callbacks -------------------------------------------------------------

void GameScene::startFade(bool fadeIn, int callbackId, float speed) {
    if (fade_.finished) {
        if (fadeIn) {
            fade_.start(0.0f, 1.0f, 5, speed);
        } else {
            fade_.start(1.0f, 0.0f, 5, speed);
        }
        fade_.update();
        fadeCallbackId_ = callbackId;
    }
}

void GameScene::startScreenShake(float intensity) {
    if (whiteFade_.finished) {
        whiteFade_.start(1.0f, 0.0f, 11, intensity);
        whiteFade_.update();
    }
}

void GameScene::scheduleCallback(int id, int delayFrames) {
    callbackIds_[callbackIndex_] = id;
    callbackDelays_[callbackIndex_] = delayFrames;
    callbackIndex_++;
    if (callbackIndex_ >= kMaxCallbacks) callbackIndex_ = 0;
}

// Delays count down by 30 per tick, as in the original.
void GameScene::runCallbacks() {
    for (int i = 0; i < kMaxCallbacks; i++) {
        if (callbackDelays_[i] > 0) {
            callbackDelays_[i] -= 30;
            if (callbackDelays_[i] <= 0) onCallback(callbackIds_[i]);
        }
    }
}

void GameScene::onCallback(int id) {
    switch (id) {
        case 2:  // game over
            if (onSaveScore) onSaveScore(score_);
            gameOverScreen_.run();
            break;
        case 3: hooks_.sfx(Sfx::Die); break;
        case 4: hooks_.sfx(Sfx::Wing); break;
        case 5: beginRun(); break;
        default: break;
    }
}

// ---- one logic tick ------------------------------------------------------------------

void GameScene::update() {
    if (tapPending_) {
        tapPending_ = false;
        handleTouch(tapX_, tapY_);
    }

    runCallbacks();

    landOffset_ -= gameSpeed_;
    if (landOffset_ <= -24) landOffset_ = 0;

    if (!bird_.isReady) {
        pipe1X_ -= gameSpeed_;
        pipe2X_ -= gameSpeed_;
        pipe3X_ -= gameSpeed_;

        // A point is scored when a pipe's left edge reaches the bird's x.
        if (gameSpeed_ > 0 && pipeCountdown_ <= 0 &&
            (pipe1X_ == bird_.x || pipe1X_ == bird_.x - 1)) {
            score_++;
            hooks_.sfx(Sfx::Point);
        }

        if (pipe1X_ < -pipeUp_->width) {
            pipe1X_ = pipe2X_;
            pipe1Height_ = pipe2Height_;
            pipe2X_ = pipe3X_;
            pipe2Height_ = pipe3Height_;
            pipe3X_ = pipe2X_ + pipeSpacing_ + pipeUp_->width;
            pipe3Height_ = MathHelper::randomRange(180, 360);
            if (pipeCountdown_ > 0) {
                pipeCountdown_--;
                if (pipeCountdown_ == 0) {
                    pipe2X_ = -pipeUp_->width;
                    pipe1X_ = -pipeUp_->width;
                }
            }
        }
    }

    bird_.update();

    if (isInMenu_) {
        // The bird floats under the title.
        bird_.x = (288 - bird_.width) >> 1;
        bird_.y = title_->height + 150 + 20;
    } else {
        if (bird_.y >= kGroundY - bird_.height && gameSpeed_ > 0) {
            hit(false, false);
        }

        if (!bird_.isDead && pipeCountdown_ <= 0 && gameSpeed_ > 0) {
            auto hits = [&](int x, int y, int w, int h) {
                return checkCollision(bird_.x, bird_.y, bird_.width, bird_.height, x, y, w, h);
            };
            const int upW = pipeUp_->width, upH = pipeUp_->height;
            const int downW = pipeDown_->width, downH = pipeDown_->height;

            // Pipe 1. As in the original, only the bottom pipe marks the bird dead here.
            if (hits(pipe1X_, (pipe1Height_ - downH) - kPipeGap, downW, downH)) {
                hit(false, true);
            } else if (hits(pipe1X_, pipe1Height_, upW, upH)) {
                hit(true, true);
            }
            // Pipe 2.
            if (hits(pipe2X_, (pipe2Height_ - downH) - kPipeGap, downW, downH)) {
                hit(true, true);
            } else if (hits(pipe2X_, pipe2Height_, upW, upH)) {
                hit(true, true);
            }
        }
    }

    if (scorePanel_.isActive && scorePanel_.state == 2 && !playButton_.isActive) {
        playButton_.setPosition((288 - ((playButton_.width + scoreButton_.width) + 16)) >> 1, 340);
        scoreButton_.setPosition(playButton_.posX + playButton_.width + 16, 340);
    }

    if (gameOverScreen_.isActive) gameOverScreen_.update(scorePanel_, score_, bestScore_);
    if (readyScreen_.isActive) readyScreen_.update();

    if (playButton_.isActive) {
        playButton_.update(pointer_);
        scoreButton_.update(pointer_);
        if (playButton_.isJustReleased) pressPlay();
        // The original opens the Google Play leaderboard here. There is none off Android.
        if (scoreButton_.isJustReleased) hooks_.sfx(Sfx::Swoosh);
        if (rateButton_.isActive) {
            rateButton_.update(pointer_);
            // The original opens the Google Play rating page here. There is none off Android.
        }
    }

    // The rest of GameManager.update().
    scorePanel_.update();
    for (Sparkle& s : sparkles_) s.update();

    if (!fade_.finished || fade_.value != 0.0f) {
        fade_.update();
        if (fade_.finished) onCallback(fadeCallbackId_);
    }
    if (!whiteFade_.finished || whiteFade_.value != 0.0f) whiteFade_.update();
}

// ---- drawing, in the original call order -------------------------------------------

void GameScene::draw() const {
    gfx_.draw(*background_, 0, 0);

    if (pipeCountdown_ <= 0) {
        gfx_.draw(*pipeUp_, pipe1X_, pipe1Height_);
        gfx_.draw(*pipeDown_, pipe1X_, (pipe1Height_ - pipeDown_->height) - kPipeGap);
        gfx_.draw(*pipeUp_, pipe2X_, pipe2Height_);
        gfx_.draw(*pipeDown_, pipe2X_, (pipe2Height_ - pipeDown_->height) - kPipeGap);
        if (pipe3X_ < 288) {
            gfx_.draw(*pipeUp_, pipe3X_, pipe3Height_);
            gfx_.draw(*pipeDown_, pipe3X_, (pipe3Height_ - pipeDown_->height) - kPipeGap);
        }
    }

    if (isInMenu_) {
        gfx_.draw(*title_, (288 - title_->width) >> 1, 150);
        bird_.draw(gfx_);
        gfx_.draw(*land_, landOffset_, 512 - land_->height);
    } else {
        if (gameOverScreen_.isActive) {
            gameOverScreen_.draw(gfx_);
        } else {
            scoreFont_.drawNumber(gfx_, score_, 144, 100, 6, 1.0f);
        }
        bird_.draw(gfx_);
        gfx_.draw(*land_, landOffset_, 512 - land_->height);
    }

    if (readyScreen_.isActive) readyScreen_.draw(gfx_);

    if (isInMenu_) gfx_.draw(*copyright_, (288 - copyright_->width) >> 1, 432 - copyright_->height);

    if (playButton_.isActive) {
        playButton_.draw(gfx_);
        scoreButton_.draw(gfx_);
        if (rateButton_.isActive) rateButton_.draw(gfx_);
    }

    scorePanel_.draw(gfx_);
    for (const Sparkle& s : sparkles_) s.draw(gfx_);

    if (!fade_.finished || fade_.value != 0.0f) {
        gfx_.drawScaled(*black_, -144, -256, 864, 1536, fade_.value);
    }
    if (!(whiteFade_.finished && whiteFade_.value == 0.0f)) {
        gfx_.drawScaled(*white_, -144, -256, 864, 1536, whiteFade_.value);
    }
}
