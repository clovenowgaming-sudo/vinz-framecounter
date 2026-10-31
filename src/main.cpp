#include "CounterState.hpp"
#include "CounterPopup.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

// ---- Recording + macro injection --------------------------------------------------------------
class $modify(FCBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& s = CounterState::get();
        auto pl = PlayLayer::get();
        if (s.recording && !s.counting && !s.injecting && pl && static_cast<GJBaseGameLayer*>(pl) == this) {
            int tick = static_cast<int>(m_gameState.m_currentProgress);
            s.recBuf.push_back(ClickInput{tick, button, down, isPlayer1});
            s.updateRecLabel();
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
        if (s.counting) {
            s.beginTrial();
        } else if (s.recording) {
            s.recBuf.clear();   // every attempt starts a fresh recording
            s.updateRecLabel();
        }
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
        if (s.recording) s.stopRecording();   // completed run becomes the macro
        PlayLayer::levelComplete();
    }

    void onQuit() {
        auto& s = CounterState::get();
        s.abort();
        if (s.recording) s.stopRecording();
        PlayLayer::onQuit();
    }
};

// ---- Pause menu: small button in the bottom-right corner --------------------------------------
class $modify(FCPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto spr = ButtonSprite::create("FC");
        spr->setScale(0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(FCPauseLayer::onFC));
        btn->setID("fc-button"_spr);

        auto winSize = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("fc-menu"_spr);
        menu->setPosition({0.f, 0.f});
        btn->setPosition({winSize.width - 32.f, 28.f});
        menu->addChild(btn);
        this->addChild(menu, 10);
    }

    void onFC(CCObject*) {
        if (auto popup = CounterPopup::create(this)) popup->show();
    }
};
