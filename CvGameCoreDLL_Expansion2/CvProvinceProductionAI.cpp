/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */

#include "CvGameCoreDLLPCH.h"
#include "CvProvinceProductionAI.h"
#include "CvPlayer.h"
#include "CvCity.h"
#include "CvGameCoreUtils.h"
#include "CvBuildingClasses.h"
#include "CvUnitClasses.h"

#include "LintFree.h"

// Parse a comma separated list of type names from a SQL define.
static void SuzerainParseTypeList(const char* szDefine, std::vector<std::string>& vOut)
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
			sToken = sToken.substr(b, e - b + 1);
		else
			sToken.clear();

		if (!sToken.empty()) vOut.push_back(sToken);
		if (iEnd == sAll.size()) break;
		iStart = iEnd + 1;
	}
}

// Reads a SQL define. Missing or non-positive values disable the feature
// rather than falling back to a value baked into the code.
static int SuzerainProdDefine(const char* szName)
{
	return GC.getDefineINT(szName, false);
}

// Parse a SQL define of the form "a:b,c:d" into pairs.
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

// Try to queue the first buildable item from a SQL type list.
static bool SuzerainTryQueueBuilding(CvCity* pCity, const char* szDefine)
{
	std::vector<std::string> vBuildings;
	SuzerainParseTypeList(szDefine, vBuildings);
	for (size_t i = 0; i < vBuildings.size(); i++)
	{
		BuildingTypes eBuilding = (BuildingTypes)GC.getInfoTypeForString(vBuildings[i].c_str());
		if (eBuilding != NO_BUILDING && pCity->canConstruct(eBuilding))
		{
			pCity->pushOrder(ORDER_CONSTRUCT, eBuilding, -1, false, false, false);
			return true;
		}
	}
	return false;
}

CvProvinceProductionAI::CvProvinceProductionAI(void)
	: m_pPlayer(NULL)
{
}

CvProvinceProductionAI::~CvProvinceProductionAI(void)
{
	Uninit();
}

