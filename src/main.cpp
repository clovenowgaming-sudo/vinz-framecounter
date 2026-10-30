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
            s.current.push_back({m_gameState.m_currentProgress, button, down, isPlayer1});
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt) {
        auto& s = CounterState::get();
        auto pl = PlayLayer::get();
        if (s.counting && s.trialRunning && pl && static_cast<GJBaseGameLayer*>(pl) == this) {
            int tick = m_gameState.m_currentProgress;
            s.inject(this, tick);
            if (tick >= s.endTick) s.requestFinish(true);
        }
        GJBaseGameLayer::processCommands(dt);
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

        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Count"), this, menu_selector(FCPauseLayer::onCount)
        );
        btn->setID("count-button"_spr);

        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            auto menu = CCMenu::create();
            menu->addChild(btn);
            auto winSize = CCDirector::get()->getWinSize();
            menu->setPosition({winSize.width - 50.f, 40.f});
            this->addChild(menu);
        }
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
