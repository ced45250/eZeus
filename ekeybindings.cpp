#include "ekeybindings.h"

#include <SDL2/SDL_keyboard.h>

#include <algorithm>
#include <fstream>
#include <map>

#include "egamedir.h"
#include "eloadtexthelper.h"

namespace {

struct eBinding {
    const char* fName;
    SDL_Keycode fKey;
    SDL_Keycode fAlt;
};

const int gCount = static_cast<int>(eKeyAction::count);
eBinding gBindings[gCount];
bool gInitialized = false;

const int gDefaultScrollSpeed = 700;
int gScrollSpeed = gDefaultScrollSpeed;

SDL_Keycode fromScancode(const SDL_Scancode s) {
    return SDL_GetKeyFromScancode(s);
}

void setDefaults() {
    const auto set = [](const eKeyAction a, const char* const name,
                        const SDL_Keycode key, const SDL_Keycode alt) {
        gBindings[static_cast<int>(a)] = {name, key, alt};
    };
    // Map scrolling: arrows + WASD (QWERTY layout; on AZERTY use ZQSD,
    // from the Controls menu or keys.txt)
    set(eKeyAction::scrollUp, "scroll_up", SDLK_UP, SDLK_w);
    set(eKeyAction::scrollDown, "scroll_down", SDLK_DOWN, SDLK_s);
    set(eKeyAction::scrollLeft, "scroll_left", SDLK_LEFT, SDLK_a);
    set(eKeyAction::scrollRight, "scroll_right", SDLK_RIGHT, SDLK_d);
    // Same physical keys as in upstream eZeus
    set(eKeyAction::speedUp, "speed_up", SDLK_KP_PLUS,
        fromScancode(SDL_SCANCODE_RIGHTBRACKET));
    set(eKeyAction::speedDown, "speed_down", SDLK_KP_MINUS,
        fromScancode(SDL_SCANCODE_LEFTBRACKET));
    set(eKeyAction::rotate, "rotate", fromScancode(SDL_SCANCODE_R),
        SDLK_UNKNOWN);
    set(eKeyAction::pause, "pause", fromScancode(SDL_SCANCODE_P),
        SDLK_UNKNOWN);
    set(eKeyAction::bookmark1, "bookmark_1", SDLK_F1, SDLK_UNKNOWN);
    set(eKeyAction::bookmark2, "bookmark_2", SDLK_F2, SDLK_UNKNOWN);
    set(eKeyAction::bookmark3, "bookmark_3", SDLK_F3, SDLK_UNKNOWN);
    set(eKeyAction::bookmark4, "bookmark_4", SDLK_F4, SDLK_UNKNOWN);
    set(eKeyAction::rotateMapLeft, "rotate_map_left", SDLK_HOME, SDLK_UNKNOWN);
    set(eKeyAction::rotateMapRight, "rotate_map_right", SDLK_END, SDLK_UNKNOWN);
}


SDL_Keycode keyFromName(const std::string& name) {
    if(name.empty()) return SDLK_UNKNOWN;
    return SDL_GetKeyFromName(name.c_str());
}

void write() {
    std::ofstream file(eKeyBindings::path());
    if(!file.good()) return;
    file << "; eZeus keyboard shortcuts\n";
    file << "; Format: action \"Key\". Each action has a second key: action_alt\n";
    file << "; Key names: letters (A, Z...), digits, Up, Down, Left, Right,\n";
    file << "; Space, Tab, F1..F12, Keypad +, Keypad -, Keypad 0..9, etc.\n";
    file << "; Use \"\" to leave a key unassigned. Delete this file to restore defaults.\n";
    file << "; scroll_speed: keyboard map scrolling speed in pixels per second\n";
    file << "; (100 to 5000, default " << gDefaultScrollSpeed << ")\n";
    file << "scroll_speed \"" << gScrollSpeed << "\"\n";
    for(const auto& b : gBindings) {
        file << b.fName << " \"" << eKeyBindings::keyName(b.fKey) << "\"\n";
        file << b.fName << "_alt \"" << eKeyBindings::keyName(b.fAlt) << "\"\n";
    }
}

}

std::string eKeyBindings::path() {
    return eGameDir::exeDir() + "../keys.txt";
}

