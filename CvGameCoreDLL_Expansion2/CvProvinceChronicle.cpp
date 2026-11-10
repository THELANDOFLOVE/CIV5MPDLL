/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */

#include "CvGameCoreDLLPCH.h"
#include "CvProvinceChronicle.h"
#include "CvPlayer.h"
#include "CvCity.h"
#include "CvGameCoreUtils.h"

#include "LintFree.h"

// Parse a SQL define of the form "key:value,key:value" into pairs.
static void SuzerainParsePairList(const char* szDefine, std::vector<std::pair<std::string, std::string> >& vOut)
{
	vOut.clear();
	const char* szValue = GC.getDefineSTRING(szDefine, false);
	if (szValue == NULL) return;

	std::string sAll(szValue);
	size_t iStart = 0;
	while (iStart <= sAll.size())
	{
		size_t iEnd = sAll.find(',', iStart);
		if (iEnd == std::string::npos) iEnd = sAll.size();

		std::string sToken = sAll.substr(iStart, iEnd - iStart);
		size_t iColon = sToken.find(':');
		if (iColon != std::string::npos)
		{
			std::string sKey = sToken.substr(0, iColon);
			std::string sVal = sToken.substr(iColon + 1);
			size_t b = sKey.find_first_not_of(" \t\r\n");
			size_t e = sKey.find_last_not_of(" \t\r\n");
			if (b != std::string::npos) sKey = sKey.substr(b, e - b + 1);
			b = sVal.find_first_not_of(" \t\r\n");
			e = sVal.find_last_not_of(" \t\r\n");
			if (b != std::string::npos) sVal = sVal.substr(b, e - b + 1);
			if (!sKey.empty() && !sVal.empty())
				vOut.push_back(std::make_pair(sKey, sVal));
		}

		if (iEnd == sAll.size()) break;
		iStart = iEnd + 1;
	}
}

// Parse a comma separated list of integers from a SQL define.
static void SuzerainParseIntList(const char* szDefine, std::vector<int>& vOut)
{
	vOut.clear();
	const char* szValue = GC.getDefineSTRING(szDefine, false);
	if (szValue == NULL) return;

	std::string sAll(szValue);
	size_t iStart = 0;
	while (iStart <= sAll.size())
	{
		size_t iEnd = sAll.find(',', iStart);
		if (iEnd == std::string::npos) iEnd = sAll.size();

		std::string sToken = sAll.substr(iStart, iEnd - iStart);
		size_t b = sToken.find_first_not_of(" \t\r\n");
		size_t e = sToken.find_last_not_of(" \t\r\n");
		if (b != std::string::npos)
		{
			sToken = sToken.substr(b, e - b + 1);
			vOut.push_back(atoi(sToken.c_str()));
		}

		if (iEnd == sAll.size()) break;
		iStart = iEnd + 1;
	}
}

// Named chronicle columns that a report type may feed.
static void SuzerainApplyStat(ChronicleEntry& entry, const std::string& sKey)
{
	if (sKey == "WARS") entry.iWarsEngaged++;
	else if (sKey == "RAID") entry.iBarbariansKilled++;
	else if (sKey == "OFFICIALS") entry.iOfficialsAppointed++;
	else if (sKey == "BUILDINGS") entry.iBuildingsBuilt++;
	else if (sKey == "UNITS") entry.iUnitsProduced++;
}

CvProvinceChronicle::CvProvinceChronicle(void)
	: m_pPlayer(NULL)
	, m_iCompileInterval(0) // set from SQL in Init
	, m_iLastCompileTurn(0)
	, m_iLastEra(-1)
{
}

CvProvinceChronicle::~CvProvinceChronicle(void)
{
	Uninit();
}

void CvProvinceChronicle::Init(CvPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
	m_iLastCompileTurn = 0;
	m_iLastEra = -1;
	m_iCompileInterval = GC.getDefineINT("SUZERAIN_CHRONICLE_COMPILE_INTERVAL", false);
}

void CvProvinceChronicle::Uninit(void)
{
	m_pPlayer = NULL;
}

// Main entry point: called each turn
void CvProvinceChronicle::DoTurn(void)
{
	if (m_pPlayer == NULL) return;
	if (!m_pPlayer->isAlive()) return;
	if (!MOD_GLOBAL_SUZERAIN) return;

	int iCurrentTurn = GC.getGame().getGameTurn();
	int iCurrentEra = GetCurrentEra();

	// Check if entered new era (auto-compile on era change)
	if (m_iLastEra >= 0 && iCurrentEra > m_iLastEra)
	{
		// Era changed, compile history of previous era
		CompileChronicle();
		m_iLastCompileTurn = iCurrentTurn;
	}
	m_iLastEra = iCurrentEra;

	// Backup: compile every configured interval while no era change happens.
	if (m_iCompileInterval > 0
		&& iCurrentTurn > 0
		&& iCurrentTurn - m_iLastCompileTurn >= m_iCompileInterval)
	{
		CompileChronicle();
		m_iLastCompileTurn = iCurrentTurn;
	}
}

