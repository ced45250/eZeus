#include "emonsteraction.h"
#include "ewaitaction.h"
#include "characters/gods/egod.h"
#include "buildings/sanctuaries/esanctuary.h"

#include "egodaction.h"
#include "emovetoaction.h"

#include "engine/egameboard.h"

#include "engine/boardData/eheatmaptask.h"
#include "buildings/eheatgetters.h"

#include "emovetoaction.h"
#include "eiteratesquare.h"

#include "egodaction.h"

#include "enumbers.h"
#include "audio/esounds.h"
#include "vec2.h"

eMonsterAction::eMonsterAction(eCharacter* const c) :
    eGodMonsterAction(c, eCharActionType::monsterAction),
    mType(eMonster::sCharacterToMonsterType(c->type())) {}

void eMonsterAction::increment(const int by) {
    const auto c = character();
    if(mType == eMonsterType::kraken) {
        // Speed set in numbers.txt
        c->setSpeed(guardian() ? eNumbers::sKrakenDefenceSpeed :
                                 eNumbers::sKrakenAttackSpeed);
    }
    const auto at = c->actionType();
    if(guardian()) {
        if(waterMonster()) {
            // Sinks enemy boats (with the troops they carry)
            huntBoats(by);
            eGodMonsterAction::increment(by);
            return;
        }
        // Attacks enemy soldiers only, never buildings
        if(at == eCharacterActionType::walk) {
            using eLFSAGA = eLookForSoldierAttackGodAct;
            const auto act = std::make_shared<eLFSAGA>(board(), c->teamId());
            const auto sound = std::make_shared<eMA_playAttackSound>(
                                   board(), c);
            lookForRangeAction(by, mLookForAttack, 1500, 5,
                               eCharacterActionType::fight2, act, sound);
        }
        eGodMonsterAction::increment(by);
        return;
    }
    if(at == eCharacterActionType::walk) {
        lookForAttack(by, mLookForAttack,
                      attackPeriod(),
                      eNumbers::sMonsterAttackRange);
    }

    eGodMonsterAction::increment(by);
}

bool eMonsterAction::decide() {
    if(guardian()) return guardianDecide();
    const auto c = character();
    switch(mStage) {
    case eMonsterAttackStage::none:
        mStage = eMonsterAttackStage::wait;
        if(!c->tile()) {
            randomPlaceOnBoard();
            if(!c->tile()) c->kill();
        } else {
            mHomeTile = c->tile();
        }
        break;
    case eMonsterAttackStage::wait: {
        mStage = eMonsterAttackStage::goTo;
        goToTarget();
        auto& board = this->board();
        board.updateMusic();
    } break;
    case eMonsterAttackStage::goTo:
        mStage = eMonsterAttackStage::patrol;
        monsterPatrol();
        break;
    case eMonsterAttackStage::patrol:
        mStage = eMonsterAttackStage::goBack;
        goBack();
        break;
    case eMonsterAttackStage::goBack: {
        mStage = eMonsterAttackStage::wait;
        if(mType == eMonsterType::scylla ||
           mType == eMonsterType::kraken) {
            moveAround(nullptr, invadePeriod(),
                       eWalkableObject::sCreateDeepWater());
        } else {
            moveAround(nullptr, invadePeriod());
        }
        auto& board = this->board();
        board.updateMusic();
    } break;
    }
    return true;
}

void eMonsterAction::read(eReadStream& src) {
    eGodMonsterAction::read(src);
    mHomeTile = src.readTile(board());
    src >> mAggressivness;
    src >> mStage;
    src >> mLookForAttack;
}

void eMonsterAction::write(eWriteStream& dst) const {
    eGodMonsterAction::write(dst);
    dst.writeTile(mHomeTile);
    dst << mAggressivness;
    dst << mStage;
    dst << mLookForAttack;
}

eTile* eMonsterAction::closestEmptySpace(const int rdx, const int rdy) const {
    const auto c = character();
    const auto cid = c->onCityId();
    auto& board = c->getBoard();
    eTile* plainTile = nullptr;
    const auto prcsTile = [&](const int i, const int j) {
        const int tx = rdx + i;
        const int ty = rdy + j;
        const auto tt = board.tile(tx, ty);
        if(!tt) return false;
        const auto ttcid = tt->cityId();
        if(ttcid != cid) return false;
        if(!plainTile && tt->walkable()) {
            plainTile = tt;
            return true;
        }
        return false;
    };

    for(int k = 0; k < 1000; k++) {
        eIterateSquare::iterateSquare(k, prcsTile, 1);
    }

    return plainTile;
}

