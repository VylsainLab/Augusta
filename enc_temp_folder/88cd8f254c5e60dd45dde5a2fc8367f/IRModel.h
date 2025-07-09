#pragma once
#include "irsdk_defines.h"
#include "irsdk_client.h"
#include "irsdk_diskclient.h"
#include <string>
#include <map>
#include <vector>

//#define DEBUG_IBT_PATH "C:\\Users\\melin\\OneDrive\\Documents\\iRacing\\telemetry\\superformulalights324_bathurst 2025-06-02 21-59-08.ibt"
//#define DEBUG_IBT_PATH "D:\\Git\\Augusta\\RaceEngineer\\Sample\\bmwm4gt3_okayama full 2025-06-15 19-15-20.ibt"
#define DEBUG_IBT_PATH "D:\\Git\\Augusta\\RaceEngineer\\Sample\\bmwm4gt3_monza full 2025-07-01 23-05-21.ibt"

#define IR_MAX_DRIVERS 64
#define IR_MAX_TIRE_COMPOUND 4

enum eSessionType
{
	PRACTICE,
	QUALI,
	RACE
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
	float _fFastestLap;
	float _fLastLap;
	bool _bIsOnTrack = true;
	bool _bIsOnPitRoad = true;
	bool _bIsPlayer = false;
	float _LapDistPct;

	//fuel
	float _fFuelLevelLiters;
	float _fFuelLevelPct;
};

struct sWeather
{
	float _fTrackTemp = 30.f;
	float _fAirTemp = 20.f;
	float _fWindSpeed = 2.5f;
	float _fWindDirection = 92.f;
	float _fRainProbablility = 23.f;
};

struct sSession
{
	std::string _strSessionYaml;
	eSessionType _eSessionType;
	float _fSessionTime = 0;
	float _fSessionTimeTotal = 0;
	sWeather _sWeather;
	std::map<uint32_t, sDriver> _mDrivers;
	sDriver* _aPositions[IR_MAX_DRIVERS] = { nullptr };
	sDriver* _pPlayer = nullptr;
	std::string _strAvailableTires;
	irsdk_Flags _Flags;
	std::vector<float> _vSectors;
};

class IReader
{
public:
	virtual int GetSessionStrVal(const char* path, char* val, int valLen) = 0;

	virtual int GetInt(const char* szName) = 0;
	virtual float GetFloat(const char* szName) = 0;
	virtual double GetDouble(const char* szName) = 0;

	virtual void ReadData(sSession &session) = 0;
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
	std::map<std::string, irsdkCVar> m_mIrsdkVars;
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
	sDriver* m_pPlayer = nullptr;
	
	IReader* m_pCurrentReader = nullptr;
	OnlineReader m_OnlineReader;
	DiskReader m_DiskReader;	
};

