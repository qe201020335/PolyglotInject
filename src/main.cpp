// Include our main header file, containing the logger
#include "main.hpp"
#include "assets.hpp"
#include "helpers.h"
#include "SiraLocalizer/UI/FontLoader.hpp"

// Include dependency headers
#include "scotland2/shared/modloader.h"

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"

#include "BGLib/Polyglot/LocalizationImporter.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationAsset.hpp"
#include "BGLib/Polyglot/LocalizationAsyncInstaller.hpp"
#include "BGLib/Polyglot/Language.hpp"
#include "BGLib/Polyglot/LanguageExtensions.hpp"
#include "BGLib/AppFlow/Initialization/AsyncInstaller.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "System/Collections/Generic/ICollection_1.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/IList_1.hpp"
#include "UnityEngine/SceneManagement/LoadSceneMode.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"

// Store the mod ID and version, so it can be sent to the modloader at startup
static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
static char *localizationString;
static PolyglotInject::SiraLocalizer::UI::FontLoader fontLoader;

void loadLocalizationFile() {
    localizationString = new char[polyglot_inject_csv::getLength() + 1];
    memcpy(localizationString, polyglot_inject_csv::getData(), polyglot_inject_csv::getLength());
    localizationString[polyglot_inject_csv::getLength()] = '\0';
    logger.info("Localization File Loaded, Length %lu", polyglot_inject_csv::getLength());
}

// Called in the early stages of game loading
// (see https://github.com/sc2ad/scotland2?tab=readme-ov-file#installationusage)
// Often used to initialize and load configs, in addition to its contents here
extern "C" __attribute__((visibility("default"))) void setup(CModInfo* info) noexcept {
    *info = modInfo.to_c();

    // Optionally register our logger so that all its messages
    // are stored in a dedicated file
    Paper::Logger::RegisterFileContextId(MOD_ID);

    loadLocalizationFile();

    logger.info("Completed setup!");
}

MAKE_HOOK_MATCH(LocalizationInstallerHook, 
                &BGLib::Polyglot::LocalizationAsyncInstaller::LoadResourcesBeforeInstall, 
                void, 
                BGLib::Polyglot::LocalizationAsyncInstaller *self, 
                System::Collections::Generic::IList_1<UnityW<UnityEngine::TextAsset>> * assets, 
                BGLib::AppFlow::Initialization::AsyncInstaller::IInstallerRegistry* registry) {
    logger.debug("%s triggered!", name());
    assets->i___System__Collections__Generic__ICollection_1_T_()->Add(makeTextAsset(localizationString));

    // Match SiraLocalizer
    using BGLib::Polyglot::Language;
    auto addedLanguages = {
        Language::Italian,
        Language::Portuguese_Brazil,
        Language::Russian,
        Language::Danish,
        Language::Swedish,
        Language::Dutch,
        Language::Simplified_Chinese,
        Language::Traditional_Chinese,
        Language::Czech,
        Language::Hungarian,
        Language::Hebrew
    };

    auto supportedLanguages = self->_mainPolyglotAsset->supportedLanguages;
    for (auto language : addedLanguages) {
        if (!supportedLanguages->Contains(language)) {
            supportedLanguages->Add(language);
        }
    }

    LocalizationInstallerHook(self, assets, registry);
}

// The game saves both Chinese as "zh" and both Portuguese as "pt", which parse back as Simplified Chinese and
// Portuguese. This make sure they are saved correctly.
MAKE_HOOK_MATCH(SerializedNameHook,
                &BGLib::Polyglot::LanguageExtensions::ToSerializedName,
                StringW,
                BGLib::Polyglot::Language lang) {
    using BGLib::Polyglot::Language;
    if (lang == Language::Traditional_Chinese) {
        return "zh-Hant";
    }
    if (lang == Language::Portuguese_Brazil) {
        return "pt-BR";
    }
    return SerializedNameHook(lang);
}

MAKE_HOOK_MATCH(SceneLoadedHook,
                &UnityEngine::SceneManagement::SceneManager::Internal_SceneLoaded,
                void,
                UnityEngine::SceneManagement::Scene scene,
                UnityEngine::SceneManagement::LoadSceneMode mode) {
    SceneLoadedHook(scene, mode);
    fontLoader.OnSceneLoaded();
}

// Called later on in the game loading, after all mods have been opened
// Often used to install hooks and use the APIs of other mods or libraries
extern "C" __attribute__((visibility("default"))) void late_load() noexcept {
    il2cpp_functions::Init();

    logger.info("Installing hooks...");
    // Install our hooks
    INSTALL_HOOK(logger, LocalizationInstallerHook)
    INSTALL_HOOK(logger, SerializedNameHook)
    INSTALL_HOOK(logger, SceneLoadedHook)
    logger.info("Installed all hooks!");

    fontLoader.Initialize();
}
