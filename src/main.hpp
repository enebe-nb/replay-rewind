#pragma once
#include <filesystem>
#include <unordered_map>
#include <SokuLib.hpp>

extern std::unordered_map<void*, unsigned int> customDataSize;

#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif
