#include "IRModel.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>

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
	int iNumVars = m_irDiskClient.getNumVars();
	m_uiNbTicks = m_irDiskClient.getDataCount();

	m_irDiskClient.getNextData();
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

	FILE* pFile = fopen("available_data.txt", "w");
	if (pFile)
	{
		fprintf(pFile,"\nNumVars = %d", m_irDiskClient.getNumVars());
		for (int i = 0; i < m_irDiskClient.getNumVars(); ++i)
		{
			const char* szName = m_irDiskClient.getVarName(i);
			irsdk_VarType iType = m_irDiskClient.getVarType(i);
			std::string strType;
			switch (iType)
			{
			case irsdk_char:
				strType = "Char";
				break;
			case irsdk_bool:
				strType = "Bool";
				break;
			case irsdk_int:
				strType = "Int";
				break;
			case irsdk_bitField:
				strType = "BitField";
				break;
			case irsdk_float:
				strType = "Float";
				break;
			case irsdk_double:
				strType = "Double";
				break;
			default:
				strType = "Unknown";
				break;
			}
			const char* szDesc = m_irDiskClient.getVarDesc(i);
			int iCount = m_irDiskClient.getVarCount(i);
			const char* szUnit = m_irDiskClient.getVarUnit(i);
			fprintf(pFile,"\n%d %s %s [%d] (%s) : %s", i, szName, szUnit, iCount, strType.c_str(), szDesc);
		}
		fclose(pFile);
	}
}

