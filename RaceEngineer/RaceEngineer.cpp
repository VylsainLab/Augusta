#include "RaceEngineer.h"

#define SL(x) x;ImGui::SameLine();

void RaceEngineer::Render(VkCommandBuffer commandBuffer)
{
    m_IRModel.Update();

    ImGui::ShowDemoWindow();

    ImGui::Begin("Debug String");    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail()), ImGuiChildFlags_None, window_flags);
    ImGui::TextUnformatted(m_IRModel.m_strSessionYaml.data(),m_IRModel.m_strSessionYaml.data() + m_IRModel.m_strSessionYaml.size());
    ImGui::EndChild();
    ImGui::End();    

    ImGui::Begin("Race Engineer");

    if (ImGui::BeginTabBar("Mode", ImGuiTabBarFlags_DrawSelectedOverline))
    {
        if (ImGui::BeginTabItem("Live Telemetry"))
        {
            ImGuiID telemetryDS = ImGui::GetID("TelemetryDS");
            ImGui::DockSpace(telemetryDS);
            DrawTelemetry();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Race Debrief"))
        {
            DrawDebrief();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void RaceEngineer::DrawTelemetry()
{
    DrawSession();

    DrawStandings();

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("Weather");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("FuelCalculator");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("TrackMap");
    ImGui::End();
}

void RaceEngineer::DrawSession()
{
    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);

    ImGui::Begin("Session");
    SL(ImGui::Text("Status:"))
    if (m_IRModel.m_bConnected)
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");

    switch (m_IRModel.m_eSessionType)
    {
    case PRACTICE:
        SL(ImGui::Text("PRACTICE:"))
        break;
    case QUALI:
        SL(ImGui::Text("QUALIFYING:"))
        break;
    case RACE:
        SL(ImGui::Text("RACE:"))
        break;
    default:
        SL(ImGui::Text("UNKNOWN:"))
        break;
    }

    float time = m_IRModel.GetFloat("SessionTime");
    int h = time / 3600;
    int m = fmod(time / 60,60);
    int s = fmod(time, 60);
    float total = m_IRModel.GetFloat("SessionTimeTotal");
    int th = total / 3600;
    int tm = fmod(total / 60,60);
    ImGui::Text("%02d:%02d:%02d / %02d:%02dh", h,m,s, th,tm );

    SL(ImGui::Text("DRIVERS: %d",m_IRModel.m_mDrivers.size()))
    ImGui::End();
}

void RaceEngineer::DrawStandings()
{
    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("Standings");

    ImGui::BeginTable("Standings", 5, ImGuiTableFlags_RowBg);

    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Number", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Licence", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("iRating", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
    ImGui::TableHeadersRow();

    for (auto driver : m_IRModel.m_mDrivers)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", driver.first);

        ImGui::TableNextColumn();
        ImGui::Text("%d", driver.second.uiCarNumber);

        ImGui::TableNextColumn();
        ImGui::Text("%s", driver.second.szName.c_str());

        ImGui::TableNextColumn();
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(ImVec4(driver.second.aLicColor[0], driver.second.aLicColor[1], driver.second.aLicColor[2], driver.second.aLicColor[3])));
        ImGui::Text("%s", driver.second.strLicence.c_str());

        ImGui::TableNextColumn();
        ImGui::Text("%.02fk", float(driver.second.uiIRating)/1000);        
    }
    ImGui::EndTable();

    ImGui::End();
}

void RaceEngineer::DrawDebrief()
{
}