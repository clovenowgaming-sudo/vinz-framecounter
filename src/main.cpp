#include "CounterState.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

// ---- Recording + macro injection --------------------------------------------------------------
class $modify(FCBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& s = CounterState::get();
        auto pl = PlayLayer::get();
        if (!s.counting && !s.injecting && pl && static_cast<GJBaseGameLayer*>(pl) == this) {
            int tick = static_cast<int>(m_gameState.m_currentProgress);
            s.current.push_back(ClickInput{tick, button, down, isPlayer1});
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto& s = CounterState::get();
        auto pl = PlayLayer::get();
        if (s.counting && s.trialRunning && pl && static_cast<GJBaseGameLayer*>(pl) == this) {
            int tick = static_cast<int>(m_gameState.m_currentProgress);
            s.inject(this, tick);
            if (tick >= s.endTick) s.requestFinish(true);
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

// ---- Trial control (restart, death, completion) -----------------------------------------------
class $modify(FCPlayLayer, PlayLayer) {
    void resetLevel() {
        auto& s = CounterState::get();
        PlayLayer::resetLevel();
        if (s.counting) s.beginTrial();
        else s.current.clear();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& s = CounterState::get();
        if (s.counting) {
            s.requestFinish(false);
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void levelComplete() {
        auto& s = CounterState::get();
        if (s.counting) {
            s.requestFinish(true);
            return;
        }
        if (!s.current.empty()) s.saved = s.current;
        PlayLayer::levelComplete();
    }

    void onQuit() {
        CounterState::get().abort();
        PlayLayer::onQuit();
    }
};

// ---- Pause menu button ------------------------------------------------------------------------
class $modify(FCPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto spr = ButtonSprite::create("Count");
        spr->setScale(0.85f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(FCPauseLayer::onCount));
        btn->setID("count-button"_spr);

        // Own menu on the right side, level with the middle button row.
        auto winSize = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("count-menu"_spr);
        menu->setPosition({0.f, 0.f});
        btn->setPosition({winSize.width - 62.f, winSize.height * 0.40f});
        menu->addChild(btn);
        this->addChild(menu, 10);
    }

    void onCount(CCObject*) {
        auto pl = PlayLayer::get();
        if (!pl) return;
        if (pl->m_isPracticeMode) {
            FLAlertLayer::create("Frame Counter", "Turn off practice mode first.", "OK")->show();
            return;
        }
        if (!CounterState::get().startCounting()) return;
        this->onResume(nullptr);
        pl->resetLevel();
    }
};
