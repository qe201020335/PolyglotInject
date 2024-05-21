#include "helpers.h"


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
    return UnityEngine::TextAsset::New_ctor(s);
}