void eMonsterAction::randomPlaceOnBoard() {
    const auto c = character();
    const auto cid = c->onCityId();
    auto& board = c->getBoard();
    const auto city = board.boardCityWithId(cid);
    const auto rtile = city->randomTile();
    const int tx = rtile->x();
    const int ty = rtile->y();
    const auto tile = closestEmptySpace(tx, ty);
    if(!tile) return;
    c->changeTile(tile);
    mHomeTile = tile;
//    const auto c = character();
//    const stdptr<eMonsterAction> tptr(this);
//    const stdptr<eCharacter> cptr(c);
//    const auto hmFinish = [tptr, this, cptr, c](eHeatMap& map) {
//        if(!tptr || !cptr) return;
//        eHeatMapDivisor divisor(map);
//        divisor.divide(10);
//        int tx;
//        int ty;
//        const bool r = divisor.maxHeatTile(tx, ty);
//        if(r) {
//            const auto& board = c->getBoard();
//            c->changeTile(board.tile(tx, ty));
//        } else {
//            setCurrentAction(nullptr);
//        }
//    };
//    const auto hg = eHeatGetters::empty;
//    const auto task = new eHeatMapTask(hg, hmFinish);
//    auto& board = c->getBoard();
//    auto& tp = board.threadPool();
//    tp.queueTask(task);
    //    wait();
}

stdsptr<eObsticleHandler> eMonsterAction::obsticleHandler() {
    return std::make_shared<eMonsterObsticleHandler>(board(), this);
}

void eMonsterAction::goToTarget() {
    if(mType == eMonsterType::scylla ||
       mType == eMonsterType::kraken) {
        const auto c = character();

        const auto underBuilding = [](eTileBase* const tile) {
            const auto type = tile->underBuildingType();
            return eBuilding::sAttackable(type);
        };

        const auto a = e::make_shared<eMoveToAction>(c);
        a->setStateRelevance(eStateRelevance::buildings |
                             eStateRelevance::terrain);
        const stdptr<eMonsterAction> tptr(this);
        a->setFoundAction([tptr, this]() {
            if(!tptr) return;
            const auto c = character();
            c->setActionType(eCharacterActionType::walk);
        });
        a->setRemoveLastTurn(true);

        a->start(underBuilding, eWalkableObject::sCreateDeepWater());
        setCurrentAction(a);
    } else {
        const stdptr<eMonsterAction> tptr(this);
        const auto tryAgain = std::make_shared<eGoToTargetTryAgain>(
                                  board(), this);
        eGodMonsterAction::goToTarget(eHeatGetters::any, tryAgain,
                                      obsticleHandler(),
                                      eWalkableHelpers::sMonsterTileDistance,
                                      eWalkableObject::sCreateAttacker(),
                                      eWalkableObject::sCreateDefault());
    }
}

void eMonsterAction::goBack() {
    if(!mHomeTile) return setCurrentAction(nullptr);
    const auto c = character();

    const auto a = e::make_shared<eMoveToAction>(c);
    a->setStateRelevance(eStateRelevance::buildings |
                         eStateRelevance::terrain);
    a->setTileDistance(eWalkableHelpers::sMonsterTileDistance);
    a->setObsticleHandler(obsticleHandler());
    a->setFindFailAction([](){});

    if(mType == eMonsterType::scylla ||
       mType == eMonsterType::kraken) {
        a->start(mHomeTile, eWalkableObject::sCreateDeepWater());
    } else {
        a->start(mHomeTile, eWalkableObject::sCreateAttacker(),
                 eWalkableObject::sCreateDefault());
    }
    setCurrentAction(a);
    c->setActionType(eCharacterActionType::walk);
}

void eMonsterAction::monsterPatrol() {
    if(mType == eMonsterType::scylla ||
       mType == eMonsterType::kraken) {
        moveAround(nullptr, moveAroundPeriod(), eWalkableObject::sCreateDeepWater());
    } else {
        goToNearestRoad();
    }
}

void eMonsterAction::destroyBuilding(eBuilding* const b) {
    const auto at = eCharacterActionType::fight2;
    const auto c = character();
    const auto chart = c->type();
    const auto finishAttackA = std::make_shared<eMA_destroyBuildingFinish>(
                                   board(), this, b);

    const auto playHitSound = std::make_shared<ePlayMonsterBuildingAttackSoundGodAct>(
                                  board(), b);
    pauseAction();
    spawnMultipleMissiles(at, chart, 500, b->centerTile(),
                          nullptr, playHitSound, finishAttackA, 3);
}

