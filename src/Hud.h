#pragma once
#include <functional>
#include <string>
#include <vector>

#include "AnimatedSprite.h"
#include "Gfx.h"
#include "MathHelper.h"
#include "Transition.h"

// Sound effects the game asks for. The original numbers them 9 to 13 in registerCallback.
enum class Sfx { Point, Swoosh, Die, Hit, Wing };

// What the screens need from the scene (the original reaches for GameManager.instance).
struct SceneHooks {
    std::function<void(Sfx)> sfx;
    std::function<void(int)> addScore;
    std::function<void(int, int)> spawnSparkle;
};

// Port of com.dotgears.ScoreRenderer: small score digits, right aligned.
class ScoreRenderer {
public:
    ScoreRenderer(const Atlas& atlas, const std::string& prefix) : frames_(atlas.byPrefix(prefix)) {
        if (!frames_.empty()) width_ = frames_[0]->width;
    }
    bool ok() const { return frames_.size() >= 10; }

    // x is the right edge. Always draws at least one digit.
    void renderScore(const Gfx& gfx, int score, int x, int y) const {
        if (!ok()) return;
        int offsetX = x - width_;
        bool first = true;
        int rest = score < 0 ? 0 : score;
        for (int n = 10; n > 0; n--) {
            if (rest > 0 || first) {
                gfx.draw(*frames_[static_cast<size_t>(rest % 10)], offsetX, y);
                offsetX -= width_;
                rest /= 10;
                first = false;
            }
        }
    }

private:
    std::vector<const AtlasSprite*> frames_;
    int width_ = 0;
};

// Port of com.dotgears.ParticleEmitter: one short blinking star.
class Sparkle : public AnimatedSprite {
public:
    explicit Sparkle(const Atlas& atlas) {
        loadFrames(atlas, "blink", 10, 10, 4, 4);
        static const int seq[5] = {0, 1, 2, 1, 0};
        defineAnimation(0, seq, 5, 10, false);
        isActive = false;
        isVisible = false;
        playAnimation(0, true);
    }

    void spawn(int px, int py) {
        initialize(px, py);
        playAnimation(0, true);
    }

    void update() override {
        if (!isActive) return;
        AnimatedSprite::update();
        if (currentAnimation >= 0 && animations[currentAnimation].isComplete) {
            isActive = false;
            isVisible = false;
        }
    }
};

// Port of com.dotgears.MedalDisplay.
class MedalDisplay {
public:
    explicit MedalDisplay(const Atlas& atlas) : sprites_(atlas.byPrefix("medals")) {}

    int x = 0, y = 0;
    int width = 0, height = 0;
    int id = 0;
    bool isActive = false;
    bool isVisible = false;

    void initialize(int medalId) {
        x = 0;
        y = 0;
        width = 44;
        height = 44;
        id = medalId;
        sparkleTimer_ = 30;
        isActive = true;
        isVisible = true;
    }

    void update(SceneHooks& hooks) {
        if (isActive && sparkleTimer_ > 0) {
            sparkleTimer_--;
            if (sparkleTimer_ <= 0) {
                sparkleTimer_ = 30;
                hooks.spawnSparkle((x - 3) + MathHelper::randomRange(0, width + 6),
                                   (y - 3) + MathHelper::randomRange(0, height + 6));
            }
        }
    }

    void draw(const Gfx& gfx) const {
        if (isVisible && static_cast<size_t>(id) < sprites_.size()) gfx.draw(*sprites_[static_cast<size_t>(id)], x, y);
    }

private:
    std::vector<const AtlasSprite*> sprites_;
    int sparkleTimer_ = 0;
};

// Port of com.dotgears.ScorePanel: slides up, counts the score, shows a medal.
class ScorePanel {
public:
    ScorePanel(const Atlas& atlas, const ScoreRenderer& renderer, SceneHooks& hooks)
        : renderer_(renderer),
          hooks_(hooks),
          medal_(atlas),
          panelSprite_(atlas.find("score_panel")),
          newSprite_(atlas.find("new")) {
        if (panelSprite_) {
            panelWidth_ = panelSprite_->width;
            panelHeight_ = panelSprite_->height;
        }
        targetY_ = (512 - panelHeight_) >> 1;
    }

    bool isActive = false;
    bool isVisible = false;
    int state = 0;  // 0 sliding in, 1 counting up, 2 done

    void show(int delay, int bestScore, int bronze, int silver, int gold, int platinum) {
        delay_ = delay;
        bestScore_ = bestScore;
        currentScore_ = 0;
        bronze_ = bronze;
        silver_ = silver;
        gold_ = gold;
        platinum_ = platinum;
        isActive = true;
        isVisible = true;
        isNewHighScore_ = false;
        panelX_ = (288 - panelWidth_) >> 1;
        panelY_ = 504;
        transition_.start(static_cast<float>(panelY_), static_cast<float>(targetY_), 11, 0.5f);
        state = 0;
        medal_.isActive = false;
        medal_.isVisible = false;
    }

