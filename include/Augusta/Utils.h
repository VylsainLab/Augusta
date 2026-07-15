#ifndef _UTILS
#define _UTILS
#include <filesystem>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

inline std::string GetRootDirectory()
{
    std::filesystem::path cwd = std::filesystem::current_path();
    return cwd.parent_path().parent_path().string() + "\\";
}

inline std::string ReplaceString(const std::string& str, const char* szSearch, const char* szReplace)
{
	std::string ret = str;
	std::string::size_type pos = 0u;
	while ((pos = ret.find(szSearch, pos)) != std::string::npos) {
		ret.replace(pos, strlen(szSearch), szReplace);
		pos += strlen(szReplace);
	}
	return ret;
}

#endif