bool eMonsterAction::lookForAttack(const int dtime,
                                   int& time, const int freq,
                                   const int range) {
    const auto c = character();
    const auto act = std::make_shared<eLookForAttackGodAct>(
                         board(), c);

    const auto at = eCharacterActionType::fight2;

    return lookForRangeAction(dtime, time, freq, range,
                              at, act, nullptr);
}

bool eMonsterAction::lookForRangeAction(const int dtime,
                                        int& time, const int freq,
                                        const int range,
                                        const eCharacterActionType at,
                                        const stdsptr<eGodAct>& act,
                                        const stdsptr<eCharActFunc>& missileSound) {
    const auto c = character();
    const auto chart = c->type();
    const auto cat = c->actionType();
    const bool walking = cat == eCharacterActionType::walk;
    if(!walking) return false;
    auto& brd = c->getBoard();
    const auto ct = c->tile();
    if(!ct) return false;
    const int tx = ct->x();
    const int ty = ct->y();

    time += dtime;
    if(time > freq) {
        time -= freq;
        std::vector<eTile*> tiles;
        const int rr = 2*range + 1;
        tiles.reserve(rr*rr);
        for(int i = -range; i <= range; i++) {
            for(int j = -range; j <= range; j++) {
                const int ttx = tx + i;
                const int tty = ty + j;
                const auto t = brd.tile(ttx, tty);
                if(!t) continue;
                tiles.push_back(t);
            }
        }
        eRand::randomShuffle(tiles);
        for(const auto t : tiles) {
            const auto tt = act->find(t);
            if(!tt) continue;

            const auto finishAttackA = std::make_shared<eMA_lookForRangeActionFinishAttack>(
                                           board(), this);

            pauseAction();
            const int attackTime = eMonster::sMonsterAttackTime(mType);
            spawnMissile(at, chart, attackTime, tt,
                         missileSound, act, finishAttackA);
            return true;
        }
    }
    return false;
}

int eMonsterAction::attackPeriod() const {
    switch(mAggressivness) {
    case eMonsterAggressivness::passive:
        return eNumbers::sPassiveMonsterAttackPeriod;
    case eMonsterAggressivness::active:
        return eNumbers::sActiveMonsterAttackPeriod;
    case eMonsterAggressivness::veryActive:
        return eNumbers::sVeryActiveMonsterAttackPeriod;
    case eMonsterAggressivness::aggressive:
        return eNumbers::sAggressiveMonsterAttackPeriod;
    }
    return eNumbers::sPassiveMonsterAttackPeriod;
}

int eMonsterAction::invadePeriod() const {
    if(mType == eMonsterType::scylla ||
       mType == eMonsterType::kraken) {
        switch(mAggressivness) {
        case eMonsterAggressivness::passive:
            return eNumbers::sPassiveWaterMonsterInvadePeriod;
        case eMonsterAggressivness::active:
            return eNumbers::sActiveWaterMonsterInvadePeriod;
        case eMonsterAggressivness::veryActive:
            return eNumbers::sVeryActiveWaterMonsterInvadePeriod;
        case eMonsterAggressivness::aggressive:
            return eNumbers::sAggressiveWaterMonsterInvadePeriod;
        }
        return eNumbers::sPassiveWaterMonsterInvadePeriod;
    } else {
        switch(mAggressivness) {
        case eMonsterAggressivness::passive:
            return eNumbers::sPassiveLandMonsterInvadePeriod;
        case eMonsterAggressivness::active:
            return eNumbers::sActiveLandMonsterInvadePeriod;
        case eMonsterAggressivness::veryActive:
            return eNumbers::sVeryActiveLandMonsterInvadePeriod;
        case eMonsterAggressivness::aggressive:
            return eNumbers::sAggressiveLandMonsterInvadePeriod;
        }
        return eNumbers::sPassiveLandMonsterInvadePeriod;
    }
}

