#include <Augusta/Debug.h>
#include <imgui.h>
#include <format>

#define LOG_PREVIEW_LENGTH 64

namespace aug
{
	bool Debug::m_bShowConsole;
	std::deque<LogEntry> Debug::m_dqLog;
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

	void Debug::Log(const ELogType& type, const std::string& strEntry)
	{
		m_dqLog.push_back({ type,strEntry });
		if (m_dqLog.size() > LOG_DEPTH)
			m_dqLog.pop_front();
	}

	void Debug::DrawConsole()
	{
		ImGui::Begin("Console");

		if (ImGui::BeginTable("LogTable", 2, ImGuiTableFlags_RowBg| ImGuiTableFlags_Borders))
		{
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed,64);
			ImGui::TableSetupColumn("Message");
			ImGui::TableHeadersRow();

			//static LogEntry* pSelectedEntry = nullptr;
			int32_t iSelectedEntry = -1;
			uint32_t uiCount = 0;
			for (auto& entry : m_dqLog)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				switch (entry.first)
				{
				case LOG_TYPE_ERROR:
					ImGui::TextColored(ImVec4(1., 0., 0., 1.), "Error");
					break;
				case LOG_TYPE_WARNING:
					ImGui::TextColored(ImVec4(1., 1., 0., 1.), "Warning");
					break;
				case LOG_TYPE_INFO:
					ImGui::TextColored(ImVec4(1., 1., 1., 1.), "Info");
					break;
				default:
					break;
				}
					
				ImGui::TableNextColumn();
				std::string strPreview = entry.second;
				if(entry.second.length()> LOG_PREVIEW_LENGTH)
					strPreview = entry.second.substr(0, LOG_PREVIEW_LENGTH) + "...";
				//ImGui::Text(strPreview.c_str());
				bool bIsSelected = iSelectedEntry==uiCount;
				std::string strID = std::format("ID%d", uiCount);
				ImGui::PushID(strID.c_str());
				if(ImGui::Selectable(strPreview.c_str(), &bIsSelected))
					iSelectedEntry = uiCount;
				ImGui::PopID();

				uiCount++;
			}

			ImGui::EndTable();

			ImGui::BeginChild("Details", ImVec2(0,250), ImGuiChildFlags_Borders);
			if (iSelectedEntry>=0 && iSelectedEntry<m_dqLog.size())
			{
				ImGui::TextWrapped(m_dqLog.at(iSelectedEntry).second.c_str());
			}
			ImGui::EndChild();
		}

		ImGui::End();
	}
}