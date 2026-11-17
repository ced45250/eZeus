#include "edionysusfollowaction.h"

#include "characters/echaracter.h"
#include "engine/egameboard.h"

eDionysusFollowAction::eDionysusFollowAction(
        eCharacter* const f, eCharacter* const c) :
    eFollowAction(f, c, eCharActionType::dionysusFollowAction) {}

eDionysusFollowAction::eDionysusFollowAction(
        eCharacter* const c) :
    eDionysusFollowAction(nullptr, c) {}

void eDionysusFollowAction::setFollower(eCharacter* const f) {
    mFollower = f;
}

void eDionysusFollowAction::read(eReadStream& src) {
    eFollowAction::read(src);
    src.readCharacter(&board(), [this](eCharacter * const c) {
        mFollower = c;
    });
}

void eDionysusFollowAction::write(eWriteStream& dst) const {
    eFollowAction::write(dst);
    dst.writeCharacter(mFollower);
}

bool eDionysusFollowAction::sShouldFollow(const eCharacterType c) {
    switch(c) {
    case eCharacterType::settler:
    case eCharacterType::homeless:
    case eCharacterType::shepherd:
    case eCharacterType::goatherd:
    case eCharacterType::grower:
    case eCharacterType::peddler:
    case eCharacterType::lumberjack:
    case eCharacterType::bronzeMiner:
    case eCharacterType::silverMiner:
    case eCharacterType::artisan:
    case eCharacterType::hunter:
    case eCharacterType::gymnast:
    case eCharacterType::philosopher:
    case eCharacterType::actor:
    case eCharacterType::competitor:
    case eCharacterType::scholar:
    case eCharacterType::astronomer:
    case eCharacterType::inventor:
    case eCharacterType::curator:
    case eCharacterType::taxCollector:
    case eCharacterType::watchman:
    case eCharacterType::waterDistributor:
    case eCharacterType::fireFighter:
    case eCharacterType::healer:
    case eCharacterType::cartTransporter:
        return true;
    default:
        return false;
    }
    return false;
}

void eDionysusFollowAction::increment(const int by) {
    // The last one in the procession draws in the citizens walking
    // close by (not only on its own tile), up to a limit
    mLookForFollower += by;
    if(!mFollower && mLookForFollower > 300) {
        mLookForFollower = 0;
        auto& board = this->board();
        const auto c = character();
        const auto tile = c->tile();
        const int range = 2;
        int length = 0;
        for(eCharacter* l = c; l && length < 100; length++) {
            const auto la = dynamic_cast<eDionysusFollowAction*>(l->action());
            l = la ? la->mFollow.get() : nullptr;
        }
        const int maxLength = 3 + 15; // satyrs + citizens
        for(int dx = -range; dx <= range && tile && !mFollower &&
                             length < maxLength; dx++) {
            for(int dy = -range; dy <= range && !mFollower; dy++) {
                const auto t = board.tile(tile->x() + dx, tile->y() + dy);
                if(!t) continue;
                const auto chars = t->characters();
                for(const auto& cc : chars) {
                    if(cc.get() == c || cc->dead()) continue;
                    const auto cType = cc->type();
                    const bool r = sShouldFollow(cType);
                    if(!r) continue;
                    const auto ccaa = cc->actionType();
                    if(ccaa == eCharacterActionType::die) continue;
                    const auto cca = cc->action();
                    const auto eDFA = eCharActionType::dionysusFollowAction;
                    if(cca && cca->type() == eDFA) continue;
                    const auto fa = e::make_shared<eDionysusFollowAction>(c, cc.get());
                    cc->setAction(fa);
                    const auto killA = std::make_shared<eChar_killWithCorpseFinish>(
                        board, cc.get(), true);
                    fa->setFinishAction(killA);
                    fa->setFailAction(killA);
                    mFollower = cc.get();
                    break;
                }
            }
        }
    }
    eFollowAction::increment(by);
}
