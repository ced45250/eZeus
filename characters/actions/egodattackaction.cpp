#include "egodattackaction.h"
#include "characters/actions/ekillcharacterfinishfail.h"
#include "characters/actions/emovetoaction.h"
#include "fileIO/efileformat.h"
#include "characters/actions/esoldieraction.h"
#include "characters/eareswarrior.h"
#include "characters/eamazon.h"

#include "buildings/ebuilding.h"
#include "engine/egameboard.h"
#include "edionysusfollowaction.h"
#include "characters/monsters/ecalydonianboar.h"
#include "characters/actions/ewaitaction.h"
#include "buildings/epalace.h"
#include "enumbers.h"

eGodAttackAction::eGodAttackAction(eCharacter* const c) :
    eGodAction(c, eCharActionType::godAttackAction) {}

eGodAttackAction::~eGodAttackAction() {
    if(mSanctuary) mSanctuary->godComeback();
}

void eGodAttackAction::increment(const int by) {
    const auto c = character();
    const auto type = this->type();
    const auto at = c->actionType();
    if(type == eGodType::ares || type == eGodType::artemis) {
        if(!mArmySpawned && c->tile() &&
           at == eCharacterActionType::walk) {
            spawnAresArmy();
        }
        if(!mArmy.empty()) {
            if(at == eCharacterActionType::die) {
                // The god is beaten: the troops flee the city on foot
                fleeArmy();
            } else if(at == eCharacterActionType::disappear) {
                // The god ends the invasion: the troops leave with it
                for(const auto& b : mArmy) b->killAll();
                mArmy.clear();
            }
        }
        mArmyFollow += by;
        if(mArmyFollow > 4000) {
            mArmyFollow = 0;
            moveAresArmy();
        }
    }
    if(at == eCharacterActionType::walk) {
        const int cursePeriod = eNumbers::sGodAttackCursePeriod;
        const int curseRange = eNumbers::sGodAttackCurseRange;
        const int lookForGodCheck = 1000;
        int attackPeriod;
        switch(type) {
        case eGodType::ares:
        case eGodType::artemis:
        case eGodType::athena:
        case eGodType::zeus:
            attackPeriod = eNumbers::sGodAttackAggressiveAttackPeriod;
            break;
        case eGodType::aphrodite:
        case eGodType::apollo:
        case eGodType::atlas:
        case eGodType::demeter:
        case eGodType::dionysus:
        case eGodType::hades:
        case eGodType::hephaestus:
        case eGodType::hera:
        case eGodType::hermes:
        case eGodType::poseidon:
        default:
            attackPeriod = eNumbers::sGodAttackAttackPeriod;
            break;
        }
        const int attackRange = eNumbers::sGodAttackAttackRange;

        const int targetedAttackPeriod = eNumbers::sGodAttackTargetedAttackPeriod;
        const int targetedAttackRange = eNumbers::sGodAttackTargetedAttackRange;
        const int targetedCursePeriod = eNumbers::sGodAttackTargetedCursePeriod;
        const int targetedCurseRange = eNumbers::sGodAttackTargetedCurseRange;
        const int godFightRange = eNumbers::sGodAttackGodFightRange;

        bool r = lookForBlessCurse(by, mLookForCurse, cursePeriod, curseRange, -1);
        if(!r) r = lookForAttack(by, mLookForAttack, attackPeriod, attackRange);
        if(!r) r = lookForTargetedAttack(by, mLookForTargetedAttack,
                                         targetedAttackPeriod, targetedAttackRange);
        if(!r) r = lookForTargetedBlessCurse(by, mLookForTargetedCurse,
                                             targetedCursePeriod, targetedCurseRange, -1);
        if(!r) lookForGodAttack(by, mLookForGod, lookForGodCheck, godFightRange);
        if(!r) {
            if(type == eGodType::apollo) {
                auto& board = this->board();
                using eLFPG = eLookForPlagueGodAct;
                const auto act = std::make_shared<eLFPG>(board);
                const auto at = eCharacterActionType::curse;
                const auto s = eGodSound::curse;
                const auto c = character();
                const auto chart = c->type();
                const int plaguePeriod = eNumbers::sGodAttackApolloPlaguePeriod;
                const int plagueRange = eNumbers::sGodAttackApolloPlagueRange;
                lookForRangeAction(by, mLookForSpecial,
                                   plaguePeriod, plagueRange,
                                   at, act, chart, s);
            } else if(type == eGodType::aphrodite) {
                auto& board = this->board();
                using eLFEG = eLookForEvictGodAct;
                const auto act = std::make_shared<eLFEG>(board);
                const auto at = eCharacterActionType::curse;
                const auto s = eGodSound::curse;
                const auto c = character();
                const auto chart = c->type();
                const int evictPeriod = eNumbers::sGodAttackAphroditeEvictPeriod;
                const int evictRange = eNumbers::sGodAttackAphroditeEvictRange;
                lookForRangeAction(by, mLookForSpecial,
                                   evictPeriod, evictRange,
                                   at, act, chart, s);
            }
        }
    }

    if(type == eGodType::atlas) {
        auto& board = this->board();
        const auto tile = c->tile();
        if(tile) {
            const auto& chars = tile->characters();
            for(const auto& cc : chars) {
                if(cc.get() == c) continue;
                const auto cType = cc->type();
                const bool r = eDionysusFollowAction::sShouldFollow(cType);
                if(!r) continue;
                const auto ccaa = cc->actionType();
                if(ccaa == eCharacterActionType::die) continue;
                const auto wa = e::make_shared<eWaitAction>(cc.get());
                wa->setTime(20000);
                cc->setActionType(eCharacterActionType::stand);
                cc->setAction(wa);
                const auto killA = std::make_shared<eChar_killWithCorpseFinish>(
                                       board, cc.get(), true);
                wa->setFinishAction(killA);
                wa->setFailAction(killA);
            }
        }
    }

    eGodAction::increment(by);
}

