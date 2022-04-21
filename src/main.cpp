#include "main.hpp"
#include "Polyglot/LocalizationImporter.hpp"
#include "Polyglot/Localization.hpp"
#include "Polyglot/LocalizationAsset.hpp"
#include "UnityEngine/TextAsset.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "assets.hpp"
#include "UnityEngine/AssetBundle.hpp"

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
static char* localization = new char[Localization_CNMOD_txt::getLength() + 1];

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

    memcpy(localization, Localization_CNMOD_txt::getData(), Localization_CNMOD_txt::getLength());
    localization[Localization_CNMOD_txt::getLength()] = '\0';

    getLogger().info("Completed setup!");
}

MAKE_HOOK_MATCH(Localization_SelectLanguage, &GlobalNamespace::MainSystemInit::Init, void, GlobalNamespace::MainSystemInit* self) {
    getLogger().info("Hook triggered!");
    getLogger().info("Replacing base game localization file!");
    Polyglot::LocalizationAsset * og_local = Polyglot::Localization::get_Instance()->get_InputFiles()->items[0];

    using LoadFromMemory = function_ptr_t<UnityEngine::AssetBundle*, Array<uint8_t>*, unsigned int>;
    static LoadFromMemory loadFromMemory = reinterpret_cast<LoadFromMemory>(il2cpp_functions::resolve_icall("UnityEngine.AssetBundle::LoadFromMemory_Internal"));

    std::vector<uint8_t> data;// = *((std::vector<uint8_t>*)&bytes);
    data.insert(data.end(), &(testbundle_asset::getData()[0]), &(testbundle_asset::getData()[testbundle_asset::getLength()]));

    Array<uint8_t>* dataArray = il2cpp_utils::vectorToArray(data);

    auto bundle = loadFromMemory(dataArray, 0);

    auto * textAsset = bundle->LoadAsset<UnityEngine::TextAsset*>("Localization_CNMOD");

    Polyglot::Localization::get_Instance()->get_InputFiles()->items[0]->set_TextAsset(textAsset);
    Polyglot::LocalizationImporter::Refresh();
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

    getLogger().info("Localization File Length: %lu", Localization_CNMOD_txt::getLength());

}
