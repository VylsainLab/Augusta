#include "IRModel.h"
#include "Utils.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <unordered_map>

void IReader::ApplyTickDataDiff(sSession& session, uint64_t uiTargetTick, uint64_t uiCurrentTick)
{
	uint64_t uiTick = uiCurrentTick;
	do
	{
		irData& tickData = m_vTickData[uiTick];

		//Resolve player first
		if (!session._pPlayer)
		{
			int iPlayerIdx = std::get<int>(tickData[m_idxPlayerCarIdx]._Value);
			session._mDrivers[iPlayerIdx]._bIsPlayer = true;
			session._pPlayer = &session._mDrivers[iPlayerIdx];
		}

		//TODO replace with switch case on hash
		for(auto & var : m_vTickData[uiTick])
		{
			if(var.first == m_idxSessionTime)
				session._dSessionTime = std::get<double>(var.second._Value);
			else if(var.first == m_idxSessionTimeTotal)
				session._dSessionTimeTotal = std::get<double>(var.second._Value);
			else if (var.first == m_idxSessionFlags)
				session._Flags = static_cast<irsdk_Flags>(std::get<int>(var.second._Value));
			else if (var.first == m_idxTrackTempCrew)
				session._sWeather._fTrackTemp = std::get<float>(var.second._Value);
			else if (var.first == m_idxAirTemp)
				session._sWeather._fAirTemp = std::get<float>(var.second._Value);
			else if (var.first == m_idxWindVel)
				session._sWeather._fWindSpeed = std::get<float>(var.second._Value);
			else if (var.first == m_idxWindDir)
				session._sWeather._fWindDirection = std::get<float>(var.second._Value) * float(M_PI) / 180.f;
			else if (var.first == m_idxPrecipitation)
				session._sWeather._fRainProbablility = std::get<float>(var.second._Value);
			else if(session._pPlayer != nullptr)
			{
				if(var.first == m_idxOnPitRoad)
					session._pPlayer->_bIsOnPitRoad = std::get<bool>(var.second._Value);
				else if(var.first == m_idxLapDistPct)
					session._pPlayer->_LapDistPct = std::get<float>(var.second._Value);
				else if (var.first == m_idxPlayerCarPosition)
					session._pPlayer->_uiPosition = std::get<int>(var.second._Value);
				else if (var.first == m_idxFuelLevel)
					session._pPlayer->_sFuel._fFuelLevelLiters = std::get<float>(var.second._Value);
				else if (var.first == m_idxFuelLevelPct)
					session._pPlayer->_sFuel._fFuelLevelPct = std::get<float>(var.second._Value);
				else if (var.first == m_idxPlayerTireCompound)
					session._pPlayer->_uiTireCompound = std::get<int>(var.second._Value);
				else if (var.first == m_idxSteeringWheelAngle)
					session._pPlayer->_fSteeringRad = std::get<float>(var.second._Value);
				else if (var.first == m_idxThrottle)
					session._pPlayer->_fThrottle = std::get<float>(var.second._Value);
				else if (var.first == m_idxBrake)
					session._pPlayer->_fBrake = std::get<float>(var.second._Value);
				else if (var.first == m_idxClutch)
					session._pPlayer->_fClutch = std::get<float>(var.second._Value);
				else if (var.first == m_idxGear)
					session._pPlayer->_iGear = std::get<int>(var.second._Value);
				else if (var.first == m_idxSpeed)
					session._pPlayer->_fSpeedMps = std::get<float>(var.second._Value);
				else if (var.first == m_idxBrakeABSactive)
					session._pPlayer->_bABSActive = std::get<bool>(var.second._Value);
			}
		}
		
		if(uiTick!=uiTargetTick)
			uiTick += uiTargetTick > uiCurrentTick ? 1 : -1;
	} while (uiTick != uiTargetTick);
}

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
	std::string strPath = GetRootDirectory() + DEBUG_IBT_PATH;
	m_irDiskClient.openFile(strPath.c_str());
	int iNumVars = m_irDiskClient.getNumVars();
	m_uiNbTicks = m_irDiskClient.getDataCount();

	irData* pPrevData = nullptr;
	std::vector<sIRVariable> vTemp(iNumVars);
	sIRVariable irVar;

	//Prefetech variables type and name to iterate faster on ticks data
	std::vector<irsdk_VarType> vVarTypes(iNumVars);
	std::unordered_map<std::string, int32_t> mVariablesDictionary;
	mVariablesDictionary.reserve(iNumVars);
	for (int i = 0; i < iNumVars; ++i)
	{
		mVariablesDictionary[m_irDiskClient.getVarName(i)] = i;
		vVarTypes[i] = m_irDiskClient.getVarType(i);
	}	

	//Read all ticks (disk data does not contain all session and other drivers data)
	m_vTickData.resize(m_uiNbTicks);
	for (uint32_t i = 0; i < m_uiNbTicks; ++i)
	{
		m_irDiskClient.getTickData(i);
		irData& tickData = m_vTickData[i];
		tickData.reserve(iNumVars);

		for (int j = 0; j < iNumVars; ++j)
		{		
			switch (vVarTypes[j])
			{
			case irsdk_char:
				break;
			case irsdk_bool:
				irVar._Value = m_irDiskClient.getVarBool(j);
				break;
			case irsdk_int:
			case irsdk_bitField:
				irVar._Value = m_irDiskClient.getVarInt(j);
				break;
			case irsdk_float:
				irVar._Value = m_irDiskClient.getVarFloat(j);
				break;
			case irsdk_double:
				irVar._Value = m_irDiskClient.getVarDouble(j);
				break;
			default:
				break;
			}

			if (i>0 && vTemp[j]._Value == irVar._Value)
				continue;			
			
			vTemp[j] = irVar;

			const std::string &strName = m_irDiskClient.getVarName(j);
			tickData[j] = irVar;
		}

		pPrevData = &tickData;
	}

	//Build variables indices list to avoid fetching by names every time
	m_idxSessionTime = mVariablesDictionary["SessionTime"];
	m_idxSessionTimeTotal = mVariablesDictionary["SessionTimeTotal"];
	m_idxSessionFlags = mVariablesDictionary["SessionFlags"];
	m_idxTrackTempCrew = mVariablesDictionary["TrackTempCrew"];
	m_idxAirTemp = mVariablesDictionary["AirTemp"];
	m_idxWindVel = mVariablesDictionary["WindVel"];
	m_idxWindDir = mVariablesDictionary["WindDir"];
	m_idxPrecipitation = mVariablesDictionary["Precipitation"];
	m_idxOnPitRoad = mVariablesDictionary["OnPitRoad"];
	m_idxLapDistPct = mVariablesDictionary["LapDistPct"];
	m_idxPlayerCarPosition = mVariablesDictionary["PlayerCarPosition"];
	m_idxFuelLevel = mVariablesDictionary["FuelLevel"];
	m_idxFuelLevelPct = mVariablesDictionary["FuelLevelPct"];
	m_idxPlayerTireCompound = mVariablesDictionary["PlayerTireCompound"];
	m_idxSteeringWheelAngle = mVariablesDictionary["SteeringWheelAngle"];
	m_idxThrottle = mVariablesDictionary["Throttle"];
	m_idxBrake = mVariablesDictionary["Brake"];
	m_idxClutch = mVariablesDictionary["Clutch"];
	m_idxGear = mVariablesDictionary["Gear"];
	m_idxSpeed = mVariablesDictionary["Speed"];
	m_idxBrakeABSactive = mVariablesDictionary["BrakeABSactive"];
	m_idxPlayerCarIdx = mVariablesDictionary["PlayerCarIdx"];
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
				m_sSessionData._vSectors.push_back(static_cast<float>(atof(szData)));
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
			m_sSessionData._aPositions[i]->_fFastestLap = static_cast<float>(atof(szData));

			sprintf(szPath, "SessionInfo:Sessions:SessionNum:{0}ResultsPositions:Position:{%d}LastTime:", i + 1);
			m_pCurrentReader->GetSessionStrVal(szPath, szData, sizeof(szData));
			m_sSessionData._aPositions[i]->_fLastLap = static_cast<float>(atof(szData));

			//TODO class position
		}

		//Fill in with unranked drivers (unsorted)
		uint32_t uiCount = static_cast<uint32_t>(vInserted.size());
		for (auto& driver : m_sSessionData._mDrivers)
		{
			if (std::find(vInserted.begin(), vInserted.end(), driver.first) == vInserted.end())
			{
				m_sSessionData._aPositions[uiCount] = &driver.second;
				driver.second._uiPosition = uiCount + 1;
				uiCount++;
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
}


