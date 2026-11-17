#include "egamecheats.h"

#include <algorithm>
#include <cctype>

#include "engine/egameboard.h"
#include "engine/eplague.h"
#include "engine/eevent.h"
#include "engine/eeventdata.h"
#include "engine/eprovide.h"
#include "engine/boardData/ecityfinances.h"
#include "buildings/ebuilding.h"
#include "buildings/estoragebuilding.h"
#include "characters/monsters/emonster.h"
#include "engine/etile.h"

namespace {

std::string normalize(const std::string& code) {
    std::string result;
    for(const char c : code) {
        if(std::isspace(static_cast<unsigned char>(c))) continue;
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

// +10 000 drachmas (a nod to the "Delian Treasury" of the original game)
std::string cheatMoney(eGameBoard& board) {
    const int amount = 10000;
    const auto pid = board.personPlayer();
    board.incDrachmas(pid, amount, eFinanceTarget::giftsReceived);
    return "Triche : +" + std::to_string(amount) + " drachmes";
}

// Fills granaries and storehouses with what they accept
std::string cheatFillStorage(eGameBoard& board) {
    int total = 0;
    const auto cids = board.personPlayerCitiesOnBoard();
    for(const auto cid : cids) {
        const auto bs = board.buildings(cid, [](eBuilding* const b) {
            return dynamic_cast<eStorageBuilding*>(b) != nullptr;
        });
        for(const auto b : bs) {
            const auto s = static_cast<eStorageBuilding*>(b);
            const auto types = eResourceTypeHelpers::extractResourceTypes(
                                   s->canAccept());
            // Fair share: one unit of each type in turn
            bool added = true;
            while(added) {
                added = false;
                for(const auto t : types) {
                    if(s->add(t, 1) > 0) {
                        added = true;
                        total++;
                    }
                }
            }
        }
        board.updateResources(cid);
    }
    if(total == 0) {
        return "Triche : aucun grenier ou entrepot a remplir";
    }
    return "Triche : greniers et entrepots remplis (+" +
           std::to_string(total) + ")";
}

// Puts out fires and fully maintains the buildings
std::string cheatFirefighters(eGameBoard& board) {
    int fires = 0;
    const auto cids = board.personPlayerCitiesOnBoard();
    for(const auto cid : cids) {
        const auto bs = board.buildings(cid, [](eBuilding* const) {
            return true;
        });
        for(const auto b : bs) {
            if(b->isOnFire()) {
                b->setOnFire(false);
                fires++;
            }
            b->eBuilding::provide(eProvide::maintanance, 100);
        }
    }
    return "Triche : " + std::to_string(fires) +
           " incendie(s) eteint(s), batiments entretenus";
}

// Slays every monster in the player's cities
std::string cheatSlayMonsters(eGameBoard& board) {
    int slain = 0;
    const auto cids = board.personPlayerCitiesOnBoard();
    for(const auto cid : cids) {
        const auto ms = board.monsters(cid);
        for(const auto m : ms) {
            if(!m || m->dead()) continue;
            bool valid = false;
            const auto mt = eMonster::sCharacterToMonsterType(
                                m->type(), &valid);
            if(!valid) continue;
            const auto mcid = m->onCityId();
            eEventData ed(mcid);
            ed.fMonster = mt;
            ed.fTile = m->tile();
            board.event(eEvent::monsterSlain, ed);
            m->killWithCorpse();
            board.addSlayedMonster(mcid, mt);
            slain++;
        }
    }
    if(slain == 0) return "Triche : aucun monstre dans la cite";
    return "Triche : " + std::to_string(slain) + " monstre(s) terrasse(s)";
}

// Heals every plague
std::string cheatHealPlagues(eGameBoard& board) {
    int healed = 0;
    const auto cids = board.personPlayerCitiesOnBoard();
    for(const auto cid : cids) {
        const auto ps = board.plagues(cid);
        for(const auto& p : ps) {
            board.healPlague(p);
            healed++;
        }
    }
    if(healed == 0) return "Triche : aucune epidemie en cours";
    return "Triche : " + std::to_string(healed) + " epidemie(s) guerie(s)";
}

// Earthquake under the mouse
std::string cheatEarthquake(eGameBoard& board, eTile* const tile) {
    if(!tile) return "Triche : placez la souris sur la carte";
    board.earthquake(tile, 60);
    return "Triche : seisme declenche";
}

// Tidal wave: starts from the mouse (or a disaster point), then the
// game looks for the nearest tidal wave zone
std::string cheatTidalWave(eGameBoard& board, eTile* const tile) {
    eTile* start = tile;
    if(!start) {
        const auto cids = board.personPlayerCitiesOnBoard();
        for(const auto cid : cids) {
            for(int id = 1; id <= 8 && !start; id++) {
                start = board.disasterTile(cid, id);
            }
            if(start) break;
        }
    }
    if(!start) return "Triche : placez la souris sur la carte";
    const bool was = board.duringTidalWave();
    board.addTidalWave(start, false);
    if(!was && !board.duringTidalWave()) {
        return "Triche : aucune zone de raz-de-maree a proximite";
    }
    return "Triche : raz-de-maree declenche";
}

// Wins the current episode at once
std::string cheatVictory(eGameBoard& board) {
    board.cheatCompleteEpisode();
    return "Triche : episode remporte !";
}

}

std::string eGameCheats::help() {
    return "Codes : tresor, abondance, pompier, hercule, panacee, victoire, seisme, raz";
}

bool eGameCheats::apply(eGameBoard& board, const std::string& code,
                        std::string& msg, eTile* const hoverTile) {
    const auto c = normalize(code);
    if(c == "tresor" || c == "deliantreasury" || c == "delian") {
        msg = cheatMoney(board);
    } else if(c == "abondance") {
        msg = cheatFillStorage(board);
    } else if(c == "pompier") {
        msg = cheatFirefighters(board);
    } else if(c == "hercule") {
        msg = cheatSlayMonsters(board);
    } else if(c == "panacee") {
        msg = cheatHealPlagues(board);
    } else if(c == "seisme") {
        msg = cheatEarthquake(board, hoverTile);
    } else if(c == "raz") {
        msg = cheatTidalWave(board, hoverTile);
    } else if(c == "victoire") {
        msg = cheatVictory(board);
    } else if(c == "aide") {
        msg = help();
    } else {
        msg = "Code inconnu. Tapez 'aide' pour la liste.";
        return false;
    }
    return true;
}
