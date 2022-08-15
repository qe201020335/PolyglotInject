#include "fontLoader.hpp"
#include "assets.hpp"
#include <mutex>
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "helpers.h"
#include "boolinq.h"
#include "UnityEngine/Resources.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "TMPro/TMP_Text.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/Graphics.hpp"

/**
 * Ported from SiraLocalizer
 * SiraLocalizer.UI.FontLoader
 */

using namespace std;

const vector<string> FontLoader::kFontNamesToRemove{"NotoSansJP-Medium SDF", "NotoSansKR-Medium SDF",
                                                    "SourceHanSansCN-Bold-SDF-Common-1(2k)",
                                                    "SourceHanSansCN-Bold-SDF-Common-2(2k)",
                                                    "SourceHanSansCN-Bold-SDF-Uncommon(2k)"};
const vector<FontLoader::FontReplacementStrategy> FontLoader::kFontReplacementStrategies{
        {
                {"Teko-Medium SDF"},
                {"Teko-Medium SDF Latin-1 Supplement", "Oswald-Medium SDF Cyrillic", "SourceHanSans-Medium SDF"}
        },

        {
                {"Teko-Bold SDF"},
                {"Teko-Bold SDF Latin-1 Supplement",   "Oswald-Bold SDF Cyrillic",   "SourceHanSans-Medium SDF"}
        }
};

FontLoader *FontLoader::instance = nullptr;

void FontLoader::Initialize() {
    sceneChangeMtx.lock();
    sceneChangeSubbed = true;
    sceneChangeMtx.unlock();
    LoadFontAssets();
}

void FontLoader::Dispose() {
    sceneChangeMtx.lock();
    sceneChangeSubbed = false;
    sceneChangeMtx.unlock();
}

FontLoader *FontLoader::getInstance() {
    if (instance == nullptr) {
        instance = new FontLoader;
    }
    return instance;
}

void FontLoader::OnActiveSceneChange() {
//    sceneChangeMtx.lock();
    if (sceneChangeSubbed) {
        ApplyFallbackFonts();
    }
//    sceneChangeMtx.unlock();
}

void FontLoader::LoadFontAssets() {
    logger.info("Loading Fonts");
    auto assetBundle = loadAssetBundle(fonts_quest_assets::getData(), fonts_quest_assets::getLength());
    if (assetBundle == nullptr) {
        logger.error("Failed to load fonts asset bundle; some characters may not display as expected");
    }

    /* I'm not smart enough to translate this c# linq query to boolinq
    foreach (string fontName in kFontReplacementStrategies.SelectMany(s => s.fontNamesToAdd).Distinct())
    {
        LoadFontAsset(assetBundle, fontName);
    }
     */
    set<string> fontsToLoad;
    for (auto strategy: kFontReplacementStrategies) {
        fontsToLoad.insert(strategy.fontNamesToAdd.begin(), strategy.fontNamesToAdd.end());
    }
    for (const auto &fontName: fontsToLoad) {
        LoadFontAsset(assetBundle, fontName);
    }

    assetBundle->Unload(false);

    ApplyFallbackFonts();
}

void FontLoader::LoadFontAsset(UnityEngine::AssetBundle *assetBundle, const string &name) {
    auto *fontAsset = assetBundle->LoadAsset<TMPro::TMP_FontAsset *>(name);

    if (fontAsset == nullptr) {
        logger.error("Font %s could not be loaded; some characters may not display as expected", name.c_str());
        return;
    }
    logger.info("Font %s loaded successfully", name.c_str());
    fallbackFontAssets.emplace_back(fontAsset);
}

void FontLoader::ApplyFallbackFonts() {
    if (fallbackFontAssets.empty()) return;

    auto fontAssets = UnityEngine::Resources::FindObjectsOfTypeAll<TMPro::TMP_FontAsset *>();

    for (auto &strategy: kFontReplacementStrategies) {
        auto fontAssetsToAddFallback =
                boolinq::from(vector<TMPro::TMP_FontAsset *>(fontAssets.begin(), fontAssets.end()))
                        .where([this, strategy](TMPro::TMP_FontAsset *fontAsset) {
                            return !boolinq::from(processedFontAssets).contains(fontAsset) &&
                                   boolinq::from(strategy.targetFontNames).contains(fontAsset->get_name());
                        }).toStdVector();

        for (auto *fontAsset: fontAssetsToAddFallback) {
            auto fallbacks = boolinq::from(strategy.fontNamesToAdd)
                    .select([this](const string &name) {
                        return boolinq::from(fallbackFontAssets)
                                .first([name](TMPro::TMP_FontAsset *asset) {
                                    return asset->get_name() == name;
                                });
                    }).toStdVector();

            AddFallbacksToFont(fontAsset, fallbacks);
        }

    }

    // force update any text that has already rendered
    for (auto *text: UnityEngine::Object::FindObjectsOfType<TMPro::TMP_Text *>()) {
        text->SetAllDirty();
    }
}

void FontLoader::AddFallbacksToFont(TMPro::TMP_FontAsset *fontAsset, vector<TMPro::TMP_FontAsset *> &fallbacks) {
    logger.info("Adding fallbacks to %s (%d)",
                static_cast<string>(fontAsset->get_name()).c_str(),
                (uint) fontAsset->GetHashCode());

    auto *newFallbackTable = System::Collections::Generic::List_1<TMPro::TMP_FontAsset *>::New_ctor();
    // insert as first possible fallback font
    for (const auto &fallback: fallbacks) {
        newFallbackTable->Add(CopyFontAsset(fallback, fontAsset->material));
    }

    for (int i = 0; i < fontAsset->get_fallbackFontAssetTable()->get_Count(); i++) {
        auto *f = fontAsset->get_fallbackFontAssetTable()->get_Item(i);
        if (!boolinq::from(kFontNamesToRemove).contains(f->get_name())) {
            newFallbackTable->Add(f);
        }
    }
    fontAsset->set_fallbackFontAssetTable(newFallbackTable);
    processedFontAssets.emplace_back(fontAsset);
}

/**
 * SiraLocalizer.UI.FontAssetHelper.CopyFontAsset
 */
TMPro::TMP_FontAsset *FontLoader::CopyFontAsset(TMPro::TMP_FontAsset *original,
                                                UnityEngine::Material *referenceMaterial,
                                                const string &newName) {
    auto *copy = UnityEngine::Object::Instantiate(original);

    // Unity doesn't copy textures when using Object.Instantiate so we have to do it manually
    auto *texture = original->get_atlasTexture();
    auto *newTexture = UnityEngine::Texture2D::New_ctor(texture->get_width(),
                                                        texture->get_height(),
                                                        texture->get_format(),
                                                        texture->get_mipmapCount(),
                                                        true);
    using GraphicsCopyTexture = function_ptr_t<void, UnityEngine::Texture *, UnityEngine::Texture *>;
    static auto GraphicsCopyTexture_Full = CRASH_UNLESS(
            reinterpret_cast<GraphicsCopyTexture>(
                    il2cpp_functions::resolve_icall("UnityEngine.TextAsset::CopyTexture_Full")));
    GraphicsCopyTexture_Full(texture, newTexture);

    auto *material = UnityEngine::Material::New_ctor(referenceMaterial);
    material->SetTexture("_MainTex", newTexture);

    copy->m_AtlasTexture = newTexture;
    if (newName.empty()) {
        copy->set_name(original->get_name());
    } else {
        copy->set_name(newName);
    }
    copy->set_atlasTextures(new ArrayW<UnityEngine::Texture2D>(newTexture));
    copy->material = material;

    return copy;
}


