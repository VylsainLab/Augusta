#include <Augusta/Debug.h>
#include <imgui.h>

namespace aug
{
	bool Debug::m_bShowConsole;
	std::queue<LogEntry> Debug::m_qLog;
	std::unordered_map<std::string, std::vector<DebugEntry>> Debug::m_mDebugees;

	void Debug::RegisterDebugee(const char* szMenu, const char* szName, const std::function<void(void)> drawFunc)
	{
		m_mDebugees[szMenu].push_back({szName,drawFunc,false});
	}

	void Debug::DrawDebugees()
	{
		if (ImGui::BeginMainMenuBar())
		{
			for (auto& menu : m_mDebugees)
			{
				if (ImGui::BeginMenu(menu.first.c_str()))
				{
					for (auto& debugEntry : menu.second)
					{
						if (ImGui::MenuItem(std::get<DEBUG_ENTRY_NAME>(debugEntry).c_str()))
							std::get<DEBUG_ENTRY_ENABLED>(debugEntry) = true;
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndMainMenuBar();
		}		

		for (auto& menu : m_mDebugees)
			for (auto& debugEntry : menu.second)
			{
				if (std::get<DEBUG_ENTRY_ENABLED>(debugEntry))
				{
					ImGui::Begin(std::get<DEBUG_ENTRY_NAME>(debugEntry).c_str(), &std::get<DEBUG_ENTRY_ENABLED>(debugEntry));
					std::get<DEBUG_ENTRY_FUNCTION>(debugEntry)();
					ImGui::End();
				}
			}
	}

	void Debug::Log(const ELogType& type, const char* szEntry)
	{
		m_qLog.push({ type,szEntry });
		if (m_qLog.size() == LOG_DEPTH)
			m_qLog.pop();
	}

	void Debug::DrawConsole()
	{
		ImGui::Begin("Console");
		ImGui::End();
	}
}