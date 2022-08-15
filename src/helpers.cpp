#include "helpers.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

UnityEngine::TextAsset* makeTextAsset(StringW &s) {
    auto * textAsset = reinterpret_cast<UnityEngine::TextAsset*>(UnityEngine::TextAsset::New_ctor());

    //////////////
    textAsset->klass = classof(UnityEngine::TextAsset*); // Sc2ad told me to do this!
    // Sc2ad also said
    // !!!NEVER DO THIS Because that is a very risky thing to do in general!!!
    //////////////

    using TextAssetCreate = function_ptr_t<void, UnityEngine::TextAsset*, StringW>;
    static auto internal_create_instance = CRASH_UNLESS(reinterpret_cast<TextAssetCreate>(il2cpp_functions::resolve_icall("UnityEngine.TextAsset::Internal_CreateInstance")));

    internal_create_instance(textAsset, s);
    return textAsset;
}

UnityEngine::TextAsset* makeTextAsset(char* &c) {
    StringW s(c);
    return makeTextAsset(s);
}

UnityEngine::AssetBundle *loadAssetBundle(std::vector<uint8_t> &data) {
    using LoadFromMemory = function_ptr_t<UnityEngine::AssetBundle *, Array<uint8_t> *, unsigned int>;
    static LoadFromMemory loadFromMemory = reinterpret_cast<LoadFromMemory>(il2cpp_functions::resolve_icall(
            "UnityEngine.AssetBundle::LoadFromMemory_Internal"));

    Array<uint8_t> *dataArray = il2cpp_utils::vectorToArray(data);

    return loadFromMemory(dataArray, 0);
}

UnityEngine::AssetBundle *loadAssetBundle(uint8_t *start, size_t length) {
    std::vector<uint8_t> data;
    data.insert(data.end(), start, start + length * sizeof(char));
    return loadAssetBundle(data);
}