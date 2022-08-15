#pragma once

#include "UnityEngine/TextAsset.hpp"
#include "UnityEngine/AssetBundle.hpp"

UnityEngine::TextAsset *makeTextAsset(StringW &s);

UnityEngine::TextAsset *makeTextAsset(char *&c);

UnityEngine::AssetBundle *loadAssetBundle(std::vector<uint8_t> &data);

UnityEngine::AssetBundle *loadAssetBundle(uint8_t *start, size_t length);