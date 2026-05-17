#pragma once
#include "irsdk_defines.h"
#include "irsdk_client.h"
#include "irsdk_diskclient.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

//#define DEBUG_IBT_PATH "C:\\Users\\melin\\OneDrive\\Documents\\iRacing\\telemetry\\superformulalights324_bathurst 2025-06-02 21-59-08.ibt"
//#define DEBUG_IBT_PATH "Augusta\\RaceEngineer\\Sample\\bmwm4gt3_okayama full 2025-06-15 19-15-20.ibt"
//#define DEBUG_IBT_PATH "Augusta\\RaceEngineer\\Sample\\bmwm4gt3_monza full 2025-07-01 23-05-21.ibt"
#define DEBUG_IBT_PATH "Augusta\\RaceEngineer\\Sample\\porsche992rgt3_lemans full 2026-01-04 16-58-35.ibt"

#define IR_MAX_DRIVERS 64
#define IR_MAX_TIRE_COMPOUND 4

/*
* Single tick of telemetry data in a generic container
* Merging irsdk session string data and variables data
* Only contains variables for that specific tick, needs to be applied as a diff
* Can be serialized to keep a complete history of the session
*/

struct sIRVariable
{
	irsdk_VarType _irType;
	using irValue = std::variant<bool, int, float, double, std::string>;
	irValue _Value;
};

/*
* Global data at a given time
* Target for sTickData diff
*/
enum eSessionType
{
	PRACTICE,
	QUALI,
	RACE
};

struct sWeather
{
	float _fTrackTemp = 30.f;
	float _fAirTemp = 20.f;
	float _fWindSpeed = 2.5f;
	float _fWindDirection = 92.f;
	float _fRainProbablility = 23.f;
};

struct sFuel
{
	float _fFuelLevelLiters;
	float _fFuelLevelPct;
	float _fFuelUsageKgph;
	std::vector<float> _vFuekPerLap;
};

struct sLap
{
	bool _bFastest;
	float _fTime;
	std::vector<float> _vSectorTimes;

	//TODO:	telemetry data (throttle, brakes, wheel input, etc...)
};

struct sDriver
{
	uint8_t _uiPosition;
	std::string _strName;
	std::string _strCountry;
	uint32_t _uiCarNumber;
	uint32_t _uiIRating;
	std::string _strLicence;
	float _aLicColor[4];
	float _fSafetyRating;
	float _fFastestLap; //TODO replace with index in lap vector
	float _fLastLap;
	bool _bIsOnTrack = true;
	bool _bIsOnPitRoad = true;
	bool _bIsPlayer = false;
	float _LapDistPct;

	uint8_t _uiTireCompound;
	
	sFuel _sFuel;
	
	std::vector<sLap> _vLaps;

	float _fSteeringRad;
	float _fThrottle;
	float _fBrake;
	float _fClutch;
	int32_t _iGear;
	float _fSpeedMps;
	bool _bABSActive;
};

struct sSession
{
	std::string _strSessionYaml; //TODO move elsewhere?
	
	eSessionType _eSessionType;
	double _dSessionTime = 0;
	double _dSessionTimeTotal = 0;	
	
	std::string _strAvailableTires;
	irsdk_Flags _Flags;
	std::vector<float> _vSectors;
	
	sWeather _sWeather;
	
	std::unordered_map<uint32_t, sDriver> _mDrivers;
	sDriver* _aPositions[IR_MAX_DRIVERS] = { nullptr };
	sDriver* _pPlayer = nullptr;
};

class IReader
{
public:
	virtual int GetSessionStrVal(const char* path, char* val, int valLen) = 0;

	virtual int GetInt(const char* szName) = 0;
	virtual float GetFloat(const char* szName) = 0;
	virtual double GetDouble(const char* szName) = 0;

	virtual void ReadData(sSession &session) = 0;
	void ApplyTickDataDiff(sSession& session, uint64_t uiTargetTick, uint64_t uiCurrentTick);

protected:
	using irData = std::unordered_map<int32_t, sIRVariable>;
	std::vector<irData> m_vTickData;
	uint64_t m_uiCurrentTick = 0;

	int32_t m_idxSessionTime;
	int32_t m_idxSessionTimeTotal;
	int32_t m_idxSessionFlags;
	int32_t m_idxTrackTempCrew;
	int32_t m_idxAirTemp;
	int32_t m_idxWindVel;
	int32_t m_idxWindDir;
	int32_t m_idxPrecipitation;
	int32_t m_idxOnPitRoad;
	int32_t m_idxLapDistPct;
	int32_t m_idxPlayerCarPosition;
	int32_t m_idxFuelLevel;
	int32_t m_idxFuelLevelPct;
	int32_t m_idxPlayerTireCompound;
	int32_t m_idxSteeringWheelAngle;
	int32_t m_idxThrottle;
	int32_t m_idxBrake;
	int32_t m_idxClutch;
	int32_t m_idxGear;
	int32_t m_idxSpeed;
	int32_t m_idxBrakeABSactive;
	int32_t m_idxPlayerCarIdx;
};

class OnlineReader : public IReader
{
public:
	OnlineReader();

	int GetSessionStrVal(const char* path, char* val, int valLen) override;

	int GetInt(const char* szName) override;
	float GetFloat(const char* szName) override;
	double GetDouble(const char* szName) override;

	bool Tick(int msTimeout);
	void ReadData(sSession& session);

protected:
	irsdkClient* m_pIrsdkClient = nullptr;
};

class DiskReader : public IReader
{
public:
	DiskReader();

	int GetSessionStrVal(const char* path, char* val, int valLen);

	int GetInt(const char* szName) override;
	float GetFloat(const char* szName) override;
	double GetDouble(const char* szName) override;

	void ReadData(sSession& session);
	void ReadTickData(int32_t iTick);

	uint32_t m_uiNbTicks;
protected:
	irsdkDiskClient m_irDiskClient;	
};

class IRModel
{
public:
	IRModel();
	~IRModel();

	void Update();
	void ReadData();

	bool m_bConnected;
	bool m_bDiskRead = false;
	sSession m_sSessionData;
	
	IReader* m_pCurrentReader = nullptr;
	OnlineReader m_OnlineReader;
	DiskReader m_DiskReader;	
};

