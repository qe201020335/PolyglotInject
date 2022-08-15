#pragma once

#include "vector"
#include "string"
#include "main.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "TMPro/TMP_FontAsset.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/AssetBundle.hpp"

using namespace std;

/**
 * Ported from SiraLocalizer
 * SiraLocalizer.UI.FontLoader
 */
class FontLoader {
public:
    static FontLoader *getInstance();

    void Initialize();

    void Dispose();

    void OnActiveSceneChange();

private:
    struct FontReplacementStrategy {
        vector<string> targetFontNames;
        vector<string> fontNamesToAdd;
    };

    static FontLoader *instance;

    static const vector<string> kFontNamesToRemove;
    static const vector<FontReplacementStrategy> kFontReplacementStrategies;

    vector<TMPro::TMP_FontAsset *> fallbackFontAssets{};
    vector<TMPro::TMP_FontAsset *> processedFontAssets{};

    Logger &logger = getLogger();

    void LoadFontAssets();

    void LoadFontAsset(UnityEngine::AssetBundle *assetBundle, const string &name);

    void ApplyFallbackFonts();

    void AddFallbacksToFont(TMPro::TMP_FontAsset *fontAsset, vector<TMPro::TMP_FontAsset *> &fallbacks);

    /**
    * SiraLocalizer.UI.FontAssetHelper.CopyFontAsset
    */
    static TMPro::TMP_FontAsset *
    CopyFontAsset(TMPro::TMP_FontAsset *original, UnityEngine::Material *referenceMaterial, const string &newName = "");

    bool sceneChangeSubbed = false;
    mutex sceneChangeMtx;
};

