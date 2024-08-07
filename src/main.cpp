#include "main.hpp"
#include "assets.hpp"
#include "helpers.h"

#include "BGLib/Polyglot/LocalizationImporter.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationAsset.hpp"
#include "BGLib/Polyglot/LocalizationAsyncInstaller.hpp"
#include "BGLib/AppFlow/Initialization/AsyncInstaller.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "System/Collections/Generic/ICollection_1.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/IList_1.hpp"

// Stores the ID and version of our mod, and is sent to the modloader upon startup
static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
static char *localizationString;

// Loads the config from disk using our modInfo, then returns it for use
// other config tools such as config-utils don't use this config, so it can be
// removed if those are in use
Configuration &getConfig() {
    static Configuration config(modInfo);
    return config;
}

void loadLocalizationFile() {
    localizationString = new char[polyglot_inject_csv::getLength() + 1];
    memcpy(localizationString, polyglot_inject_csv::getData(), polyglot_inject_csv::getLength());
    localizationString[polyglot_inject_csv::getLength()] = '\0';
    PaperLogger.info("Localization File Loaded, Length %lu", polyglot_inject_csv::getLength());
}

// Called at the early stages of game loading
MOD_EXTERN_FUNC void setup(CModInfo *info) noexcept {
    *info = modInfo.to_c();

    getConfig().Load();

    // File logging
    Paper::Logger::RegisterFileContextId(PaperLogger.tag);

    loadLocalizationFile();

    PaperLogger.info("Completed setup!");
}

MAKE_HOOK_MATCH(LocalizationInstallerHook, 
                &BGLib::Polyglot::LocalizationAsyncInstaller::LoadResourcesBeforeInstall, 
                void, 
                BGLib::Polyglot::LocalizationAsyncInstaller *self, 
                System::Collections::Generic::IList_1<UnityW<UnityEngine::TextAsset>> * assets, 
                BGLib::AppFlow::Initialization::AsyncInstaller::IInstallerRegistry* registry) {
    PaperLogger.debug("%s triggered!", name());
    assets->i___System__Collections__Generic__ICollection_1_T_()->Add(makeTextAsset(localizationString));
    self->_mainPolyglotAsset->supportedLanguages->Add(BGLib::Polyglot::Language::Simplified_Chinese);
    LocalizationInstallerHook(self, assets, registry);
}

// Called later on in the game loading - a good time to install function hooks
MOD_EXTERN_FUNC void late_load() noexcept {
    il2cpp_functions::Init();

    PaperLogger.info("Installing hooks...");
    // Install our hooks
    INSTALL_HOOK(PaperLogger, LocalizationInstallerHook)
    PaperLogger.info("Installed all hooks!");
}