int eMonsterAction::moveAroundPeriod() const {
    if(mType == eMonsterType::scylla ||
        mType == eMonsterType::kraken) {
        switch(mAggressivness) {
        case eMonsterAggressivness::passive:
            return eNumbers::sPassiveWaterMonsterMoveAroundPeriod;
        case eMonsterAggressivness::active:
            return eNumbers::sActiveWaterMonsterMoveAroundPeriod;
        case eMonsterAggressivness::veryActive:
            return eNumbers::sVeryActiveWaterMonsterMoveAroundPeriod;
        case eMonsterAggressivness::aggressive:
            return eNumbers::sAggressiveWaterMonsterMoveAroundPeriod;
        }
        return eNumbers::sPassiveWaterMonsterMoveAroundPeriod;
    } else {
        switch(mAggressivness) {
        case eMonsterAggressivness::passive:
            return eNumbers::sPassiveLandMonsterMoveAroundPeriod;
        case eMonsterAggressivness::active:
            return eNumbers::sActiveLandMonsterMoveAroundPeriod;
        case eMonsterAggressivness::veryActive:
            return eNumbers::sVeryActiveLandMonsterMoveAroundPeriod;
        case eMonsterAggressivness::aggressive:
            return eNumbers::sAggressiveLandMonsterMoveAroundPeriod;
        }
        return eNumbers::sPassiveLandMonsterMoveAroundPeriod;
    }
}

bool eMonsterAction::guardian() const {
    const auto c = character();
    const auto pid = c->getBoard().cityIdToPlayerId(c->cityId());
    return static_cast<int>(pid) >= 0;
}

eCharacter* eMonsterAction::nearestEnemySoldier(const int maxDist) const {
    const auto c = character();
    const auto ct = c->tile();
    if(!ct) return nullptr;
    const auto team = c->teamId();
    eCharacter* result = nullptr;
    int best = maxDist + 1;
    for(const auto cc : c->getBoard().characters()) {
        if(!cc || cc->dead() || !cc->isSoldier()) continue;
        if(!eTeamIdHelpers::isEnemy(cc->teamId(), team)) continue;
        const auto t = cc->tile();
        if(!t) continue;
        const int d = std::max(std::abs(t->x() - ct->x()),
                               std::abs(t->y() - ct->y()));
        if(d < best) {
            best = d;
            result = cc;
        }
    }
    return result;
}

bool eMonsterAction::waterMonster() const {
    return mType == eMonsterType::scylla ||
           mType == eMonsterType::kraken;
}

eCharacter* eMonsterAction::nearestEnemyBoat(const int maxDist,
                                             int& dist) const {
    const auto c = character();
    const auto ct = c->tile();
    if(!ct) return nullptr;
    const auto team = c->teamId();
    eCharacter* result = nullptr;
    dist = maxDist + 1;
    for(const auto cc : c->getBoard().characters()) {
        if(!cc || cc->dead()) continue;
        if(cc->type() != eCharacterType::enemyBoat) continue;
        if(!eTeamIdHelpers::isEnemy(cc->teamId(), team)) continue;
        const auto t = cc->tile();
        if(!t) continue;
        const int d = std::max(std::abs(t->x() - ct->x()),
                               std::abs(t->y() - ct->y()));
        if(d < dist) {
            dist = d;
            result = cc;
        }
    }
    return result;
}

void eMonsterAction::huntBoats(const int by) {
    const auto c = character();
    if(mSinkAnim > 0) {
        mSinkAnim -= by;
        if(mSinkAnim <= 0) c->setActionType(eCharacterActionType::walk);
    }
    const int period = 250;
    mLookForBoat += by;
    if(mLookForBoat < period) return;
    mLookForBoat -= period;
    mBoatRetarget -= period;
    int dist;
    const auto boat = nearestEnemyBoat(40, dist);
    if(!boat) return;
    if(dist <= 2) {
        if(const auto bt = boat->tile()) {
            const auto ct = c->tile();
            const vec2d line{double(bt->x() - ct->x()),
                             double(bt->y() - ct->y())};
            c->setOrientation(sAngleOrientation(line.angle()));
        }
        boat->kill();
        c->setActionType(eCharacterActionType::fight);
        board().ifVisible(c->tile(), [c]() {
            eSounds::playAttackSound(c);
        });
        mSinkAnim = 1500;
        return;
    }
    if(mBoatRetarget > 0) return;
    mBoatRetarget = 2000;
    const auto a = e::make_shared<eMoveToAction>(c);
    a->setStateRelevance(eStateRelevance::terrain);
    a->setFindFailAction([](){});
    a->setRemoveLastTurn(true);
    a->start(boat->tile(), eWalkableObject::sCreateDeepWater());
    setCurrentAction(a);
    c->setActionType(eCharacterActionType::walk);
    mStage = eMonsterAttackStage::goTo;
}

