#pragma once
#include "CounterState.hpp"
#include <filesystem>
#include <vector>

class CounterPopup : public geode::Popup {
public:
    static CounterPopup* create(PauseLayer* pause);

protected:
    bool init(PauseLayer* pause);

    Ref<PauseLayer> m_pause;
    CCLabelBMFont* m_status = nullptr;
    ButtonSprite* m_recSpr = nullptr;
    std::vector<CCNode*> m_fileNodes;
    std::vector<std::filesystem::path> m_files;
    int m_page = 0;

    CCMenuItemSpriteExtra* addButton(char const* text, SEL_MenuHandler cb, CCPoint offset, int width, int tag = 0);
    void refresh();

    void onRecord(CCObject*);
    void onCount(CCObject*);
    void onExport(CCObject*);
    void onPrev(CCObject*);
    void onNext(CCObject*);
    void onPick(CCObject*);
};
