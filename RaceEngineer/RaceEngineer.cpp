#include "RaceEngineer.h"
#include "Nations.h"
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>

#define SL(x) x;ImGui::SameLine();

RaceEngineer::RaceEngineer(const std::string& name, uint16_t width, uint16_t height, bool bResizable, bool bVisible)
    : aug::Application(name, width, height, bResizable, bVisible)
{
    InitImGui();

    ImGuiIO& io = ImGui::GetIO();
    m_pBodyFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Calibri.ttf", 18, NULL, io.Fonts->GetGlyphRangesDefault());

    static ImWchar ranges[] = { 0x1, 0x1FFFF, 0 };
    static ImFontConfig cfg;
    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.MergeMode = false;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    m_pEmojiFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.ttf", 24.0f, &cfg, ranges);

    cfg.GlyphOffset.y = -5;
    m_pFlagFont = io.Fonts->AddFontFromFileTTF("D:\\Git\\Dependencies\\Fonts\\flags color world.ttf", 24.0f, &cfg, ranges);
}

void RaceEngineer::Render(VkCommandBuffer commandBuffer)
{
    m_IRModel.Update();
    sSession& session = m_IRModel.m_sSessionData;

    ImGui::PushFont(m_pBodyFont);
    ImGui::ShowDemoWindow();

    ImGui::Begin("Debug String");    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail()), ImGuiChildFlags_None, window_flags);
    ImGui::TextUnformatted(session._strSessionYaml.data(),session._strSessionYaml.data() + session._strSessionYaml.size());
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
    ImGui::PopFont();
}

void RaceEngineer::DrawTelemetry()
{
    DrawSession();

    DrawStandings();

    //DrawWeather();

    DrawFuelCalculator();

    DrawTrackMap();
}

void RaceEngineer::DrawSession()
{
    sSession& session = m_IRModel.m_sSessionData;

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);

    ImGui::Begin("Session");
    SL(ImGui::Text("Status:"))
    if (m_IRModel.m_bConnected)
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");

    switch (session._eSessionType)
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

    int h = session._fSessionTime / 3600;
    int m = fmod(session._fSessionTime / 60,60);
    int s = fmod(session._fSessionTime, 60);
    int th = session._fSessionTimeTotal / 3600;
    int tm = fmod(session._fSessionTimeTotal / 60,60);
    ImGui::Text("%02d:%02d:%02d / %02d:%02dh", h,m,s, th,tm );

    ImGui::Text("DRIVERS: %d", session._mDrivers.size());

    SL(ImGui::Text("TIRES: "));
    ImGui::PushFont(m_pEmojiFont);
    for (char &c : session._strAvailableTires)
    {
        switch (c)
        {
        case 'H':
            SL(ImGui::TextColored(ImVec4(1., 1., 1., 1.), u8"\u25ce"));
            break;
        case 'M':
            SL(ImGui::TextColored(ImVec4(1., 1., 0., 1.), u8"\u25ce"));
            break;
        case 'S':
            SL(ImGui::TextColored(ImVec4(1., 0., 0., 1.), u8"\u25ce"));
            break;
        case 'W':
            SL(ImGui::TextColored(ImVec4(0.5, 0.5, 1., 1.), u8"\u25ce"));
            break;
        default:
            break;
        }
    }
    ImGui::PopFont();

    ImGui::NewLine();

    static float fTick = 0;
    float prevTick = fTick;
    ImGui::SliderFloat("Tick", &fTick, 0., 1.);
    if(prevTick!=fTick)
        m_IRModel.m_DiskReader.ReadTickData(fTick * m_IRModel.m_DiskReader.m_uiNbTicks);

    ImGui::End();
}

void RaceEngineer::DrawStandings()
{
    sSession& session = m_IRModel.m_sSessionData;

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("Standings");

    ImGui::BeginTable("Standings", 7, ImGuiTableFlags_RowBg);

    ImGui::TableSetupColumn("Pos", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Licence", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("iRating", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Fastest", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Last", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
    ImGui::TableHeadersRow();

    ImVec4 fastest(0.75,0.,0.75,1.);
    ImVec4 other(1.,1.,1.,1.);
    for (auto driver : session._aPositions)
    {
        if (driver == nullptr)
            continue;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", driver->_uiPosition);

        ImGui::TableNextColumn();
        ImGui::Text("%d", driver->_uiCarNumber);

        ImGui::TableNextColumn();
        if (!mFlags[driver->_strCountry].empty())
        {
            ImGui::PushFont(m_pFlagFont);
            SL(ImGui::Text(mFlags[driver->_strCountry].c_str()))
            ImGui::PopFont();
        }
        else
        {
            ImGui::PushFont(m_pEmojiFont);
            SL(ImGui::Text(u8"\u2753"))
            ImGui::PopFont();
            //ImGui::Text(driver.second.strCountry.c_str());
        }

        ImGui::Text("%s", driver->_strName.c_str());

        ImGui::TableNextColumn();
        ImVec4 color(driver->_aLicColor[0], driver->_aLicColor[1], driver->_aLicColor[2], driver->_aLicColor[3]);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(color));
        bool bLight = (color.x + color.y + color.z) < 1.5;
        ImGui::TextColored( ImVec4(bLight,bLight,bLight,1.0f), "%s", driver->_strLicence.c_str());

        ImGui::TableNextColumn();
        ImGui::Text("%.02fk", float(driver->_uiIRating)/1000);

        ImGui::TableNextColumn();
        if(driver->_fFastestLap>0.)
            ImGui::TextColored(driver->_uiPosition==1?fastest:other, "%d:%.03f", int(driver->_fFastestLap / 60), fmod(driver->_fFastestLap, 60.));

        ImGui::TableNextColumn();
        if (driver->_fLastLap > 0.)
            ImGui::Text("%d:%.03f", int(driver->_fLastLap/60), fmod(driver->_fLastLap,60.));
    }
    ImGui::EndTable();

    ImGui::End();
}

