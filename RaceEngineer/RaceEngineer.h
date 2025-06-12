#pragma once
#include <Augusta/Application.h>
#include "IRModel.h"

class RaceEngineer : public aug::Application
{
public:
	RaceEngineer(const std::string& name, uint16_t width, uint16_t height, bool bResizable, bool bVisible)
		: aug::Application(name, width, height, bResizable, bVisible)
	{
		InitImGui();
	}

	virtual void Render(VkCommandBuffer commandBuffer) override;

	void DrawTelemetry();
	void DrawSession();
	void DrawStandings();

	void DrawDebrief();

protected:
	IRModel m_IRModel;
};

