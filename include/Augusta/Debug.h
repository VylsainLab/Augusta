#pragma once
#include <string>
#include <deque>
#include <unordered_map>
#include <memory>
#include <functional>
#include <fstream>

namespace aug
{
	//number of messages kept in the console
	#define LOG_DEPTH 1024

	enum ELogType
	{
		LOG_TYPE_INFO,
		LOG_TYPE_WARNING,
		LOG_TYPE_ERROR,
		LOG_TYPE_VERBOSE,
		LOG_TYPE_COUNT
	};

	struct SDebugEntry
	{
		std::string _strName;
		std::function<void(void)> _drawFunc;
		bool _bEnabled;
	};

	typedef std::pair<ELogType,std::string> LogEntry;

	class Debug
	{
	public:
		static void RegisterDebugee(const char* szMenu, const char* szName, const std::function<void(void)> drawFunc);

		static void DrawDebugees();

		static void Log(const ELogType& type, const std::string& strEntry);
		static void DrawConsole();
		static void ShowConsole(bool bShow) { m_bShowConsole = bShow; }

	protected:
		static bool m_bShowConsole;
		static std::deque<LogEntry> m_dqLog;
		static std::unordered_map<std::string, std::vector<SDebugEntry>> m_mDebugees;
		static std::ofstream m_LogFile;
	};
}