#pragma once
#include <cmath>

// Decides how many logic ticks to run for each rendered frame.
//
// The game logic runs at 60 ticks per second. A plain "accumulate time, run ticks" loop
// stutters when the display also runs at about 60 Hz: tiny timing noise makes one frame
// run 0 ticks and the next one run 2. When the display refresh is a multiple of 60 and
// vsync really paces the frames, this runs exactly one tick every N frames instead.
// If vsync is not pacing the loop, or the refresh is something else (75, 144 Hz), it
// falls back to the time based loop.
class FramePacer {
public:
    // Call when the display refresh rate is known or changes. 0 means unknown.
    void setDisplayHz(double hz) {
        if (hz == hz_) return;
        hz_ = hz;
        framesPerTick_ = 0;
        period_ = hz > 0.0 ? 1.0 / hz : 0.0;
        if (hz > 0.0) {
            int n = static_cast<int>(std::lround(hz / 60.0));
            // Accepts 59.94, 60, 119.88, 120 and so on, but not 75 or 144.
            if (n >= 1 && n <= 8 && std::fabs(hz / (60.0 * n) - 1.0) < 0.02) framesPerTick_ = n;
        }
        locked_ = false;
        frameCounter_ = 0;
        acc_ = 0.0;
        sum_ = 0.0;
        count_ = 0;
    }

    // dt is the time since the previous frame, in seconds. Returns the ticks to run now.
    int onFrame(double dt) {
        // Every 30 frames, check that frames really arrive at the display rate.
        sum_ += dt;
        count_++;
        if (count_ >= 30) {
            double avg = sum_ / count_;
            bool nowLocked = framesPerTick_ > 0 && std::fabs(avg / period_ - 1.0) < 0.15;
            if (nowLocked != locked_) {
                locked_ = nowLocked;
                frameCounter_ = 0;
                acc_ = 0.0;
            }
            sum_ = 0.0;
            count_ = 0;
        }

        if (locked_) {
            if (++frameCounter_ >= framesPerTick_) {
                frameCounter_ = 0;
                return 1;
            }
            return 0;
        }

        acc_ += dt;
        if (acc_ > 0.25) acc_ = 0.25;
        int ticks = 0;
        while (acc_ >= kStep) {
            acc_ -= kStep;
            ticks++;
        }
        return ticks;
    }

    bool locked() const { return locked_; }
    int framesPerTick() const { return framesPerTick_; }

private:
    static constexpr double kStep = 1.0 / 60.0;
    double hz_ = -1.0;
    double period_ = 0.0;
    int framesPerTick_ = 0;
    bool locked_ = false;
    int frameCounter_ = 0;
    double acc_ = 0.0;
    double sum_ = 0.0;
    int count_ = 0;
};
