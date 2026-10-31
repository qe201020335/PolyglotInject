using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using IPA.Logging;
using IPA.Utilities;
using Newtonsoft.Json;
using PolyglotInject.SiraLocalizer.Utilities;
using UnityEngine.Networking;

namespace PolyglotInject.SiraLocalizer.Providers.Crowdin
{
    internal class CrowdinDownloader
    {
        private const string kCrowdinHost = "https://distributions.crowdin.net";
        private const string kDistributionKey = "b8d0ace786d64ba14775878o9lk";

        private static readonly string kDataFolder = Path.Combine(UnityGame.UserDataPath, "PolyglotInject");
        private static readonly string kLocalizationsFolder = Path.Combine(kDataFolder, "Localizations", "Downloaded");
        private static readonly string kDownloadedFolder = Path.Combine(kLocalizationsFolder, "Content");
        private static readonly string kManifestFilePath = Path.Combine(kLocalizationsFolder, "manifest.json");

        private static readonly Regex kValidPathRegex = new(@"^\/[A-Za-z\-_]+(?:\/[A-Za-z\-_]+)*\.csv$");

        private Logger _logger => Plugin.Log;

        public string name => "Crowdin";
        

        public async Task DownloadLocalizationsAsync(CancellationToken cancellationToken)
        {
            string manifestContent = await GetRemoteManifestContentAsync();

            if (manifestContent == null)
            {
                _logger.Error("Got empty manifest from Crowdin");
                return;
            }

            CrowdinDistributionManifest manifest = DeserializeManifest(manifestContent);

            if (manifest == null)
            {
                return;
            }

            // wipe existing files to avoid conflicts if names changed
            if (Directory.Exists(kDownloadedFolder))
            {
                Directory.Delete(kDownloadedFolder, true);
            }

            Directory.CreateDirectory(kDownloadedFolder);

            foreach (string filePath in manifest.files)
            {
                ParsedPathData parsed = ParsePath(filePath);

                await DownloadFileAsync(parsed.relativePath, manifest.timestamp, parsed.pathOnDisk);
            }

            using StreamWriter writer = new(kManifestFilePath);
            await writer.WriteAsync(manifestContent);
        }

        private CrowdinDistributionManifest DeserializeManifest(string manifestContent)
        {
            try
            {
                return JsonConvert.DeserializeObject<CrowdinDistributionManifest>(manifestContent);
            }
            catch (JsonException ex)
            {
                _logger.Error("Failed to deserialize manifest");
                _logger.Error(ex);

                return null;
            }
        }

        private async Task<string> GetRemoteManifestContentAsync()
        {
            string url = $"{kCrowdinHost}/{kDistributionKey}/manifest.json";

            _logger.Info($"Fetching Crowdin manifest");

            using var request = UnityWebRequest.Get(url);
            await request.SendWebRequest();

            if (request.result == UnityWebRequest.Result.ProtocolError)
            {
                _logger.Error($"'{url}' responded with {request.responseCode} ({request.error})");
                return null;
            }
            else if (request.result != UnityWebRequest.Result.Success)
            {
                _logger.Error($"Request to '{url}' failed: {request.result}");
                return null;
            }

            return request.downloadHandler.text;
        }

        private ParsedPathData ParsePath(string filePath)
        {
            if (!kValidPathRegex.IsMatch(filePath))
            {
                throw new ArgumentException($"Path '{filePath}' is invalid", nameof(filePath));
            }

            string relativePath = filePath.Substring(1);
            string pathOnDisk = Path.Combine(kDownloadedFolder, relativePath);
            string id = Path.ChangeExtension(relativePath, null);

            return new ParsedPathData
            {
                id = id,
                pathOnDisk = pathOnDisk,
                relativePath = relativePath,
            };
        }

        private readonly struct ParsedPathData
        {
            public string id { get; init; }

            public string pathOnDisk { get; init; }

            public string relativePath { get; init; }
        }

        private async Task<CrowdinDistributionManifest> ReadLocalManifestAsync()
        {
            try
            {
                using StreamReader reader = new(kManifestFilePath);
                return DeserializeManifest(await reader.ReadToEndAsync());
            }
            catch (IOException ex)
            {
                _logger.Error("Failed to read local manifest");
                _logger.Error(ex);

                return null;
            }
        }

        private async Task DownloadFileAsync(string relativePath, long timestamp, string filePath)
        {
            _logger.Info($"Downloading '{relativePath}'");

            string url = $"{kCrowdinHost}/{kDistributionKey}/content/{relativePath}?timestamp={timestamp}";
            using var request = UnityWebRequest.Get(url);
            await request.SendWebRequest();

            if (request.result == UnityWebRequest.Result.ProtocolError)
            {
                _logger.Error($"'{url}' responded with {request.responseCode} ({request.error})");
                return;
            }
            else if (request.result != UnityWebRequest.Result.Success)
            {
                _logger.Error($"Request to '{url}' failed: {request.result}");
                return;
            }

            Directory.CreateDirectory(Path.GetDirectoryName(filePath));

            byte[] data = request.downloadHandler.data;

            // depending on the Unity version UnityWebRequest might accept but not decompress gzipped data so check for magic bytes
            if (data[0] == 0x1f && data[1] == 0x8b)
            {
                using var memoryStream = new MemoryStream(data);
                using var gzipStream = new GZipStream(memoryStream, CompressionMode.Decompress);
                using var fileStream = new FileStream(filePath, FileMode.Create);
                await gzipStream.CopyToAsync(fileStream);
            }
            else
            {
                using var fileStream = new FileStream(filePath, FileMode.Create);
                await fileStream.WriteAsync(data, 0, data.Length);
            }
        }
    }
}
