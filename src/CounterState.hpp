#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

// One recorded input event (press or release) at a physics tick.
struct ClickInput {
    int tick;
    int button;   // 1 = jump, 2 = left, 3 = right (same as GDR)
    bool down;
    bool p1;
};

// Result for one press: allowed offsets (in ticks) around the original tick.
struct WindowResult {
    int tick;
    bool valid;
    int minOff;
    int maxOff;
};

class CounterState {
public:
    static CounterState& get() {
        static CounterState s;
        return s;
    }

    // Macro used for counting (recorded or imported)
    std::vector<ClickInput> loaded;
    std::string macroName;

    // Recording
    bool recording = false;
    std::vector<ClickInput> recBuf;

    // Counting session
    bool counting = false;
    bool trialRunning = false;
    bool injecting = false;
    int endTick = 0;

    void startRecording();
    void stopRecording();
    void updateRecLabel();
    int pressCount() const;

    bool startCounting();
    void beginTrial();
    void inject(GJBaseGameLayer* layer, int tick);
    void requestFinish(bool passed);
    void finishTrial(bool passed);
    void finishAll();
    void abort();

private:
    std::vector<ClickInput> macro;     // frozen copy used for the session
    std::vector<ClickInput> plan;      // macro with the tested press shifted
    std::vector<size_t> targets;       // indexes of presses in macro
    std::vector<WindowResult> results;
    size_t targetPos = 0;
    size_t cursor = 0;
    int dir = 0;                       // 0 = baseline, -1 = early side, +1 = late side
    int off = 0;
    int minOk = 0;
    int maxOk = 0;
    bool curValid = false;
    Ref<CCLabelBMFont> label;
    Ref<CCLabelBMFont> recLabel;

    void updateLabel();
    void removeLabel();
    void removeRecLabel();
    void restoreSpeed();
};
