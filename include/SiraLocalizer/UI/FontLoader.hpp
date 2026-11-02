#pragma once

#include "TMPro/TMP_FontAsset.hpp"
#include "UnityEngine/AssetBundle.hpp"

#include <functional>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace PolyglotInject::SiraLocalizer::UI {
    class FontLoader {
       public:
        void Initialize();
        void OnSceneLoaded();

       private:
        void LoadFontAssetsAsync();
        void LoadFallbackFontsAsync(std::function<void()> onLoaded);
        void LoadFontAsset(UnityEngine::AssetBundle* assetBundle, std::string_view name);
        void ApplyFallbackFonts();
        void AddFallbacksToFont(TMPro::TMP_FontAsset* fontAsset, std::vector<UnityW<TMPro::TMP_FontAsset>> const& fallbacks);

        std::vector<UnityW<TMPro::TMP_FontAsset>> _fallbackFontAssets;
        // Instance ids, because the address of a destroyed font can be reused.
        std::unordered_set<int> _processedFontAssets;
    };
}