bool eGodAttackAction::lookForAttack(const int dtime,
                                     int& time, const int freq,
                                     const int range) {
    const auto c = character();
    const auto act = std::make_shared<eLookForAttackGodAct>(board(), c);

    const auto at = eCharacterActionType::fight2;
    const auto s = eGodSound::attack;
    const auto chart = c->type();

    return lookForRangeAction(dtime, time, freq, range,
                              at, act, chart, s);
}

bool eGodAttackAction::lookForTargetedAttack(const int dtime,
                                             int& time, const int freq,
                                             const int range) {
    const auto c = character();
    const auto act = std::make_shared<eLookForTargetedAttackGodAct>(
                         board(), type());

    const auto at = eCharacterActionType::fight2;
    const auto s = eGodSound::attack;
    const auto chart = c->type();

    return lookForRangeAction(dtime, time, freq, range,
                              at, act, chart, s);
}

stdsptr<eObsticleHandler> eGodAttackAction::obsticleHandler() {
    return std::make_shared<eGodObsticleHandler>(board(), this);
}

void eGodAttackAction::destroyBuilding(eBuilding* const b) {
    const auto finishAttackA = std::make_shared<eGAA_destroyBuildingFinish>(
                                   board(), this, b);
    const auto playHitSound = std::make_shared<ePlayMonsterBuildingAttackSoundGodAct>(
                                  board(), b);
    pauseAction();
    const auto at = eCharacterActionType::fight2;
    const auto s = eGodSound::attack;
    const auto c = character();
    const auto chart = c->type();
    spawnGodMultipleMissiles(at, chart, b->centerTile(),
                             s, playHitSound, finishAttackA, 3);
}

void eGodAttackAction::goToTarget() {
    const auto gt = type();
    const auto hg = eHeatGetters::godLeaning(gt);
    const stdptr<eGodAction> tptr(this);
    const auto tele = std::make_shared<eTeleportFindFailFunc>(board(), this);
    eGodMonsterAction::goToTarget(hg, tele, obsticleHandler());
}

bool eGodAttackAction::decide() {
    const auto c = character();
    switch(mStage) {
    case eGodAttackStage::none:
        mStage = eGodAttackStage::appear;
        randomPlaceOnBoard();
        if(!c->tile()) {
            c->kill();
        } else {
            appear();
        }
        break;
    case eGodAttackStage::appear:
        mStage = eGodAttackStage::goTo1;
        initialize();
        goToTarget();
        break;
    case eGodAttackStage::goTo1: {
        mStage = eGodAttackStage::patrol1;
        goToNearestRoad();
    }   break;
    case eGodAttackStage::patrol1:
        mStage = eGodAttackStage::goTo2;
        goToTarget();
        break;
    case eGodAttackStage::goTo2: {
        mStage = eGodAttackStage::patrol2;
        goToNearestRoad();
    }   break;
    case eGodAttackStage::patrol2:
        mStage = eGodAttackStage::disappear;
        disappear();
        break;
    case eGodAttackStage::disappear:
        c->kill();
        break;
    }
    return true;
}