void DiskReader::ReadTickData(int32_t iTick)
{
	m_irDiskClient.getTickData(iTick);
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
	if (!m_pCurrentReader)
		return;

	if (!m_bConnected && !m_bDiskRead)
	{
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
			if (_stricmp(szData, "Practice") == 0)
				m_sSessionData._eSessionType = PRACTICE;
			else if (_stricmp(szData, "Qualify") == 0)
				m_sSessionData._eSessionType = QUALI;
			else if (_stricmp(szData, "Race") == 0)
				m_sSessionData._eSessionType = RACE;
		}	

		//Track sectors
		char szPath[128];
		int i = 0;
		bool bFound = true;
		while(bFound)
		{
			sprintf(szPath, "SplitTimeInfo:Sectors:SectorNum:{%d}SectorStartPct:", i);
			bFound = m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			if (bFound)
				m_sSessionData._vSectors.push_back(atof(szData));
			++i;
		}

		//DRIVERS		
		for (uint8_t i = 0; i < IR_MAX_DRIVERS; ++i)
		{
			sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}UserName:", i);
			if (!m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData)))
				continue;

			m_sSessionData._mDrivers[i]._strName = szData;

			sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}FlairName:", i);

			if (m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData)) == 0) //fallback for IBTs created before the introduction of flairs
			{
				sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}ClubName:", i);
				m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			}
			m_sSessionData._mDrivers[i]._strCountry = szData;

			sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}CarNumberRaw:", i);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			m_sSessionData._mDrivers[i]._uiCarNumber = atoi(szData);

			sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}IRating:", i);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			m_sSessionData._mDrivers[i]._uiIRating = atoi(szData);

			sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}LicString:", i);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			m_sSessionData._mDrivers[i]._strLicence = szData;

			sprintf(szPath, "DriverInfo:Drivers:CarIdx:{%d}LicColor:", i);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			HEXAtoFloat4(szData, 1.0, m_sSessionData._mDrivers[i]._aLicColor);

			//TODO: car class, car brand, team name
		}

		//POSITIONS
		std::vector<uint8_t> vInserted;
		for (uint8_t i = 0; i < IR_MAX_DRIVERS; ++i)
		{
			sprintf(szPath, "SessionInfo:Sessions:SessionNum:{0}ResultsPositions:Position:{%d}CarIdx:", i + 1);
			if (!m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData)))
				continue;

			int32_t iCarIdx = atoi(szData);
			vInserted.push_back(iCarIdx);
			m_sSessionData._aPositions[i] = &m_sSessionData._mDrivers.at(iCarIdx);
			m_sSessionData._aPositions[i]->_uiPosition = i + 1;

			sprintf(szPath, "SessionInfo:Sessions:SessionNum:{0}ResultsPositions:Position:{%d}FastestTime:", i + 1);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			m_sSessionData._aPositions[i]->_fFastestLap = atof(szData);

			sprintf(szPath, "SessionInfo:Sessions:SessionNum:{0}ResultsPositions:Position:{%d}LastTime:", i + 1);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			m_sSessionData._aPositions[i]->_fLastLap = atof(szData);

			//TODO class position
		}

		//Fill in with unranked drivers (unsorted)
		int32_t iCount = vInserted.size();
		for (auto& driver : m_sSessionData._mDrivers)
		{
			if (std::find(vInserted.begin(), vInserted.end(), driver.first) == vInserted.end())
			{
				m_sSessionData._aPositions[iCount] = &driver.second;
				driver.second._uiPosition = iCount + 1;
				iCount++;
			}
		}

		for (uint8_t i = 0; i < IR_MAX_TIRE_COMPOUND; ++i)
		{
			sprintf(szPath, "DriverInfo:DriverTires:TireIndex:{%d}TireCompoundType:", i);
			if (!m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData)))
				continue;

			m_sSessionData._strAvailableTires += szData[1];
		}


		m_bDiskRead = true;
	}

	int iSessionTick = m_pCurrentReader->GetFloat("SessionTick");
	m_sSessionData._fSessionTime = m_pCurrentReader->GetFloat("SessionTime");
	m_sSessionData._fSessionTimeTotal = m_pCurrentReader->GetFloat("SessionTimeTotal");
	m_sSessionData._Flags = (irsdk_Flags)m_pCurrentReader->GetInt("SessionFlags");
	//printf("\nFlag: %d", m_sSessionData._Flags);

	m_sSessionData._sWeather._fTrackTemp = m_pCurrentReader->GetFloat("TrackTempCrew");
	m_sSessionData._sWeather._fAirTemp = m_pCurrentReader->GetFloat("AirTemp");
	m_sSessionData._sWeather._fWindSpeed = m_pCurrentReader->GetFloat("WindVel");
	m_sSessionData._sWeather._fWindDirection = m_pCurrentReader->GetFloat("WindDir") * M_PI / 180.;
	m_sSessionData._sWeather._fRainProbablility = m_pCurrentReader->GetFloat("Precipitation");

	int iPlayerIdx = m_pCurrentReader->GetInt("PlayerCarIdx");
	m_sSessionData._mDrivers[iPlayerIdx]._bIsPlayer = true;
	m_sSessionData._pPlayer = &m_sSessionData._mDrivers[iPlayerIdx];

	m_sSessionData._pPlayer->_bIsOnPitRoad = m_pCurrentReader->GetInt("OnPitRoad");

	m_sSessionData._pPlayer->_LapDistPct = m_pCurrentReader->GetFloat("LapDistPct");
	m_sSessionData._pPlayer->_uiPosition = m_pCurrentReader->GetInt("PlayerCarPosition");

	m_sSessionData._pPlayer->_sFuel._fFuelLevelLiters = m_pCurrentReader->GetFloat("FuelLevel");
	m_sSessionData._pPlayer->_sFuel._fFuelLevelPct = m_pCurrentReader->GetFloat("FuelLevelPct");

	m_sSessionData._pPlayer->_uiTireCompound = m_pCurrentReader->GetFloat("PlayerTireCompound");

	m_sSessionData._pPlayer->_fSteeringRad = m_pCurrentReader->GetFloat("SteeringWheelAngle");
	m_sSessionData._pPlayer->_fThrottle = m_pCurrentReader->GetFloat("Throttle");
	m_sSessionData._pPlayer->_fBrake = m_pCurrentReader->GetFloat("Brake");
	m_sSessionData._pPlayer->_fClutch = m_pCurrentReader->GetFloat("Clutch");
	m_sSessionData._pPlayer->_iGear = m_pCurrentReader->GetInt("Gear");
	m_sSessionData._pPlayer->_fSpeedMps = m_pCurrentReader->GetFloat("Speed");
	m_sSessionData._pPlayer->_bABSActive = m_pCurrentReader->GetInt("BrakeABSactive");
}