bool eMonsterAction::dragonDecide() {
    const auto c = character();
    auto& board = c->getBoard();
    const auto s = board.sanctuary(c->cityId(), eGodType::ares);
    const auto ares = s ? s->god() : nullptr;
    if(!ares || ares->dead()) {
        // Leaves with Ares
        c->kill();
        return true;
    }
    const auto ct = c->tile();
    const auto at = ares->tile();
    const auto dist = [](eTile* const a, eTile* const b) {
        return std::max(std::abs(a->x() - b->x()),
                        std::abs(a->y() - b->y()));
    };
    // Enemy troops close to the dragon or to Ares
    eCharacter* enemy = nullptr;
    {
        const auto team = c->teamId();
        int best = 11;
        for(const auto cc : board.characters()) {
            if(!cc || cc->dead() || !cc->isSoldier()) continue;
            if(!eTeamIdHelpers::isEnemy(cc->teamId(), team)) continue;
            const auto t = cc->tile();
            if(!t) continue;
            int d = dist(t, ct);
            if(at) d = std::min(d, dist(t, at));
            if(d < best) {
                best = d;
                enemy = cc;
            }
        }
    }
    eTile* target = nullptr;
    if(enemy) {
        target = enemy->tile();
        mStage = eMonsterAttackStage::goTo;
    } else if(at && ares->visible() && dist(ct, at) > 2) {
        target = at;
        mStage = eMonsterAttackStage::patrol;
    }
    if(target) {
        const auto a = e::make_shared<eMoveToAction>(c);
        a->setStateRelevance(eStateRelevance::buildings |
                             eStateRelevance::terrain);
        const stdptr<eCharacter> cptr(c);
        const stdptr<eCharacter> aptr(ares);
        const bool follow = !enemy;
        a->setFindFailAction([cptr, aptr, follow]() {
            // Ares teleported somewhere unreachable: the dragon follows
            if(!follow || !cptr || !aptr || !aptr->tile()) return;
            cptr->changeTile(aptr->tile());
        });
        a->setRemoveLastTurn(true);
        a->start(target, eWalkableObject::sCreateDefault());
        setCurrentAction(a);
        c->setActionType(eCharacterActionType::walk);
        return true;
    }
    // Next to Ares: wait a little, then look again
    mStage = eMonsterAttackStage::wait;
    if(at) {
        const vec2d line{double(at->x() - ct->x()),
                         double(at->y() - ct->y())};
        if(line.length() > 0) c->setOrientation(sAngleOrientation(line.angle()));
    }
    c->setActionType(eCharacterActionType::stand);
    const auto w = e::make_shared<eWaitAction>(c);
    w->setTime(500);
    setCurrentAction(w);
    return true;
}

bool eMonsterAction::guardianDecide() {
    const auto c = character();
    if(!c->tile()) {
        c->kill();
        return true;
    }
    if(!mHomeTile) mHomeTile = c->tile();
    if(mType == eMonsterType::dragon) return dragonDecide();
    if(waterMonster()) {
        // Patrols the city's deep water
        mStage = eMonsterAttackStage::patrol;
        moveAround(nullptr, moveAroundPeriod(),
                   eWalkableObject::sCreateDeepWater());
        return true;
    }
    // Runs to enemy soldiers in the city, otherwise patrols the roads
    if(const auto s = nearestEnemySoldier(60)) {
        const auto a = e::make_shared<eMoveToAction>(c);
        a->setStateRelevance(eStateRelevance::buildings |
                             eStateRelevance::terrain);
        a->setFindFailAction([](){});
        a->setRemoveLastTurn(true);
        a->start(s->tile(), eWalkableObject::sCreateDefault());
        setCurrentAction(a);
        c->setActionType(eCharacterActionType::walk);
        mStage = eMonsterAttackStage::goTo;
        return true;
    }
    if(mStage == eMonsterAttackStage::patrol) {
        mStage = eMonsterAttackStage::wait;
        moveAround(nullptr, moveAroundPeriod());
    } else {
        mStage = eMonsterAttackStage::patrol;
        goToNearestRoad();
    }
    return true;
}

void eMA_playAttackSound::call() {
    if(!mCptr) return;
    const auto c = mCptr.get();
    board().ifVisible(c->tile(), [c]() {
        eSounds::playAttackSound(c);
    });
}
