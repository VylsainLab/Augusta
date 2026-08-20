#include <Augusta/Debug.h>
#include <imgui.h>
#include <format>
#include <iostream>

#define LOG_PREVIEW_LENGTH 64

namespace aug
{
	bool Debug::m_bShowConsole;
	std::deque<LogEntry> Debug::m_dqLog;
	std::unordered_map<std::string, std::vector<DebugEntry>> Debug::m_mDebugees;
	std::ofstream Debug::m_LogFile;

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
		{
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
	}

	void Debug::Log(const ELogType& type, const std::string& strEntry)
	{
		m_dqLog.push_back({ type,strEntry });
		if (m_dqLog.size() > LOG_DEPTH)
			m_dqLog.pop_front();

		//log to standard output
		std::string strOutput;
		switch (type)
		{
		case LOG_TYPE_INFO:
			strOutput = std::format("\n\nINFO: {}", strEntry);
			std::cout << strOutput;
			break;
		case LOG_TYPE_WARNING:
			strOutput = std::format("\n\nWARNING: {}", strEntry);
			std::cout << strOutput;
			break;
		case LOG_TYPE_ERROR:
			strOutput = std::format("\n\nERROR: {}", strEntry);
			std::cerr << strOutput;
			break;
		case LOG_TYPE_VERBOSE:
			strOutput = std::format("\n\nVERBOSE: {}", strEntry);
			std::cerr << strOutput;
			break;
		}

		//log to file
		if (!m_LogFile.is_open())
		{
			m_LogFile.open("Augusta.log", std::ios_base::out);
		}

		if (!strOutput.empty())
		{
			m_LogFile << strOutput;
		}
	}

	void Debug::DrawConsole()
	{
		ImGui::Begin("Console");

		ImVec2 size(0, ImGui::GetTextLineHeightWithSpacing() * 15);
		if (ImGui::BeginTable("LogTable", 2, ImGuiTableFlags_RowBg| ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, size))
		{
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed,64);
			ImGui::TableSetupColumn("Message");
			ImGui::TableHeadersRow();

			static int32_t iSelectedEntry = -1;
			uint32_t uiCount = 0;
			for (auto& entry : m_dqLog)
			{
				if (entry.first == LOG_TYPE_VERBOSE)
					continue;

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
				
				bool bIsSelected = iSelectedEntry==uiCount;
				std::string strID = std::format("ID{}", uiCount);
				ImGui::PushID(strID.c_str());
				ImGui::Selectable(strPreview.c_str(), &bIsSelected);
				if(bIsSelected)
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