void eGodAttackAction::spawnAresArmy() {
    mArmySpawned = true;
    const auto c = character();
    const auto gtile = c->tile();
    if(!gtile) return;
    auto& board = this->board();
    // The troops arrive a bit away from the god
    eTile* tile = nullptr;
    for(int r = 8; r <= 14 && !tile; r++) {
        for(int dx = -r; dx <= r && !tile; dx++) {
            for(int dy = -r; dy <= r && !tile; dy++) {
                if(std::max(std::abs(dx), std::abs(dy)) != r) continue;
                const auto t = board.tile(gtile->x() + dx, gtile->y() + dy);
                if(!t || !t->walkable() || t->underBuilding()) continue;
                if(t->cityId() != c->onCityId()) continue;
                tile = t;
            }
        }
    }
    if(!tile) tile = gtile;
    const auto cid = c->cityId();
    const auto ocid = c->onCityId();
    const int nBanners = 2;
    const int perBanner = eNumbers::sSoldiersPerAresArtemisBanner;
    for(int i = 0; i < nBanners; i++) {
        const auto b = e::make_shared<eSoldierBanner>(
                           eBannerType::enemy, board);
        b->setOnCityId(ocid);
        b->setCityId(cid);
        b->backFromHome();
        b->moveTo(tile->x(), tile->y());
        mArmy.push_back(b);
        for(int j = 0; j < perBanner; j++) {
            stdsptr<eSoldier> s;
            if(type() == eGodType::artemis) {
                // Artemis comes with her Amazon archers
                const auto a = e::make_shared<eAmazon>(board);
                a->setIsArcher(true);
                s = a;
            } else {
                s = e::make_shared<eAresWarrior>(board);
            }
            s->setCityId(cid);
            s->setOnCityId(ocid);
            const auto a = e::make_shared<eSoldierAction>(s.get());
            a->setSpreadPeriod(true);
            s->setAction(a);
            s->changeTile(tile);
            s->setActionType(eCharacterActionType::walk);
            b->incCount();
            s->setBanner(b.get());
        }
    }
    std::vector<eSoldierBanner*> bs;
    for(const auto& b : mArmy) bs.push_back(b.get());
    eSoldierBanner::sPlace(bs, tile->x(), tile->y(), board, 3, 0);
}

void eGodAttackAction::moveAresArmy() {
    // On their own: to the nearest enemy soldiers, otherwise to the
    // nearest building of the city
    std::vector<eSoldierBanner*> bs;
    for(const auto& b : mArmy) {
        if(b->count() > 0 && b->tile()) bs.push_back(b.get());
    }
    if(bs.empty()) return;
    const auto from = bs.front()->tile();
    auto& board = this->board();
    const auto c = character();
    const auto team = c->teamId();
    const auto ocid = c->onCityId();
    eTile* target = nullptr;
    int best = 26;
    for(const auto ch : board.characters()) {
        if(!ch || ch->dead() || !ch->isSoldier()) continue;
        if(!eTeamIdHelpers::isEnemy(ch->teamId(), team)) continue;
        const auto t = ch->tile();
        if(!t) continue;
        const int d = std::max(std::abs(t->x() - from->x()),
                               std::abs(t->y() - from->y()));
        if(d < best) {
            best = d;
            target = t;
        }
    }
    if(!target) {
        best = __INT_MAX__;
        const auto bds = board.buildings(ocid, [](eBuilding* const b) {
            const auto bt = b->type();
            return bt != eBuildingType::road &&
                   eBuilding::sAttackable(bt);
        });
        for(const auto b : bds) {
            const auto t = b->centerTile();
            if(!t) continue;
            const int d = std::abs(t->x() - from->x()) +
                          std::abs(t->y() - from->y());
            if(d < best) {
                best = d;
                target = t;
            }
        }
    }
    if(!target) return;
    eSoldierBanner::sPlace(bs, target->x(), target->y(), board, 3, 0);
}