void eKeyBindings::initialize() {
    gInitialized = true;
    setDefaults();
    {
        std::ifstream test(path());
        if(!test.good()) {
            write();
            return;
        }
    }
    std::map<std::string, std::string> map;
    if(!eLoadTextHelper::load(path(), map)) return;
    for(auto& b : gBindings) {
        const std::string name = b.fName;
        const auto it = map.find(name);
        if(it != map.end()) {
            const auto k = keyFromName(it->second);
            if(k == SDLK_UNKNOWN && !it->second.empty()) {
                printf("keys.txt: unknown key \"%s\" for %s\n",
                       it->second.c_str(), name.c_str());
            } else {
                b.fKey = k;
            }
        }
        const auto ita = map.find(name + "_alt");
        if(ita != map.end()) {
            const auto k = keyFromName(ita->second);
            if(k == SDLK_UNKNOWN && !ita->second.empty()) {
                printf("keys.txt: unknown key \"%s\" for %s_alt\n",
                       ita->second.c_str(), name.c_str());
            } else {
                b.fAlt = k;
            }
        }
    }
    const auto its = map.find("scroll_speed");
    if(its != map.end()) {
        try {
            const int v = std::stoi(its->second);
            gScrollSpeed = std::clamp(v, minScrollSpeed(), maxScrollSpeed());
        } catch(...) {
            printf("keys.txt: invalid scroll_speed \"%s\"\n",
                   its->second.c_str());
        }
    }
    write();
}

int eKeyBindings::scrollSpeed() {
    if(!gInitialized) initialize();
    return gScrollSpeed;
}

bool eKeyBindings::matches(const eKeyAction action,
                           const SDL_Scancode pressed) {
    if(!gInitialized) initialize();
    const auto k = SDL_GetKeyFromScancode(pressed);
    if(k == SDLK_UNKNOWN) return false;
    const auto& b = gBindings[static_cast<int>(action)];
    return k == b.fKey || k == b.fAlt;
}

bool eKeyBindings::held(const eKeyAction action) {
    if(!gInitialized) initialize();
    const auto state = SDL_GetKeyboardState(nullptr);
    const auto& b = gBindings[static_cast<int>(action)];
    for(const auto k : {b.fKey, b.fAlt}) {
        if(k == SDLK_UNKNOWN) continue;
        const auto s = SDL_GetScancodeFromKey(k);
        if(s != SDL_SCANCODE_UNKNOWN && state[s]) return true;
    }
    return false;
}

std::string eKeyBindings::keyName(const SDL_Keycode k) {
    if(k == SDLK_UNKNOWN) return "";
    return SDL_GetKeyName(k);
}

std::string eKeyBindings::actionLabel(const eKeyAction action) {
    switch(action) {
    case eKeyAction::scrollUp: return "Scroll up";
    case eKeyAction::scrollDown: return "Scroll down";
    case eKeyAction::scrollLeft: return "Scroll left";
    case eKeyAction::scrollRight: return "Scroll right";
    case eKeyAction::speedUp: return "Speed up";
    case eKeyAction::speedDown: return "Slow down";
    case eKeyAction::rotate: return "Rotate building";
    case eKeyAction::pause: return "Pause";
    case eKeyAction::bookmark1: return "Bookmark 1";
    case eKeyAction::bookmark2: return "Bookmark 2";
    case eKeyAction::bookmark3: return "Bookmark 3";
    case eKeyAction::bookmark4: return "Bookmark 4";
    case eKeyAction::rotateMapLeft: return "Rotate map left";
    case eKeyAction::rotateMapRight: return "Rotate map right";
    case eKeyAction::count: break;
    }
    return "";
}

SDL_Keycode eKeyBindings::key(const eKeyAction action, const bool alt) {
    if(!gInitialized) initialize();
    const auto& b = gBindings[static_cast<int>(action)];
    return alt ? b.fAlt : b.fKey;
}

void eKeyBindings::setKey(const eKeyAction action, const bool alt,
                          const SDL_Keycode key) {
    if(!gInitialized) initialize();
    if(key != SDLK_UNKNOWN) {
        for(auto& b : gBindings) {
            if(b.fKey == key) b.fKey = SDLK_UNKNOWN;
            if(b.fAlt == key) b.fAlt = SDLK_UNKNOWN;
        }
    }
    auto& b = gBindings[static_cast<int>(action)];
    if(alt) b.fAlt = key;
    else b.fKey = key;
}

int eKeyBindings::minScrollSpeed() { return 100; }
int eKeyBindings::maxScrollSpeed() { return 5000; }

void eKeyBindings::setScrollSpeed(const int speed) {
    gScrollSpeed = std::clamp(speed, minScrollSpeed(), maxScrollSpeed());
}

void eKeyBindings::resetDefaults() {
    setDefaults();
    gScrollSpeed = gDefaultScrollSpeed;
}

void eKeyBindings::save() {
    write();
}