void CvProvinceProductionAI::Init(CvPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

void CvProvinceProductionAI::Uninit(void)
{
	m_pPlayer = NULL;
}

// Main entry point - choose what this city should produce
void CvProvinceProductionAI::ChooseProduction(int iProv, CvCity* pCity)
{
	if (m_pPlayer == NULL || pCity == NULL) return;
	if (!MOD_GLOBAL_SUZERAIN) return;

	int iNationalStrategy = m_pPlayer->GetNationalStrategy();
	bool bWar = IsProvinceAtWar(iProv);
	// Province post fields: 1 name, 2 ability, 3 character, 4 lean.
	int iGovernorAbility = m_pPlayer->GetProvinceField(iProv, 2);

	// Always need workers first
	if (NeedWorkers(iProv, pCity))
	{
		UnitTypes eWorker = (UnitTypes)GC.getInfoTypeForString("UNIT_WORKER");
		if (eWorker != NO_UNIT && pCity->canTrain(eWorker))
		{
			pCity->pushOrder(ORDER_TRAIN, eWorker, -1, false, false, false);
			return;
		}
	}

	// Need basic infrastructure?
	if (NeedBasicInfrastructure(pCity))
	{
		pCity->AI_chooseProduction(false);
		return;
	}

	// Military need takes priority in wartime or border provinces
	if (NeedGarrison(iProv, pCity))
	{
		bool bDefensive = !bWar; // In peace, train cheap garrison; in war, train strong units
		UnitTypes eBestUnit = SelectBestMilitaryUnit(pCity, bDefensive);
		if (eBestUnit != NO_UNIT)
		{
			pCity->pushOrder(ORDER_TRAIN, eBestUnit, -1, false, false, false);
			return;
		}
	}

	// Follow national strategy. Which list a strategy drives is configured in SQL.
	std::vector<std::pair<std::string, std::string> > vStrategyMap;
	SuzerainParsePairList("SUZERAIN_STRATEGY_LIST_MAP", vStrategyMap);

	char szStrategyIndex[16];
	sprintf_s(szStrategyIndex, "%d", iNationalStrategy);

	std::string sCategory;
	for (size_t i = 0; i < vStrategyMap.size(); i++)
	{
		if (vStrategyMap[i].first == szStrategyIndex)
		{
			sCategory = vStrategyMap[i].second;
			break;
		}
	}

	if (!sCategory.empty())
	{
		if (sCategory == "MILITARY")
		{
			const char* szMilitaryBuilding = GC.getDefineSTRING("SUZERAIN_PRODUCTION_MILITARY_BUILDING", false);
			if (szMilitaryBuilding != NULL && szMilitaryBuilding[0] != '\0')
			{
				BuildingTypes eMilitaryBuilding = (BuildingTypes)GC.getInfoTypeForString(szMilitaryBuilding);
				if (eMilitaryBuilding != NO_BUILDING && pCity->canConstruct(eMilitaryBuilding))
				{
					pCity->pushOrder(ORDER_CONSTRUCT, eMilitaryBuilding, -1, false, false, false);
					return;
				}
			}
		}
		else
		{
			const char* szListDefine = NULL;
			if (sCategory == "LONGTERM") szListDefine = "SUZERAIN_PRODUCTION_LONGTERM_BUILDINGS";
			else if (sCategory == "ECONOMIC") szListDefine = "SUZERAIN_PRODUCTION_ECONOMIC_BUILDINGS";
			else if (sCategory == "CULTURAL") szListDefine = "SUZERAIN_PRODUCTION_CULTURAL_BUILDINGS";
			else if (sCategory == "SCIENTIFIC") szListDefine = "SUZERAIN_PRODUCTION_SCIENTIFIC_BUILDINGS";

			if (szListDefine != NULL && SuzerainTryQueueBuilding(pCity, szListDefine))
				return;
		}
	}

	// Governor ability determines how far ahead the province plans
	int iHighAbility = SuzerainProdDefine("SUZERAIN_PRODUCTION_HIGH_ABILITY");
	int iLowAbility = SuzerainProdDefine("SUZERAIN_PRODUCTION_LOW_ABILITY");

	if (iHighAbility <= 0 || iLowAbility <= 0)
	{
		pCity->AI_chooseProduction(false); // tiers not configured
	}
	else if (iGovernorAbility >= iHighAbility)
	{
		ChooseLongTermBuilding(iProv, pCity);
	}
	else if (iGovernorAbility >= iLowAbility)
	{
		pCity->AI_chooseProduction(false);
	}
	else
	{
		ChooseQuickReturns(iProv, pCity);
	}
}

// Helper: does province need more workers?
bool CvProvinceProductionAI::NeedWorkers(int iProv, CvCity* pCity)
{
	if (m_pPlayer == NULL) return false;

	int iWorkers = 0;
	int iLoop = 0;
	for (CvUnit* pUnit = m_pPlayer->firstUnit(&iLoop); pUnit != NULL; pUnit = m_pPlayer->nextUnit(&iLoop))
	{
		if (pUnit->AI_getUnitAIType() != UNITAI_WORKER) continue;

		CvPlot* pUnitPlot = pUnit->plot();
		if (pUnitPlot == NULL) continue;
		CvCity* pNearest = m_pPlayer->findClosestCity(*pUnitPlot);
		if (pNearest != NULL && m_pPlayer->GetCityProvince(pNearest->GetID()) == iProv)
		{
			iWorkers++;
		}
	}

	int iPerCity = SuzerainProdDefine("SUZERAIN_PROVINCE_WORKERS_PER_CITY");
	if (iPerCity <= 0) return false; // worker target not configured

	int iCities = m_pPlayer->GetProvinceCityCount(iProv);
	int iTarget = iCities * iPerCity;

	return iWorkers < iTarget;
}

// Helper: does province need more garrison units?
bool CvProvinceProductionAI::NeedGarrison(int iProv, CvCity* pCity)
{
	if (m_pPlayer == NULL) return false;

	int iCurrent = m_pPlayer->GetProvinceGarrisonCount(iProv);
	int iTarget = m_pPlayer->GetProvinceGarrisonTarget(iProv);

	int iThreshold = SuzerainProdDefine("SUZERAIN_PRODUCTION_GARRISON_THRESHOLD_PERCENT");
	if (iThreshold <= 0) return false; // threshold not configured
	return iCurrent < (iTarget * iThreshold / 100);
}

// Helper: is this a border province?
bool CvProvinceProductionAI::IsProvinceBorder(int iProv) const
{
	if (m_pPlayer == NULL) return false;

	int iRadius = SuzerainProdDefine("SUZERAIN_PRODUCTION_BORDER_RADIUS");
	if (iRadius <= 0) return false; // border scan not configured

	int iCityLoop = 0;
	for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
	{
		if (m_pPlayer->GetCityProvince(pCity->GetID()) != iProv)
			continue;

		for (int iRange = 1; iRange <= iRadius; iRange++)
		{
			for (int dx = -iRange; dx <= iRange; dx++)
			{
				for (int dy = -iRange; dy <= iRange; dy++)
				{
					CvPlot* pPlot = GC.getMap().plot(pCity->getX() + dx, pCity->getY() + dy);
					if (pPlot == NULL) continue;

					if (pPlot->getOwner() != NO_PLAYER && pPlot->getOwner() != m_pPlayer->GetID())
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

// Helper: is province at war?
bool CvProvinceProductionAI::IsProvinceAtWar(int iProv) const
{
	if (m_pPlayer == NULL) return false;
	return m_pPlayer->GetProvinceWar(iProv) > 0;
}

// Helper: choose building for long-term development (high wisdom)
void CvProvinceProductionAI::ChooseLongTermBuilding(int iProv, CvCity* pCity)
{
	if (pCity == NULL) return;

	if (SuzerainTryQueueBuilding(pCity, "SUZERAIN_PRODUCTION_LONGTERM_BUILDINGS"))
		return;

	pCity->AI_chooseProduction(false);
}

// Helper: choose quick-return production (low wisdom)
void CvProvinceProductionAI::ChooseQuickReturns(int iProv, CvCity* pCity)
{
	if (pCity == NULL) return;

	if (SuzerainTryQueueBuilding(pCity, "SUZERAIN_PRODUCTION_QUICK_BUILDINGS"))
		return;

	const char* szFallbackUnit = GC.getDefineSTRING("SUZERAIN_PRODUCTION_QUICK_UNIT_FALLBACK", false);
	if (szFallbackUnit != NULL && szFallbackUnit[0] != '\0')
	{
		UnitTypes eUnit = (UnitTypes)GC.getInfoTypeForString(szFallbackUnit);
		if (eUnit != NO_UNIT && pCity->canTrain(eUnit))
		{
			pCity->pushOrder(ORDER_TRAIN, eUnit, -1, false, false, false);
			return;
		}
	}

	pCity->AI_chooseProduction(false);
}

// Helper: select best military unit for current tech level
UnitTypes CvProvinceProductionAI::SelectBestMilitaryUnit(CvCity* pCity, bool bDefensive)
{
	if (pCity == NULL) return NO_UNIT;

	std::vector<std::string> vUnits;
	SuzerainParseTypeList(bDefensive
		? "SUZERAIN_PRODUCTION_DEFENSIVE_UNITS"
		: "SUZERAIN_PRODUCTION_OFFENSIVE_UNITS", vUnits);

	for (size_t i = 0; i < vUnits.size(); i++)
	{
		UnitTypes eUnit = (UnitTypes)GC.getInfoTypeForString(vUnits[i].c_str());
		if (eUnit != NO_UNIT && pCity->canTrain(eUnit))
		{
			return eUnit;
		}
	}

	return NO_UNIT;
}

// Helper: select best economic building
BuildingTypes CvProvinceProductionAI::SelectEconomicBuilding(CvCity* pCity)
{
	if (pCity == NULL) return NO_BUILDING;

	std::vector<std::string> vBuildings;
	SuzerainParseTypeList("SUZERAIN_PRODUCTION_ECONOMIC_BUILDINGS", vBuildings);

	for (size_t i = 0; i < vBuildings.size(); i++)
	{
		BuildingTypes eBuilding = (BuildingTypes)GC.getInfoTypeForString(vBuildings[i].c_str());
		if (eBuilding != NO_BUILDING && pCity->canConstruct(eBuilding))
		{
			return eBuilding;
		}
	}

	return NO_BUILDING;
}

// Helper: does city need basic infrastructure?
bool CvProvinceProductionAI::NeedBasicInfrastructure(CvCity* pCity)
{
	if (pCity == NULL) return false;

	std::vector<std::string> vBuildings;
	SuzerainParseTypeList("SUZERAIN_PRODUCTION_BASIC_BUILDINGS", vBuildings);

	for (size_t i = 0; i < vBuildings.size(); i++)
	{
		BuildingTypes eBuilding = (BuildingTypes)GC.getInfoTypeForString(vBuildings[i].c_str());
		if (eBuilding == NO_BUILDING) continue;
		if (!pCity->GetCityBuildings()->GetNumBuilding(eBuilding) && pCity->canConstruct(eBuilding))
		{
			return true;
		}
	}

	return false;
}
