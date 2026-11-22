#pragma once

// Port of com.dotgears.Transition. The original calls the idle state "isActive";
// here `finished` is true when no transition is running.
// One update() is one logic tick. A transition lasts (60 * factor) ticks.
class Transition {
public:
    bool finished = true;
    float value = 0.0f;

    // type 0 is linear, 5 is cubic ease out, 11 is quint ease out. Those are the only
    // types the game uses. Easing is sampled in 1% steps like the original tables.
    void start(float startValue, float endValue, int type, float factor) {
        startValue_ = startValue;
        endValue_ = endValue;
        distance_ = endValue - startValue;
        duration_ = static_cast<int>(60.0f * factor);
        if (duration_ < 1) duration_ = 1;
        durationFactor_ = 1.0f / static_cast<float>(duration_);
        currentPercent_ = 0;
        type_ = type;
        finished = false;
        value = startValue;
    }

    void update() {
        if (finished) return;
        currentPercent_++;
        float current = static_cast<float>(currentPercent_) * durationFactor_;
        switch (type_) {
            case 5: current = cubicOut(percent(current)); break;
            case 11: current = quintOut(percent(current)); break;
            default: break;
        }
        value = (current * distance_) + startValue_;
        if (currentPercent_ == duration_) {
            finished = true;
            value = endValue_;
        }
    }

private:
    static int percent(float v) {
        int p = static_cast<int>(v * 100.0f);
        return p < 0 ? 0 : (p > 100 ? 100 : p);
    }
    static float cubicOut(int p) {
        double d = p / 100.0 - 1.0;
        return static_cast<float>(d * d * d + 1.0);
    }
    static float quintOut(int p) {
        double d = p / 100.0 - 1.0;
        return static_cast<float>(d * d * d * d * d + 1.0);
    }

    float startValue_ = 0.0f;
    float endValue_ = 0.0f;
    float distance_ = 0.0f;
    int duration_ = 1;
    int currentPercent_ = 0;
    float durationFactor_ = 1.0f;
    int type_ = 0;
};
