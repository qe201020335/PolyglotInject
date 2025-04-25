#include "helpers.h"

UnityEngine::TextAsset* makeTextAsset(char* &c) {
    StringW s(c);
    return UnityEngine::TextAsset::New_ctor(s);
}