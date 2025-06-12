#pragma once
#include "irsdk_defines.h"
#include "irsdk_client.h"
#include <string>
#include <map>

enum eSessionType
{
	PRACTICE,
	QUALI,
	RACE
};

struct Driver
{
	std::string szName;
	uint32_t uiCarNumber;
	uint32_t uiIRating;
	std::string strLicence;
	float aLicColor[4];
	float fSafetyRating;
};

class IRModel
{
public:
	IRModel();
	~IRModel();

	void Update();

	int GetInt(const char* szName);
	float GetFloat(const char* szName);

	bool m_bConnected;
	std::string m_strSessionYaml;
	eSessionType m_eSessionType;
	
	std::map<std::string, irsdkCVar> m_mIrsdkVars;
	std::map<uint32_t, Driver> m_mDrivers;
};