// Compile history: convert raw reports to summaries
void CvProvinceChronicle::CompileChronicle(void)
{
	if (m_pPlayer == NULL) return;

	int iCurrentTurn = GC.getGame().getGameTurn();
	// A missing interval compiles from the start of the game instead of a
	// hardcoded window.
	int iStartTurn = (m_iCompileInterval > 0) ? (iCurrentTurn - m_iCompileInterval) : 0;
	if (iStartTurn < 0) iStartTurn = 0;

	int iProvinceCount = m_pPlayer->GetProvinceCount();

	// Compile a summary for each province
	for (int iProv = 0; iProv < iProvinceCount; iProv++)
	{
		ChronicleEntry entry;
		entry.iStartTurn = iStartTurn;
		entry.iEndTurn = iCurrentTurn;
		entry.iProvince = iProv;
		entry.iEra = GetCurrentEra();

		// Record initial data
		// Note: This records current values; ideally should restore from historical data
		// But in simplified implementation, we only record end values
		entry.iGoldStart = m_pPlayer->GetProvinceTreasury(iProv);
		entry.iPopulationStart = 0;

		int iCityLoop = 0;
		for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
		{
			if (m_pPlayer->GetCityProvince(pCity->GetID()) == iProv)
				entry.iPopulationStart += pCity->getPopulation();
		}

		// Collect event statistics
		CollectEventStatistics(iStartTurn, iCurrentTurn, iProv, entry);

		// Filter major events
		FilterMajorEvents(iStartTurn, iCurrentTurn, iProv, entry);

		// Record final data
		entry.iGoldEnd = m_pPlayer->GetProvinceTreasury(iProv);
		entry.iPopulationEnd = entry.iPopulationStart; // Simplified: assume population unchanged

		// Append the entry to the serialized flat storage
		std::vector<int> vChronicle = m_pPlayer->m_aiProvinceChronicle;
		int iBase = (int)vChronicle.size();
		vChronicle.resize(iBase + CHRONICLE_STRIDE, 0);

		vChronicle[iBase + CHRONICLE_FIELD_START_TURN] = entry.iStartTurn;
		vChronicle[iBase + CHRONICLE_FIELD_END_TURN] = entry.iEndTurn;
		vChronicle[iBase + CHRONICLE_FIELD_PROVINCE] = entry.iProvince;
		vChronicle[iBase + CHRONICLE_FIELD_ERA] = entry.iEra;
		vChronicle[iBase + CHRONICLE_FIELD_BARBARIANS] = entry.iBarbariansKilled;
		vChronicle[iBase + CHRONICLE_FIELD_WARS] = entry.iWarsEngaged;
		vChronicle[iBase + CHRONICLE_FIELD_OFFICIALS] = entry.iOfficialsAppointed;
		vChronicle[iBase + CHRONICLE_FIELD_BUILDINGS] = entry.iBuildingsBuilt;
		vChronicle[iBase + CHRONICLE_FIELD_UNITS] = entry.iUnitsProduced;
		vChronicle[iBase + CHRONICLE_FIELD_GOLD_START] = entry.iGoldStart;
		vChronicle[iBase + CHRONICLE_FIELD_GOLD_END] = entry.iGoldEnd;
		vChronicle[iBase + CHRONICLE_FIELD_POP_START] = entry.iPopulationStart;
		vChronicle[iBase + CHRONICLE_FIELD_POP_END] = entry.iPopulationEnd;

		int iEventCount = (int)(entry.vMajorEvents.size() / CHRONICLE_EVENT_STRIDE);
		if (iEventCount > CHRONICLE_MAX_EVENTS) iEventCount = CHRONICLE_MAX_EVENTS;
		vChronicle[iBase + CHRONICLE_FIELD_EVENT_COUNT] = iEventCount;
		for (int e = 0; e < iEventCount; e++)
		{
			for (int f = 0; f < CHRONICLE_EVENT_STRIDE; f++)
				vChronicle[iBase + CHRONICLE_FIELD_EVENTS + e * CHRONICLE_EVENT_STRIDE + f] =
					entry.vMajorEvents[e * CHRONICLE_EVENT_STRIDE + f];
		}

		m_pPlayer->m_aiProvinceChronicle = vChronicle;
	}

	// Clean up compiled raw reports
	// Only keep raw data from last m_iCompileInterval turns
	std::vector<int> vReport = m_pPlayer->m_aiProvinceReport;
	size_t iWrite = 0;
	for (size_t i = 0; i + 4 < vReport.size(); i += 5)
	{
		int iTurn = vReport[i];
		if (iTurn > iStartTurn) // Keep recent raw data
		{
			for (int j = 0; j < 5; j++)
				vReport[iWrite++] = vReport[i + j];
		}
	}
	vReport.resize(iWrite);
	m_pPlayer->m_aiProvinceReport = vReport;
}

