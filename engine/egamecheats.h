#ifndef EGAMECHEATS_H
#define EGAMECHEATS_H

#include <string>

class eGameBoard;
class eTile;

// Cheat codes.
// In game: Ctrl+C opens the input, type the code then Enter.
// Escape cancels, Backspace deletes a letter.
namespace eGameCheats {
    // Applies the code. Returns true if the code is known.
    // msg receives the text shown to the player.
    // hoverTile: tile under the mouse (may be null)
    bool apply(eGameBoard& board, const std::string& code,
               std::string& msg, eTile* const hoverTile = nullptr);

    // List of the available codes.
    std::string help();
}

#endif // EGAMECHEATS_H
