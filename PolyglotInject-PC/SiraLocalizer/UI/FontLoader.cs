using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using PolyglotInject.SiraLocalizer.Utilities;
using TMPro;
using UnityEngine;
using UnityEngine.SceneManagement;
using Object = UnityEngine.Object;

namespace PolyglotInject.SiraLocalizer.UI
{
    internal class FontLoader : IDisposable
    {
        private static readonly string[] kFontNamesToRemove = ["NotoSansJP-Medium SDF", "NotoSansKR-Medium SDF", "SourceHanSansCN-Bold-SDF-Common-1(2k)", "SourceHanSansCN-Bold-SDF-Common-2(2k)", "SourceHanSansCN-Bold-SDF-Uncommon(2k)"];
        private static readonly FontReplacementStrategy[] kFontReplacementStrategies =
        [
            new FontReplacementStrategy(
                ["Teko-Medium SDF"],
                ["Teko-Medium SDF Latin-1 Supplement", "Oswald-Medium SDF Cyrillic", "NotoSansHebrew-Medium SDF", "SourceHanSans-Medium SDF"]),
            new FontReplacementStrategy(
                ["Teko-Bold SDF"],
                ["Teko-Bold SDF Latin-1 Supplement", "Oswald-Bold SDF Cyrillic", "NotoSansHebrew-Bold SDF", "SourceHanSans-Medium SDF"]),
        ];

        private readonly List<TMP_FontAsset> _fallbackFontAssets = [];
        private readonly List<TMP_FontAsset> _createdFontAssets = [];
        private readonly List<TMP_FontAsset> _processedFontAssets = [];

        public async void Initialize()
        {
            SceneManager.sceneLoaded += OnSceneLoaded;

            try
            {
                await LoadFontAssetsAsync();
            }
            catch (Exception ex)
            {
                Plugin.Log.Error(ex);
            }
        }

        public void Dispose()
        {
            SceneManager.sceneLoaded -= OnSceneLoaded;

            foreach (TMP_FontAsset fontAsset in _processedFontAssets)
            {
                fontAsset.fallbackFontAssetTable.RemoveAll(f => _createdFontAssets.Contains(f) || _fallbackFontAssets.Contains(f));
            }

            foreach (TMP_FontAsset fontAsset in _createdFontAssets)
            {
                Object.Destroy(fontAsset);
            }

            foreach (TMP_FontAsset fontAsset in _fallbackFontAssets)
            {
                Object.Destroy(fontAsset);
            }

            _processedFontAssets.Clear();
            _createdFontAssets.Clear();
            _fallbackFontAssets.Clear();
        }

        private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
        {
            ApplyFallbackFonts();
        }

        private async Task LoadFontAssetsAsync()
        {
            Plugin.Log.Info("Loading fonts");

            await LoadFallbackFontsAsync();

            ApplyFallbackFonts();
        }

        private async Task LoadFallbackFontsAsync()
        {
            using Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("PolyglotInject.SiraLocalizer.Resources.Assets")!;
            AssetBundleCreateRequest assetBundleCreateRequest = await AssetBundle.LoadFromStreamAsync(stream);
            AssetBundle assetBundle = assetBundleCreateRequest.assetBundle;

            if (assetBundle == null)
            {
                Plugin.Log.Error("Failed to load fonts asset bundle; some characters may not display as expected");
                return;
            }

            await assetBundle.LoadAllAssetsAsync();

            foreach (string fontName in kFontReplacementStrategies.SelectMany(s => s.fontNamesToAdd).Distinct())
            {
                LoadFontAsset(assetBundle, fontName);
            }

            await assetBundle.UnloadAsync(false);
        }

        private void LoadFontAsset(AssetBundle assetBundle, string name)
        {
            // this is basically instant since we've called LoadAllAssetsAsync previously
            TMP_FontAsset fontAsset = assetBundle.LoadAsset<TMP_FontAsset>(name);

            if (fontAsset == null)
            {
                Plugin.Log.Error($"Font '{name}' could not be loaded; some characters may not display as expected");
                return;
            }

            Plugin.Log.Info($"Font '{name}' loaded successfully");

            _fallbackFontAssets.Add(fontAsset);
        }

        private void ApplyFallbackFonts()
        {
            if (!_fallbackFontAssets.Any()) return;

            TMP_FontAsset[] fontAssets = Resources.FindObjectsOfTypeAll<TMP_FontAsset>();

            foreach (FontReplacementStrategy strategy in kFontReplacementStrategies)
            {
                IEnumerable<TMP_FontAsset> originalFontAssets = fontAssets.Where(f => !_processedFontAssets.Contains(f) && strategy.targetFontNames.Contains(f.name));

                foreach (TMP_FontAsset fontAsset in originalFontAssets)
                {
                    AddFallbacksToFont(fontAsset, strategy.fontNamesToAdd.Select(n => _fallbackFontAssets.Find(f => f.name == n)).Where(f => f != null));
                }
            }

            // force update any text that has already rendered
            foreach (TMP_Text text in Object.FindObjectsByType<TMP_Text>(FindObjectsSortMode.None))
            {
                text.SetAllDirty();
            }
        }

        private void AddFallbacksToFont(TMP_FontAsset fontAsset, IEnumerable<TMP_FontAsset> fallbacks)
        {
            Plugin.Log.Info($"Adding fallbacks to '{fontAsset.name}' ({fontAsset.GetInstanceID()})");

            fontAsset.fallbackFontAssetTable.RemoveAll(f => kFontNamesToRemove.Contains(f.name));

            foreach (TMP_FontAsset fallback in fallbacks.Reverse())
            {
                // creating a copy is necessary to prevent fonts from overwriting each others' atlases
                TMP_FontAsset fallbackCopy = FontAssetHelper.CopyFontAsset(fallback, fontAsset.material);
                _createdFontAssets.Add(fallbackCopy);

                // insert as first possible fallback font
                fontAsset.fallbackFontAssetTable.Insert(0, fallbackCopy);
            }

            _processedFontAssets.Add(fontAsset);
        }

        private record FontReplacementStrategy(string[] targetFontNames, string[] fontNamesToAdd);
    }
}