    void update() {
        if (!isActive) return;
        if (!transition_.finished) transition_.update();

        switch (state) {
            case 0:
                panelY_ = static_cast<int>(transition_.value);
                if (transition_.finished) {
                    if (delay_ <= 0) {
                        state = 2;
                    } else {
                        state = 1;
                        transition_.start(0.0f, static_cast<float>(delay_), 0, 0.5f);
                    }
                }
                break;
            case 1:
                currentScore_ = static_cast<int>(transition_.value);
                if (transition_.finished) {
                    state = 2;
                    hooks_.addScore(currentScore_);
                    if (currentScore_ > bestScore_) {
                        bestScore_ = currentScore_;
                        isNewHighScore_ = true;
                    }
                    if (currentScore_ >= platinum_) {
                        medal_.initialize(0);
                    } else if (currentScore_ >= gold_) {
                        medal_.initialize(1);
                    } else if (currentScore_ >= silver_) {
                        medal_.initialize(2);
                    } else if (currentScore_ >= bronze_) {
                        medal_.initialize(3);
                    }
                    medal_.x = panelX_ + 32;
                    medal_.y = panelY_ + 44;
                }
                break;
            case 2:
                medal_.update(hooks_);
                break;
            default: break;
        }
    }

    void draw(const Gfx& gfx) const {
        if (!isVisible || !panelSprite_) return;
        gfx.draw(*panelSprite_, panelX_, panelY_);
        renderer_.renderScore(gfx, currentScore_, panelX_ + 210, panelY_ + 36);
        renderer_.renderScore(gfx, bestScore_, panelX_ + 210, panelY_ + 78);
        if (isNewHighScore_ && newSprite_) gfx.draw(*newSprite_, panelX_ + 142, panelY_ + 60);
        medal_.draw(gfx);
    }

private:
    const ScoreRenderer& renderer_;
    SceneHooks& hooks_;
    MedalDisplay medal_;
    const AtlasSprite* panelSprite_ = nullptr;
    const AtlasSprite* newSprite_ = nullptr;
    int panelWidth_ = 0;
    int panelHeight_ = 0;
    int targetY_ = 0;
    int panelX_ = 0;
    int panelY_ = 0;
    int bronze_ = 0, silver_ = 0, gold_ = 0, platinum_ = 0;
    bool isNewHighScore_ = false;
    int currentScore_ = 0;
    int bestScore_ = 0;
    int delay_ = 0;
    Transition transition_;

};

// Port of com.dotgears.flappy.ReadyScreen: "get ready" text and the tap hint.
class ReadyScreen {
public:
    explicit ReadyScreen(const Atlas& atlas)
        : ready_(atlas.find("text_ready")), tutorial_(atlas.find("tutorial")) {}

    bool isActive = false;
    int state = 0;  // 0 fading in, 1 waiting for the first tap, 2 fading out
    Transition transition;

    void run() {
        isActive = true;
        transition.start(0.0f, 1.0f, 0, 0.5f);
        state = 0;
    }

    void update() {
        transition.update();
        switch (state) {
            case 0:
                if (transition.finished) state = 1;
                break;
            default: break;
        }
    }

    void draw(const Gfx& gfx) const {
        if (!ready_ || !tutorial_) return;
        gfx.draw(*ready_, (288 - ready_->width) >> 1, 146, transition.value);
        gfx.draw(*tutorial_, (288 - tutorial_->width) >> 1, 220, transition.value);
    }

private:
    const AtlasSprite* ready_ = nullptr;
    const AtlasSprite* tutorial_ = nullptr;
};

// Port of com.dotgears.flappy.GameOverScreen: the "game over" text, then the score panel.
class GameOverScreen {
public:
    GameOverScreen(const Atlas& atlas, SceneHooks& hooks)
        : hooks_(hooks), text_(atlas.find("text_game_over")) {}

    bool isActive = false;

    void run() {
        isActive = true;
        transition_.start(0.0f, 1.0f, 11, 1.0f);
        positionY_ = -1;
        velocity_ = -2.0f;
        gravity_ = 0.25f;
        state_ = 0;
        hooks_.sfx(Sfx::Swoosh);
    }

    void update(ScorePanel& panel, int score, int bestScore) {
        transition_.update();
        if (positionY_ < 0) {
            positionY_ = static_cast<int>(static_cast<float>(positionY_) + velocity_);
            velocity_ += gravity_;
        } else {
            positionY_ = 0;
        }

        switch (state_) {
            case 0:
                if (transition_.finished) {
                    state_ = 1;
                    panel.show(score, bestScore, 10, 20, 30, 40);
                    hooks_.sfx(Sfx::Swoosh);
                }
                break;
            case 1:
                if (panel.state == 2) state_ = 2;
                break;
            default: break;
        }
    }

    void draw(const Gfx& gfx) const {
        if (text_) gfx.draw(*text_, (288 - text_->width) >> 1, positionY_ + 130, transition_.value);
    }

private:
    SceneHooks& hooks_;
    const AtlasSprite* text_ = nullptr;
    Transition transition_;
    int positionY_ = 0;
    float velocity_ = 0.0f;
    float gravity_ = 0.0f;
    int state_ = 0;
};