void RaceEngineer::DrawWeather()
{
    sSession& session = m_IRModel.m_sSessionData;

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("Weather");
    ImGui::Text(u8"Track temp: %.02f\u00B0C", session._sWeather._fTrackTemp);
    ImGui::Text(u8"Air temp: %.02f\u00B0C", session._sWeather._fAirTemp);
    ImGui::Text(u8"Wind: %.02fm/s %.0f\u00B0", session._sWeather._fWindSpeed, session._sWeather._fWindDirection);
    ImGui::Text(u8"Rain: %.0f\u0025", session._sWeather._fRainProbablility);
    ImGui::End();
}

ImVec2 RaceEngineer::GetPosFromRadiusAzimuth(const ImVec2 &c, const float& r, const float& a)
{
    float x = c.x + r * cos(a);
    float y = c.y + r * sin(a);
    return ImVec2(x,y);
}

void RaceEngineer::DrawTrackMap()
{
    sSession& session = m_IRModel.m_sSessionData;

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("TrackMap");    

    ImVec2 draw_area = ImGui::GetContentRegionAvail();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(cursor.x + draw_area.x * 0.5f, cursor.y + draw_area.y * 0.5f);    

    ImGui::Text(u8"Track temp: %.02f\u00B0C", session._sWeather._fTrackTemp);
    ImGui::Text(u8"Air temp: %.02f\u00B0C", session._sWeather._fAirTemp);
    ImGui::Text(u8"Wind: %.02fm/s %.0f\u00B0", session._sWeather._fWindSpeed, session._sWeather._fWindDirection);
    ImGui::Text(u8"Rain: %.0f %%", session._sWeather._fRainProbablility);

    

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float fRadius = 0.45 * std::min(draw_area.x, draw_area.y);

    //sectors
    for (auto fSector : session._vSectors)
    {
        float fAzimuth = (fSector * 2 * M_PI) - (M_PI / 2);
        
        draw_list->AddLine(GetPosFromRadiusAzimuth(center, 0.9 * fRadius, fAzimuth), GetPosFromRadiusAzimuth(center, 1.1 * fRadius, fAzimuth), IM_COL32(255, 255, 255, 255), 3);
    }

    //track    
    draw_list->AddCircle(center, fRadius, IM_COL32(255, 255, 255, 255), 100, 5);

    //pitlane
    draw_list->PathArcTo(center, 0.8 * fRadius, 2.5*M_PI/2, 3.5*M_PI/2, 50);
    draw_list->PathStroke(IM_COL32(128, 128, 128, 255), ImDrawFlags_None, 3);

    //draw player car
    if (session._pPlayer)
    {
        float fAzimuth = (session._pPlayer->_LapDistPct * 2 * M_PI)-(M_PI/2);
        float r = (session._pPlayer->_bIsOnPitRoad ? 0.8 : 1.) * fRadius;
        ImVec2 pos = GetPosFromRadiusAzimuth(center, r, fAzimuth);
        draw_list->AddCircleFilled(pos, 15, IM_COL32(255, 0, 0, 255), 20);
        draw_list->AddCircle(pos, 15, IM_COL32(255, 255, 255, 255), 20);

        char szPos[8];
        sprintf(szPos, "%d", session._pPlayer->_uiPosition);
        ImVec2 textSize = ImGui::CalcTextSize(szPos);
        draw_list->AddText(ImVec2(pos.x-0.5*textSize.x, pos.y-0.5*textSize.y), IM_COL32(0, 0, 0, 255), szPos);
    }

    ImGui::End();
    
}

void RaceEngineer::DrawFuelCalculator()
{
    sDriver* pPlayer = m_IRModel.m_sSessionData._pPlayer;

    ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
    ImGui::Begin("FuelCalculator");

    if (pPlayer)
    {
        char szFuelLvl[8];
        sprintf(szFuelLvl, "%.01f L", pPlayer->_fFuelLevelLiters);
        ImGui::ProgressBar(pPlayer->_fFuelLevelPct, ImVec2(0.0f, 0.0f), szFuelLvl);
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::Text("%.01f L", pPlayer->_fFuelLevelLiters / pPlayer->_fFuelLevelPct);
    }

    ImGui::End();
}

void RaceEngineer::DrawDebrief()
{
}