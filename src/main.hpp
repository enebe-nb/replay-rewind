#pragma once
#include <filesystem>
#include <unordered_map>

extern std::filesystem::path modulePath;
extern std::unordered_map<void*, size_t> gameObject35C;

#ifdef _DEBUG
#include <fstream>
extern std::ofstream logging;
#endif
