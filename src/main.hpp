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
};

extern std::filesystem::path modulePath;
extern std::unordered_map<void*, GOHeader> gameObject35C;

#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif
