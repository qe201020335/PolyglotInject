// Include our main header file, containing the logger
#include "main.hpp"
#include "assets.hpp"
#include "helpers.h"
#include "SiraLocalizer/UI/FontLoader.hpp"

// Include dependency headers
#include "scotland2/shared/modloader.h"

#include "beatsaber-hook/shared/hooking.hpp"
#include "beatsaber-hook/shared/api.hpp"

#include "BGLib/Polyglot/LocalizationImporter.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationAsset.hpp"
#include "BGLib/Polyglot/LocalizationAsyncInstaller.hpp"
#include "BGLib/Polyglot/LocalizationLanguage.hpp"
#include "BGLib/Polyglot/LocalizationLanguageExtensions.hpp"
#include "BGLib/AppFlow/Initialization/AsyncInstaller.hpp"
#include "BGLib/AppFlow/Initialization/IInstallerRegistry.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "System/Collections/Generic/ICollection_1.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/IList_1.hpp"
#include "UnityEngine/SceneManagement/LoadSceneMode.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"

#include <algorithm>
#include <cctype>
#include <string>

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
                BGLib::AppFlow::Initialization::IInstallerRegistry* registry) {
    logger.debug("%s triggered!", name());
    assets->i___System__Collections__Generic__ICollection_1_T_()->Add(makeTextAsset(localizationString));

    // Match SiraLocalizer
    using BGLib::Polyglot::LocalizationLanguage;
    auto addedLanguages = {
        LocalizationLanguage::Italian,
        LocalizationLanguage::Portuguese_Brazil,
        LocalizationLanguage::Russian,
        LocalizationLanguage::Danish,
        LocalizationLanguage::Swedish,
        LocalizationLanguage::Dutch,
        LocalizationLanguage::Simplified_Chinese,
        LocalizationLanguage::Traditional_Chinese,
        LocalizationLanguage::Czech,
        LocalizationLanguage::Hungarian,
        LocalizationLanguage::Hebrew
    };

    auto supportedLanguages = self->_mainPolyglotAsset->supportedLanguages;
    for (auto language : addedLanguages) {
        if (!supportedLanguages->Contains(language)) {
            supportedLanguages->Add(language);
        }
    }

    LocalizationInstallerHook(self, assets, registry);
}

static bool isSimplifiedChinese(StringW serializedName) {
    if (!serializedName) {
        return false;
    }

    std::string name = serializedName;
    std::ranges::transform(name, name.begin(), [](unsigned char c) -> char {
        return c == '-' ? '_' : std::tolower(c);
    });
    return name == "zh_ch" || name == "zh";
}

// The game saves Simplified Chinese as "zh_CH" but expects "zh_CN" when parsing it back.
// Anything else starting with "zh" becomes Traditional Chinese.
// 1.40.8 saves "zh" so we should map this to Simplified Chinese too.
MAKE_HOOK_MATCH(SimplifiedChineseParseHook,
                &BGLib::Polyglot::LocalizationLanguageExtensions::ToLocalizationLanguage,
                BGLib::Polyglot::LocalizationLanguage,
                StringW serializedName) {
    logger.debug("{} triggered!", name());
    if (isSimplifiedChinese(serializedName)) {
        return BGLib::Polyglot::LocalizationLanguage::Simplified_Chinese;
    }
    return SimplifiedChineseParseHook(serializedName);
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
    i2c::functions::initialize();

    logger.info("Installing hooks...");
    // Install our hooks
    INSTALL_HOOK(logger, LocalizationInstallerHook)
    INSTALL_HOOK(logger, SimplifiedChineseParseHook)
    INSTALL_HOOK(logger, SceneLoadedHook)
    logger.info("Installed all hooks!");

    fontLoader.Initialize();
}
