#pragma once
#include "irsdk_defines.h"
#include "irsdk_client.h"
#include "irsdk_diskclient.h"
#include <string>
#include <map>

#define DEBUG_IBT_PATH "D:\\Git\\Augusta\\RaceEngineer\\Sample\\bmwm4gt3_okayama full 2025-06-15 19-15-20.ibt"//"C:\\Users\\melin\\OneDrive\\Documents\\iRacing\\telemetry\\superformulalights324_bathurst 2025-06-02 21-59-08.ibt"

enum eSessionType
{
	PRACTICE,
	QUALI,
	RACE
};

struct sDriver
{
	std::string strName;
	std::string strCountry;
	uint32_t uiCarNumber;
	uint32_t uiIRating;
	std::string strLicence;
	float aLicColor[4];
	float fSafetyRating;
};

struct sSession
{
	std::string _strSessionYaml;
	eSessionType _eSessionType;
	float fSessionTime = 0;
	float fSessionTimeTotal = 0;
	std::map<uint32_t, sDriver> _mDrivers;
	std::string strAvailableTires;
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

