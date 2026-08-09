#include <thread>

#include "GlobalNamespace/ScoreUIController.hpp"
#include "GlobalNamespace/IScoreController.hpp"
#include "GlobalNamespace/BeatmapObjectManager.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "bsml/shared/Helpers/getters.hpp"

#include "main.hpp"
#include "../src/utils/Requests/requests.hpp"

#include "metacore/shared/stats.hpp"

void StatUpdateLoop() {
    while (true) {
        if (inSingleplayerGameplay || inMultiplayerGameplay) {
            auto getScore = MetaCore::Stats::GetScore(2);
            auto getNotesMissed = MetaCore::Stats::GetNotesMissed(2);
            auto getNotesBadCut = MetaCore::Stats::GetNotesBadCut(2);
            auto getBombsHit = MetaCore::Stats::GetBombsHit(2);

            nlohmann::json data;
            data["type"] = "BeatmapStatUpdate";
            data["score"] = getScore;
            data["notesMissed"] = getNotesMissed;
            data["notesBadCut"] = getNotesBadCut;
            data["bombsHit"] = getBombsHit;
            CreateRequest("POST", "/sendData", data);

            logger.info("ScoreUpdate sent to server");
            logger.info("Score: {}", getScore);
            logger.info("Notes Missed: {}", getNotesMissed);
            logger.info("Notes Bad Cut: {}", getNotesBadCut);
            logger.info("Bombs Hit: {}", getBombsHit);

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void StatUpdate() {
    std::thread(StatUpdateLoop).detach();
}