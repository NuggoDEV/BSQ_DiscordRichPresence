#include "GlobalNamespace/ScoreUIController.hpp"
#include "GlobalNamespace/IScoreController.hpp"
#include "GlobalNamespace/BeatmapObjectManager.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "bsml/shared/Helpers/getters.hpp"

#include "main.hpp"
#include "../src/utils/Requests/requests.hpp"

int g_missCount = 0;
int g_badCutCount = 0;

int GetMissCount() {
    return g_missCount;
}

int GetBadCutCount() {
    return g_badCutCount;
}

void StatUpdate() {
    while (true) {
        if (inGameplay) {
            auto diContainer = BSML::Helpers::GetDiContainer();
            auto ScoreUI = diContainer->Resolve<GlobalNamespace::ScoreUIController*>();
            auto getScore = ScoreUI->_scoreController->get_multipliedScore();

            nlohmann::json data;
            data["type"] = "StatUpdate";
            data["score"] = getScore;
            data["missCount"] = GetMissCount();
            data["badCutCount"] = GetBadCutCount();

            CreateRequest("POST", "/sendData", data);

            logger.debug("ScoreUpdate sent to server");

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

MAKE_HOOK_MATCH(BeatmapObjectManager_HandleNoteControllerNoteWasMissed,
                &::GlobalNamespace::BeatmapObjectManager::HandleNoteControllerNoteWasMissed,
                void,
                ::GlobalNamespace::BeatmapObjectManager* self,
                ::GlobalNamespace::NoteController* noteController)
{
    BeatmapObjectManager_HandleNoteControllerNoteWasMissed(self, noteController);
    g_missCount++;
}

MAKE_HOOK_MATCH(BeatmapObjectManager_HandleNoteControllerNoteWasCut,
                &::GlobalNamespace::BeatmapObjectManager::HandleNoteControllerNoteWasCut,
                void,
                ::GlobalNamespace::BeatmapObjectManager* self,
                ::GlobalNamespace::NoteController* noteController,
                ::ByRef<::GlobalNamespace::NoteCutInfo> noteCutInfo)
{
    BeatmapObjectManager_HandleNoteControllerNoteWasCut(self, noteController, noteCutInfo);

    if (!noteCutInfo->get_allIsOK()) {
        g_badCutCount++;
    }
}

void InstallBeatmapObjectHooks() {
    INSTALL_HOOK(logger, BeatmapObjectManager_HandleNoteControllerNoteWasMissed);
    INSTALL_HOOK(logger, BeatmapObjectManager_HandleNoteControllerNoteWasCut);
}