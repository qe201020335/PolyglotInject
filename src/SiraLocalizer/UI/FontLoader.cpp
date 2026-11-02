#include "SiraLocalizer/UI/FontLoader.hpp"

#include "main.hpp"

#include "beatsaber-hook/shared/utils/typedefs-wrappers.hpp"
#include "metacore/shared/unity.hpp"

#include "System/Collections/Generic/List_1.hpp"
#include "TMPro/TMP_Text.hpp"
#include "UnityEngine/AssetBundleCreateRequest.hpp"
#include "UnityEngine/AssetBundleRequest.hpp"
#include "UnityEngine/AssetBundleUnloadOperation.hpp"
#include "UnityEngine/FindObjectsSortMode.hpp"
#include "UnityEngine/HideFlags.hpp"
#include "UnityEngine/Resources.hpp"

#include <algorithm>
#include <set>
#include <string>

using namespace TMPro;
using namespace UnityEngine;

namespace PolyglotInject::SiraLocalizer::UI {
    struct FontReplacementStrategy {
        std::vector<std::string_view> targetFontNames;
        std::vector<std::string_view> fontNamesToAdd;
    };

    static constexpr auto kFontBundlePath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/PolyglotInject/fonts.bundle";

    static std::vector<std::string_view> const kFontNamesToRemove = {
        "NotoSansJP-Medium SDF",
        "NotoSansKR-Medium SDF",
        "SourceHanSansCN-Bold-SDF-Common-1(2k)",
        "SourceHanSansCN-Bold-SDF-Common-2(2k)",
        "SourceHanSansCN-Bold-SDF-Uncommon(2k)",
    };
    static std::vector<FontReplacementStrategy> const kFontReplacementStrategies = {
        {{"Teko-Medium SDF"},
         {"Teko-Medium SDF Latin-1 Supplement", "Oswald-Medium SDF Cyrillic", "NotoSansHebrew-Medium SDF", "SourceHanSans-Medium SDF"}},
        {{"Teko-Bold SDF"},
         {"Teko-Bold SDF Latin-1 Supplement", "Oswald-Bold SDF Cyrillic", "NotoSansHebrew-Bold SDF", "SourceHanSans-Medium SDF"}},
    };

    // Stands in for await: runs next on the main thread once the operation is done.
    template <typename T>
    static void Await(T* operation, std::function<void(T*)> next) {
        // The GC can't see into these lambdas, so the operation needs a handle to survive until then.
        SafePtr<T> handle(operation);
        MetaCore::Engine::ScheduleMainThread([handle]() { return handle->isDone; }, [handle, next = std::move(next)]() {
            try {
                next(handle.ptr());
            } catch (std::exception const& e) {
                logger.error("Failed to load fonts: {}", e.what());
            }
        });
    }

    void FontLoader::Initialize() {
        MetaCore::Engine::ScheduleMainThread([this]() { LoadFontAssetsAsync(); });
    }

    void FontLoader::OnSceneLoaded() {
        try {
            ApplyFallbackFonts();
        } catch (std::exception const& e) {
            logger.error("Failed to apply fallback fonts: {}", e.what());
        }
    }

    void FontLoader::LoadFontAssetsAsync() {
        logger.info("Loading fonts");

        LoadFallbackFontsAsync([this]() { ApplyFallbackFonts(); });
    }

    void FontLoader::LoadFallbackFontsAsync(std::function<void()> onLoaded) {
        Await<AssetBundleCreateRequest>(AssetBundle::LoadFromFileAsync(kFontBundlePath), [this, onLoaded](auto* assetBundleCreateRequest) {
            UnityW<AssetBundle> loadedAssetBundle = assetBundleCreateRequest->assetBundle;

            if (!loadedAssetBundle) {
                logger.error("Failed to load fonts asset bundle; some characters may not display as expected");
                return;
            }

            SafePtrUnity<AssetBundle> assetBundle(loadedAssetBundle.ptr());

            Await<AssetBundleRequest>(assetBundle->LoadAllAssetsAsync(), [this, assetBundle, onLoaded](auto*) {
                std::set<std::string_view> fontNames;
                for (auto const& strategy : kFontReplacementStrategies) {
                    fontNames.insert(strategy.fontNamesToAdd.begin(), strategy.fontNamesToAdd.end());
                }

                for (auto fontName : fontNames) {
                    LoadFontAsset(assetBundle.ptr(), fontName);
                }

                Await<AssetBundleUnloadOperation>(assetBundle->UnloadAsync(false), [onLoaded](auto*) { onLoaded(); });
            });
        });
    }

    void FontLoader::LoadFontAsset(AssetBundle* assetBundle, std::string_view name) {
        // this is basically instant since we've called LoadAllAssetsAsync previously
        UnityW<Object> asset = assetBundle->LoadAsset(std::string(name), csTypeOf(TMP_FontAsset*));

        if (!asset) {
            logger.error("Font '{}' could not be loaded; some characters may not display as expected", name);
            return;
        }

        logger.info("Font '{}' loaded successfully", name);

        // Resources.UnloadUnusedAssets can't see this list either, and the fonts may not be in a fallback table yet.
        asset->hideFlags = HideFlags::DontUnloadUnusedAsset;
        _fallbackFontAssets.emplace_back(il2cpp_utils::cast<TMP_FontAsset>(asset.ptr()));
    }

    void FontLoader::ApplyFallbackFonts() {
        if (_fallbackFontAssets.empty()) {
            return;
        }

        auto fontAssets = Resources::FindObjectsOfTypeAll<TMP_FontAsset*>();

        for (auto const& strategy : kFontReplacementStrategies) {
            std::vector<UnityW<TMP_FontAsset>> fallbacks;
            for (auto name : strategy.fontNamesToAdd) {
                auto fallback = std::ranges::find_if(_fallbackFontAssets, [name](auto f) { return std::string(f->name) == name; });
                if (fallback != _fallbackFontAssets.end()) {
                    fallbacks.push_back(*fallback);
                }
            }

            for (auto fontAsset : fontAssets) {
                if (!_processedFontAssets.contains(fontAsset->GetInstanceID()) &&
                    std::ranges::contains(strategy.targetFontNames, std::string(fontAsset->name))) {
                    AddFallbacksToFont(fontAsset, fallbacks);
                }
            }
        }

        // force update any text that has already rendered
        for (auto text : Object::FindObjectsByType(csTypeOf(TMP_Text*), FindObjectsSortMode::None)) {
            il2cpp_utils::cast<TMP_Text>(text.ptr())->SetAllDirty();
        }
    }

    void FontLoader::AddFallbacksToFont(TMP_FontAsset* fontAsset, std::vector<UnityW<TMP_FontAsset>> const& fallbacks) {
        logger.info("Adding fallbacks to '{}' ({})", fontAsset->name, fontAsset->GetInstanceID());

        auto fallbackFontAssetTable = fontAsset->fallbackFontAssetTable;

        for (int i = fallbackFontAssetTable->Count - 1; i >= 0; i--) {
            auto fallback = fallbackFontAssetTable->get_Item(i);
            if (fallback && std::ranges::contains(kFontNamesToRemove, std::string(fallback->name))) {
                fallbackFontAssetTable->RemoveAt(i);
            }
        }

        for (int i = 0; i < fallbacks.size(); i++) {
            fallbackFontAssetTable->Insert(i, fallbacks[i]);
        }

        _processedFontAssets.insert(fontAsset->GetInstanceID());
    }
}
