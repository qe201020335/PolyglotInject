#include "main.hpp"
#include "BGLib/Polyglot/LocalizationImporter.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationAsset.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "assets.hpp"
#include "helpers.h"

// Stores the ID and version of our mod, and is sent to the modloader upon startup
static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
static char* sira_new;

// Loads the config from disk using our modInfo, then returns it for use
// other config tools such as config-utils don't use this config, so it can be
// removed if those are in use
Configuration &getConfig() {
  static Configuration config(modInfo);
  return config;
}

void loadLocalizationFile() {
    sira_new = new char[sira_new_csv::getLength() + 1];
    memcpy(sira_new, sira_new_csv::getData(), sira_new_csv::getLength());
    sira_new[sira_new_csv::getLength()] = '\0';
    PaperLogger.info("Sira Localization File Loaded, Length %lu", sira_new_csv::getLength());
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

MAKE_HOOK_MATCH(MainSystemInitHook, &GlobalNamespace::MainSystemInit::Init, void, GlobalNamespace::MainSystemInit* self) {
    PaperLogger.debug("%s triggered!", name());
    PaperLogger.info("Injecting base game localization file!");

//    Polyglot::Localization::get_Instance()->get_InputFiles()->items[0]->set_TextAsset(makeTextAsset(sira_new));

    auto * localizationAsset = BGLib::Polyglot::LocalizationAsset::New_ctor(makeTextAsset(sira_new), BGLib::Polyglot::GoogleDriveDownloadFormat::CSV);
    BGLib::Polyglot::Localization::get_Instance()->get_InputFiles()->Add(localizationAsset);
    PaperLogger.info("Localization file injected!");

    BGLib::Polyglot::LocalizationImporter::Refresh();

    PaperLogger.debug("Number of loaded localization assets: %d", BGLib::Polyglot::Localization::get_Instance()->get_InputFiles()->Count);
    MainSystemInitHook(self);
}

MAKE_HOOK_MATCH(LocalizationImporterInitHook, &BGLib::Polyglot::LocalizationImporter::Initialize, void, BGLib::Polyglot::LocalizationModel* settings) {
    PaperLogger.debug("%s triggered!", name());
    LocalizationImporterInitHook(settings);
    auto instance = BGLib::Polyglot::Localization::get_Instance();
    if (!instance->get_SupportedLanguages()->Contains(BGLib::Polyglot::Language::Simplified_Chinese)) {
        instance->get_SupportedLanguages()->Add(BGLib::Polyglot::Language::Simplified_Chinese);
    }
}
// Called later on in the game loading - a good time to install function hooks
MOD_EXTERN_FUNC void late_load() noexcept {
  il2cpp_functions::Init();

  PaperLogger.info("Installing hooks...");
    // Install our hooks
    INSTALL_HOOK(PaperLogger, MainSystemInitHook)
    INSTALL_HOOK(PaperLogger, LocalizationImporterInitHook)
  PaperLogger.info("Installed all hooks!");
}