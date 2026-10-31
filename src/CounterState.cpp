#include "CounterState.hpp"
#include <algorithm>
#include <sstream>

static constexpr int MAX_OFFSET = 60;          // max ticks tested each side
static constexpr float COUNT_TIME_SCALE = 8.f; // speed while counting
static constexpr float TICK_MS = 1000.f / 240.f;

// ---- Recording --------------------------------------------------------------------------------

int CounterState::pressCount() const {
    int n = 0;
    for (auto const& in : loaded) if (in.down) n++;
    return n;
}

void CounterState::startRecording() {
    recording = true;
    recBuf.clear();
    updateRecLabel();
}

void CounterState::stopRecording() {
    if (!recording) return;
    recording = false;
    removeRecLabel();
    if (!recBuf.empty()) {
        loaded = recBuf;
        macroName = "recorded";
        Notification::create(fmt::format("Recorded {} inputs", loaded.size()), NotificationIcon::Success)->show();
    }
    recBuf.clear();
}

void CounterState::updateRecLabel() {
    auto pl = PlayLayer::get();
    if (!pl || !recording) return;
    if (!recLabel) {
        recLabel = CCLabelBMFont::create("", "bigFont.fnt");
        recLabel->setAnchorPoint({0.f, 1.f});
        recLabel->setScale(0.4f);
        recLabel->setColor({255, 80, 80});
        pl->addChild(recLabel, 1000);
    }
    auto winSize = CCDirector::get()->getWinSize();
    recLabel->setPosition({8.f, winSize.height - 8.f});
    recLabel->setString(fmt::format("REC  {} inputs", recBuf.size()).c_str());
}

void CounterState::removeRecLabel() {
    if (recLabel) {
        recLabel->removeFromParent();
        recLabel = nullptr;
    }
}

// ---- Counting ---------------------------------------------------------------------------------

bool CounterState::startCounting() {
    if (!PlayLayer::get()) return false;

    macro = loaded;
    targets.clear();
    for (size_t i = 0; i < macro.size(); i++) {
        if (macro[i].down) targets.push_back(i);
    }
    if (targets.empty()) {
        Notification::create("No macro loaded. Record or import one first.", NotificationIcon::Warning)->show();
        return false;
    }

    results.clear();
    targetPos = 0;
    dir = 0;
    off = 0;
    minOk = maxOk = 0;
    curValid = false;
    trialRunning = false;
    counting = true;

    CCDirector::get()->getScheduler()->setTimeScale(COUNT_TIME_SCALE);
    return true;
}

void CounterState::beginTrial() {
    if (!counting || targetPos >= targets.size()) return;

    size_t ti = targets[targetPos];
    int shifted = std::max(0, macro[ti].tick + off);

    plan = macro;
    plan[ti].tick = shifted;
    std::stable_sort(plan.begin(), plan.end(), [](ClickInput const& a, ClickInput const& b) {
        return a.tick < b.tick;
    });
    cursor = 0;

    // The trial passes if the player survives until the next press (or 240 ticks after the last one).
    endTick = macro[ti].tick + 240;
    for (size_t j = ti + 1; j < macro.size(); j++) {
        if (macro[j].down) {
            endTick = macro[j].tick + 1;
            break;
        }
    }
    endTick = std::max(endTick, shifted + 2);

    trialRunning = true;
    updateLabel();
}

void CounterState::inject(GJBaseGameLayer* layer, int tick) {
    while (cursor < plan.size() && plan[cursor].tick <= tick) {
        ClickInput in = plan[cursor++];
        injecting = true;
        layer->handleButton(in.down, in.button, in.p1);
        injecting = false;
    }
}

void CounterState::requestFinish(bool passed) {
    if (!counting || !trialRunning) return;
    trialRunning = false;
    Loader::get()->queueInMainThread([passed]() {
        CounterState::get().finishTrial(passed);
    });
}

void CounterState::finishTrial(bool passed) {
    if (!counting) return;
    size_t ti = targets[targetPos];
    bool nextTarget = false;

    if (dir == 0) {
        if (!passed) {
            curValid = false;
            nextTarget = true;
        } else {
            curValid = true;
            minOk = maxOk = 0;
            dir = -1;
            off = -1;
        }
    } else if (dir == -1) {
        if (passed && off > -MAX_OFFSET) {
            minOk = off;
            off--;
        } else {
            if (passed) minOk = off;
            dir = 1;
            off = 1;
        }
    } else {
        if (passed && off < MAX_OFFSET) {
            maxOk = off;
            off++;
        } else {
            if (passed) maxOk = off;
            nextTarget = true;
        }
    }

    if (nextTarget) {
        results.push_back({macro[ti].tick, curValid, minOk, maxOk});
        targetPos++;
        if (targetPos >= targets.size()) {
            finishAll();
            return;
        }
        dir = 0;
        off = 0;
        curValid = false;
    }

    if (auto pl = PlayLayer::get()) pl->resetLevel();
    else abort();
}

void CounterState::finishAll() {
    counting = false;
    trialRunning = false;
    restoreSpeed();
    removeLabel();

    std::ostringstream out;
    for (size_t i = 0; i < results.size(); i++) {
        auto const& r = results[i];
        out << "#" << (i + 1) << "  tick " << r.tick << ":  ";
        if (!r.valid) {
            out << "baseline failed (macro did not replay cleanly)\n";
        } else {
            int w = r.maxOff - r.minOff + 1;
            out << w << (w == 1 ? " tick" : " ticks")
                << "  (" << r.minOff << " to +" << r.maxOff << ")  ~"
                << static_cast<int>(w * TICK_MS * 100.f) / 100.f << " ms\n";
        }
    }
    log::info("Frame windows:\n{}", out.str());

    FLAlertLayer::create(nullptr, "Frame windows", out.str(), "OK", nullptr, 420.f, true, 290.f, 0.9f)->show();

    if (auto pl = PlayLayer::get()) pl->resetLevel();
}

void CounterState::abort() {
    if (!counting) return;
    counting = false;
    trialRunning = false;
    restoreSpeed();
    removeLabel();
}

void CounterState::restoreSpeed() {
    CCDirector::get()->getScheduler()->setTimeScale(1.f);
}

void CounterState::updateLabel() {
    auto pl = PlayLayer::get();
    if (!pl) return;
    if (!label) {
        label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setAnchorPoint({0.f, 1.f});
        label->setScale(0.45f);
        pl->addChild(label, 1000);
    }
    auto winSize = CCDirector::get()->getWinSize();
    label->setPosition({8.f, winSize.height - 8.f});
    label->setString(fmt::format("Counting click {}/{}  offset {:+}", targetPos + 1, targets.size(), off).c_str());
}

void CounterState::removeLabel() {
    if (label) {
        label->removeFromParent();
        label = nullptr;
    }
}
