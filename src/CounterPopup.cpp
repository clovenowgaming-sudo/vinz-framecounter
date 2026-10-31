#include "CounterPopup.hpp"
#include "MacroIO.hpp"

static constexpr int PER_PAGE = 3;

CounterPopup* CounterPopup::create(PauseLayer* pause) {
    auto ret = new CounterPopup();
    if (ret->init(pause)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

CCMenuItemSpriteExtra* CounterPopup::addButton(char const* text, SEL_MenuHandler cb, CCPoint offset, int width, int tag) {
    auto spr = ButtonSprite::create(text, width, true, "goldFont.fnt", "GJ_button_01.png", 28.f, 0.7f);
    auto btn = CCMenuItemSpriteExtra::create(spr, this, cb);
    btn->setTag(tag);
    m_buttonMenu->addChildAtPosition(btn, Anchor::Center, offset);
    return btn;
}

bool CounterPopup::init(PauseLayer* pause) {
    if (!Popup::init(400.f, 250.f)) return false;
    m_pause = pause;
    this->setTitle("Frame Window Counter");

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(0.7f);
    m_mainLayer->addChildAtPosition(m_status, Anchor::Top, {0.f, -45.f});

    // Row 1: Record / Count / Export
    auto recBtn = addButton("Record", menu_selector(CounterPopup::onRecord), {-120.f, 35.f}, 100);
    m_recSpr = static_cast<ButtonSprite*>(recBtn->getNormalImage());
    addButton("Count", menu_selector(CounterPopup::onCount), {0.f, 35.f}, 100);
    addButton("Export", menu_selector(CounterPopup::onExport), {120.f, 35.f}, 100);

    // Import section title
    auto importLbl = CCLabelBMFont::create("Import macro (.gdr2)", "bigFont.fnt");
    importLbl->setScale(0.4f);
    m_mainLayer->addChildAtPosition(importLbl, Anchor::Center, {0.f, 5.f});

    // Page arrows
    addButton("<", menu_selector(CounterPopup::onPrev), {-165.f, -45.f}, 30);
    addButton(">", menu_selector(CounterPopup::onNext), {165.f, -45.f}, 30);

    this->refresh();
    return true;
}

void CounterPopup::refresh() {
    auto& s = CounterState::get();

    m_status->setString(fmt::format(
        "Macro: {} ({} clicks){}",
        s.macroName.empty() ? "none" : s.macroName,
        s.pressCount(),
        s.recording ? "  [REC]" : ""
    ).c_str());

    if (m_recSpr) m_recSpr->setString(s.recording ? "Stop" : "Record");

    for (auto node : m_fileNodes) node->removeFromParent();
    m_fileNodes.clear();

    m_files = macroio::listMacros();
    int pages = std::max(1, (static_cast<int>(m_files.size()) + PER_PAGE - 1) / PER_PAGE);
    m_page = std::clamp(m_page, 0, pages - 1);

    if (m_files.empty()) {
        auto lbl = CCLabelBMFont::create("No .gdr2 files found", "chatFont.fnt");
        lbl->setScale(0.7f);
        m_mainLayer->addChildAtPosition(lbl, Anchor::Center, {0.f, -45.f});
        m_fileNodes.push_back(lbl);
        auto hint = CCLabelBMFont::create("Put them in Download or the mod's macros folder", "chatFont.fnt");
        hint->setScale(0.5f);
        m_mainLayer->addChildAtPosition(hint, Anchor::Center, {0.f, -65.f});
        m_fileNodes.push_back(hint);
        return;
    }

    for (int i = 0; i < PER_PAGE; i++) {
        int idx = m_page * PER_PAGE + i;
        if (idx >= static_cast<int>(m_files.size())) break;

        std::string name = m_files[idx].filename().string();
        if (name.size() > 30) name = name.substr(0, 27) + "...";

        auto spr = ButtonSprite::create(name.c_str(), 240, true, "bigFont.fnt", "GJ_button_04.png", 24.f, 0.5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(CounterPopup::onPick));
        btn->setTag(idx);
        m_buttonMenu->addChildAtPosition(btn, Anchor::Center, {0.f, -28.f - 26.f * i});
        m_fileNodes.push_back(btn);
    }
}

void CounterPopup::onRecord(CCObject*) {
    auto& s = CounterState::get();
    if (s.recording) {
        s.stopRecording();
        this->refresh();
        return;
    }

    auto pl = PlayLayer::get();
    if (!pl) return;
    if (pl->m_isPracticeMode) {
        FLAlertLayer::create("Frame Counter", "Turn off practice mode first.", "OK")->show();
        return;
    }

    s.startRecording();
    Ref<PauseLayer> pause = m_pause;
    this->onClose(nullptr);
    if (pause) pause->onResume(nullptr);
    pl->resetLevel();
}

void CounterPopup::onCount(CCObject*) {
    auto& s = CounterState::get();
    if (s.recording) s.stopRecording();

    auto pl = PlayLayer::get();
    if (!pl) return;
    if (pl->m_isPracticeMode) {
        FLAlertLayer::create("Frame Counter", "Turn off practice mode first.", "OK")->show();
        return;
    }
    if (!s.startCounting()) return;

    Ref<PauseLayer> pause = m_pause;
    this->onClose(nullptr);
    if (pause) pause->onResume(nullptr);
    pl->resetLevel();
}

void CounterPopup::onExport(CCObject*) {
    auto& s = CounterState::get();
    if (s.recording) s.stopRecording();
    if (s.loaded.empty()) {
        Notification::create("No macro to export", NotificationIcon::Warning)->show();
        return;
    }

    std::string levelName = "macro";
    int levelId = 0;
    if (auto pl = PlayLayer::get()) {
        levelName = pl->m_level->m_levelName;
        levelId = static_cast<int>(pl->m_level->m_levelID.value());
    }

    std::string info;
    if (macroio::exportMacro(s.loaded, levelName, levelId, info)) {
        Notification::create("Exported: " + info, NotificationIcon::Success)->show();
    } else {
        Notification::create("Export failed: " + info, NotificationIcon::Error)->show();
    }
    this->refresh();
}

void CounterPopup::onPrev(CCObject*) {
    m_page--;
    this->refresh();
}

void CounterPopup::onNext(CCObject*) {
    m_page++;
    this->refresh();
}

void CounterPopup::onPick(CCObject* sender) {
    int idx = static_cast<CCNode*>(sender)->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_files.size())) return;

    auto& s = CounterState::get();
    std::vector<ClickInput> inputs;
    std::string err;
    if (macroio::importMacro(m_files[idx], inputs, err)) {
        s.loaded = std::move(inputs);
        s.macroName = m_files[idx].stem().string();
        Notification::create(fmt::format("Imported {} inputs", s.loaded.size()), NotificationIcon::Success)->show();
    } else {
        Notification::create("Import failed: " + err, NotificationIcon::Error)->show();
    }
    Ref<CounterPopup> self = this;
    Loader::get()->queueInMainThread([self]() {
        self->refresh();
    });
}
