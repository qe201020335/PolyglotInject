using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using BGLib.Polyglot;
using HarmonyLib;
using IPA;
using IPA.Utilities;
using PolyglotInject.SiraLocalizer.Providers.Crowdin;
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

        [Init]
        public Plugin(IPALogger logger)
        {
            Instance = this;
            Log = logger;
            Plugin.Log.Info("OnApplicationStart");
            
            using var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("PolyglotInject.assets.polyglot-inject.csv")!;
            using var reader = new StreamReader(stream);
            LocalicationString = reader.ReadToEnd();
            
            HarmonyInstance.PatchAll();
        }

        [OnEnable]
        public async Task OnEnable()
        {
            var downloader = new CrowdinDownloader();
            Log.Info("Download SiraLocalizer localization assets.");
            await downloader.DownloadLocalizationsAsync(default);
            Log.Info("SiraLocalizer localization assets downloaded.");
        }
    }
    
    [HarmonyPatch(typeof(LocalizationAsyncInstaller), nameof(LocalizationAsyncInstaller.LoadResourcesBeforeInstall))]
    public class LocalizationAssetPatch
    {
        
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
            __instance._mainPolyglotAsset.supportedLanguages.Add(Language.Simplified_Chinese);
        }

        private static void DumpLocalization(ref IList<TextAsset> assets)
        {
            var basePath = Path.Combine(UnityGame.UserDataPath, "PolyglotInject");
            if (!Directory.Exists(basePath))
            {
                Directory.CreateDirectory(basePath);
            }
            
            var count = 0;
            foreach (var asset in assets)
            {
                count++;
                using var file = File.CreateText(Path.Combine(basePath, $"localization_asset_{count}.csv"));
                file.Write(asset.text);
                file.Close();
            }
            
            Plugin.Log.Info($"Dumped {count} localization assets.");
        }
    }
}
