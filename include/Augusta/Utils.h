#ifndef _UTILS
#define _UTILS
#include <filesystem>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

inline std::string GetRootDirectory()
{
    std::filesystem::path cwd = std::filesystem::current_path();
    return cwd.parent_path().parent_path().string() + "\\";
}

#endif