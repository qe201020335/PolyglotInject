using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using BGLib.Polyglot;
using HarmonyLib;
using IPA;
using IPA.Loader;
using IPA.Utilities;
using PolyglotInject.SiraLocalizer.Providers.Crowdin;
using PolyglotInject.SiraLocalizer.UI;
using UnityEngine;
using IPALogger = IPA.Logging.Logger;

namespace PolyglotInject
{
    [Plugin(RuntimeOptions.SingleStartInit)]
    public class Plugin
    {
        internal static Harmony HarmonyInstance { get; private set; } = new Harmony("com.github.qe201020335.PolyglotInject");
        
        internal static Plugin Instance { get; private set; }
        /// <summary>
        /// Use to send log messages through BSIPA.
        /// </summary>
        internal static IPALogger Log { get; private set; }

        internal string LocalicationString;

        private FontLoader? _fontLoader;

        [Init]
        public Plugin(IPALogger logger)
        {
            Instance = this;
            Log = logger;
            Plugin.Log.Info("OnApplicationStart");
            
            if (PluginManager.GetPluginFromId("SiraLocalizer") != null)
            {
                Log.Warn("SiraLocalizer is installed, not installing patches to avoid conflicts.");
                return;
            }
            using var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("PolyglotInject.assets.polyglot-inject.csv")!;
            using var reader = new StreamReader(stream);
            LocalicationString = reader.ReadToEnd();
            
            HarmonyInstance.PatchAll();
            _fontLoader = new FontLoader();
        }

        [OnEnable]
        public async Task OnEnable()
        {
            _fontLoader?.Initialize();

            var downloader = new CrowdinDownloader();
            Log.Info("Download SiraLocalizer localization assets.");
            await downloader.DownloadLocalizationsAsync(default);
            Log.Info("SiraLocalizer localization assets downloaded.");
        }

        [OnDisable]
        public void OnDisable()
        {
            _fontLoader?.Dispose();
        }
    }
    
    [HarmonyPatch(typeof(LocalizationAsyncInstaller), nameof(LocalizationAsyncInstaller.LoadResourcesBeforeInstall))]
    public class LocalizationAssetPatch
    {
        /// <summary>
        /// Match SiraLocalizer
        /// </summary>
        private static readonly Language[] AddedLanguages =
        [
            Language.Italian,
            Language.Portuguese_Brazil,
            Language.Russian,
            Language.Danish,
            Language.Swedish,
            Language.Dutch,
            Language.Simplified_Chinese,
            Language.Traditional_Chinese,
            Language.Czech,
            Language.Hungarian,
            Language.Hebrew
        ];

        [HarmonyPrefix]
        private static void Prefix(LocalizationAsyncInstaller __instance, ref IList<TextAsset> assets)
        {
            Plugin.Log.Info("Prefix");

            try
            {
                DumpLocalization(ref assets);
            }
            catch (Exception e)
            {
                Plugin.Log.Error($"Failed to dump localization assets: {e}");
            }
            
            assets.Add(new TextAsset(Plugin.Instance.LocalicationString));

            var supportedLanguages = __instance._mainPolyglotAsset.supportedLanguages;
            foreach (var language in AddedLanguages)
            {
                if (!supportedLanguages.Contains(language))
                {
                    supportedLanguages.Add(language);
                }
            }
        }

        private static void DumpLocalization(ref IList<TextAsset> assets)
        {
            var basePath = Path.Combine(UnityGame.UserDataPath, "PolyglotInject");
            if (!Directory.Exists(basePath))
            {
                Directory.CreateDirectory(basePath);
            }
            
            var count = 0;
            var merged = new StringBuilder();
            // The game's asset order isn't stable between runs, so put the main asset first by size.
            foreach (var asset in assets.OrderByDescending(asset => asset.text.Length))
            {
                count++;
                using var file = File.CreateText(Path.Combine(basePath, $"localization_asset_{count}.csv"));
                file.Write(asset.text);
                file.Close();

                // Not every asset ends with a newline, and the next one must start on its own row.
                merged.Append(asset.text);
                if (!asset.text.EndsWith("\n"))
                {
                    merged.Append('\n');
                }
            }

            // Input for scriptes/UpdateFromOld.py. Every asset repeats the same header rows, which the script dedupes by key.
            File.WriteAllText(Path.Combine(basePath, "beat-saber.csv"), merged.ToString());

            Plugin.Log.Info($"Dumped {count} localization assets.");
        }
    }
}