// Collect event statistics for given period
void CvProvinceChronicle::CollectEventStatistics(int iStartTurn, int iEndTurn, int iProvince, ChronicleEntry& entry)
{
	if (m_pPlayer == NULL) return;

	std::vector<int> vReport = m_pPlayer->m_aiProvinceReport;

	// Report type -> statistic mapping is configuration, not code.
	std::vector<std::pair<std::string, std::string> > vStatMap;
	SuzerainParsePairList("SUZERAIN_CHRONICLE_STAT_MAP", vStatMap);

	for (size_t i = 0; i + 4 < vReport.size(); i += 5)
	{
		int iTurn = vReport[i];
		int iProv = vReport[i + 1];
		int iType = vReport[i + 2];

		// Filter: only count events in specified period and province
		if (iTurn < iStartTurn || iTurn > iEndTurn) continue;
		if (iProv != iProvince) continue;

		for (size_t m = 0; m < vStatMap.size(); m++)
		{
			if (atoi(vStatMap[m].first.c_str()) == iType)
				SuzerainApplyStat(entry, vStatMap[m].second);
		}
	}
}

// Filter major events (up to 5)
void CvProvinceChronicle::FilterMajorEvents(int iStartTurn, int iEndTurn, int iProvince, ChronicleEntry& entry)
{
	if (m_pPlayer == NULL) return;

	std::vector<int> vReport = m_pPlayer->m_aiProvinceReport;

	// Which report types count as major events is configuration, not code.
	std::vector<int> vMajorTypes;
	SuzerainParseIntList("SUZERAIN_CHRONICLE_MAJOR_REPORTS", vMajorTypes);

	// Collect all events and sort by importance
	std::vector<std::vector<int>> vAllEvents;

	for (size_t i = 0; i + 4 < vReport.size(); i += 5)
	{
		int iTurn = vReport[i];
		int iProv = vReport[i + 1];
		int iType = vReport[i + 2];
		int iA = vReport[i + 3];
		int iB = vReport[i + 4];

		if (iTurn < iStartTurn || iTurn > iEndTurn) continue;
		if (iProv != iProvince) continue;

		int iImportance = EVENT_MINOR; // Default to minor
		for (size_t m = 0; m < vMajorTypes.size(); m++)
		{
			if (vMajorTypes[m] == iType)
			{
				iImportance = EVENT_IMPORTANT;
				break;
			}
		}

		// Only record important and above events
		if (iImportance >= EVENT_IMPORTANT)
		{
			std::vector<int> vEvent;
			vEvent.push_back(iTurn);
			vEvent.push_back(iType);
			vEvent.push_back(iA);
			vEvent.push_back(iB);
			vEvent.push_back(iImportance);
			vAllEvents.push_back(vEvent);
		}
	}

	// Sort by importance (high to low)
	std::sort(vAllEvents.begin(), vAllEvents.end(),
		[](const std::vector<int>& a, const std::vector<int>& b) {
			return a[4] > b[4]; // Sort by importance descending
		});

	// Take the top entries the layout can hold.
	for (size_t i = 0; i < vAllEvents.size() && i < (size_t)CHRONICLE_MAX_EVENTS; i++)
	{
		for (int j = 0; j < 5; j++)
			entry.vMajorEvents.push_back(vAllEvents[i][j]);
	}
}

// Get current era
int CvProvinceChronicle::GetCurrentEra() const
{
	if (m_pPlayer == NULL) return 0;
	return (int)m_pPlayer->GetCurrentEra();
}

// Query raw reports (for viewing details)
std::vector<int> CvProvinceChronicle::GetRawReports(int iStartTurn, int iEndTurn, int iProvince) const
{
	std::vector<int> vResult;
	if (m_pPlayer == NULL) return vResult;

	std::vector<int> vReport = m_pPlayer->m_aiProvinceReport;

	for (size_t i = 0; i + 4 < vReport.size(); i += 5)
	{
		int iTurn = vReport[i];
		int iProv = vReport[i + 1];

		if (iTurn >= iStartTurn && iTurn <= iEndTurn && iProv == iProvince)
		{
			for (int j = 0; j < 5; j++)
				vResult.push_back(vReport[i + j]);
		}
	}

	return vResult;
}
