#include "IRModel.h"

#define IR_MAX_DRIVERS 64

IRModel::IRModel()
{
}

IRModel::~IRModel()
{
}

void HEXAtoIV4(const char* hex, float a, float *outColor) 
{
	int r, g, b;
	std::sscanf(hex, "0x%02x%02x%02x", &r, &g, &b);
	outColor[0] = float(r)/255;
	outColor[1] = float(g)/255;
	outColor[2] = float(b)/255;
	outColor[3] = a;
}

void IRModel::Update()
{
	irsdkClient& irsdk = irsdkClient::instance();
	irsdk.waitForData(16);

	m_bConnected = irsdk.isConnected();

	if (m_bConnected)
	{
		if (irsdk.wasSessionStrUpdated())
		{
			m_strSessionYaml = irsdk.getSessionStr();
			FILE* pFile = fopen("session.yaml", "w");
			if (pFile)
			{
				fwrite(m_strSessionYaml.data(), m_strSessionYaml.size(), 1, pFile);
				fclose(pFile);
			}

			char szData[256];
			if (irsdk.getSessionStrVal("SessionInfo:Sessions:SessionType:", szData, sizeof(szData)))
			{
				if (_stricmp(szData, "Practice"))
					m_eSessionType == PRACTICE;
				else if (_stricmp(szData, "Qualify"))
					m_eSessionType == QUALI;
				else if (_stricmp(szData, "Race"))
					m_eSessionType == RACE;
			}

			char szPath[64];
			for (uint8_t i = 0; i < IR_MAX_DRIVERS; ++i)
			{
				sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}UserName:", i);
				if (!irsdk.getSessionStrVal(szPath, szData, sizeof(szData)))
					continue;

				m_mDrivers[i].szName = szData;

				sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}CarNumberRaw:", i);
				irsdk.getSessionStrVal(szPath, szData, sizeof(szData));
				m_mDrivers[i].uiCarNumber = atoi(szData);

				sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}IRating:", i);
				irsdk.getSessionStrVal(szPath, szData, sizeof(szData));
				m_mDrivers[i].uiIRating = atoi(szData);

				sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}LicString:", i);
				irsdk.getSessionStrVal(szPath, szData, sizeof(szData));
				m_mDrivers[i].strLicence = szData;

				sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}LicColor:", i);
				irsdk.getSessionStrVal(szPath, szData, sizeof(szData));
				HEXAtoIV4(szData,1.0,m_mDrivers[i].aLicColor);
			}
		}		

		for (auto& var : m_mIrsdkVars)
			if (!var.second.isValid())
				var.second.setVarName(var.first.c_str());
	}
}


int IRModel::GetInt(const char* szName)
{
	if (m_mIrsdkVars[szName].isValid())
		return m_mIrsdkVars[szName].getInt();
	return 0.0f;
}

float IRModel::GetFloat(const char* szName)
{
	if (m_mIrsdkVars[szName].isValid())
		return m_mIrsdkVars[szName].getFloat();
	return 0.0f;
}
