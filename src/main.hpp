#pragma once
#include <filesystem>
#include <unordered_map>
#include <SokuLib.hpp>

struct GOHeader {
    void* owner;
    SokuLib::Action actionId;
    float x, y;
    SokuLib::Direction dir;
    unsigned char layerMaybe;
    size_t size;
    int parentIndex;
};

extern std::filesystem::path modulePath;
extern std::unordered_map<void*, GOHeader> goHeaders;

#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif
