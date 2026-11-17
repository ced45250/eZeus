#ifndef EKEYBINDINGS_H
#define EKEYBINDINGS_H

#include <string>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>

// Configurable keyboard shortcuts.
// Read from eZeus/keys.txt (created with default values if missing).
// Each action accepts two keys: "action" and "action_alt".
enum class eKeyAction {
    scrollUp,
    scrollDown,
    scrollLeft,
    scrollRight,
    speedUp,
    speedDown,
    rotate,
    pause,
    bookmark1,
    bookmark2,
    bookmark3,
    bookmark4,
    rotateMapLeft,
    rotateMapRight,

    count
};

namespace eKeyBindings {
    // Reads keys.txt, then writes it back with any missing entries
    void initialize();
    // Does the pressed key trigger the action?
    bool matches(const eKeyAction action, const SDL_Scancode pressed);
    // Is one of the action's keys held down?
    bool held(const eKeyAction action);
    // Keyboard map scrolling speed, in pixels per second
    int scrollSpeed();

    // Used by the in-game options menu
    std::string actionLabel(const eKeyAction action);
    SDL_Keycode key(const eKeyAction action, const bool alt);
    // Assigns a key; the same key is removed from any other action
    void setKey(const eKeyAction action, const bool alt,
                const SDL_Keycode key);
    void setScrollSpeed(const int speed);
    int minScrollSpeed();
    int maxScrollSpeed();
    void resetDefaults();
    std::string keyName(const SDL_Keycode k);
    // Writes keys.txt
    void save();
    std::string path();
}

#endif // EKEYBINDINGS_H
