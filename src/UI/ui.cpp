#include "ui.hpp"
#include "main.hpp"
#include "../config.hpp"
#include "bsml/shared/BSML.hpp"
#include "UnityEngine/Application.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/MultiplayerLobbyConnectionController.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "web-utils/shared/WebUtils.hpp"

#include "../src/utils/Requests/requests.hpp"

using namespace GlobalNamespace;

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if (!firstActivation)
        return;

    auto container = BSML::Lite::CreateScrollableSettingsContainer(self);

    AddConfigValueInputString(container, getConfig().PCIPSetting);
    AddConfigValueInputString(container, getConfig().PortSetting);


    BSML::Lite::CreateUIButton(container, "Open Instructions", []() {
        UnityEngine::Application::OpenURL("https://github.com/RainzDev/BSQ_DiscordRichPresence#-quick-start");
    });
}

MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierachy, bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierachy, screenSystemEnabling);

    if (firstActivation && getConfig().FirstTime.GetValue()) {
        auto modal = BSML::Lite::CreateModal(self->transform, {100, 60}, []() {});

        auto verticalLayout = BSML::Lite::CreateVerticalLayoutGroup(modal);
    
        auto text = BSML::Lite::CreateText(verticalLayout, "Thank you for installing the mod! To setup your Discord RPC, please\nlook through the instructions by pressing \"Open Instructions\". ");
        text->set_enableWordWrapping(true);
        text->set_alignment(TMPro::TextAlignmentOptions::Center);

        auto horizontalLayout = BSML::Lite::CreateHorizontalLayoutGroup(verticalLayout);

        BSML::Lite::CreateUIButton(horizontalLayout, "Open Instructions", []() {
            UnityEngine::Application::OpenURL("https://github.com/RainzDev/BSQ_DiscordRichPresence#-quick-start");
        });
        BSML::Lite::CreateUIButton(horizontalLayout, "Close", [modal]() {
            getConfig().FirstTime.SetValue(false);
            modal->Hide();
        });

        modal->Show();

        return;
    }

    WebUtils::StringResponse getVersion = CreateRequest("GET", "/version", {});

    if (getVersion.IsSuccessful()) {
        auto data = getVersion.GetParsedData();
    } else {
        logger.debug("Failed to retrieve version.");

        auto modal = BSML::Lite::CreateModal(self->transform, {100, 40}, []() {});

        auto verticalLayout = BSML::Lite::CreateVerticalLayoutGroup(modal);

        auto text = BSML::Lite::CreateText(verticalLayout, "Your local server could not be updated due to changes made. Please\nopen the instructions to download the new version of the local server");
    
        text->set_enableWordWrapping(true);
        text->set_alignment(TMPro::TextAlignmentOptions::Center);

        auto horizontalLayout = BSML::Lite::CreateHorizontalLayoutGroup(verticalLayout);

        BSML::Lite::CreateUIButton(horizontalLayout, "Open Instructions", []() {
            UnityEngine::Application::OpenURL("https://github.com/RainzDev/BSQ_DiscordRichPresence#2-install-local-server");
        });
        BSML::Lite::CreateUIButton(horizontalLayout, "Close", [modal]() {
            modal->Hide();
        });

        modal->Show();

        return;

    }
}

void InstallUIHooks() {
    INSTALL_HOOK(logger, MainMenuViewController_DidActivate);
}