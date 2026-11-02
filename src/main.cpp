// Include our main header file, containing the logger
#include "main.hpp"
#include "assets.hpp"
#include "helpers.h"

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
#include "BGLib/AppFlow/Initialization/AsyncInstaller.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "System/Collections/Generic/ICollection_1.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/IList_1.hpp"

// Store the mod ID and version, so it can be sent to the modloader at startup
static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
static char *localizationString;

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
    self->_mainPolyglotAsset->supportedLanguages->Add(BGLib::Polyglot::Language::Simplified_Chinese);
    LocalizationInstallerHook(self, assets, registry);
}

// Called later on in the game loading, after all mods have been opened
// Often used to install hooks and use the APIs of other mods or libraries
extern "C" __attribute__((visibility("default"))) void late_load() noexcept {
    il2cpp_functions::Init();

    logger.info("Installing hooks...");
    // Install our hooks
    INSTALL_HOOK(logger, LocalizationInstallerHook)
    logger.info("Installed all hooks!");
}
