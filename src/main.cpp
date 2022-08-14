#include "main.hpp"
#include "Polyglot/LocalizationImporter.hpp"
#include "Polyglot/Localization.hpp"
#include "Polyglot/LocalizationAsset.hpp"
#include "UnityEngine/TextAsset.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "assets.hpp"
#include "helpers.h"
static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
static char* sira_new;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

void loadLocalizationFile(){
    sira_new = new char[sira_new_csv::getLength() + 1];
    memcpy(sira_new, sira_new_csv::getData(), sira_new_csv::getLength());
    sira_new[sira_new_csv::getLength()] = '\0';
    getLogger().info("Sira Localization File Loaded, Length %lu", sira_new_csv::getLength());
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file

    loadLocalizationFile();

    getLogger().info("Completed setup!");
}

MAKE_HOOK_MATCH(MainSystemInitHook, &GlobalNamespace::MainSystemInit::Init, void, GlobalNamespace::MainSystemInit* self) {
    getLogger().info("%s triggered!", name());
    getLogger().info("Replacing base game localization file!");

    Polyglot::Localization::get_Instance()->get_InputFiles()->items[0]->set_TextAsset(makeTextAsset(sira_new));

//    auto * localizationAsset = new Polyglot::LocalizationAsset();
//    localizationAsset->set_TextAsset(makeTextAsset(localization));
//    localizationAsset->set_Format(Polyglot::GoogleDriveDownloadFormat::CSV);
//    Polyglot::Localization::get_Instance()->get_InputFiles()->Add(localizationAsset);

    Polyglot::LocalizationImporter::Refresh();

    getLogger().debug("Loaded localization assets: %d", Polyglot::Localization::get_Instance()->get_InputFiles()->size);
    MainSystemInitHook(self);
}

MAKE_HOOK_MATCH(LocalizationImporterInitHook, &Polyglot::LocalizationImporter::Initialize, void) {
    getLogger().info("%s triggered!", name());
    LocalizationImporterInitHook();
    auto instance = Polyglot::Localization::get_Instance();
    if (!instance->get_SupportedLanguages()->Contains(Polyglot::Language::Simplified_Chinese)) {
        instance->get_SupportedLanguages()->Add(Polyglot::Language::Simplified_Chinese);
    }
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    LoggerContextObject logger = getLogger().WithContext("load");

    getLogger().info("Installing hooks...");
    // Install our hooks (none defined yet)
    INSTALL_HOOK(logger, MainSystemInitHook)
    INSTALL_HOOK(logger, LocalizationImporterInitHook)
    getLogger().info("Installed all hooks!");
}
