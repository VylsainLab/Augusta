#pragma once
#include <string>
#include <queue>
#include <unordered_map>
#include <memory>
#include <functional>

namespace aug
{
	//number of messages kept in the console
	#define LOG_DEPTH 100

	enum ELogType
	{
		LOG_TYPE_INFO,
		LOG_TYPE_WARNING,
		LOG_TYPE_ERROR,
		LOG_TYPE_COUNT
	};

	#define DEBUG_ENTRY_NAME		0
	#define DEBUG_ENTRY_FUNCTION	1
	#define DEBUG_ENTRY_ENABLED		2
	typedef std::tuple<std::string, std::function<void(void)>, bool> DebugEntry;
	class Debug
	{
	public:
		static void RegisterDebugee(const char* szMenu, const char* szName, const std::function<void(void)> drawFunc);

		static void DrawDebugees();

		static void Log(const ELogType& type, const char* szEntry);
		static void DrawConsole();
		static void ShowConsole(bool bShow) { m_bShowConsole = bShow; }

	protected:
		static bool m_bShowConsole;
		static std::queue<std::string> m_qLog;		
		static std::unordered_map<std::string, std::vector<DebugEntry>> m_mDebugees;
	};
}