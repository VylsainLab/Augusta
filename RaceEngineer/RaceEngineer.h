#pragma once
#include <Augusta/Application.h>
#include "IRModel.h"

class RaceEngineer : public aug::Application
{
public:
	RaceEngineer(const std::string& name, uint16_t width, uint16_t height, bool bResizable, bool bVisible);

	virtual void Render(VkCommandBuffer commandBuffer) override;

	void DrawTelemetry();
	void DrawSession();
	void DrawStandings();
	void DrawWeather();
	void DrawTrackMap();

	void DrawDebrief();

protected:
	IRModel m_IRModel;

	ImFont* m_pTitleFont;
	ImFont* m_pBodyFont;
	ImFont* m_pEmojiFont;
	ImFont* m_pFlagFont;
};

