#pragma once
#include <filesystem>

inline std::string GetRootDirectory()
{
    std::filesystem::path cwd = std::filesystem::current_path();
    return cwd.parent_path().parent_path().string() + "\\";
}