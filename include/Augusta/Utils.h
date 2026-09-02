#pragma once

#include <filesystem>
#include <glm/glm.hpp>

#define PI       3.14159265358979323846f

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

inline void GetDirectionFromAzimuthElevation(const float& fAzimuth, const float& fElevation, glm::vec3& dir)
{
	float radElevation = fElevation * PI / 180.f;
	float cosElevation = std::cos(radElevation);

	float radAzimuth = fAzimuth * PI / 180.f;
	dir.x = cosElevation * std::cos(radAzimuth);
	dir.z = cosElevation * std::sin(radAzimuth);

	dir.y = std::sin(radElevation);
}

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