void eGodAttackAction::fleeArmy() {
    auto& board = this->board();
    const auto c = character();
    const auto ocid = c->onCityId();
    const auto rect = board.boardCityTileBRect(ocid);
    for(const auto& b : mArmy) {
        const std::vector<eSoldier*> solds = b->soldiers();
        for(const auto s : solds) {
            if(!s || s->dead()) continue;
            const auto st = s->tile();
            s->setBanner(nullptr);
            if(!st) {
                s->kill();
                continue;
            }
            // Nearest walkable tile on the edge of the city
            eTile* exit = nullptr;
            int best = __INT_MAX__;
            // (the city rectangle is in board (dtile) coordinates)
            const auto check = [&](const int x, const int y) {
                const auto t = board.dtile(x, y);
                if(!t || !t->walkable()) return;
                const int d = std::abs(t->x() - st->x()) +
                              std::abs(t->y() - st->y());
                if(d < best) {
                    best = d;
                    exit = t;
                }
            };
            for(int x = rect.x; x < rect.x + rect.w; x++) {
                check(x, rect.y);
                check(x, rect.y + rect.h - 1);
            }
            for(int y = rect.y; y < rect.y + rect.h; y++) {
                check(rect.x, y);
                check(rect.x + rect.w - 1, y);
            }
            const auto a = e::make_shared<eMoveToAction>(s);
            const auto kill = std::make_shared<eKillCharacterFinishFail>(
                                  board, s);
            a->setFinishAction(kill);
            a->setFailAction(kill);
            const stdptr<eCharacter> sptr(s);
            a->setFindFailAction([sptr]() {
                if(sptr) sptr->kill();
            });
            s->setAction(a);
            s->setActionType(eCharacterActionType::walk);
            if(exit) a->start(exit);
            else s->kill();
        }
    }
    mArmy.clear();
}

void eGodAttackAction::read(eReadStream& src) {
    eGodAction::read(src);
    src >> mStage;
    src >> mLookForCurse;
    src >> mLookForTargetedCurse;
    src >> mLookForAttack;
    src >> mLookForTargetedAttack;
    src >> mLookForGod;
    src >> mLookForSpecial;
    auto& board = this->board();
    src.readBuilding(&board, [this](eBuilding* const b) {
        mSanctuary = static_cast<eSanctuary*>(b);
    });
    if(src.formatVersion() >= eFileFormat::aresArmy) {
        src >> mArmySpawned;
        int nb;
        src >> nb;
        for(int i = 0; i < nb; i++) {
            eBannerType type;
            src >> type;
            const auto b = e::make_shared<eSoldierBanner>(type, board);
            b->read(src);
            mArmy.push_back(b);
        }
    } else {
        mArmySpawned = true;
    }
}

void eGodAttackAction::write(eWriteStream& dst) const {
    eGodAction::write(dst);
    dst << mStage;
    dst << mLookForCurse;
    dst << mLookForTargetedCurse;
    dst << mLookForAttack;
    dst << mLookForTargetedAttack;
    dst << mLookForGod;
    dst << mLookForSpecial;
    dst.writeBuilding(mSanctuary);
    dst << mArmySpawned;
    dst << static_cast<int>(mArmy.size());
    for(const auto& b : mArmy) {
        dst << b->type();
        b->write(dst);
    }
}

void eGodAttackAction::setSanctuary(const stdptr<eSanctuary>& s) {
    mSanctuary = s;
}

void eGodAttackAction::initialize() {
    auto& board = this->board();
    const auto cid = cityId();
    const auto c = character();
    const auto tile = c->tile();
    if(!tile) return;
    const auto type = this->type();
    if(type == eGodType::dionysus) {
        eCharacter* f = c;
        eDionysusFollowAction* fa = nullptr;
        for(int i = 0; i < 3; i++) {
            const auto s = e::make_shared<eSatyr>(board);
            s->changeTile(tile);
            const auto a = e::make_shared<eDionysusFollowAction>(
                               f, s.get());
            s->setAction(a);
            f = s.get();
            if(fa) fa->setFollower(s.get());
            fa = a.get();
        }
    } else if(type == eGodType::hades) {
        const auto p = board.palace(cid);
        if(p) p->setBlessed(-1.);
        const auto& chars = board.characters();
        for(const auto c : chars) {
            const auto cType = c->type();
            const bool r = eDionysusFollowAction::sShouldFollow(cType);
            if(!r) continue;
            if(c->dead()) continue;
            c->killWithCorpse();
        }
    }
}
