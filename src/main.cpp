#include "main.hpp"
#include "Polyglot/LocalizationImporter.hpp"
#include "Polyglot/Localization.hpp"
#include "Polyglot/LocalizationAsset.hpp"
#include "UnityEngine/TextAsset.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
using namespace Polyglot;
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

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

MAKE_HOOK_MATCH(Localization_SelectLanguage, &GlobalNamespace::MainSystemInit::Init, void, GlobalNamespace::MainSystemInit* self) {
    getLogger().info("Hook triggered!");
    Polyglot::LocalizationAsset * og_local = Polyglot::Localization::get_Instance()->get_InputFiles()->items[0];
    getLogger().debug("%s",std::string(og_local->get_TextAsset()->get_text()).c_str());
    Localization_SelectLanguage(self);
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    LoggerContextObject logger = getLogger().WithContext("load");

    getLogger().info("Installing hooks...");
    // Install our hooks (none defined yet)
    INSTALL_HOOK(logger, Localization_SelectLanguage)
    getLogger().info("Installed all hooks!");

    getLogger().info("Test Log");
//    Polyglot::LocalizationAsset * og_local = Polyglot::Localization::get_Instance()->get_InputFiles()->items[0];
//
//    auto aaa = Polyglot::Localization::get_Instance();
//    if (aaa == nullptr) {
//        getLogger().error("Localization is null");
//    }

//    getLogger().debug("%s",std::string(og_local->get_TextAsset()->get_text()).c_str());
//    INSTALL_HOOK(logger, Localization_SelectLanguage);

}
