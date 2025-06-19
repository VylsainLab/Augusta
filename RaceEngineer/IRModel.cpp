#include "IRModel.h"

#define IR_MAX_DRIVERS 64
#define IR_MAX_TIRE_COMPOUND 4

OnlineReader::OnlineReader()
{
	m_pIrsdkClient = &irsdkClient::instance();
}

int OnlineReader::GetSessionStrVal(const char* path, char* val, int valLen)
{
	if (m_pIrsdkClient)
		return m_pIrsdkClient->getSessionStrVal(path, val, valLen);
	return 0;
}

int OnlineReader::GetInt(const char* szName)
{
	if (m_pIrsdkClient)
		return m_pIrsdkClient->getVarInt(szName);
	return 0;
}

float OnlineReader::GetFloat(const char* szName)
{
	if (m_pIrsdkClient)
		return m_pIrsdkClient->getVarFloat(szName);
	return 0.0f;
}

double OnlineReader::GetDouble(const char* szName)
{
	if (m_pIrsdkClient)
		return m_pIrsdkClient->getVarDouble(szName);
	return 0.0;
}

bool OnlineReader::Tick(int msTimeout)
{
	if (m_pIrsdkClient)
	{
		m_pIrsdkClient->waitForData(16);
		return m_pIrsdkClient->isConnected();
	}
	return false;
}

void OnlineReader::ReadData(sSession& session)
{
	session._strSessionYaml = m_pIrsdkClient->getSessionStr();
}

DiskReader::DiskReader()
{
	m_irDiskClient.openFile(DEBUG_IBT_PATH);
}

int DiskReader::GetSessionStrVal(const char* path, char* val, int valLen)
{
	return m_irDiskClient.getSessionStrVal(path, val, valLen);
}

int DiskReader::GetInt(const char* szName)
{
	return m_irDiskClient.getVarInt(szName);
}

float DiskReader::GetFloat(const char* szName)
{
	float fValue = m_irDiskClient.getVarFloat(szName);
	if (isnan(fValue) || isinf(fValue))
		return 0.0f;
	return fValue;
}

double DiskReader::GetDouble(const char* szName)
{
	return m_irDiskClient.getVarDouble(szName);
}

void DiskReader::ReadData(sSession& session)
{
	session._strSessionYaml = m_irDiskClient.getSessionStr();
}



IRModel::IRModel()
{
}

IRModel::~IRModel()
{
}

void HEXAtoFloat4(const char* hex, float a, float *outColor) 
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
	m_bConnected = m_OnlineReader.Tick(16);
	if (m_bConnected)
		m_pCurrentReader = &m_OnlineReader;
	else
		m_pCurrentReader = &m_DiskReader;

	ReadData();
}

void IRModel::ReadData()
{
	if (!m_pCurrentReader || (!m_bConnected && m_bDiskRead))
		return;

	m_pCurrentReader->ReadData(m_sSessionData);

	//save in readable format for debug purposes
	FILE* pFile = fopen("session.yaml", "w");
	if (pFile)
	{
		fwrite(m_sSessionData._strSessionYaml.data(), m_sSessionData._strSessionYaml.size(), 1, pFile);
		fclose(pFile);
	}

	char szData[256];
	if (m_pCurrentReader->GetSessionStrVal("SessionInfo:Sessions:SessionType:", szData, sizeof(szData)))
	{
		if (_stricmp(szData, "Practice")==0)
			m_sSessionData._eSessionType == PRACTICE;
		else if (_stricmp(szData, "Qualify")==0)
			m_sSessionData._eSessionType == QUALI;
		else if (_stricmp(szData, "Race")==0)
			m_sSessionData._eSessionType == RACE;
	}

	m_sSessionData.fSessionTime = m_pCurrentReader->GetFloat("SessionTime");
	m_sSessionData.fSessionTimeTotal = m_pCurrentReader->GetFloat("SessionTimeTotal");

	char szPath[64];
	for (uint8_t i = 0; i < IR_MAX_DRIVERS; ++i)
	{
		sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}UserName:", i);
		if (!m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData)))
			continue;

		m_sSessionData._mDrivers[i].strName = szData;

		sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}FlairName:", i);
		
		if (m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData))==0) //fallback for IBTs created before the introduction of flairs
		{
			sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}ClubName:", i);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
		}
		m_sSessionData._mDrivers[i].strCountry = szData;

		sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}CarNumberRaw:", i);
		m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
		m_sSessionData._mDrivers[i].uiCarNumber = atoi(szData);

		sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}IRating:", i);
		m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
		m_sSessionData._mDrivers[i].uiIRating = atoi(szData);

		sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}LicString:", i);
		m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
		m_sSessionData._mDrivers[i].strLicence = szData;

		sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}LicColor:", i);
		m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
		HEXAtoFloat4(szData, 1.0, m_sSessionData._mDrivers[i].aLicColor);
	}

	for (uint8_t i = 0; i < IR_MAX_TIRE_COMPOUND; ++i)
	{
		sprintf(szPath, "DriverInfo:DriverTires:TireIndex:{%d}TireCompoundType:", i);
		if (!m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData)))
			continue;

		m_sSessionData.strAvailableTires += szData[1];
	}
	m_sSessionData.strAvailableTires = "HMSW";

	m_bDiskRead = true;
}


