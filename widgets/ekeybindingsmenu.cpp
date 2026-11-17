#include "ekeybindingsmenu.h"

#include "eframedbutton.h"
#include "elabel.h"

#include <algorithm>

namespace {
const std::string gPressKey = "Press a key...";
const std::string gNone = "-";
const std::string gHintIdle =
        "Click a key to change it. Backspace clears it.";
const std::string gHintCapture =
        "Press the new key. Escape cancels, Backspace clears.";

std::string keyText(const SDL_Keycode k) {
    const auto n = eKeyBindings::keyName(k);
    return n.empty() ? gNone : n;
}
}

eKeyBindingsMenu::~eKeyBindingsMenu() {
    releaseKeyboard();
}

void eKeyBindingsMenu::initialize(const eAction& closeAct) {
    setType(eFrameType::message);
    setLargePadding();
    const int p = padding();
    const int sp = resolution().smallPadding();

    const auto content = new eWidget(window());
    addWidget(content);
    content->move(p, p);

    const auto title = new eLabel("Keyboard shortcuts", window());
    title->fitContent();
    content->addWidget(title);

    // Rows are laid out once all name labels exist (same column width)
    std::vector<std::pair<eWidget*, eLabel*>> rows;
    const std::vector<std::string> keyOptions{
        gPressKey, "Keypad -", "Backspace", "Right Shift"};

    const auto makeKeyButton = [&](const eAction& press) {
        const auto b = new eFramedButton(window());
        b->setUnderline(false);
        b->setSmallFontSize();
        b->setSmallPadding();
        b->fitOptions(keyOptions);
        b->fitHeight();
        b->setPressAction(press);
        return b;
    };

    for(int i = 0; i < static_cast<int>(eKeyAction::count); i++) {
        const auto a = static_cast<eKeyAction>(i);
        const auto row = new eWidget(window());

        const auto name = new eLabel(eKeyBindings::actionLabel(a), window());
        name->setSmallFontSize();
        name->fitContent();
        row->addWidget(name);

        eRow r;
        r.fAction = a;
        r.fKey = makeKeyButton([this, a]() { startCapture(a, false); });
        r.fAlt = makeKeyButton([this, a]() { startCapture(a, true); });
        row->addWidget(r.fKey);
        row->addWidget(r.fAlt);
        content->addWidget(row);
        rows.push_back({row, name});
        mRows.push_back(r);
    }

    {
        const auto row = new eWidget(window());
        const auto name = new eLabel("Scroll speed", window());
        name->setSmallFontSize();
        name->fitContent();
        row->addWidget(name);

        const auto minus = new eFramedButton("-", window());
        minus->setUnderline(false);
        minus->setSmallPadding();
        minus->fitContent();
        minus->setPressAction([this]() { changeSpeed(-100); });
        row->addWidget(minus);

        mSpeedLabel = new eLabel(window());
        mSpeedLabel->setSmallFontSize();
        mSpeedLabel->fitOptions({"5000 px/s"});
        mSpeedLabel->setTextAlignment(eAlignment::center);
        row->addWidget(mSpeedLabel);

        const auto plus = new eFramedButton("+", window());
        plus->setUnderline(false);
        plus->setSmallPadding();
        plus->fitContent();
        plus->setPressAction([this]() { changeSpeed(100); });
        row->addWidget(plus);
        content->addWidget(row);
        rows.push_back({row, name});
    }

    int nameW = 0;
    for(const auto& r : rows) nameW = std::max(nameW, r.second->width());
    for(const auto& r : rows) {
        r.second->setWidth(nameW);
        r.first->stackHorizontally(sp);
        r.first->fitContent();
        for(const auto c : r.first->children()) {
            c->setY((r.first->height() - c->height())/2);
        }
    }

    mHint = new eLabel(window());
    mHint->setSmallFontSize();
    mHint->fitOptions({gHintIdle, gHintCapture});
    mHint->setText(gHintIdle);
    content->addWidget(mHint);

    {
        const auto row = new eWidget(window());
        const auto defaults = new eFramedButton("Restore defaults", window());
        defaults->setUnderline(false);
        defaults->fitContent();
        defaults->setPressAction([this]() {
            stopCapture();
            eKeyBindings::resetDefaults();
            eKeyBindings::save();
            updateTexts();
        });
        row->addWidget(defaults);

        const auto back = new eFramedButton("Back", window());
        back->setUnderline(false);
        back->fitContent();
        back->setPressAction([this, closeAct]() {
            stopCapture();
            eKeyBindings::save();
            if(closeAct) closeAct();
        });
        row->addWidget(back);
        row->stackHorizontally(2*sp);
        row->fitContent();
        content->addWidget(row);
    }

    content->stackVertically(sp);
    content->fitContent();
    for(const auto c : content->children()) {
        c->align(eAlignment::hcenter);
    }
    resize(content->width() + 2*p, content->height() + 2*p);

    updateTexts();
}

void eKeyBindingsMenu::startCapture(const eKeyAction action, const bool alt) {
    mCapturing = true;
    mCapAction = action;
    mCapAlt = alt;
    grabKeyboard();
    updateTexts();
}

void eKeyBindingsMenu::stopCapture() {
    mCapturing = false;
    releaseKeyboard();
    updateTexts();
}

void eKeyBindingsMenu::updateTexts() {
    for(const auto& r : mRows) {
        const bool capKey = mCapturing && r.fAction == mCapAction && !mCapAlt;
        const bool capAlt = mCapturing && r.fAction == mCapAction && mCapAlt;
        r.fKey->setText(capKey ? gPressKey :
                        keyText(eKeyBindings::key(r.fAction, false)));
        r.fAlt->setText(capAlt ? gPressKey :
                        keyText(eKeyBindings::key(r.fAction, true)));
    }
    if(mSpeedLabel) {
        mSpeedLabel->setText(std::to_string(eKeyBindings::scrollSpeed()) +
                             " px/s");
    }
    if(mHint) mHint->setText(mCapturing ? gHintCapture : gHintIdle);
}

void eKeyBindingsMenu::changeSpeed(const int by) {
    eKeyBindings::setScrollSpeed(eKeyBindings::scrollSpeed() + by);
    eKeyBindings::save();
    updateTexts();
}

bool eKeyBindingsMenu::keyPressEvent(const eKeyPressEvent& e) {
    if(!mCapturing) return false;
    const auto k = e.key();
    switch(k) {
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
    case SDL_SCANCODE_LGUI:
    case SDL_SCANCODE_RGUI:
        return true;
    case SDL_SCANCODE_ESCAPE:
        stopCapture();
        return true;
    case SDL_SCANCODE_BACKSPACE:
    case SDL_SCANCODE_DELETE:
        eKeyBindings::setKey(mCapAction, mCapAlt, SDLK_UNKNOWN);
        break;
    default: {
        const auto kc = SDL_GetKeyFromScancode(k);
        if(kc == SDLK_UNKNOWN) return true;
        eKeyBindings::setKey(mCapAction, mCapAlt, kc);
    } break;
    }
    eKeyBindings::save();
    stopCapture();
    return true;
}
