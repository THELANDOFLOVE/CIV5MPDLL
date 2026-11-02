/*	-------------------------------------------------------------------------------------------------------
	City-State Unique Ability System for Super Power V11 (Pillars of Sovereignty)
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreDLLUtil.h"
#include "CvCityStateUAClasses.h"
#include "CvPlayer.h"
#include "CvCultureClasses.h"
#include "CvDatabaseUtility.h"

#include "LintFree.h"

//======================================================================================================
// CvSpecialCityTypeEntry
//======================================================================================================
namespace {
SpecialCityConditionTypes ParseSpecialCityCondition(const char* szType)
{
	if (szType == NULL) return SPECIAL_CITY_CONDITION_NONE;
	if (strcmp(szType, "HAS_RESOURCE") == 0) return SPECIAL_CITY_CONDITION_HAS_RESOURCE;
	if (strcmp(szType, "HAS_FEATURE") == 0) return SPECIAL_CITY_CONDITION_HAS_FEATURE;
	if (strcmp(szType, "IS_RIVER") == 0) return SPECIAL_CITY_CONDITION_IS_RIVER;
	if (strcmp(szType, "IS_COASTAL") == 0) return SPECIAL_CITY_CONDITION_IS_COASTAL;
	if (strcmp(szType, "IS_PUPPET") == 0) return SPECIAL_CITY_CONDITION_IS_PUPPET;
	if (strcmp(szType, "IS_OTHER_CONTINENT") == 0) return SPECIAL_CITY_CONDITION_IS_OTHER_CONTINENT;
	if (strcmp(szType, "HAS_LAND_AND_SEA_INTERNATIONAL_TR") == 0) return SPECIAL_CITY_CONDITION_HAS_LAND_AND_SEA_INTERNATIONAL_TR;
	if (strcmp(szType, "NO_INTERNATIONAL_TR") == 0) return SPECIAL_CITY_CONDITION_NO_INTERNATIONAL_TR;
	return SPECIAL_CITY_CONDITION_NONE;
}

// Conditions parsed from ConditionType alone, with no Value column to validate.
bool IsBooleanSpecialCityCondition(SpecialCityConditionTypes eType)
{
	return eType == SPECIAL_CITY_CONDITION_IS_RIVER
	    || eType == SPECIAL_CITY_CONDITION_IS_COASTAL
	    || eType == SPECIAL_CITY_CONDITION_IS_PUPPET
	    || eType == SPECIAL_CITY_CONDITION_IS_OTHER_CONTINENT
	    || eType == SPECIAL_CITY_CONDITION_HAS_LAND_AND_SEA_INTERNATIONAL_TR
	    || eType == SPECIAL_CITY_CONDITION_NO_INTERNATIONAL_TR;
}
}

CvSpecialCityTypeEntry::CvSpecialCityTypeEntry(void)
	: m_bConditionsInvalid(false)
{
}

CvSpecialCityTypeEntry::~CvSpecialCityTypeEntry(void)
{
}

bool CvSpecialCityTypeEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	m_vConditionsOr.clear();
	m_vConditionsAnd.clear();
	m_bConditionsInvalid = false;

	// Both condition tables are CHILD tables (no ID column), keyed by SpecialCityType.
	const char* aszTables[2] = {
		"CityStateUAEffect_SpecialCityTypeConditionsOr",
		"CityStateUAEffect_SpecialCityTypeConditionsAnd"
	};
	std::vector<SpecialCityConditionEntry>* apvTarget[2] = { &m_vConditionsOr, &m_vConditionsAnd };

	for (int iTable = 0; iTable < 2; iTable++)
	{
		std::string strKey(aszTables[iTable]);
		Database::Results* pResults = kUtility.GetResults(strKey);
		if (pResults == NULL)
		{
			std::string strQuery = "select ConditionType, Value from ";
			strQuery += aszTables[iTable];
			strQuery += " where SpecialCityType = ?";
			pResults = kUtility.PrepareResults(strKey, strQuery.c_str());
		}
		if (pResults == NULL)
		{
			// A query that cannot even be prepared leaves the condition table empty, which IsCityMatch
			// would read as "no restriction" (fail-open). Fail closed, same as the unparsable-row path.
			m_bConditionsInvalid = true;
			CvAssertMsg(false, "CvSpecialCityTypeEntry: failed to prepare the condition query; this special city type will match no city");
			continue;
		}

		int iRawRows = 0;
		pResults->Bind(1, GetType());
		while (pResults->Step())
		{
			iRawRows++;

			SpecialCityConditionEntry entry;
			entry.m_eConditionType = ParseSpecialCityCondition(pResults->GetText(0));
			entry.m_iValue = -1;
			if (entry.m_eConditionType == SPECIAL_CITY_CONDITION_HAS_RESOURCE
			 || entry.m_eConditionType == SPECIAL_CITY_CONDITION_HAS_FEATURE)
			{
				const char* szValue = pResults->GetText(1);
				if (szValue != NULL)
					entry.m_iValue = GC.getInfoTypeForString(szValue, true);
			}

			if (entry.m_eConditionType != SPECIAL_CITY_CONDITION_NONE
			 && (entry.m_iValue >= 0 || IsBooleanSpecialCityCondition(entry.m_eConditionType)))
			{
				apvTarget[iTable]->push_back(entry);
			}
			else
			{
				CvAssertMsg(false, "CvSpecialCityTypeEntry: dropping unparsable condition row; check ConditionType/Value spelling in the special city type condition tables");
			}
		}

		// A table with rows but no parsable ones must not silently become "no restriction" -- an empty
		// Or table is read as "matches anything" by IsCityMatch, so that would be fail-open. Fail closed.
		if (iRawRows > 0 && apvTarget[iTable]->empty())
		{
			m_bConditionsInvalid = true;
			CvAssertMsg(false, "CvSpecialCityTypeEntry: every condition row failed to parse; this special city type will match no city");
		}
	}

	// A named type with no condition rows at all has no predicate, and IsCityMatch would read the empty
	// Or table as "no restriction" and match every city. Only a config omission can produce this (a
	// pure-AND type always has And rows), so fail closed rather than silently buffing every city.
	if (m_vConditionsOr.empty() && m_vConditionsAnd.empty())
	{
		m_bConditionsInvalid = true;
		CvAssertMsg(false, "CvSpecialCityTypeEntry: special city type has no condition rows; it would match every city");
	}

	return true;
}

bool CvSpecialCityTypeEntry::EvaluateCondition(const SpecialCityConditionEntry& kCondition, const CvCity* pCity) const
{
	if (pCity == NULL) return false;

	switch (kCondition.m_eConditionType)
	{
	case SPECIAL_CITY_CONDITION_HAS_RESOURCE:
		// "Developed" = the city owns the resource and it is improved (bImproved = true)
		return pCity->GetNumResourceLocal((ResourceTypes)kCondition.m_iValue, true) > 0;
	case SPECIAL_CITY_CONDITION_HAS_FEATURE:
		return pCity->IsHasFeatureLocal((FeatureTypes)kCondition.m_iValue);
	case SPECIAL_CITY_CONDITION_IS_RIVER:
		// City center sits on a river, same test the Water Mill / Garden build requirement uses.
		return pCity->plot() != NULL && pCity->plot()->isRiver();
	case SPECIAL_CITY_CONDITION_IS_COASTAL:
		// Default min water size counts the sea only; lakes do not qualify as coastal.
		return pCity->isCoastal();
	case SPECIAL_CITY_CONDITION_IS_PUPPET:
		// IsPuppet() is false for cities under annexation, so those are excluded as well.
		return pCity->IsPuppet();
	case SPECIAL_CITY_CONDITION_IS_OTHER_CONTINENT:
	{
		// Different landmass from the owner's original capital, the same test Panama's cross-continent
		// trade route uses. Fails closed when the original capital is gone (GetOriginalCapitalX/Y = -1).
		const CvPlayer& kPlayer = GET_PLAYER(pCity->getOwner());
		if (kPlayer.GetOriginalCapitalX() < 0 || kPlayer.GetOriginalCapitalY() < 0)
			return false;
		CvPlot* pCapitalPlot = GC.getMap().plot(kPlayer.GetOriginalCapitalX(), kPlayer.GetOriginalCapitalY());
		return pCity->plot() != NULL && pCapitalPlot != NULL
			&& pCity->plot()->getLandmass() != pCapitalPlot->getLandmass();
	}
	case SPECIAL_CITY_CONDITION_HAS_LAND_AND_SEA_INTERNATIONAL_TR:
	{
		// True when the city is the origin of at least one international land route AND at least one
		// international sea route. Domestic routes (both ends on the same team) do not count; the
		// international test goes through IsConnectionInternational so it matches the engine-wide
		// team-based definition rather than comparing player IDs.
		CvGameTrade* pTrade = GC.getGame().GetGameTrade();
		if (pTrade == NULL) return false;
		bool bLand = false;
		bool bSea = false;
		for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++)
		{
			if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) continue;
			const TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
			if (pConnection->m_iOriginX != pCity->getX() || pConnection->m_iOriginY != pCity->getY()) continue;
			if (!pTrade->IsConnectionInternational(*pConnection)) continue;
			if (pConnection->m_eDomain == DOMAIN_LAND) bLand = true;
			else if (pConnection->m_eDomain == DOMAIN_SEA) bSea = true;
			if (bLand && bSea) return true;
		}
		return false;
	}
	case SPECIAL_CITY_CONDITION_NO_INTERNATIONAL_TR:
	{
		// True when the city is neither the origin nor the destination of any international trade route.
		// Mirrors the origin test in HAS_LAND_AND_SEA_INTERNATIONAL_TR and adds the destination side.
		CvGameTrade* pTrade = GC.getGame().GetGameTrade();
		if (pTrade == NULL) return true;
		for (uint iTradeRoute = 0; iTradeRoute < pTrade->m_aTradeConnections.size(); iTradeRoute++)
		{
			if (pTrade->IsTradeRouteIndexEmpty(iTradeRoute)) continue;
			const TradeConnection* pConnection = &(pTrade->m_aTradeConnections[iTradeRoute]);
			if (!pTrade->IsConnectionInternational(*pConnection)) continue;
			if ((pConnection->m_iOriginX == pCity->getX() && pConnection->m_iOriginY == pCity->getY()) ||
			    (pConnection->m_iDestX   == pCity->getX() && pConnection->m_iDestY   == pCity->getY()))
				return false;
		}
		return true;
	}
	default:
		return false;
	}
}

bool CvSpecialCityTypeEntry::IsCityMatch(const CvCity* pCity) const
{
	if (pCity == NULL) return false;

	// Fail closed when the type's own condition rows could not be parsed: the empty Or table below
	// would otherwise be read as "no restriction" and match every city.
	if (m_bConditionsInvalid)
		return false;

	// Every And-row must match; an empty And table imposes no restriction.
	for (size_t i = 0; i < m_vConditionsAnd.size(); i++)
	{
		if (!EvaluateCondition(m_vConditionsAnd[i], pCity))
			return false;
	}

	// An empty Or table imposes no restriction; otherwise at least one Or-row must match.
	if (m_vConditionsOr.empty())
		return true;
	for (size_t i = 0; i < m_vConditionsOr.size(); i++)
	{
		if (EvaluateCondition(m_vConditionsOr[i], pCity))
			return true;
	}
	return false;
}

//======================================================================================================
// CvCityStateUASpecialCityTypeXMLEntries
//======================================================================================================
CvCityStateUASpecialCityTypeXMLEntries::CvCityStateUASpecialCityTypeXMLEntries(void)
{
}

CvCityStateUASpecialCityTypeXMLEntries::~CvCityStateUASpecialCityTypeXMLEntries(void)
{
	DeleteArray();
}

std::vector<CvSpecialCityTypeEntry*>& CvCityStateUASpecialCityTypeXMLEntries::GetEntries()
{
	return m_paEntries;
}

int CvCityStateUASpecialCityTypeXMLEntries::GetNumEntries() const
{
	return (int)m_paEntries.size();
}

CvSpecialCityTypeEntry* CvCityStateUASpecialCityTypeXMLEntries::GetEntry(int index) const
{
	if (index >= 0 && index < (int)m_paEntries.size())
		return m_paEntries[index];
	return NULL;
}

void CvCityStateUASpecialCityTypeXMLEntries::DeleteArray()
{
	for (size_t i = 0; i < m_paEntries.size(); i++)
	{
		SAFE_DELETE(m_paEntries[i]);
	}
	m_paEntries.clear();
}

// True when the array carries any non-zero entry. The fixed-size CSUA sub-table arrays are always
// allocated (Initialize2DArray / PopulateArrayByValue), so this is the only way to tell whether an
// effect actually uses one of them; CvCityStateUAEffectEntry::CacheResults stores the result in the
// matching "Has*" flag so ApplyEffect can skip the whole-dimension scan for effects that do not.
static bool HasAnyNonZero(const int* piArray, int iCount)
{
	if (piArray == NULL)
		return false;

	for (int i = 0; i < iCount; i++)
	{
		if (piArray[i] != 0)
			return true;
	}
	return false;
}

//======================================================================================================
// CvCityStateUAEffectEntry
//======================================================================================================
CvCityStateUAEffectEntry::CvCityStateUAEffectEntry(void)
	: m_iFaithPurchaseGreatPeopleCostRiseModifier(0)
	, m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW(0)
	, m_piGreatPersonPoints(nullptr)
	, m_bFaithPurchaseAllGreatPeople(false)
	, m_bGPNoDeathAfterGreatWork(false)
	, m_iGPConcertTourismRetentionPercent(0)
	, m_iGreatMusicianConcertGoldPercent(0)
	, m_iCapitalAndSecondCapitalCultureModifier(0)
	, m_iImmigrationRatePerImmigrant(0)
	, m_iImmigrationRateMax(0)
	, m_iEmigrationRatePerImmigrant(0)
	, m_iEmigrationRateMax(0)
	, m_bPuppetNoTechCostPenalty(false)
	, m_iPuppetTechCostPartial(0)
	, m_bCanPillageNeutralTradeRoute(false)
	, m_iPlunderTradeRouteGold(0)
	, m_iPlunderTradeRouteXP(0)
	, m_iPlunderTradeRouteOpinionPenalty(0)
	, m_iGarrisonCityDefenseModifier(0)
	, m_iMilitaryUnitProductionXP(0)
	, m_iZOCRangeBonus(0)
	, m_bLandUnitsImmuneRiverCrossing(false)
	, m_iWoundedFixedDamage(0)
	, m_iEnemyFixedDamageModifierInBorders(0)
	, m_iCulturePerWarPeace(0)
	, m_iEnemyCombatModifierInBordersPerBeenDoW(0)
	, m_iUnitProductionModifierPerCity(0)
	, m_iCombatBonusPerTechDifference(0)
	, m_iCityAttackIgnoreBuildingDefensePercent(0)
	, m_iMilitaryXPPerTurnModifier(0)
	, m_iMilitaryXPSeaAir(0)
	, m_iCoupChanceModifier(0)
	, m_bCoupFailSpySurvives(false)
	, m_iStealTechSpeedPerSpy(0)
	, m_iSpyKillChancePerSpy(0)
	, m_iReligionSpreadSpeedModifier(0)
	, m_iPapalRecognitionVotes(0)
	, m_iPapalRecognitionAllyVotes(0)
	, m_piHolyCityYieldModifierPerFollowingCity(nullptr)
	, m_iReligiousPressureModifierPerHolyCity(0)
	, m_bDenounceImmunity(false)
	, m_piCapitalYieldModifierPerFollowingCity(nullptr)
	, m_iLandTradeRouteDistancePerTradeSlot(0)
	, m_iTradeRouteGoldPercentNonNeighbor(0)
	, m_iHappinessPerGoldDonated(0)
	, m_iGoldDonationInterval(0)
	, m_iWonderProductionPerDonationHappiness(0)
	, m_iIdeologyPressurePerDonationHappiness(0)
	, m_iGoldDonationInfluenceModifierPerSeaRoute(0)
	, m_piFriendCityStateYieldModifiers(nullptr)
	, m_piAllyCityStateYieldModifiers(nullptr)
	, m_piPolicyYieldModifiers(nullptr)
	, m_iGoldenAgeThresholdPerPopulation(0)
	, m_iLuxuryHappinessModifier(0)
	, m_iFoodKeptModifierPerLuxury(0)
	, m_iTradeRouteGoldModifierPerLuxuryType(0)
	, m_iTradeRouteGoldModifierPerDistance(0)
	, m_iUnhappinessReductionPerCrossContinentRoute(0)
	, m_iTradeRouteGoldPercentInternational(0)
	, m_iTradeRouteGoldModifierPerInternationalRoute(0)
	, m_iFoodModifierPerHappyLuxuryType(0)
	, m_iFoodModifierPerHappyLuxuryCap(0)
	, m_iResearchAgreementBreakBonusPercent(0)
	, m_iEnemyCityNoHealBesiegeCount(0)
	, m_ppiBuildingClassYieldModifiers(NULL)
	, m_piSpecialistPointRate(nullptr)
	, m_piGreatPersonOneShotModifier(nullptr)
	, m_piSpyGarrisonYieldModifiers(nullptr)
	, m_iSpyKillGainSpyProgress(0)
	, m_ppiResourceYieldModifiers(NULL)
	, m_iCoastalCityGrowthThresholdModifier(0)
	, m_iDiplomaticPrestigePerCity(0)
	, m_ppiImprovementYieldModifiers(NULL)
	, m_piImprovementHappiness(nullptr)
	, m_piBuildingClassHappiness(nullptr)
	, m_iLocalHappinessCapModifier(0)
	, m_iGoldenAgeBuildingMaintenanceMod(0)
	, m_piTradeRouteGoldPerSurplusResource(nullptr)
	, m_bHasBuildingClassYieldModifiers(false)
	, m_bHasResourceYieldModifiers(false)
	, m_bHasImprovementYieldModifiers(false)
	, m_bHasImprovementHappiness(false)
	, m_bHasBuildingClassHappiness(false)
	, m_bHasTradeRouteGoldPerSurplusResource(false)
	, m_bHasSpecialistPointRate(false)
	, m_bHasGreatPersonOneShotModifier(false)
	, m_bHasFaithGPClassCostModifier(false)
	, m_bHasGreatPersonPoints(false)
	, m_iHappinessPerFollowingCity(0)
	, m_iFaithInfluencePurchaseCostDivisor(0)
	, m_iFaithInfluencePurchasePerTurnLimit(0)
	, m_bFaithBeliefPurchase(false)
	, m_iInquisitorRetentionPercent(0)
	, m_bInquisitorCanUseOnCityState(false)
	, m_bFaithPantheonPurchase(false)
	, m_iGreatPersonRateModifierPerGreatWork(0)
	, m_iFaithRefundPerDonationPercent(0)
	, m_iDiplomaticPrestigePerMajorityCiv(0)
	, m_iInfluencePerTurnPerFollowCityMod(0)
	, m_iFollowingCityDivisor(0)
	, m_piImmigrantYieldModifiers(nullptr)
	, m_iImmigrantCashPercent(0)
	, m_iImmigrantCashCapBase(0)
	, m_iCoastalCityHappiness(0)
	, m_piHappinessYieldModifiers(nullptr)
	, m_piHappinessYieldModifierCaps(nullptr)
	, m_piFaithGPClassCostModifier(nullptr)
	, m_piGoldenAgeYieldModifiers(nullptr)
	, m_iHolySiteHappiness(0)
	, m_iGreatPersonRateModifierPerNationalWonder(0)
	, m_iLeagueVotesPerDoF(0)
	, m_iWorldWonderHappiness(0)
{
}

CvCityStateUAEffectEntry::~CvCityStateUAEffectEntry(void)
{
	SAFE_DELETE_ARRAY(m_piGreatPersonPoints);
	CvDatabaseUtility::SafeDelete2DArray(m_ppiBuildingClassYieldModifiers);
	SAFE_DELETE_ARRAY(m_piSpecialistPointRate);

	SAFE_DELETE_ARRAY(m_piGreatPersonOneShotModifier);
	SAFE_DELETE_ARRAY(m_piSpyGarrisonYieldModifiers);
	SAFE_DELETE_ARRAY(m_piFriendCityStateYieldModifiers);
	SAFE_DELETE_ARRAY(m_piAllyCityStateYieldModifiers);
	SAFE_DELETE_ARRAY(m_piPolicyYieldModifiers);
	SAFE_DELETE_ARRAY(m_piCapitalYieldModifierPerFollowingCity);
	SAFE_DELETE_ARRAY(m_piHolyCityYieldModifierPerFollowingCity);
	CvDatabaseUtility::SafeDelete2DArray(m_ppiResourceYieldModifiers);
	CvDatabaseUtility::SafeDelete2DArray(m_ppiImprovementYieldModifiers);
	SAFE_DELETE_ARRAY(m_piImprovementHappiness);
	SAFE_DELETE_ARRAY(m_piBuildingClassHappiness);
	SAFE_DELETE_ARRAY(m_piTradeRouteGoldPerSurplusResource);
	SAFE_DELETE_ARRAY(m_piImmigrantYieldModifiers);
	SAFE_DELETE_ARRAY(m_piHappinessYieldModifiers);
	SAFE_DELETE_ARRAY(m_piHappinessYieldModifierCaps);
	SAFE_DELETE_ARRAY(m_piFaithGPClassCostModifier);
	SAFE_DELETE_ARRAY(m_piGoldenAgeYieldModifiers);
}

bool CvCityStateUAEffectEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	// Reset the presence flags; they are set below as each sub-table is read back, so ApplyEffect can
	// skip dimensions this effect does not use. Setting them here (not in the constructor) also keeps
	// a reload of the same entry correct.
	m_bHasBuildingClassYieldModifiers				= false;
	m_bHasResourceYieldModifiers					= false;
	m_bHasImprovementYieldModifiers					= false;
	m_bHasImprovementHappiness						= false;
	m_bHasBuildingClassHappiness					= false;
	m_bHasTradeRouteGoldPerSurplusResource			= false;
	m_bHasSpecialistPointRate						= false;
	m_bHasGreatPersonOneShotModifier				= false;
	m_bHasFaithGPClassCostModifier					= false;
	m_bHasGreatPersonPoints							= false;

	m_iFaithPurchaseGreatPeopleCostRiseModifier			= kResults.GetInt("FaithPurchaseGreatPeopleCostRiseModifier");
	m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW		= kResults.GetInt("FaithPurchaseGreatPeopleCostRiseModifierPerGW");
	kUtility.PopulateArrayByValue(m_piGreatPersonPoints, "Specialists", "CityStateUAEffect_GreatPersonPoints", "SpecialistType", "EffectType", GetType(), "Points");
	m_bHasGreatPersonPoints = HasAnyNonZero(m_piGreatPersonPoints, GC.getNumSpecialistInfos());
	m_bFaithPurchaseAllGreatPeople					= kResults.GetBool("FaithPurchaseAllGreatPeople");

	m_bGPNoDeathAfterGreatWork						= kResults.GetBool("GPNoDeathAfterGreatWork");
	m_iGPConcertTourismRetentionPercent				= kResults.GetInt("GPConcertTourismRetentionPercent");

	m_iGreatPersonRateModifierPerNationalWonder		= kResults.GetInt("GreatPersonRateModifierPerNationalWonder");
	m_iLeagueVotesPerDoF							= kResults.GetInt("LeagueVotesPerDoF");
	m_iWorldWonderHappiness							= kResults.GetInt("WorldWonderHappiness");
	m_iCultureVictoryProgressModifier				= kResults.GetInt("CultureVictoryProgressModifier");

	m_iGreatMusicianConcertGoldPercent				= kResults.GetInt("GreatMusicianConcertGoldPercent");

	m_iCapitalAndSecondCapitalCultureModifier		= kResults.GetInt("CapitalAndSecondCapitalCultureModifier");

	m_iImmigrationRatePerImmigrant					= kResults.GetInt("ImmigrationRatePerImmigrant");
	m_iImmigrationRateMax							= kResults.GetInt("ImmigrationRateMax");
	m_iEmigrationRatePerImmigrant					= kResults.GetInt("EmigrationRatePerImmigrant");
	m_iEmigrationRateMax							= kResults.GetInt("EmigrationRateMax");

	m_bPuppetNoTechCostPenalty						= kResults.GetBool("PuppetNoTechCostPenalty");
	m_iPuppetTechCostPartial						= kResults.GetInt("PuppetTechCostPartial");

	m_bCanPillageNeutralTradeRoute					= kResults.GetBool("CanPillageNeutralTradeRoute");
	m_iPlunderTradeRouteGold						= kResults.GetInt("PlunderTradeRouteGold");
	m_iPlunderTradeRouteXP							= kResults.GetInt("PlunderTradeRouteXP");
	m_iPlunderTradeRouteOpinionPenalty				= kResults.GetInt("PlunderTradeRouteOpinionPenalty");

	m_iGarrisonCityDefenseModifier					= kResults.GetInt("GarrisonCityDefenseModifier");
	m_iMilitaryUnitProductionXP						= kResults.GetInt("MilitaryUnitProductionXP");
	m_iZOCRangeBonus								= kResults.GetInt("ZOCRangeBonus");

	m_bLandUnitsImmuneRiverCrossing				= kResults.GetBool("LandUnitsImmuneRiverCrossing");

	m_iWoundedFixedDamage							= kResults.GetInt("WoundedFixedDamage");

	m_iEnemyFixedDamageModifierInBorders			= kResults.GetInt("EnemyFixedDamageModifierInBorders");
	m_iCulturePerWarPeace							= kResults.GetInt("CulturePerWarPeace");
	m_iEnemyCombatModifierInBordersPerBeenDoW		= kResults.GetInt("EnemyCombatModifierInBordersPerBeenDoW");

	m_iUnitProductionModifierPerCity				= kResults.GetInt("UnitProductionModifierPerCity");
	m_iCombatBonusPerTechDifference				= kResults.GetInt("CombatBonusPerTechDifference");

	//Mbanza Kongo: each owned city provides the listed resource (conditions mirror Policy_CityResources)
	{
		m_vResourcePerCity.clear();
		std::string strKeyResPerCity("CityStateUAEffect_ResourcePerCity");
		Database::Results* pResultsResPerCity = kUtility.GetResults(strKeyResPerCity);
		if(pResultsResPerCity == NULL)
		{
			pResultsResPerCity = kUtility.PrepareResults(strKeyResPerCity, "select Resources.ID as ResourceID, Quantity, CityScaleType, LargerScaleValid, MustCoastal from CityStateUAEffect_ResourcePerCity left join Resources on Resources.Type = ResourceType where EffectType = ?");
		}

		pResultsResPerCity->Bind(1, GetType());
		while(pResultsResPerCity->Step())
		{
			ResourcePerCityEntry entry;
			entry.m_iResource = pResultsResPerCity->GetInt(0);
			entry.m_iQuantity = pResultsResPerCity->GetInt(1);
			entry.m_iCityScale = GC.getInfoTypeForString(pResultsResPerCity->GetText(2));
			entry.m_bLargerScaleValid = pResultsResPerCity->GetBool(3);
			entry.m_bMustCoastal = pResultsResPerCity->GetBool(4);
			m_vResourcePerCity.push_back(entry);
		}
	}

	m_iCityAttackIgnoreBuildingDefensePercent		= kResults.GetInt("CityAttackIgnoreBuildingDefensePercent");
	m_iMilitaryXPPerTurnModifier					= kResults.GetInt("MilitaryXPPerTurnModifier");
	m_iMilitaryXPSeaAir								= kResults.GetInt("MilitaryXPSeaAir");

	m_iCoupChanceModifier							= kResults.GetInt("CoupChanceModifier");
	m_bCoupFailSpySurvives							= kResults.GetBool("CoupFailSpySurvives");
	m_iStealTechSpeedPerSpy							= kResults.GetInt("StealTechSpeedPerSpy");
	m_iSpyKillChancePerSpy							= kResults.GetInt("SpyKillChancePerSpy");

	m_iReligionSpreadSpeedModifier					= kResults.GetInt("ReligionSpreadSpeedModifier");
	m_iPapalRecognitionVotes						= kResults.GetInt("PapalRecognitionVotes");
	m_iPapalRecognitionAllyVotes					= kResults.GetInt("PapalRecognitionAllyVotes");
	// Vatican: per following city, the holy city gains a yield percentage modifier (per YieldType, 100 = +1%)
	kUtility.PopulateArrayByValue(m_piHolyCityYieldModifierPerFollowingCity, "Yields", "CityStateUAEffect_HolyCityYieldModifierPerFollowingCity", "YieldType", "EffectType", GetType(), "Modifier");
	// Jerusalem: per holy city owned religious pressure (founder's religion), ally denounce immunity, per following-city capital yield modifier
	m_iReligiousPressureModifierPerHolyCity			= kResults.GetInt("ReligiousPressureModifierPerHolyCity");
	m_bDenounceImmunity								= kResults.GetBool("DenounceImmunity");
	kUtility.PopulateArrayByValue(m_piCapitalYieldModifierPerFollowingCity, "Yields", "CityStateUAEffect_CapitalYieldModifierPerFollowingCity", "YieldType", "EffectType", GetType(), "Modifier");

	m_iLandTradeRouteDistancePerTradeSlot			= kResults.GetInt("LandTradeRouteDistancePerTradeSlot");
	m_iTradeRouteGoldPercentNonNeighbor				= kResults.GetInt("TradeRouteGoldPercentNonNeighbor");

	m_iHappinessPerGoldDonated						= kResults.GetInt("HappinessPerGoldDonated");
	m_iGoldDonationInterval							= kResults.GetInt("GoldDonationInterval");
	m_iWonderProductionPerDonationHappiness			= kResults.GetInt("WonderProductionPerDonationHappiness");
	m_iIdeologyPressurePerDonationHappiness			= kResults.GetInt("IdeologyPressurePerDonationHappiness");

	m_iGoldDonationInfluenceModifierPerSeaRoute		= kResults.GetInt("GoldDonationInfluenceModifierPerSeaRoute");
	m_iGoldenAgeThresholdPerPopulation				= kResults.GetInt("GoldenAgeThresholdPerPopulation");

	//CityState UA (Genoa / Vilnius): per-unit yield % modifiers, keyed by YieldType (Rate=100 => +1% per friend/ally CS or unlocked policy)
	kUtility.PopulateArrayByValue(m_piFriendCityStateYieldModifiers, "Yields", "CityStateUAEffect_FriendCityStateYieldModifiers", "YieldType", "EffectType", GetType(), "YieldMod");
	kUtility.PopulateArrayByValue(m_piAllyCityStateYieldModifiers, "Yields", "CityStateUAEffect_AllyCityStateYieldModifiers", "YieldType", "EffectType", GetType(), "YieldMod");
	kUtility.PopulateArrayByValue(m_piPolicyYieldModifiers, "Yields", "CityStateUAEffect_PolicyYieldModifiers", "YieldType", "EffectType", GetType(), "YieldMod");

	m_iLuxuryHappinessModifier						= kResults.GetInt("LuxuryHappinessModifier");
	//Ragusa: percent modifier on the city's local-happiness cap
	m_iLocalHappinessCapModifier						= kResults.GetInt("LocalHappinessCapModifier");
	//Monaco: golden-age building maintenance modifier for the ally
	m_iGoldenAgeBuildingMaintenanceMod				= kResults.GetInt("GoldenAgeBuildingMaintenanceMod");
	m_iFoodKeptModifierPerLuxury						= kResults.GetInt("FoodKeptModifierPerLuxury");
	m_iTradeRouteGoldModifierPerLuxuryType			= kResults.GetInt("TradeRouteGoldModifierPerLuxuryType");
	m_iTradeRouteGoldModifierPerDistance				= kResults.GetInt("TradeRouteGoldModifierPerDistance");
	m_iUnhappinessReductionPerCrossContinentRoute	= kResults.GetInt("UnhappinessReductionPerCrossContinentRoute");

	m_iTradeRouteGoldPercentInternational = kResults.GetInt("TradeRouteGoldPercentInternational");
	m_iTradeRouteGoldModifierPerInternationalRoute = kResults.GetInt("TradeRouteGoldModifierPerInternationalRoute");
	m_iFoodModifierPerHappyLuxuryType = kResults.GetInt("FoodModifierPerHappyLuxuryType");
	m_iFoodModifierPerHappyLuxuryCap = kResults.GetInt("FoodModifierPerHappyLuxuryCap");
	m_iResearchAgreementBreakBonusPercent = kResults.GetInt("ResearchAgreementBreakBonusPercent");

	m_iEnemyCityNoHealBesiegeCount					= kResults.GetInt("EnemyCityNoHealBesiegeCount");
	m_iSpyKillGainSpyProgress						= kResults.GetInt("SpyKillGainSpyProgress");

	m_iCoastalCityGrowthThresholdModifier			= kResults.GetInt("CoastalCityGrowthThresholdModifier");
	m_iDiplomaticPrestigePerCity					= kResults.GetInt("DiplomaticPrestigePerCity");

	//CityState UA (Melbourne): city owning the specified improved resource grants yield percentage modifiers
	{
		kUtility.Initialize2DArray(m_ppiResourceYieldModifiers, "Resources", "Yields");

		std::string strKey("CityStateUAEffect_ResourceYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Resources.ID as ResourceID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_ResourceYieldModifiers inner join Resources on Resources.Type = ResourceType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			const int iResourceID = pResults->GetInt(0);
			const int iYieldID = pResults->GetInt(1);
			const int iYieldMod = pResults->GetInt(2);

			m_ppiResourceYieldModifiers[iResourceID][iYieldID] = iYieldMod;
			m_bHasResourceYieldModifiers = true;
		}
	}

	//CityState UA (Antananarivo): each worked plot holding the specified improvement grants yield percentage modifiers
	{
		kUtility.Initialize2DArray(m_ppiImprovementYieldModifiers, "Improvements", "Yields");

		std::string strKey("CityStateUAEffect_ImprovementYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Improvements.ID as ImprovementID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_ImprovementYieldModifiers inner join Improvements on Improvements.Type = ImprovementType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			const int iImprovementID = pResults->GetInt(0);
			const int iYieldID = pResults->GetInt(1);
			const int iYieldMod = pResults->GetInt(2);

			m_ppiImprovementYieldModifiers[iImprovementID][iYieldID] = iYieldMod;
			m_bHasImprovementYieldModifiers = true;
		}
	}

	//CityState UA (Zanzibar): each worked plot holding the specified improvement grants flat local happiness
	kUtility.PopulateArrayByValue(m_piImprovementHappiness, "Improvements", "CityStateUAEffect_ImprovementHappiness", "ImprovementType", "EffectType", GetType(), "Happiness");
	m_bHasImprovementHappiness = HasAnyNonZero(m_piImprovementHappiness, GC.getNumImprovementInfos());
	//CityState UA (Ragusa): each owned building of the specified class grants flat local happiness
	kUtility.PopulateArrayByValue(m_piBuildingClassHappiness, "BuildingClasses", "CityStateUAEffect_BuildingClassHappiness", "BuildingClassType", "EffectType", GetType(), "Happiness");
	m_bHasBuildingClassHappiness = HasAnyNonZero(m_piBuildingClassHappiness, GC.getNumBuildingClassInfos());
	//Gangtok
	m_iHappinessPerFollowingCity = kResults.GetInt("HappinessPerFollowingCity");
	m_iFaithInfluencePurchaseCostDivisor = kResults.GetInt("FaithInfluencePurchaseCostDivisor");
	m_iFaithInfluencePurchasePerTurnLimit = kResults.GetInt("FaithInfluencePurchasePerTurnLimit");
	m_bFaithBeliefPurchase = kResults.GetBool("FaithBeliefPurchase");
	m_iInquisitorRetentionPercent = kResults.GetInt("InquisitorRetentionPercent");
	m_bInquisitorCanUseOnCityState = kResults.GetBool("InquisitorCanUseOnCityState");
	m_bFaithPantheonPurchase = kResults.GetBool("FaithPantheonPurchase");
	m_iGreatPersonRateModifierPerGreatWork = kResults.GetInt("GreatPersonRateModifierPerGreatWork");
	m_iFaithRefundPerDonationPercent = kResults.GetInt("FaithRefundPerDonationPercent");
	m_iDiplomaticPrestigePerMajorityCiv = kResults.GetInt("DiplomaticPrestigePerMajorityCiv");
	m_iInfluencePerTurnPerFollowCityMod = kResults.GetInt("InfluencePerTurnPerFollowCityMod");
	m_iFollowingCityDivisor = kResults.GetInt("FollowingCityDivisor");
	// Sydney: per immigrant received yield % modifier per YieldType (Modifier=100 => +1%); cash reward per immigrant
	kUtility.PopulateArrayByValue(m_piImmigrantYieldModifiers, "Yields", "CityStateUAEffect_ImmigrantYieldModifiers", "YieldType", "EffectType", GetType(), "Modifier");
	// Sydney: the cash-per-immigrant reward lives in its own sub-table, not in CityStateUAEffects
	m_iImmigrantCashPercent = 0;
	m_iImmigrantCashCapBase = 0;
	{
		std::string strKeyImmCash("CityStateUAEffect_ImmigrantCashReward");
		Database::Results* pResultsImmCash = kUtility.GetResults(strKeyImmCash);
		if(pResultsImmCash == NULL)
		{
			pResultsImmCash = kUtility.PrepareResults(strKeyImmCash, "select CashPercent, CashCapBase from CityStateUAEffect_ImmigrantCashReward where EffectType = ?");
		}

		if(pResultsImmCash != NULL)
		{
			pResultsImmCash->Bind(1, GetType());
			if(pResultsImmCash->Step())
			{
				m_iImmigrantCashPercent = pResultsImmCash->GetInt(0);
				m_iImmigrantCashCapBase = pResultsImmCash->GetInt(1);
			}
		}
	}

	//BuildingClassYieldModifiers (Prague / Yerevan)
	{
		kUtility.Initialize2DArray(m_ppiBuildingClassYieldModifiers, "BuildingClasses", "Yields");

		std::string strKey("CityStateUAEffect_BuildingClassYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select BuildingClasses.ID as BuildingClassID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_BuildingClassYieldModifiers inner join BuildingClasses on BuildingClasses.Type = BuildingClassType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			const int BuildingClassID = pResults->GetInt(0);
			const int iYieldID = pResults->GetInt(1);
			const int iYieldMod = pResults->GetInt(2);

			m_ppiBuildingClassYieldModifiers[BuildingClassID][iYieldID] = iYieldMod;
			m_bHasBuildingClassYieldModifiers = true;
		}
	}
	//Brussels: specialist great person point accumulation rate (%)
	kUtility.PopulateArrayByValue(m_piSpecialistPointRate, "Specialists", "CityStateUAEffect_SpecialistPointRate", "SpecialistType", "EffectType", GetType(), "Rate");
	m_bHasSpecialistPointRate = HasAnyNonZero(m_piSpecialistPointRate, GC.getNumSpecialistInfos());
	//Prague: city with our own spy garrisoned grants yield percentage modifiers (per YieldType)
	kUtility.PopulateArrayByValue(m_piSpyGarrisonYieldModifiers, "Yields", "CityStateUAEffect_SpyGarrisonYieldModifiers", "YieldType", "EffectType", GetType(), "YieldMod");
	//Brussels: each great work of a class grants great person points to a specialist
	{
		m_vGreatWorkGreatPersonPoints.clear();
		std::string strKey("CityStateUAEffect_GreatWorkGreatPersonPoints");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select GreatWorkClasses.ID as GreatWorkClassID, Specialists.ID as SpecialistID, Rate, CapitalOnly from CityStateUAEffect_GreatWorkGreatPersonPoints inner join GreatWorkClasses on GreatWorkClasses.Type = GreatWorkClassType inner join Specialists on Specialists.Type = SpecialistType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			GreatWorkGreatPersonPointsEntry entry;
			entry.m_iGreatWorkClassType = pResults->GetInt(0);
			entry.m_iSpecialistType = pResults->GetInt(1);
			entry.m_iRate = pResults->GetInt(2);
			entry.m_bCapitalOnly = (pResults->GetInt(3) != 0);
			m_vGreatWorkGreatPersonPoints.push_back(entry);
		}
	}
	//Budapest: each owned unit holding a promotion changes the player's total unit maintenance (negative = cheaper)
	{
		m_vUnitMaintenanceByPromotion.clear();
		std::string strKey("CityStateUAEffect_UnitMaintenanceByPromotion");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select UnitPromotions.ID as PromotionID, MaintenanceChange from CityStateUAEffect_UnitMaintenanceByPromotion inner join UnitPromotions on UnitPromotions.Type = PromotionType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			UnitMaintenanceByPromotionEntry entry;
			entry.m_iPromotion = pResults->GetInt(0);
			entry.m_iChange = pResults->GetInt(1);
			m_vUnitMaintenanceByPromotion.push_back(entry);
		}
	}
	//Almaty: a unit holding a promotion gains extra max HP = ownerKills * ownerSurplusResource * Percent/100
	{
		m_vKillMaxHpByPromotion.clear();
		std::string strKey("CityStateUAEffect_KillMaxHpByPromotion");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select UnitPromotions.ID as PromotionID, Resources.ID as ResourceID, Percent from CityStateUAEffect_KillMaxHpByPromotion inner join UnitPromotions on UnitPromotions.Type = PromotionType inner join Resources on Resources.Type = ResourceType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			KillMaxHpByPromotionEntry entry;
			entry.m_iPromotion = pResults->GetInt(0);
			entry.m_iResource = pResults->GetInt(1);
			entry.m_iPercent = pResults->GetInt(2);
			m_vKillMaxHpByPromotion.push_back(entry);
		}
	}
	//Brussels: specified unit class's one-shot great person output modifier (%)
	kUtility.PopulateArrayByValue(m_piGreatPersonOneShotModifier, "UnitClasses", "CityStateUAEffect_GreatPersonOneShotModifier", "UnitClassType", "EffectType", GetType(), "Modifier");
	m_bHasGreatPersonOneShotModifier = HasAnyNonZero(m_piGreatPersonOneShotModifier, GC.getNumUnitClassInfos());
	{
		m_vBuildingGPP.clear();
		std::string strKey2("CityStateUAEffect_BuildingGreatPersonPoints");
		Database::Results* pResults2 = kUtility.GetResults(strKey2);
		if(pResults2 == NULL)
		{
			pResults2 = kUtility.PrepareResults(strKey2, "select BuildingClasses.ID as BuildingClassID, Specialists.ID as SpecialistID, Points from CityStateUAEffect_BuildingGreatPersonPoints inner join BuildingClasses on BuildingClasses.Type = BuildingClassType inner join Specialists on Specialists.Type = SpecialistType where EffectType = ?");
		}
		pResults2->Bind(1, GetType());
		while(pResults2->Step())
		{
			BuildingGreatPersonPointsEntry entry;
			entry.m_iBuildingClass = pResults2->GetInt(0);
			entry.m_iSpecialist = pResults2->GetInt(1);
			entry.m_iPoints = pResults2->GetInt(2);
			m_vBuildingGPP.push_back(entry);
		}
	}
	{
		m_vBornAllyInfluenceMod.clear();
		std::string strKey3("CityStateUAEffect_BornGreatPersonAllyInfluenceMod");
		Database::Results* pResults3 = kUtility.GetResults(strKey3);
		if(pResults3 == NULL)
		{
			pResults3 = kUtility.PrepareResults(strKey3, "select UnitClasses.ID as UnitClassID, ModPerBorn from CityStateUAEffect_BornGreatPersonAllyInfluenceMod inner join UnitClasses on UnitClasses.Type = UnitClassType where EffectType = ?");
		}
		pResults3->Bind(1, GetType());
		while(pResults3->Step())
		{
			BornGreatPersonAllyInfluenceModEntry entry;
			entry.m_iUnitClassType = pResults3->GetInt(0);
			entry.m_iModPerBorn = pResults3->GetInt(1);
			m_vBornAllyInfluenceMod.push_back(entry);
		}
	}
	{
		m_vBornGreatPersonSpecialistYield.clear();
		std::string strKey("CityStateUAEffect_BornGreatPersonSpecialistYield");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Specialists.ID as SpecialistID, UnitClasses.ID as UnitClassID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_BornGreatPersonSpecialistYield inner join Specialists on Specialists.Type = SpecialistType inner join UnitClasses on UnitClasses.Type = UnitClassType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			BornGreatPersonSpecialistYieldEntry entry;
			entry.m_iSpecialistType = pResults->GetInt(0);
			entry.m_iUnitClassType = pResults->GetInt(1);
			entry.m_iYieldType = pResults->GetInt(2);
			entry.m_iYieldMod = pResults->GetInt(3);
			m_vBornGreatPersonSpecialistYield.push_back(entry);
		}
	}
	//Colombo: flat per-era yield on the international trade route (InternalTR) to this city-state (UCS)
	{
		m_vInternalTRToUCSPerEraYield.clear();
		std::string strKey("CityStateUAEffect_InternalTRToUCSPerEraYield");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldValue from CityStateUAEffect_InternalTRToUCSPerEraYield inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			InternalTRToUCSPerEraYieldEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldValue = pResults->GetInt(1);
			m_vInternalTRToUCSPerEraYield.push_back(entry);
		}
	}
	//Colombo: in cities with a trade route to this city-state (UCS), a percentage of the input yield is granted as extra output yield
	{
		m_vYieldToYieldViaTRToUCS.clear();
		std::string strKey("CityStateUAEffect_YieldToYieldViaTRToUCS");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select YieldsIn.ID as InYieldID, YieldsOut.ID as OutYieldID, Percent, RequireRouteToThisCS from CityStateUAEffect_YieldToYieldViaTRToUCS inner join Yields as YieldsIn on YieldsIn.Type = InYieldType inner join Yields as YieldsOut on YieldsOut.Type = OutYieldType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			YieldToYieldViaTRToUCSEntry entry;
			entry.m_iInYieldType = pResults->GetInt(0);
			entry.m_iOutYieldType = pResults->GetInt(1);
			entry.m_iPercent = pResults->GetInt(2);
			entry.m_bRequireRouteToThisCS = (pResults->GetInt(3) != 0);
			m_vYieldToYieldViaTRToUCS.push_back(entry);
		}
	}
	//Monaco: unconditional conversion, Mod% of the city's input yield is granted as extra output yield
	{
		m_vYieldToYield.clear();
		std::string strKey("CityStateUAEffect_YieldToYield");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select YieldsIn.ID as InYieldID, YieldsOut.ID as OutYieldID, Mod from CityStateUAEffect_YieldToYield inner join Yields as YieldsIn on YieldsIn.Type = InYieldType inner join Yields as YieldsOut on YieldsOut.Type = OutYieldType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			YieldToYieldEntry entry;
			entry.m_iInYieldType = pResults->GetInt(0);
			entry.m_iOutYieldType = pResults->GetInt(1);
			entry.m_iMod = pResults->GetInt(2);
			m_vYieldToYield.push_back(entry);
		}
	}
	//Monaco: outcomes of the first-gold-donation wager
	{
		m_vGoldDonationGamble.clear();
		std::string strKey("CityStateUAEffect_GoldDonationGamble");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Weight, Multiplier from CityStateUAEffect_GoldDonationGamble where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			GoldDonationGambleEntry entry;
			entry.m_iWeight = pResults->GetInt(0);
			entry.m_iMultiplier = pResults->GetInt(1);
			m_vGoldDonationGamble.push_back(entry);
		}
	}
	//Valletta: buying the specified building class grants all units of the specified domain XP
	{
		m_vPurchasedBuildingXP.clear();
		std::string strKey("CityStateUAEffect_PurchasedBuildingXP");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select BuildingClasses.ID as BuildingClassID, Domains.ID as DomainID, XP from CityStateUAEffect_PurchasedBuildingXP inner join BuildingClasses on BuildingClasses.Type = BuildingClassType inner join Domains on Domains.Type = DomainType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			PurchasedBuildingXPEntry entry;
			entry.m_iBuildingClass = pResults->GetInt(0);
			entry.m_iDomain = pResults->GetInt(1);
			entry.m_iXP = pResults->GetInt(2);
			m_vPurchasedBuildingXP.push_back(entry);
		}
	}
	//Valletta: born unit of the specified unit class grants a configurable yield equal to YieldMod% of influence with the specified city-state (MinorCivType)
	{
		m_vUnitBornYield.clear();
		std::string strKey("CityStateUAEffect_UnitBornYield");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select MinorCivilizations.ID as MinorCivID, UnitClasses.ID as UnitClassID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_UnitBornYield inner join MinorCivilizations on MinorCivilizations.Type = MinorCivType inner join UnitClasses on UnitClasses.Type = UnitClassType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			UnitBornYieldEntry entry;
			entry.m_iMinorCivType = pResults->GetInt(0);
			entry.m_iUnitClass = pResults->GetInt(1);
			entry.m_iYieldType = pResults->GetInt(2);
			entry.m_iYieldMod = pResults->GetInt(3);
			m_vUnitBornYield.push_back(entry);
		}
	}
	//Hormuz: each unit of surplus strategic resource grants trade-route gold % (per ResourceType)
	kUtility.PopulateArrayByValue(m_piTradeRouteGoldPerSurplusResource, "Resources", "CityStateUAEffect_TradeRouteGoldPerSurplusResource", "ResourceType", "EffectType", GetType(), "Modifier");
	m_bHasTradeRouteGoldPerSurplusResource = HasAnyNonZero(m_piTradeRouteGoldPerSurplusResource, GC.getNumResourceInfos());
	//Vancouver: global happiness per coastal city (basis points, 100 = +1 happiness per coastal city)
	m_iCoastalCityHappiness = kResults.GetInt("CoastalCityHappiness");
	//Vancouver: per point of net happiness, a yield % modifier per YieldType (YieldMod in basis points, Cap in percent)
	kUtility.PopulateArrayByValue(m_piHappinessYieldModifiers, "Yields", "CityStateUAEffect_HappinessYieldModifiers", "YieldType", "EffectType", GetType(), "YieldMod");
	kUtility.PopulateArrayByValue(m_piHappinessYieldModifierCaps, "Yields", "CityStateUAEffect_HappinessYieldModifiers", "YieldType", "EffectType", GetType(), "Cap");
	//Yerevan: global happiness per worked holy-site improvement (basis points, 100 = +1 global happiness per worked holy site)
	m_iHolySiteHappiness = kResults.GetInt("HolySiteHappiness");
	//Yerevan: literacy rate (owned techs / total techs x 100) grants a yield % modifier per YieldType (YieldMod = basis per literacy point)
	{
		m_vLiteracyYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_LiteracyYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod from CityStateUAEffect_LiteracyYieldModifiers inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			LiteracyYieldModifierEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			m_vLiteracyYieldModifiers.push_back(entry);
		}
	}
	//Yerevan: each born great person of a UnitClassType grants a yield % modifier per YieldType (YieldMod = basis per born GP)
	{
		m_vBornGreatPersonYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_BornGreatPersonYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select UnitClasses.ID as UnitClassID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_BornGreatPersonYieldModifiers inner join UnitClasses on UnitClasses.Type = UnitClassType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			BornGreatPersonNationwideYieldEntry entry;
			entry.m_iUnitClassType = pResults->GetInt(0);
			entry.m_iYieldType = pResults->GetInt(1);
			entry.m_iYieldMod = pResults->GetInt(2);
			m_vBornGreatPersonYieldModifiers.push_back(entry);
		}
	}
	//Yerevan: local plot is an improvement and an adjacent plot's improvement is AdjacentImprovementType -> +Yield
	//ImprovementType (local, affected) is optional; leave NULL/empty = any improved plot (resolved as -1 here).
	{
		m_vAdjacentImprovementYieldChanges.clear();
		std::string strKey("CityStateUAEffect_AdjacentImprovementYieldChanges");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey,
				"select "
				"  case when t.ImprovementType is null then -1 else (select ID from Improvements where Type=t.ImprovementType) end, "
				"  (select ID from Improvements where Type=t.AdjacentImprovementType), "
				"  (select ID from Yields where Type=t.YieldType), "
				"  t.Yield "
				"from CityStateUAEffect_AdjacentImprovementYieldChanges t "
				"where t.EffectType = ?");
		}
		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			AdjacentImprovementYieldChangeEntry entry;
			entry.m_iImprovementType = pResults->GetInt(0);
			entry.m_iAdjacentImprovementType = pResults->GetInt(1);
			entry.m_iYieldType = pResults->GetInt(2);
			entry.m_iYield = pResults->GetInt(3);
			m_vAdjacentImprovementYieldChanges.push_back(entry);
		}
	}
	//Ife: per-unitclass FAITH great-people cost discount (CostRiseModifier in percent, negative = discount)
	kUtility.PopulateArrayByValue(m_piFaithGPClassCostModifier, "UnitClasses", "CityStateUAEffect_FaithGPClassCostModifier", "UnitClassType", "EffectType", GetType(), "CostRiseModifier");
	m_bHasFaithGPClassCostModifier = HasAnyNonZero(m_piFaithGPClassCostModifier, GC.getNumUnitClassInfos());
	//Ife: each great work / artifact of a GreatWorkClassType grants a yield % modifier per YieldType
	{
		m_vGreatWorkYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_GreatWorkYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select GreatWorkClasses.ID as GreatWorkClassID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_GreatWorkYieldModifiers inner join GreatWorkClasses on GreatWorkClasses.Type = GreatWorkClassType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			GreatWorkYieldModifierEntry entry;
			entry.m_iGreatWorkClassType = pResults->GetInt(0);
			entry.m_iYieldType = pResults->GetInt(1);
			entry.m_iYieldMod = pResults->GetInt(2);
			m_vGreatWorkYieldModifiers.push_back(entry);
		}
	}
	//Bucharest: each world wonder owned grants a yield % modifier per YieldType, nation-wide
	{
		m_vWorldWonderYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_WorldWonderYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod, Cap from CityStateUAEffect_WorldWonderYieldModifiers inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			WorldWonderYieldModifierEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			entry.m_iCap = pResults->GetInt(2);
			m_vWorldWonderYieldModifiers.push_back(entry);
		}
	}
	//Mogadishu: each international trade route the ally runs to a city-state grants a yield % modifier
	{
		m_vCityStateTradeRouteYieldModifiersGlobal.clear();
		std::string strKey("CityStateUAEffect_CityStateTradeRouteYieldModifiersGlobal");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod from CityStateUAEffect_CityStateTradeRouteYieldModifiersGlobal inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			CityStateTradeRouteYieldModifierGlobalEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			m_vCityStateTradeRouteYieldModifiersGlobal.push_back(entry);
		}
	}
	//Kabul: each international land trade route grants trade-route gold %, plus a hills-origin bonus
	{
		m_vLandTradeRouteGoldModifiers.clear();
		std::string strKey("CityStateUAEffect_LandTradeRouteGoldModifier");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select YieldMod, HillsBonus from CityStateUAEffect_LandTradeRouteGoldModifier where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			LandTradeRouteGoldModifierEntry entry;
			entry.m_iYieldMod = pResults->GetInt(0);
			entry.m_iHillsBonus = pResults->GetInt(1);
			m_vLandTradeRouteGoldModifiers.push_back(entry);
		}
	}
	//Kabul: each international land trade route to any other player (city-states included) grants a yield % per era
	{
		m_vInternationalLandTradeRouteYieldPerEra.clear();
		std::string strKey("CityStateUAEffect_InternationalLandTradeRouteYieldPerEra");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod from CityStateUAEffect_InternationalLandTradeRouteYieldPerEra inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			InternationalLandTradeRouteYieldEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			m_vInternationalLandTradeRouteYieldPerEra.push_back(entry);
		}
	}
	//Bucharest: each diplomat stationed in a foreign major civilization's city grants a yield % modifier
	{
		m_vDiplomatAbroadYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_DiplomatAbroadYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod, Cap from CityStateUAEffect_DiplomatAbroadYieldModifiers inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			DiplomatAbroadYieldModifierEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			entry.m_iCap = pResults->GetInt(2);
			m_vDiplomatAbroadYieldModifiers.push_back(entry);
		}
	}
	//Quebec: for each met major civilization at Unknown influence toward the player, a yield % modifier
	{
		m_vUnknownInfluenceYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_UnknownInfluenceYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod, Cap from CityStateUAEffect_UnknownInfluenceYieldModifiers inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			UnknownInfluenceYieldModifierEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			entry.m_iCap = pResults->GetInt(2);
			m_vUnknownInfluenceYieldModifiers.push_back(entry);
		}
	}
	//Kiev: each League vote held grants a yield % modifier per YieldType, nation-wide
	{
		m_vLeagueVoteYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_LeagueVoteYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod from CityStateUAEffect_LeagueVoteYieldModifiers inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			LeagueVoteYieldModifierEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			m_vLeagueVoteYieldModifiers.push_back(entry);
		}
	}
	//Ife: while in a golden age, grant a yield % modifier per YieldType (YieldMod in percent, 25 = +25%)
	kUtility.PopulateArrayByValue(m_piGoldenAgeYieldModifiers, "Yields", "CityStateUAEffect_GoldenAgeYieldModifiers", "YieldType", "EffectType", GetType(), "YieldMod");
	//Bogota: a city matching a special city type gains a yield % modifier
	{
		m_vSpecialCityYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_SpecialCityYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select SpecialCityTypes.ID as SpecialCityTypeID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_SpecialCityYieldModifiers inner join CityStateUAEffect_SpecialCityTypes as SpecialCityTypes on SpecialCityTypes.Type = SpecialCityType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			SpecialCityYieldModifierEntry entry;
			entry.m_iSpecialCityType = pResults->GetInt(0);
			entry.m_iYieldType = pResults->GetInt(1);
			entry.m_iYieldMod = pResults->GetInt(2);
			m_vSpecialCityYieldModifiers.push_back(entry);
		}
	}
	//Bogota: per owned city matching a special city type, ALL cities gain a yield % modifier
	{
		m_vSpecialCityCountYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_SpecialCityCountYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select SpecialCityTypes.ID as SpecialCityTypeID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_SpecialCityCountYieldModifiers inner join CityStateUAEffect_SpecialCityTypes as SpecialCityTypes on SpecialCityTypes.Type = SpecialCityType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			SpecialCityCountYieldModifierEntry entry;
			entry.m_iSpecialCityType = pResults->GetInt(0);
			entry.m_iYieldType = pResults->GetInt(1);
			entry.m_iYieldMod = pResults->GetInt(2);
			m_vSpecialCityCountYieldModifiers.push_back(entry);
		}
	}
	//Kuala Lumpur: per N population living in cities matching a special city type, a nation-wide yield %
	{
		m_vSpecialCityPopulationYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_SpecialCityPopulationYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select SpecialCityTypes.ID as SpecialCityTypeID, PerPopulation, Yields.ID as YieldID, YieldMod from CityStateUAEffect_SpecialCityPopulationYieldModifiers inner join CityStateUAEffect_SpecialCityTypes as SpecialCityTypes on SpecialCityTypes.Type = SpecialCityType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			SpecialCityPopulationYieldModifierEntry entry;
			entry.m_iSpecialCityType = pResults->GetInt(0);
			entry.m_iPerPopulation = pResults->GetInt(1);
			entry.m_iYieldType = pResults->GetInt(2);
			entry.m_iYieldMod = pResults->GetInt(3);
			m_vSpecialCityPopulationYieldModifiers.push_back(entry);
		}
	}
	//Tyre: a city matching a special city type takes Percent% less damage
	{
		m_vSpecialCityDamageReductions.clear();
		std::string strKey("CityStateUAEffect_SpecialCityDamageReduction");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select SpecialCityTypes.ID as SpecialCityTypeID, Percent from CityStateUAEffect_SpecialCityDamageReduction inner join CityStateUAEffect_SpecialCityTypes as SpecialCityTypes on SpecialCityTypes.Type = SpecialCityType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			SpecialCityDamageReductionEntry entry;
			entry.m_iSpecialCityType = pResults->GetInt(0);
			entry.m_iPercent = pResults->GetInt(1);
			m_vSpecialCityDamageReductions.push_back(entry);
		}
	}
	//Singapore: each owned building class grants a nation-wide yield % modifier per YieldType
	{
		m_vBuildingClassGlobalYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_BuildingClassGlobalYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select BuildingClasses.ID as BuildingClassID, Yields.ID as YieldID, YieldMod from CityStateUAEffect_BuildingClassGlobalYieldModifiers inner join BuildingClasses on BuildingClasses.Type = BuildingClassType inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			BuildingClassGlobalYieldModifierEntry entry;
			entry.m_iBuildingClass = pResults->GetInt(0);
			entry.m_iYieldType = pResults->GetInt(1);
			entry.m_iYieldMod = pResults->GetInt(2);
			m_vBuildingClassGlobalYieldModifiers.push_back(entry);
		}
	}
	//Singapore: each owned building class lowers the city-count research threshold by TechCostMod points
	{
		m_vBuildingClassTechCostModifiers.clear();
		std::string strKey("CityStateUAEffect_BuildingClassTechCostModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select BuildingClasses.ID as BuildingClassID, TechCostMod from CityStateUAEffect_BuildingClassTechCostModifiers inner join BuildingClasses on BuildingClasses.Type = BuildingClassType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			BuildingClassTechCostModifierEntry entry;
			entry.m_iBuildingClass = pResults->GetInt(0);
			entry.m_iTechCostMod = pResults->GetInt(1);
			m_vBuildingClassTechCostModifiers.push_back(entry);
		}
	}
	//Milan: each point of luxury happiness grants a nation-wide yield % modifier per YieldType
	{
		m_vLuxuryHappinessYieldModifiers.clear();
		std::string strKey("CityStateUAEffect_LuxuryHappinessYieldModifiers");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Yields.ID as YieldID, YieldMod, Cap from CityStateUAEffect_LuxuryHappinessYieldModifiers inner join Yields on Yields.Type = YieldType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			LuxuryHappinessYieldModifierEntry entry;
			entry.m_iYieldType = pResults->GetInt(0);
			entry.m_iYieldMod = pResults->GetInt(1);
			entry.m_iCap = pResults->GetInt(2);
			m_vLuxuryHappinessYieldModifiers.push_back(entry);
		}
	}
	//Milan: a unit takes Percent% less damage when the opposing side lacks the configured tech
	{
		m_vCombatDamageReductionVsNoTech.clear();
		std::string strKey("CityStateUAEffect_CombatDamageReductionVsNoTech");
		Database::Results* pResults = kUtility.GetResults(strKey);
		if(pResults == NULL)
		{
			pResults = kUtility.PrepareResults(strKey, "select Technologies.ID as TechID, Percent from CityStateUAEffect_CombatDamageReductionVsNoTech inner join Technologies on Technologies.Type = TechType where EffectType = ?");
		}

		pResults->Bind(1, GetType());
		while(pResults->Step())
		{
			CombatDamageReductionVsNoTechEntry entry;
			entry.m_iTech = pResults->GetInt(0);
			entry.m_iPercent = pResults->GetInt(1);
			m_vCombatDamageReductionVsNoTech.push_back(entry);
		}
	}

	return true;
}

int CvCityStateUAEffectEntry::GetFaithPurchaseGreatPeopleCostRiseModifier() const { return m_iFaithPurchaseGreatPeopleCostRiseModifier; }
int CvCityStateUAEffectEntry::GetFaithPurchaseGreatPeopleCostRiseModifierPerGW() const { return m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW; }
int CvCityStateUAEffectEntry::GetGreatPersonPoints(int i) const { CvAssertMsg(i < GC.getNumSpecialistInfos(), "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piGreatPersonPoints ? m_piGreatPersonPoints[i] : 0; }
bool CvCityStateUAEffectEntry::IsFaithPurchaseAllGreatPeople() const { return m_bFaithPurchaseAllGreatPeople; }

bool CvCityStateUAEffectEntry::IsGPNoDeathAfterGreatWork() const { return m_bGPNoDeathAfterGreatWork; }
int CvCityStateUAEffectEntry::GetGPConcertTourismRetentionPercent() const { return m_iGPConcertTourismRetentionPercent; }

int CvCityStateUAEffectEntry::GetGreatMusicianConcertGoldPercent() const { return m_iGreatMusicianConcertGoldPercent; }

int CvCityStateUAEffectEntry::GetCapitalAndSecondCapitalCultureModifier() const { return m_iCapitalAndSecondCapitalCultureModifier; }

int CvCityStateUAEffectEntry::GetImmigrationRatePerImmigrant() const { return m_iImmigrationRatePerImmigrant; }
int CvCityStateUAEffectEntry::GetImmigrationRateMax() const { return m_iImmigrationRateMax; }
int CvCityStateUAEffectEntry::GetEmigrationRatePerImmigrant() const { return m_iEmigrationRatePerImmigrant; }
int CvCityStateUAEffectEntry::GetEmigrationRateMax() const { return m_iEmigrationRateMax; }

bool CvCityStateUAEffectEntry::IsPuppetNoTechCostPenalty() const { return m_bPuppetNoTechCostPenalty; }
int CvCityStateUAEffectEntry::GetPuppetTechCostPartial() const { return m_iPuppetTechCostPartial; }

bool CvCityStateUAEffectEntry::IsCanPillageNeutralTradeRoute() const { return m_bCanPillageNeutralTradeRoute; }
int CvCityStateUAEffectEntry::GetPlunderTradeRouteGold() const { return m_iPlunderTradeRouteGold; }
int CvCityStateUAEffectEntry::GetPlunderTradeRouteXP() const { return m_iPlunderTradeRouteXP; }
int CvCityStateUAEffectEntry::GetPlunderTradeRouteOpinionPenalty() const { return m_iPlunderTradeRouteOpinionPenalty; }

int CvCityStateUAEffectEntry::GetGarrisonCityDefenseModifier() const { return m_iGarrisonCityDefenseModifier; }

int CvCityStateUAEffectEntry::GetMilitaryUnitProductionXP() const { return m_iMilitaryUnitProductionXP; }

int CvCityStateUAEffectEntry::GetZOCRangeBonus() const { return m_iZOCRangeBonus; }

bool CvCityStateUAEffectEntry::IsLandUnitsImmuneRiverCrossing() const { return m_bLandUnitsImmuneRiverCrossing; }
int CvCityStateUAEffectEntry::GetWoundedFixedDamage() const { return m_iWoundedFixedDamage; }

int CvCityStateUAEffectEntry::GetEnemyFixedDamageModifierInBorders() const { return m_iEnemyFixedDamageModifierInBorders; }
int CvCityStateUAEffectEntry::GetCulturePerWarPeace() const { return m_iCulturePerWarPeace; }
int CvCityStateUAEffectEntry::GetEnemyCombatModifierInBordersPerBeenDoW() const { return m_iEnemyCombatModifierInBordersPerBeenDoW; }

int CvCityStateUAEffectEntry::GetUnitProductionModifierPerCity() const { return m_iUnitProductionModifierPerCity; }
int CvCityStateUAEffectEntry::GetCombatBonusPerTechDifference() const { return m_iCombatBonusPerTechDifference; }

int CvCityStateUAEffectEntry::GetCityAttackIgnoreBuildingDefensePercent() const { return m_iCityAttackIgnoreBuildingDefensePercent; }
int CvCityStateUAEffectEntry::GetMilitaryXPPerTurnModifier() const { return m_iMilitaryXPPerTurnModifier; }
int CvCityStateUAEffectEntry::GetMilitaryXPSeaAir() const { return m_iMilitaryXPSeaAir; }

int CvCityStateUAEffectEntry::GetCoupChanceModifier() const { return m_iCoupChanceModifier; }
bool CvCityStateUAEffectEntry::GetCoupFailSpySurvives() const { return m_bCoupFailSpySurvives; }
int CvCityStateUAEffectEntry::GetStealTechSpeedPerSpy() const { return m_iStealTechSpeedPerSpy; }
int CvCityStateUAEffectEntry::GetSpyKillChancePerSpy() const { return m_iSpyKillChancePerSpy; }

int CvCityStateUAEffectEntry::GetReligionSpreadSpeedModifier() const { return m_iReligionSpreadSpeedModifier; }
// Vatican: Papal Recognition league delegate votes granted to each following civilization (mainstream votes)
int CvCityStateUAEffectEntry::GetPapalRecognitionVotes() const { return m_iPapalRecognitionVotes; }
// Vatican: Papal Recognition league delegate votes granted to the ally per following civilization (including itself)
int CvCityStateUAEffectEntry::GetPapalRecognitionAllyVotes() const { return m_iPapalRecognitionAllyVotes; }
// Vatican: per following city, the holy city gains a yield percentage modifier (per YieldType, 100 = +1%)
int CvCityStateUAEffectEntry::GetHolyCityYieldModifierPerFollowingCity(int i) const { CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piHolyCityYieldModifierPerFollowingCity ? m_piHolyCityYieldModifierPerFollowingCity[i] : 0; }

// Jerusalem: per holy city owned religious pressure (applies to the founder's religion)
int CvCityStateUAEffectEntry::GetReligiousPressureModifierPerHolyCity() const { return m_iReligiousPressureModifierPerHolyCity; }
// Jerusalem: ally of this city-state cannot be denounced
bool CvCityStateUAEffectEntry::IsDenounceImmunity() const { return m_bDenounceImmunity; }
// Jerusalem / Wittenberg: per following-city capital yield modifier (per YieldType, 100 = +1%)
int CvCityStateUAEffectEntry::GetCapitalYieldModifierPerFollowingCity(int i) const { CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piCapitalYieldModifierPerFollowingCity ? m_piCapitalYieldModifierPerFollowingCity[i] : 0; }

int CvCityStateUAEffectEntry::GetLandTradeRouteDistancePerTradeSlot() const { return m_iLandTradeRouteDistancePerTradeSlot; }
int CvCityStateUAEffectEntry::GetTradeRouteGoldPercentNonNeighbor() const { return m_iTradeRouteGoldPercentNonNeighbor; }

int CvCityStateUAEffectEntry::GetHappinessPerGoldDonated() const { return m_iHappinessPerGoldDonated; }
int CvCityStateUAEffectEntry::GetGoldDonationInterval() const { return m_iGoldDonationInterval; }
int CvCityStateUAEffectEntry::GetWonderProductionPerDonationHappiness() const { return m_iWonderProductionPerDonationHappiness; }
int CvCityStateUAEffectEntry::GetIdeologyPressurePerDonationHappiness() const { return m_iIdeologyPressurePerDonationHappiness; }
int CvCityStateUAEffectEntry::GetGoldDonationInfluenceModifierPerSeaRoute() const { return m_iGoldDonationInfluenceModifierPerSeaRoute; }
int CvCityStateUAEffectEntry::GetFriendCityStateYieldModifier(YieldTypes eYieldType) const
{
	CvAssertMsg(eYieldType >= 0 && eYieldType < NUM_YIELD_TYPES, "Index out of bounds");
	return m_piFriendCityStateYieldModifiers ? m_piFriendCityStateYieldModifiers[(int)eYieldType] : 0;
}
int CvCityStateUAEffectEntry::GetAllyCityStateYieldModifier(YieldTypes eYieldType) const
{
	CvAssertMsg(eYieldType >= 0 && eYieldType < NUM_YIELD_TYPES, "Index out of bounds");
	return m_piAllyCityStateYieldModifiers ? m_piAllyCityStateYieldModifiers[(int)eYieldType] : 0;
}
int CvCityStateUAEffectEntry::GetPolicyYieldModifier(YieldTypes eYieldType) const
{
	CvAssertMsg(eYieldType >= 0 && eYieldType < NUM_YIELD_TYPES, "Index out of bounds");
	return m_piPolicyYieldModifiers ? m_piPolicyYieldModifiers[(int)eYieldType] : 0;
}
int CvCityStateUAEffectEntry::GetGoldenAgeThresholdPerPopulation() const { return m_iGoldenAgeThresholdPerPopulation; }

int CvCityStateUAEffectEntry::GetLuxuryHappinessModifier() const { return m_iLuxuryHappinessModifier; }
int CvCityStateUAEffectEntry::GetLocalHappinessCapModifier() const { return m_iLocalHappinessCapModifier; }
int CvCityStateUAEffectEntry::GetGoldenAgeBuildingMaintenanceMod() const { return m_iGoldenAgeBuildingMaintenanceMod; }
int CvCityStateUAEffectEntry::GetFoodKeptModifierPerLuxury() const { return m_iFoodKeptModifierPerLuxury; }
int CvCityStateUAEffectEntry::GetTradeRouteGoldModifierPerLuxuryType() const { return m_iTradeRouteGoldModifierPerLuxuryType; }
int CvCityStateUAEffectEntry::GetTradeRouteGoldModifierPerDistance() const { return m_iTradeRouteGoldModifierPerDistance; }
int CvCityStateUAEffectEntry::GetUnhappinessReductionPerCrossContinentRoute() const { return m_iUnhappinessReductionPerCrossContinentRoute; }

int CvCityStateUAEffectEntry::GetTradeRouteGoldPercentInternational() const { return m_iTradeRouteGoldPercentInternational; }
int CvCityStateUAEffectEntry::GetTradeRouteGoldModifierPerInternationalRoute() const { return m_iTradeRouteGoldModifierPerInternationalRoute; }
int CvCityStateUAEffectEntry::GetFoodModifierPerHappyLuxuryType() const { return m_iFoodModifierPerHappyLuxuryType; }
int CvCityStateUAEffectEntry::GetFoodModifierPerHappyLuxuryCap() const { return m_iFoodModifierPerHappyLuxuryCap; }
int CvCityStateUAEffectEntry::GetResearchAgreementBreakBonusPercent() const { return m_iResearchAgreementBreakBonusPercent; }

int CvCityStateUAEffectEntry::GetBuildingClassYieldModifiers(int i, int j) const
{
	CvAssertMsg(i < GC.getNumBuildingClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppiBuildingClassYieldModifiers ? m_ppiBuildingClassYieldModifiers[i][j] : 0;
}

int CvCityStateUAEffectEntry::GetSpecialistPointRate(int i) const
{
	CvAssertMsg(i < GC.getNumSpecialistInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piSpecialistPointRate ? m_piSpecialistPointRate[i] : 0;
}

int CvCityStateUAEffectEntry::GetSpyGarrisonYieldModifiers(int i) const
{
	CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piSpyGarrisonYieldModifiers ? m_piSpyGarrisonYieldModifiers[i] : 0;
}

int CvCityStateUAEffectEntry::GetSpyKillGainSpyProgress() const
{
	return m_iSpyKillGainSpyProgress;
}

int CvCityStateUAEffectEntry::GetResourceYieldModifiers(int i, int j) const
{
	CvAssertMsg(i < GC.getNumResourceInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppiResourceYieldModifiers ? m_ppiResourceYieldModifiers[i][j] : 0;
}

int CvCityStateUAEffectEntry::GetCoastalCityGrowthThresholdModifier() const
{
	return m_iCoastalCityGrowthThresholdModifier;
}

int CvCityStateUAEffectEntry::GetDiplomaticPrestigePerCity() const
{
	return m_iDiplomaticPrestigePerCity;
}

int CvCityStateUAEffectEntry::GetImprovementYieldModifiers(int i, int j) const
{
	CvAssertMsg(i < GC.getNumImprovementInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	CvAssertMsg(j < NUM_YIELD_TYPES, "Index out of bounds");
	CvAssertMsg(j > -1, "Index out of bounds");
	return m_ppiImprovementYieldModifiers ? m_ppiImprovementYieldModifiers[i][j] : 0;
}

int CvCityStateUAEffectEntry::GetImprovementHappiness(int i) const
{
	CvAssertMsg(i < GC.getNumImprovementInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	if (!m_piImprovementHappiness || i < 0 || i >= GC.getNumImprovementInfos())
		return 0;
	return m_piImprovementHappiness[i];
}

int CvCityStateUAEffectEntry::GetBuildingClassHappiness(int i) const
{
	CvAssertMsg(i < GC.getNumBuildingClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	if (!m_piBuildingClassHappiness || i < 0 || i >= GC.getNumBuildingClassInfos())
		return 0;
	return m_piBuildingClassHappiness[i];
}

int CvCityStateUAEffectEntry::GetTradeRouteGoldPerSurplusResource(int i) const
{
	CvAssertMsg(i < GC.getNumResourceInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	if (!m_piTradeRouteGoldPerSurplusResource || i < 0 || i >= GC.getNumResourceInfos())
		return 0;
	return m_piTradeRouteGoldPerSurplusResource[i];
}

int CvCityStateUAEffectEntry::GetHappinessPerFollowingCity() const { return m_iHappinessPerFollowingCity; }
int CvCityStateUAEffectEntry::GetFaithInfluencePurchaseCostDivisor() const { return m_iFaithInfluencePurchaseCostDivisor; }
int CvCityStateUAEffectEntry::GetFaithInfluencePurchasePerTurnLimit() const { return m_iFaithInfluencePurchasePerTurnLimit; }
bool CvCityStateUAEffectEntry::GetFaithBeliefPurchase() const { return m_bFaithBeliefPurchase; }
int CvCityStateUAEffectEntry::GetInquisitorRetentionPercent() const { return m_iInquisitorRetentionPercent; }
bool CvCityStateUAEffectEntry::GetInquisitorCanUseOnCityState() const { return m_bInquisitorCanUseOnCityState; }
bool CvCityStateUAEffectEntry::GetFaithPantheonPurchase() const { return m_bFaithPantheonPurchase; }
int CvCityStateUAEffectEntry::GetGreatPersonRateModifierPerGreatWork() const { return m_iGreatPersonRateModifierPerGreatWork; }
int CvCityStateUAEffectEntry::GetGreatPersonRateModifierPerNationalWonder() const { return m_iGreatPersonRateModifierPerNationalWonder; }
int CvCityStateUAEffectEntry::GetLeagueVotesPerDoF() const { return m_iLeagueVotesPerDoF; }
int CvCityStateUAEffectEntry::GetWorldWonderHappiness() const { return m_iWorldWonderHappiness; }
int CvCityStateUAEffectEntry::GetFaithRefundPerDonationPercent() const { return m_iFaithRefundPerDonationPercent; }
int CvCityStateUAEffectEntry::GetDiplomaticPrestigePerMajorityCiv() const { return m_iDiplomaticPrestigePerMajorityCiv; }
int CvCityStateUAEffectEntry::GetInfluencePerTurnPerFollowCityMod() const { return m_iInfluencePerTurnPerFollowCityMod; }
int CvCityStateUAEffectEntry::GetFollowingCityDivisor() const { return m_iFollowingCityDivisor; }
// Sydney: per immigrant received yield % modifier (per YieldType, 100 = +1%)
int CvCityStateUAEffectEntry::GetImmigrantYieldModifier(int i) const { CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piImmigrantYieldModifiers ? m_piImmigrantYieldModifiers[i] : 0; }
bool CvCityStateUAEffectEntry::HasImmigrantYieldModifiers() const { return m_piImmigrantYieldModifiers != NULL; }
int CvCityStateUAEffectEntry::GetCoastalCityHappiness() const { return m_iCoastalCityHappiness; }
int CvCityStateUAEffectEntry::GetHolySiteHappiness() const { return m_iHolySiteHappiness; }
int CvCityStateUAEffectEntry::GetHappinessYieldModifier(int i) const { CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piHappinessYieldModifiers ? m_piHappinessYieldModifiers[i] : 0; }
int CvCityStateUAEffectEntry::GetHappinessYieldModifierCap(int i) const { CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piHappinessYieldModifierCaps ? m_piHappinessYieldModifierCaps[i] : 0; }
int CvCityStateUAEffectEntry::GetImmigrantCashPercent() const { return m_iImmigrantCashPercent; }
int CvCityStateUAEffectEntry::GetImmigrantCashCapBase() const { return m_iImmigrantCashCapBase; }
int CvCityStateUAEffectEntry::GetFaithGPClassCostModifier(int i) const { CvAssertMsg(i < GC.getNumUnitClassInfos(), "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piFaithGPClassCostModifier ? m_piFaithGPClassCostModifier[i] : 0; }
int CvCityStateUAEffectEntry::GetGoldenAgeYieldModifier(int i) const { CvAssertMsg(i < NUM_YIELD_TYPES, "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return m_piGoldenAgeYieldModifiers ? m_piGoldenAgeYieldModifiers[i] : 0; }

int CvCityStateUAEffectEntry::GetGreatPersonOneShotModifier(int i) const
{
	CvAssertMsg(i < GC.getNumUnitClassInfos(), "Index out of bounds");
	CvAssertMsg(i > -1, "Index out of bounds");
	return m_piGreatPersonOneShotModifier ? m_piGreatPersonOneShotModifier[i] : 0;
}

int CvCityStateUAEffectEntry::GetInternalTRToUCSPerEraYield(int eYield) const
{
	int iTotal = 0;
	for (size_t i = 0; i < m_vInternalTRToUCSPerEraYield.size(); i++)
	{
		if (m_vInternalTRToUCSPerEraYield[i].m_iYieldType == eYield)
			iTotal += m_vInternalTRToUCSPerEraYield[i].m_iYieldValue;
	}
	return iTotal;
}

int CvCityStateUAEffectEntry::GetYieldToYieldViaTRToUCS(int eInYield, int eOutYield) const
{
	int iTotal = 0;
	for (size_t i = 0; i < m_vYieldToYieldViaTRToUCS.size(); i++)
	{
		if (m_vYieldToYieldViaTRToUCS[i].m_iInYieldType == eInYield && m_vYieldToYieldViaTRToUCS[i].m_iOutYieldType == eOutYield)
			iTotal += m_vYieldToYieldViaTRToUCS[i].m_iPercent;
	}
	return iTotal;
}

// Mogadishu: whether the entry for (eInYield -> eOutYield) requires a trade route TO this city-state
// (true) or any international trade route originating from the city (false). Defaults to true so
// existing entries (Colombo / Cape Town) keep their route-to-this-city-state semantics.
bool CvCityStateUAEffectEntry::YieldToYieldViaTRToUCSRequiresRouteToThisCS(int eInYield, int eOutYield) const
{
	for (size_t i = 0; i < m_vYieldToYieldViaTRToUCS.size(); i++)
	{
		if (m_vYieldToYieldViaTRToUCS[i].m_iInYieldType == eInYield && m_vYieldToYieldViaTRToUCS[i].m_iOutYieldType == eOutYield)
			return m_vYieldToYieldViaTRToUCS[i].m_bRequireRouteToThisCS;
	}
	return true;
}

// Monaco: unconditional conversion, sum of Mod for all rows matching (eInYield -> eOutYield)
int CvCityStateUAEffectEntry::GetYieldToYield(int eInYield, int eOutYield) const
{
	int iTotal = 0;
	for (size_t i = 0; i < m_vYieldToYield.size(); i++)
	{
		if (m_vYieldToYield[i].m_iInYieldType == eInYield && m_vYieldToYield[i].m_iOutYieldType == eOutYield)
			iTotal += m_vYieldToYield[i].m_iMod;
	}
	return iTotal;
}

int CvCityStateUAEffectEntry::GetEnemyCityNoHealBesiegeCount() const { return m_iEnemyCityNoHealBesiegeCount; }
//======================================================================================================
// CvCityStateUAEffectXMLEntries
//======================================================================================================
CvCityStateUAEffectXMLEntries::CvCityStateUAEffectXMLEntries(void)
{
}

CvCityStateUAEffectXMLEntries::~CvCityStateUAEffectXMLEntries(void)
{
	DeleteArray();
}

std::vector<CvCityStateUAEffectEntry*>& CvCityStateUAEffectXMLEntries::GetEffectEntries()
{
	return m_paEffectEntries;
}

int CvCityStateUAEffectXMLEntries::GetNumEffects() const
{
	return (int)m_paEffectEntries.size();
}

CvCityStateUAEffectEntry* CvCityStateUAEffectXMLEntries::GetEntry(int index) const
{
	if (index >= 0 && index < (int)m_paEffectEntries.size())
		return m_paEffectEntries[index];
	return NULL;
}

void CvCityStateUAEffectXMLEntries::DeleteArray()
{
	for (size_t i = 0; i < m_paEffectEntries.size(); i++)
	{
		SAFE_DELETE(m_paEffectEntries[i]);
	}
	m_paEffectEntries.clear();
}

//======================================================================================================
// CvCityStateUAEntry
//======================================================================================================
CvCityStateUAEntry::CvCityStateUAEntry(void)
	: m_iAllyEffectID(-1)
	, m_iFriendEffectID(-1)
	, m_piExtraFlavorValue(NULL)
	, m_piVictoryFlavorValue(NULL)
{
}

CvCityStateUAEntry::~CvCityStateUAEntry(void)
{
	SAFE_DELETE_ARRAY(m_piExtraFlavorValue);
	SAFE_DELETE_ARRAY(m_piVictoryFlavorValue);
}

bool CvCityStateUAEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if (!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	m_iAllyEffectID = GC.getInfoTypeForString(kResults.GetText("AllyEffectType"), true);
	m_iFriendEffectID = GC.getInfoTypeForString(kResults.GetText("FriendEffectType"), true);

	const char* szType = GetType();
	kUtility.SetFlavors(m_piExtraFlavorValue, "CityStateUA_ExtraFlavors", "CityStateUAType", szType);
	kUtility.PopulateArrayByValue(m_piVictoryFlavorValue, "AIGrandStrategies", "CityStateUA_VictoryFlavors",
	                              "AIGrandStrategyType", "CityStateUAType", szType, "Flavor");

	return true;
}

int CvCityStateUAEntry::GetAllyEffectID() const { return m_iAllyEffectID; }
int CvCityStateUAEntry::GetFriendEffectID() const { return m_iFriendEffectID; }

int CvCityStateUAEntry::GetExtraFlavorValue(int i) const
{
	if (i < 0 || i >= GC.getNumFlavorTypes())
		return 0;

	return m_piExtraFlavorValue ? m_piExtraFlavorValue[i] : 0;
}

int CvCityStateUAEntry::GetVictoryFlavorValue(int i) const
{
	if (i < 0 || i >= GC.getNumAIGrandStrategyInfos())
		return 0;

	return m_piVictoryFlavorValue ? m_piVictoryFlavorValue[i] : 0;
}

//======================================================================================================
// CvCityStateUAXMLEntries
//======================================================================================================
CvCityStateUAXMLEntries::CvCityStateUAXMLEntries(void)
{
}

CvCityStateUAXMLEntries::~CvCityStateUAXMLEntries(void)
{
	DeleteArray();
}

std::vector<CvCityStateUAEntry*>& CvCityStateUAXMLEntries::GetUAEntries()
{
	return m_paUAEntries;
}

int CvCityStateUAXMLEntries::GetNumUAs() const
{
	return (int)m_paUAEntries.size();
}

CvCityStateUAEntry* CvCityStateUAXMLEntries::GetEntry(int index) const
{
	if (index >= 0 && index < (int)m_paUAEntries.size())
		return m_paUAEntries[index];
	return NULL;
}

int CvCityStateUAXMLEntries::GetEntryIndexByType(const char* szType) const
{
	if (szType == NULL) return -1;
	for (size_t i = 0; i < m_paUAEntries.size(); i++)
	{
		if (m_paUAEntries[i] && strcmp(m_paUAEntries[i]->GetType(), szType) == 0)
			return (int)i;
	}
	return -1;
}

void CvCityStateUAXMLEntries::ResolveMinorCivUAIndexes()
{
	for (int i = 0; i < GC.getNumMinorCivInfos(); i++)
	{
		CvMinorCivInfo* pMinorCivInfo = GC.getMinorCivInfo((MinorCivTypes)i);
		if (pMinorCivInfo == NULL)
			continue;

		const char* szUAType = pMinorCivInfo->GetUAType();
		if (szUAType == NULL || szUAType[0] == '\0')
		{
			// No UA assigned: legitimate for city-states that carry none, so just leave the index at -1.
			pMinorCivInfo->SetCityStateUAIndex(-1);
			continue;
		}

		const int iIndex = GetEntryIndexByType(szUAType);
		if (iIndex < 0)
		{
			// A non-empty UAType that matches no CityStateUAs row is a config error (typo, or the row was
			// never inserted): every CSUA query site reads -1 as "no UA", so the ability would silently do
			// nothing. CVASSERT_ENABLE is off in this build, so the log line is what surfaces it.
			CvAssertMsg(iIndex >= 0, "City-state UAType matches no CityStateUAs row");
			LOGFILEMGR.GetLog("xml.log", 0)->Msg("CSUA: minor civ %s declares UAType '%s' which matches no CityStateUAs row", pMinorCivInfo->GetType(), szUAType);
		}
		pMinorCivInfo->SetCityStateUAIndex(iIndex);
	}
}

void CvCityStateUAXMLEntries::DeleteArray()
{
	for (size_t i = 0; i < m_paUAEntries.size(); i++)
	{
		SAFE_DELETE(m_paUAEntries[i]);
	}
	m_paUAEntries.clear();
}

//======================================================================================================
// CvPlayerCityStateUA
//======================================================================================================
CvPlayerCityStateUA::CvPlayerCityStateUA()
	: m_pPlayer(NULL)
	, m_iBuildingClassYieldModifierCount(0)
	, m_iFaithPurchaseGreatPeopleCostRiseModifier(0)
	, m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW(0)
	, m_iFaithPurchaseAllGreatPeopleCount(0)
	, m_iGPNoDeathAfterGreatWorkCount(0)
	, m_iGPConcertTourismRetentionPercent(0)
	, m_iGreatMusicianConcertGoldPercent(0)
	, m_iCapitalAndSecondCapitalCultureModifier(0)
	, m_iImmigrationRatePerImmigrant(0)
	, m_iImmigrationRateMax(0)
	, m_iEmigrationRatePerImmigrant(0)
	, m_iEmigrationRateMax(0)
	, m_iPuppetNoTechCostPenaltyCount(0)
	, m_iPuppetTechCostPartial(0)
	, m_iCanPillageNeutralTradeRouteCount(0)
	, m_iPlunderTradeRouteGold(0)
	, m_iPlunderTradeRouteXP(0)
	, m_iPlunderTradeRouteOpinionPenalty(0)
	, m_iGarrisonCityDefenseModifier(0)
	, m_iMilitaryUnitProductionXP(0)
	, m_iZOCRangeBonus(0)
	, m_iLandUnitsImmuneRiverCrossingCount(0)
	, m_iWoundedFixedDamage(0)
	, m_iEnemyFixedDamageModifierInBorders(0)
	, m_iCulturePerWarPeace(0)
	, m_iEnemyCombatModifierInBordersPerBeenDoW(0)
	, m_iUnitProductionModifierPerCity(0)
	, m_iCombatBonusPerTechDifference(0)
	, m_iCityAttackIgnoreBuildingDefensePercent(0)
	, m_iMilitaryXPPerTurnModifier(0)
	, m_iMilitaryXPSeaAir(0)
	, m_iCoupChanceModifier(0)
	, m_iCoupFailSpySurvives(0)
	, m_iStealTechSpeedPerSpy(0)
	, m_iSpyKillChancePerSpy(0)
	, m_iReligionSpreadSpeedModifier(0)
	, m_iPapalRecognitionVotes(0)
	, m_iPapalRecognitionAllyVotes(0)
	, m_iReligiousPressureModifierPerHolyCity(0)
	, m_iDenounceImmunityCount(0)
	, m_iLandTradeRouteDistancePerTradeSlot(0)
	, m_iTradeRouteGoldPercentNonNeighbor(0)
	, m_iHappinessPerGoldDonated(0)
	, m_iGoldDonationInterval(0)
	, m_iWonderProductionPerDonationHappiness(0)
	, m_iIdeologyPressurePerDonationHappiness(0)
	, m_iGoldDonationInfluenceModifierPerSeaRoute(0)
	, m_iGoldenAgeThresholdPerPopulation(0)
	, m_iLuxuryHappinessModifier(0)
	, m_iLocalHappinessCapModifier(0)
	, m_iGoldenAgeBuildingMaintenanceMod(0)
	, m_iFoodKeptModifierPerLuxury(0)
	, m_iTradeRouteGoldModifierPerLuxuryType(0)
	, m_iTradeRouteGoldModifierPerDistance(0)
	, m_iUnhappinessReductionPerCrossContinentRoute(0)
	, m_iTradeRouteGoldPercentInternational(0)
	, m_iTradeRouteGoldModifierPerInternationalRoute(0)
	, m_iFoodModifierPerHappyLuxuryType(0)
	, m_iFoodModifierPerHappyLuxuryCap(0)
	, m_iResearchAgreementBreakBonusPercent(0)
	, m_iEnemyCityNoHealBesiegeCount(0)
	, m_ppiBuildingClassYieldModifiers(NULL)
	, m_iSpyGarrisonYieldModifierCount(0)
	, m_iSpyKillGainSpyProgress(0)
	, m_ppiResourceYieldModifiers(NULL)
	, m_iResourceYieldModifierCount(0)
	, m_iCoastalCityGrowthThresholdModifier(0)
	, m_iDiplomaticPrestigePerCity(0)
	, m_ppiImprovementYieldModifiers(NULL)
	, m_iImprovementYieldModifierCount(0)
	, m_iImprovementHappinessCount(0)
	, m_iBuildingClassHappinessCount(0)
	, m_iTradeRouteGoldPerSurplusResourceCount(0)
	, m_iHappinessPerFollowingCity(0)
	, m_iFaithInfluencePurchaseCostDivisor(0)
	, m_iFaithInfluencePurchasePerTurnLimit(0)
	, m_iFaithBeliefPurchaseCount(0)
	, m_iInquisitorRetentionPercent(0)
	, m_iInquisitorCanUseOnCityStateCount(0)
	, m_iFaithPantheonPurchaseCount(0)
	, m_iGreatPersonRateModifierPerGreatWork(0)
	, m_iFaithRefundPerDonationPercent(0)
	, m_iDiplomaticPrestigePerMajorityCiv(0)
	, m_iInfluencePerTurnPerFollowCityMod(0)
	, m_iFollowingCityDivisor(0)
	, m_iImmigrantCashPercent(0)
	, m_iImmigrantCashCapBase(0)
	, m_iCoastalCityHappiness(0)
	, m_iHolySiteHappiness(0)
	, m_iCachedLiteracyPercent(0)
	, m_iCachedWorkedHolySites(0)
	, m_iCachedHappyLuxuryCount(0)
	, m_iCachedWorldWonderCount(0)
	, m_iCachedDiplomatAbroadCount(0)
	, m_iCultureVictoryProgressModifier(0)
	, m_iCachedUnknownInfluenceCount(0)
	, m_iGreatPersonRateModifierPerNationalWonder(0)
	, m_iLeagueVotesPerDoF(0)
	, m_iCachedNationalWonderCount(0)
	, m_iCachedLeagueVotes(0)
	, m_iCachedPuppetCount(0)
	, m_iWorldWonderHappiness(0)
	, m_iUCSYieldConversionCount(0)
{
}

CvPlayerCityStateUA::~CvPlayerCityStateUA()
{
	Uninit();
}

void CvPlayerCityStateUA::Init(CvPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
	Reset();
}

void CvPlayerCityStateUA::Uninit()
{
	m_pPlayer = NULL;
	Reset();
}

void CvPlayerCityStateUA::Reset()
{
	m_iFaithPurchaseGreatPeopleCostRiseModifier = 0;
	m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW = 0;
	m_aiGreatPersonPoints.assign(GC.getNumSpecialistInfos(), 0);
	m_iFaithPurchaseAllGreatPeopleCount = 0;
	m_iGPNoDeathAfterGreatWorkCount = 0;
	m_iGPConcertTourismRetentionPercent = 0;
	m_iGreatMusicianConcertGoldPercent = 0;
	m_iCapitalAndSecondCapitalCultureModifier = 0;
	m_iImmigrationRatePerImmigrant = 0;
	m_iImmigrationRateMax = 0;
	m_iEmigrationRatePerImmigrant = 0;
	m_iEmigrationRateMax = 0;
	m_iPuppetNoTechCostPenaltyCount = 0;
	m_iPuppetTechCostPartial = 0;
	m_iCanPillageNeutralTradeRouteCount = 0;
	m_iPlunderTradeRouteGold = 0;
	m_iPlunderTradeRouteXP = 0;
	m_iPlunderTradeRouteOpinionPenalty = 0;
	m_vKillMaxHpByPromotion.clear();
	m_iGarrisonCityDefenseModifier = 0;
	m_iMilitaryUnitProductionXP = 0;
	m_iZOCRangeBonus = 0;
	m_iLandUnitsImmuneRiverCrossingCount = 0;
	m_iWoundedFixedDamage = 0;
	m_iEnemyFixedDamageModifierInBorders = 0;
	m_iCulturePerWarPeace = 0;
	m_iEnemyCombatModifierInBordersPerBeenDoW = 0;
	m_iUnitProductionModifierPerCity = 0;
	m_iCombatBonusPerTechDifference = 0;
	m_vResourcePerCity.clear();
	m_iCityAttackIgnoreBuildingDefensePercent = 0;
	m_iMilitaryXPPerTurnModifier = 0;
	m_iMilitaryXPSeaAir = 0;
	m_iCoupChanceModifier = 0;
	m_iCoupFailSpySurvives = 0;
	m_iStealTechSpeedPerSpy = 0;
	m_iSpyKillChancePerSpy = 0;
	m_iReligionSpreadSpeedModifier = 0;
	m_iPapalRecognitionVotes = 0;
	m_iPapalRecognitionAllyVotes = 0;
	m_aiHolyCityYieldModifierPerFollowingCity.assign(NUM_YIELD_TYPES, 0);
	m_iReligiousPressureModifierPerHolyCity = 0;
	m_iDenounceImmunityCount = 0;
	m_aiCapitalYieldModifierPerFollowingCity.assign(NUM_YIELD_TYPES, 0);
	m_iLandTradeRouteDistancePerTradeSlot = 0;
	m_iTradeRouteGoldPercentNonNeighbor = 0;
	m_iHappinessPerGoldDonated = 0;
	m_iGoldDonationInterval = 0;
	m_iWonderProductionPerDonationHappiness = 0;
	m_iIdeologyPressurePerDonationHappiness = 0;
	m_iGoldDonationInfluenceModifierPerSeaRoute = 0;
	m_aiFriendCityStateYieldModifiers.assign(NUM_YIELD_TYPES, 0);
	m_aiAllyCityStateYieldModifiers.assign(NUM_YIELD_TYPES, 0);
	m_aiPolicyYieldModifiers.assign(NUM_YIELD_TYPES, 0);
	m_iGoldenAgeThresholdPerPopulation = 0;
	m_iLuxuryHappinessModifier = 0;
	m_iLocalHappinessCapModifier = 0;
	m_iGoldenAgeBuildingMaintenanceMod = 0;
	m_vGoldDonationGamble.clear();
	// Colombo / Cape Town / Mogadishu / Monaco: cleared here and re-accumulated by ApplyEffect
	m_iUCSYieldConversionCount = 0;
	m_iFoodKeptModifierPerLuxury = 0;
	m_iTradeRouteGoldModifierPerLuxuryType = 0;
	m_iTradeRouteGoldModifierPerDistance = 0;
	m_iUnhappinessReductionPerCrossContinentRoute = 0;
	m_iTradeRouteGoldPercentInternational = 0;
	m_iTradeRouteGoldModifierPerInternationalRoute = 0;
	m_iFoodModifierPerHappyLuxuryType = 0;
	m_iFoodModifierPerHappyLuxuryCap = 0;
	m_vCityStateTradeRouteYieldModifiersGlobal.clear();
	m_vLandTradeRouteGoldModifiers.clear();
	m_vInternationalLandTradeRouteYieldPerEra.clear();
	m_iResearchAgreementBreakBonusPercent = 0;
	m_iBuildingClassYieldModifierCount = 0;
	m_iSpyKillGainSpyProgress = 0;
	m_iSpyGarrisonYieldModifierCount = 0;
	m_aiSpyGarrisonYieldModifiers.assign(NUM_YIELD_TYPES, 0);
	m_iResourceYieldModifierCount = 0;
	m_iCoastalCityGrowthThresholdModifier = 0;
	m_iDiplomaticPrestigePerCity = 0;
	m_iImprovementYieldModifierCount = 0;
	m_iImprovementHappinessCount = 0;
	m_aiImprovementHappiness.assign(GC.getNumImprovementInfos(), 0);
	m_iBuildingClassHappinessCount = 0;
	m_aiBuildingClassHappiness.assign(GC.getNumBuildingClassInfos(), 0);
	m_iTradeRouteGoldPerSurplusResourceCount = 0;
	m_aiTradeRouteGoldPerSurplusResource.assign(GC.getNumResourceInfos(), 0);
	m_iHappinessPerFollowingCity = 0;
	m_iFaithInfluencePurchaseCostDivisor = 0;
	m_iFaithInfluencePurchasePerTurnLimit = 0;
	m_iFaithBeliefPurchaseCount = 0;
	m_iInquisitorRetentionPercent = 0;
	m_iInquisitorCanUseOnCityStateCount = 0;
	m_iFaithPantheonPurchaseCount = 0;
	m_iGreatPersonRateModifierPerGreatWork = 0;
	m_iFaithRefundPerDonationPercent = 0;
	m_iDiplomaticPrestigePerMajorityCiv = 0;
	m_iInfluencePerTurnPerFollowCityMod = 0;
	m_iFollowingCityDivisor = 0;
	m_aiImmigrantYieldModifiers.assign(NUM_YIELD_TYPES, 0);
	m_iImmigrantCashPercent = 0;
	m_iImmigrantCashCapBase = 0;
	m_iCoastalCityHappiness = 0;
	m_aiHappinessYieldModifiers.assign(NUM_YIELD_TYPES, 0);
	m_aiHappinessYieldModifierCaps.assign(NUM_YIELD_TYPES, 0);
	m_aiFaithGPClassCostModifier.assign(GC.getNumUnitClassInfos(), 0);
	m_vGreatWorkYieldModifiers.clear();
	m_aiGoldenAgeYieldModifiers.assign(NUM_YIELD_TYPES, 0);
	m_aiCachedGreatWorkCount.clear();
	m_iHolySiteHappiness = 0;
	m_vLiteracyYieldModifiers.clear();
	m_vBornGreatPersonYieldModifiers.clear();
	m_vAdjacentImprovementYieldChanges.clear();
	m_iCachedLiteracyPercent = 0;
	m_iCachedWorkedHolySites = 0;
	m_iCachedHappyLuxuryCount = 0;
	m_vWorldWonderYieldModifiers.clear();
	m_vDiplomatAbroadYieldModifiers.clear();
	m_iCachedWorldWonderCount = 0;
	m_iCachedDiplomatAbroadCount = 0;
	// Quebec
	m_iCultureVictoryProgressModifier = 0;
	m_vUnknownInfluenceYieldModifiers.clear();
	m_iCachedUnknownInfluenceCount = 0;
	m_iGreatPersonRateModifierPerNationalWonder = 0;
	m_iLeagueVotesPerDoF = 0;
	m_vLeagueVoteYieldModifiers.clear();
	m_iCachedNationalWonderCount = 0;
	m_iCachedLeagueVotes = 0;
	m_iWorldWonderHappiness = 0;
	m_aiSpecialistPointRate.assign(GC.getNumSpecialistInfos(), 0);
	m_vGreatWorkGreatPersonPoints.clear();
	m_aiGreatPersonOneShotModifier.assign(GC.getNumUnitClassInfos(), 0);
	// Mirrors CvDatabaseUtility::Initialize2DArray (non-static, so allocate manually here)
	CvDatabaseUtility::SafeDelete2DArray(m_ppiBuildingClassYieldModifiers);
	{
		const int iNumBC = GC.getNumBuildingClassInfos();
		if (iNumBC > 0)
		{
			const unsigned int iNumBytes = iNumBC * sizeof(int*) + iNumBC * NUM_YIELD_TYPES * sizeof(int);
			unsigned char* pData = FNEW(unsigned char[iNumBytes], c_eCiv5GameplayDLL, 0);
			m_ppiBuildingClassYieldModifiers = (int**)pData;
			m_ppiBuildingClassYieldModifiers[0] = (int*)(pData + iNumBC * sizeof(int*));
			for (int j = 0; j < NUM_YIELD_TYPES; ++j) m_ppiBuildingClassYieldModifiers[0][j] = 0;
			for (int i = 1; i < iNumBC; i++)
			{
				m_ppiBuildingClassYieldModifiers[i] = m_ppiBuildingClassYieldModifiers[i-1] + NUM_YIELD_TYPES;
				for (int j = 0; j < NUM_YIELD_TYPES; ++j) m_ppiBuildingClassYieldModifiers[i][j] = 0;
			}
		}
	}
	// Mirrors CvDatabaseUtility::Initialize2DArray (non-static, so allocate manually here)
	CvDatabaseUtility::SafeDelete2DArray(m_ppiResourceYieldModifiers);
	{
		const int iNumRes = GC.getNumResourceInfos();
		if (iNumRes > 0)
		{
			const unsigned int iNumBytes = iNumRes * sizeof(int*) + iNumRes * NUM_YIELD_TYPES * sizeof(int);
			unsigned char* pData = FNEW(unsigned char[iNumBytes], c_eCiv5GameplayDLL, 0);
			m_ppiResourceYieldModifiers = (int**)pData;
			m_ppiResourceYieldModifiers[0] = (int*)(pData + iNumRes * sizeof(int*));
			for (int j = 0; j < NUM_YIELD_TYPES; ++j) m_ppiResourceYieldModifiers[0][j] = 0;
			for (int i = 1; i < iNumRes; i++)
			{
				m_ppiResourceYieldModifiers[i] = m_ppiResourceYieldModifiers[i-1] + NUM_YIELD_TYPES;
				for (int j = 0; j < NUM_YIELD_TYPES; ++j) m_ppiResourceYieldModifiers[i][j] = 0;
			}
		}
	}
	// Mirrors CvDatabaseUtility::Initialize2DArray (non-static, so allocate manually here)
	CvDatabaseUtility::SafeDelete2DArray(m_ppiImprovementYieldModifiers);
	{
		const int iNumImp = GC.getNumImprovementInfos();
		if (iNumImp > 0)
		{
			const unsigned int iNumBytes = iNumImp * sizeof(int*) + iNumImp * NUM_YIELD_TYPES * sizeof(int);
			unsigned char* pData = FNEW(unsigned char[iNumBytes], c_eCiv5GameplayDLL, 0);
			m_ppiImprovementYieldModifiers = (int**)pData;
			m_ppiImprovementYieldModifiers[0] = (int*)(pData + iNumImp * sizeof(int*));
			for (int j = 0; j < NUM_YIELD_TYPES; ++j) m_ppiImprovementYieldModifiers[0][j] = 0;
			for (int i = 1; i < iNumImp; i++)
			{
				m_ppiImprovementYieldModifiers[i] = m_ppiImprovementYieldModifiers[i-1] + NUM_YIELD_TYPES;
				for (int j = 0; j < NUM_YIELD_TYPES; ++j) m_ppiImprovementYieldModifiers[i][j] = 0;
			}
		}
	}
	m_vBornGreatPersonSpecialistYield.clear();
	m_vBuildingGPP.clear();
	m_vBornAllyInfluenceMod.clear();
	m_iEnemyCityNoHealBesiegeCount = 0;
	m_vPurchasedBuildingXP.clear();
	m_vUnitBornYield.clear();
	m_vUnitMaintenanceByPromotion.clear();
	// Bogota
	m_vSpecialCityYieldModifiers.clear();
	m_vSpecialCityCountYieldModifiers.clear();
	m_avCachedSpecialCityIDs.clear();
	// Kuala Lumpur
	m_vSpecialCityPopulationYieldModifiers.clear();
	m_aiCachedSpecialCityPopulation.clear();
	m_iCachedPuppetCount = 0;
	// Tyre
	m_vSpecialCityDamageReductions.clear();
	// Singapore
	m_vBuildingClassGlobalYieldModifiers.clear();
	m_vBuildingClassTechCostModifiers.clear();
	// Milan
	m_vLuxuryHappinessYieldModifiers.clear();
	m_iCachedLuxuryHappiness = 0;
	m_vCombatDamageReductionVsNoTech.clear();
}

void CvPlayerCityStateUA::ApplyEffect(int iEffectID, int iChange)
{
	if (iEffectID < 0 || iChange == 0) return;

	CvCityStateUAEffectEntry* pEffect = GC.getCityStateUAEffectEntry(iEffectID);
	if (!pEffect) return;

	m_iFaithPurchaseGreatPeopleCostRiseModifier			+= pEffect->GetFaithPurchaseGreatPeopleCostRiseModifier() * iChange;
	m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW		+= pEffect->GetFaithPurchaseGreatPeopleCostRiseModifierPerGW() * iChange;
	if (pEffect->HasGreatPersonPoints())
	{
		for (int iSpec = 0; iSpec < GC.getNumSpecialistInfos(); iSpec++)
		{
			m_aiGreatPersonPoints[iSpec] += pEffect->GetGreatPersonPoints(iSpec) * iChange;
		}
	}
	m_iFaithPurchaseAllGreatPeopleCount += (pEffect->IsFaithPurchaseAllGreatPeople() ? iChange : 0);

	m_iGPNoDeathAfterGreatWorkCount += (pEffect->IsGPNoDeathAfterGreatWork() ? iChange : 0);
	m_iGPConcertTourismRetentionPercent				+= pEffect->GetGPConcertTourismRetentionPercent() * iChange;

	m_iGreatMusicianConcertGoldPercent				+= pEffect->GetGreatMusicianConcertGoldPercent() * iChange;

	m_iCapitalAndSecondCapitalCultureModifier		+= pEffect->GetCapitalAndSecondCapitalCultureModifier() * iChange;

	m_iImmigrationRatePerImmigrant					+= pEffect->GetImmigrationRatePerImmigrant() * iChange;
	m_iImmigrationRateMax							+= pEffect->GetImmigrationRateMax() * iChange;
	m_iEmigrationRatePerImmigrant					+= pEffect->GetEmigrationRatePerImmigrant() * iChange;
	m_iEmigrationRateMax							+= pEffect->GetEmigrationRateMax() * iChange;

	m_iPuppetNoTechCostPenaltyCount += (pEffect->IsPuppetNoTechCostPenalty() ? iChange : 0);
	m_iPuppetTechCostPartial						+= pEffect->GetPuppetTechCostPartial() * iChange;

	m_iCanPillageNeutralTradeRouteCount += (pEffect->IsCanPillageNeutralTradeRoute() ? iChange : 0);

	m_iPlunderTradeRouteGold						+= pEffect->GetPlunderTradeRouteGold() * iChange;
	m_iPlunderTradeRouteXP							+= pEffect->GetPlunderTradeRouteXP() * iChange;
	m_iPlunderTradeRouteOpinionPenalty				+= pEffect->GetPlunderTradeRouteOpinionPenalty() * iChange;
	{
		const std::vector<KillMaxHpByPromotionEntry>& vEntries = pEffect->GetKillMaxHpByPromotionEntries();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			KillMaxHpByPromotionEntry entry = vEntries[i];
			m_vKillMaxHpByPromotion.push_back(entry);
		}
	}

	m_iGarrisonCityDefenseModifier					+= pEffect->GetGarrisonCityDefenseModifier() * iChange;
	m_iMilitaryUnitProductionXP						+= pEffect->GetMilitaryUnitProductionXP() * iChange;
	m_iZOCRangeBonus								+= pEffect->GetZOCRangeBonus() * iChange;

	m_iLandUnitsImmuneRiverCrossingCount += (pEffect->IsLandUnitsImmuneRiverCrossing() ? iChange : 0);

	{
		const std::vector<UnitMaintenanceByPromotionEntry>& vEntries = pEffect->GetUnitMaintenanceByPromotionEntries();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			UnitMaintenanceByPromotionEntry entry = vEntries[i];
			entry.m_iChange *= iChange;
			m_vUnitMaintenanceByPromotion.push_back(entry);
		}
	}
	m_iWoundedFixedDamage							+= pEffect->GetWoundedFixedDamage() * iChange;

	m_iEnemyFixedDamageModifierInBorders			+= pEffect->GetEnemyFixedDamageModifierInBorders() * iChange;
	m_iCulturePerWarPeace							+= pEffect->GetCulturePerWarPeace() * iChange;
	m_iEnemyCombatModifierInBordersPerBeenDoW		+= pEffect->GetEnemyCombatModifierInBordersPerBeenDoW() * iChange;

	m_iUnitProductionModifierPerCity				+= pEffect->GetUnitProductionModifierPerCity() * iChange;
	m_iCombatBonusPerTechDifference				+= pEffect->GetCombatBonusPerTechDifference() * iChange;

	{
		const std::vector<ResourcePerCityEntry>& vEntries = pEffect->GetResourcePerCityEntries();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			ResourcePerCityEntry entry = vEntries[i];
			entry.m_iQuantity *= iChange;
			m_vResourcePerCity.push_back(entry);
		}
	}

	m_iCityAttackIgnoreBuildingDefensePercent		+= pEffect->GetCityAttackIgnoreBuildingDefensePercent() * iChange;
	m_iMilitaryXPPerTurnModifier					+= pEffect->GetMilitaryXPPerTurnModifier() * iChange;
	m_iMilitaryXPSeaAir								+= pEffect->GetMilitaryXPSeaAir() * iChange;

	// Sofia (spy/coup UA)
	m_iCoupChanceModifier							+= pEffect->GetCoupChanceModifier() * iChange;
	m_iCoupFailSpySurvives							+= (pEffect->GetCoupFailSpySurvives() ? iChange : 0);
	m_iStealTechSpeedPerSpy							+= pEffect->GetStealTechSpeedPerSpy() * iChange;
	m_iSpyKillChancePerSpy							+= pEffect->GetSpyKillChancePerSpy() * iChange;

	m_iReligionSpreadSpeedModifier					+= pEffect->GetReligionSpreadSpeedModifier() * iChange;
	m_iPapalRecognitionVotes						+= pEffect->GetPapalRecognitionVotes() * iChange;
	m_iPapalRecognitionAllyVotes					+= pEffect->GetPapalRecognitionAllyVotes() * iChange;
	// Vatican: per following city, the holy city gains a yield percentage modifier (per YieldType)
	for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		int iHolyCityMod = pEffect->GetHolyCityYieldModifierPerFollowingCity(iYield);
		if (iHolyCityMod != 0)
			m_aiHolyCityYieldModifierPerFollowingCity[iYield] += iHolyCityMod * iChange;
	}
	// Jerusalem: per holy city religious pressure, ally denounce immunity, per following-city capital yield modifier
	m_iReligiousPressureModifierPerHolyCity			+= pEffect->GetReligiousPressureModifierPerHolyCity() * iChange;
	m_iDenounceImmunityCount						+= (pEffect->IsDenounceImmunity() ? iChange : 0);
	for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		int iFollowingMod = pEffect->GetCapitalYieldModifierPerFollowingCity(iYield);
		if (iFollowingMod != 0)
			m_aiCapitalYieldModifierPerFollowingCity[iYield] += iFollowingMod * iChange;
	}

	m_iLandTradeRouteDistancePerTradeSlot			+= pEffect->GetLandTradeRouteDistancePerTradeSlot() * iChange;
	m_iTradeRouteGoldPercentNonNeighbor				+= pEffect->GetTradeRouteGoldPercentNonNeighbor() * iChange;

	m_iHappinessPerGoldDonated						+= pEffect->GetHappinessPerGoldDonated() * iChange;
	m_iGoldDonationInterval							+= pEffect->GetGoldDonationInterval() * iChange;
	m_iWonderProductionPerDonationHappiness			+= pEffect->GetWonderProductionPerDonationHappiness() * iChange;
	m_iIdeologyPressurePerDonationHappiness			+= pEffect->GetIdeologyPressurePerDonationHappiness() * iChange;
	m_iGoldDonationInfluenceModifierPerSeaRoute		+= pEffect->GetGoldDonationInfluenceModifierPerSeaRoute() * iChange;
	//Genoa / Vilnius: per-unit yield % modifiers (per friend/ally CS or unlocked policy)
	for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		int iFriendMod = pEffect->GetFriendCityStateYieldModifier((YieldTypes)iYield);
		if (iFriendMod != 0) m_aiFriendCityStateYieldModifiers[iYield] += iFriendMod * iChange;
		int iAllyMod = pEffect->GetAllyCityStateYieldModifier((YieldTypes)iYield);
		if (iAllyMod != 0) m_aiAllyCityStateYieldModifiers[iYield] += iAllyMod * iChange;
		int iPolicyMod = pEffect->GetPolicyYieldModifier((YieldTypes)iYield);
		if (iPolicyMod != 0) m_aiPolicyYieldModifiers[iYield] += iPolicyMod * iChange;
	}
	m_iGoldenAgeThresholdPerPopulation				+= pEffect->GetGoldenAgeThresholdPerPopulation() * iChange;

	m_iLuxuryHappinessModifier						+= pEffect->GetLuxuryHappinessModifier() * iChange;
	m_iLocalHappinessCapModifier						+= pEffect->GetLocalHappinessCapModifier() * iChange;
	//Monaco: golden-age building maintenance modifier + first-donation wager outcomes
	m_iGoldenAgeBuildingMaintenanceMod				+= pEffect->GetGoldenAgeBuildingMaintenanceMod() * iChange;
	{
		const std::vector<GoldDonationGambleEntry>& vGamble = pEffect->GetGoldDonationGambleEntries();
		for (size_t i = 0; i < vGamble.size(); i++)
		{
			m_vGoldDonationGamble.push_back(vGamble[i]);
		}
	}
	// Colombo / Cape Town / Mogadishu / Monaco: flag that this player holds at least one yield-to-yield
	// conversion effect, so CvCity::GetYieldRateFromUCSConversion (called per city per yield) can skip
	// the city-state scan entirely for players that hold none.
	if (!pEffect->GetYieldToYieldViaTRToUCSEntries().empty() || !pEffect->GetYieldToYieldEntries().empty())
	{
		m_iUCSYieldConversionCount += iChange;
	}
	m_iFoodKeptModifierPerLuxury						+= pEffect->GetFoodKeptModifierPerLuxury() * iChange;
	m_iTradeRouteGoldModifierPerLuxuryType			+= pEffect->GetTradeRouteGoldModifierPerLuxuryType() * iChange;
	m_iTradeRouteGoldModifierPerDistance			+= pEffect->GetTradeRouteGoldModifierPerDistance() * iChange;
	m_iUnhappinessReductionPerCrossContinentRoute	+= pEffect->GetUnhappinessReductionPerCrossContinentRoute() * iChange;
	m_iTradeRouteGoldPercentInternational			+= pEffect->GetTradeRouteGoldPercentInternational() * iChange;
	m_iTradeRouteGoldModifierPerInternationalRoute	+= pEffect->GetTradeRouteGoldModifierPerInternationalRoute() * iChange;
	m_iFoodModifierPerHappyLuxuryType				+= pEffect->GetFoodModifierPerHappyLuxuryType() * iChange;
	m_iFoodModifierPerHappyLuxuryCap				+= pEffect->GetFoodModifierPerHappyLuxuryCap() * iChange;
	m_iResearchAgreementBreakBonusPercent			+= pEffect->GetResearchAgreementBreakBonusPercent() * iChange;
	m_iEnemyCityNoHealBesiegeCount					+= pEffect->GetEnemyCityNoHealBesiegeCount() * iChange;
	m_iSpyKillGainSpyProgress						+= pEffect->GetSpyKillGainSpyProgress() * iChange;
	m_iCoastalCityGrowthThresholdModifier			+= pEffect->GetCoastalCityGrowthThresholdModifier() * iChange;
	m_iDiplomaticPrestigePerCity					+= pEffect->GetDiplomaticPrestigePerCity() * iChange;
	{
		if (m_ppiBuildingClassYieldModifiers && pEffect->HasBuildingClassYieldModifiers())
		{
			for (int iBC = 0; iBC < GC.getNumBuildingClassInfos(); iBC++)
			{
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
				{
					int iMod = pEffect->GetBuildingClassYieldModifiers(iBC, iYield);
					if (iMod != 0)
					{
						m_ppiBuildingClassYieldModifiers[iBC][iYield] += iMod * iChange;
						m_iBuildingClassYieldModifierCount += iChange;
					}
				}
			}
		}
	}
	//CityState UA (Melbourne): city owning the specified improved resource grants yield percentage modifiers
	{
		if (m_ppiResourceYieldModifiers && pEffect->HasResourceYieldModifiers())
		{
			for (int iRes = 0; iRes < GC.getNumResourceInfos(); iRes++)
			{
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
				{
					int iMod = pEffect->GetResourceYieldModifiers(iRes, iYield);
					if (iMod != 0)
					{
						m_ppiResourceYieldModifiers[iRes][iYield] += iMod * iChange;
						m_iResourceYieldModifierCount += iChange;
					}
				}
			}
		}
	}
	//CityState UA (Antananarivo): each worked plot holding the specified improvement grants yield percentage modifiers
	{
		if (m_ppiImprovementYieldModifiers && pEffect->HasImprovementYieldModifiers())
		{
			for (int iImp = 0; iImp < GC.getNumImprovementInfos(); iImp++)
			{
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
				{
					int iMod = pEffect->GetImprovementYieldModifiers(iImp, iYield);
					if (iMod != 0)
					{
						m_ppiImprovementYieldModifiers[iImp][iYield] += iMod * iChange;
						m_iImprovementYieldModifierCount += iChange;
					}
				}
			}
		}
	}
	//CityState UA (Zanzibar): each worked plot holding the specified improvement grants flat local happiness
	if (pEffect->HasImprovementHappiness())
	{
		for (int iImp = 0; iImp < GC.getNumImprovementInfos(); iImp++)
		{
			int iHappy = pEffect->GetImprovementHappiness(iImp);
			if (iHappy != 0)
			{
				m_aiImprovementHappiness[iImp] += iHappy * iChange;
				m_iImprovementHappinessCount += iChange;
			}
		}
	}
	//CityState UA (Ragusa): each owned building of the specified class grants flat local happiness
	if (pEffect->HasBuildingClassHappiness())
	{
		for (int iBC = 0; iBC < GC.getNumBuildingClassInfos(); iBC++)
		{
			int iHappy = pEffect->GetBuildingClassHappiness(iBC);
			if (iHappy != 0)
			{
				m_aiBuildingClassHappiness[iBC] += iHappy * iChange;
				m_iBuildingClassHappinessCount += iChange;
			}
		}
	}
	//CityState UA (Hormuz): each unit of surplus strategic resource grants trade-route gold % (per ResourceType)
	if (pEffect->HasTradeRouteGoldPerSurplusResource())
	{
		for (int iRes = 0; iRes < GC.getNumResourceInfos(); iRes++)
		{
			int iTRMod = pEffect->GetTradeRouteGoldPerSurplusResource(iRes);
			if (iTRMod != 0)
			{
				m_aiTradeRouteGoldPerSurplusResource[iRes] += iTRMod * iChange;
				m_iTradeRouteGoldPerSurplusResourceCount += iChange;
			}
		}
	}
	//Gangtok
	m_iHappinessPerFollowingCity += pEffect->GetHappinessPerFollowingCity() * iChange;
	m_iFaithInfluencePurchaseCostDivisor += pEffect->GetFaithInfluencePurchaseCostDivisor() * iChange;
	m_iFaithInfluencePurchasePerTurnLimit += pEffect->GetFaithInfluencePurchasePerTurnLimit() * iChange;
	m_iFaithBeliefPurchaseCount += (pEffect->GetFaithBeliefPurchase() ? iChange : 0);
	m_iInquisitorRetentionPercent += pEffect->GetInquisitorRetentionPercent() * iChange;
	m_iInquisitorCanUseOnCityStateCount += (pEffect->GetInquisitorCanUseOnCityState() ? iChange : 0);
	m_iFaithPantheonPurchaseCount += (pEffect->GetFaithPantheonPurchase() ? iChange : 0);
	m_iGreatPersonRateModifierPerGreatWork += pEffect->GetGreatPersonRateModifierPerGreatWork() * iChange;
	m_iFaithRefundPerDonationPercent += pEffect->GetFaithRefundPerDonationPercent() * iChange;
	m_iDiplomaticPrestigePerMajorityCiv += pEffect->GetDiplomaticPrestigePerMajorityCiv() * iChange;
	m_iInfluencePerTurnPerFollowCityMod += pEffect->GetInfluencePerTurnPerFollowCityMod() * iChange;
	m_iFollowingCityDivisor += pEffect->GetFollowingCityDivisor() * iChange;
	// Sydney: per immigrant received yield % modifiers (per YieldType); cash reward per immigrant
	for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		int iImmigrantMod = pEffect->GetImmigrantYieldModifier(iYield);
		if (iImmigrantMod != 0)
			m_aiImmigrantYieldModifiers[iYield] += iImmigrantMod * iChange;
	}
	m_iImmigrantCashPercent += pEffect->GetImmigrantCashPercent() * iChange;
	m_iImmigrantCashCapBase += pEffect->GetImmigrantCashCapBase() * iChange;
	//Vancouver: global happiness per coastal city, and per point of net happiness a yield % modifier per YieldType (with per-yield cap)
	m_iCoastalCityHappiness += pEffect->GetCoastalCityHappiness() * iChange;
	for (int iY = 0; iY < NUM_YIELD_TYPES; iY++)
	{
		int iHMod = pEffect->GetHappinessYieldModifier(iY);
		if (iHMod != 0) m_aiHappinessYieldModifiers[iY] += iHMod * iChange;
		int iHCapC = pEffect->GetHappinessYieldModifierCap(iY);
		if (iHCapC != 0) m_aiHappinessYieldModifierCaps[iY] += iHCapC * iChange;
	}
	//Ife: per-unitclass FAITH great-people cost discount
	if (pEffect->HasFaithGPClassCostModifier())
	{
		for (int iUC = 0; iUC < GC.getNumUnitClassInfos(); iUC++)
		{
			int iClassMod = pEffect->GetFaithGPClassCostModifier(iUC);
			if (iClassMod != 0) m_aiFaithGPClassCostModifier[iUC] += iClassMod * iChange;
		}
	}
	//Ife: each great work / artifact grants a yield % modifier per YieldType (per GreatWorkClass)
	{
		const std::vector<GreatWorkYieldModifierEntry>& vGWEntries = pEffect->GetGreatWorkYieldModifiers();
		for (size_t i = 0; i < vGWEntries.size(); i++)
		{
			GreatWorkYieldModifierEntry entry = vGWEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vGreatWorkYieldModifiers.push_back(entry);
		}
	}
	//Bucharest: each world wonder owned grants a yield % modifier per YieldType
	{
		const std::vector<WorldWonderYieldModifierEntry>& vWWEntries = pEffect->GetWorldWonderYieldModifiers();
		for (size_t i = 0; i < vWWEntries.size(); i++)
		{
			WorldWonderYieldModifierEntry entry = vWWEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vWorldWonderYieldModifiers.push_back(entry);
		}
	}
	//Quebec: when another civilization computes its culture-victory progress against the player, inflate the
	//player's lifetime culture; also per met major civ at Unknown influence toward the player
	m_iCultureVictoryProgressModifier += pEffect->GetCultureVictoryProgressModifier() * iChange;
	{
		const std::vector<UnknownInfluenceYieldModifierEntry>& vUIEntries = pEffect->GetUnknownInfluenceYieldModifiers();
		for (size_t i = 0; i < vUIEntries.size(); i++)
		{
			UnknownInfluenceYieldModifierEntry entry = vUIEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vUnknownInfluenceYieldModifiers.push_back(entry);
		}
	}
	//Mogadishu: each international trade route the ally runs to a city-state grants a yield % modifier
	{
		const std::vector<CityStateTradeRouteYieldModifierGlobalEntry>& vTREntries = pEffect->GetCityStateTradeRouteYieldModifiersGlobal();
		for (size_t i = 0; i < vTREntries.size(); i++)
		{
			CityStateTradeRouteYieldModifierGlobalEntry entry = vTREntries[i];
			entry.m_iYieldMod *= iChange;
			m_vCityStateTradeRouteYieldModifiersGlobal.push_back(entry);
		}
	}
	//Kabul: each international land trade route grants trade-route gold %, plus a hills-origin bonus
	{
		const std::vector<LandTradeRouteGoldModifierEntry>& vLTEntries = pEffect->GetLandTradeRouteGoldModifiers();
		for (size_t i = 0; i < vLTEntries.size(); i++)
		{
			LandTradeRouteGoldModifierEntry entry = vLTEntries[i];
			entry.m_iYieldMod *= iChange;
			entry.m_iHillsBonus *= iChange;
			m_vLandTradeRouteGoldModifiers.push_back(entry);
		}
	}
	//Kabul: each international land trade route to any other player (city-states included) grants a yield % per era
	{
		const std::vector<InternationalLandTradeRouteYieldEntry>& vMLEntries = pEffect->GetInternationalLandTradeRouteYieldPerEra();
		for (size_t i = 0; i < vMLEntries.size(); i++)
		{
			InternationalLandTradeRouteYieldEntry entry = vMLEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vInternationalLandTradeRouteYieldPerEra.push_back(entry);
		}
	}
	//Bucharest: each diplomat stationed in a foreign major civilization's city grants a yield % modifier
	{
		const std::vector<DiplomatAbroadYieldModifierEntry>& vDAEntries = pEffect->GetDiplomatAbroadYieldModifiers();
		for (size_t i = 0; i < vDAEntries.size(); i++)
		{
			DiplomatAbroadYieldModifierEntry entry = vDAEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vDiplomatAbroadYieldModifiers.push_back(entry);
		}
	}
	//Kiev: +X% great-person rate per national wonder completed, and +X League votes per Declaration of Friendship
	m_iGreatPersonRateModifierPerNationalWonder		+= pEffect->GetGreatPersonRateModifierPerNationalWonder() * iChange;
	m_iLeagueVotesPerDoF							+= pEffect->GetLeagueVotesPerDoF() * iChange;
	//Ur: global happiness per world wonder owned by the ally/friend
	m_iWorldWonderHappiness							+= pEffect->GetWorldWonderHappiness() * iChange;
	//Kiev: each League vote held grants a yield % modifier per YieldType
	{
		const std::vector<LeagueVoteYieldModifierEntry>& vLVEntries = pEffect->GetLeagueVoteYieldModifiers();
		for (size_t i = 0; i < vLVEntries.size(); i++)
		{
			LeagueVoteYieldModifierEntry entry = vLVEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vLeagueVoteYieldModifiers.push_back(entry);
		}
	}
	//Ife: while in a golden age, yield % modifier per YieldType
	for (int iY = 0; iY < NUM_YIELD_TYPES; iY++)
	{
		int iGAMod = pEffect->GetGoldenAgeYieldModifier(iY);
		if (iGAMod != 0) m_aiGoldenAgeYieldModifiers[iY] += iGAMod * iChange;
	}
	//Prague: city with our own spy garrisoned grants yield percentage modifiers
	for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		int iMod = pEffect->GetSpyGarrisonYieldModifiers(iYield);
		if (iMod != 0)
		{
			m_aiSpyGarrisonYieldModifiers[iYield] += iMod * iChange;
			m_iSpyGarrisonYieldModifierCount += iChange;
		}
	}
	//Brussels: specialist great person point accumulation rate
	if (pEffect->HasSpecialistPointRate())
	{
		for (int iSpec = 0; iSpec < GC.getNumSpecialistInfos(); iSpec++)
		{
			m_aiSpecialistPointRate[iSpec] += pEffect->GetSpecialistPointRate(iSpec) * iChange;
		}
	}
	{
		const std::vector<GreatWorkGreatPersonPointsEntry>& vEntries = pEffect->GetGreatWorkGreatPersonPointsEntries();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			GreatWorkGreatPersonPointsEntry entry = vEntries[i];
			entry.m_iRate *= iChange;
			m_vGreatWorkGreatPersonPoints.push_back(entry);
		}
	}
	//Brussels: specified unit class's one-shot great person output modifier
	if (pEffect->HasGreatPersonOneShotModifier())
	{
		for (int iUC = 0; iUC < GC.getNumUnitClassInfos(); iUC++)
		{
			m_aiGreatPersonOneShotModifier[iUC] += pEffect->GetGreatPersonOneShotModifier(iUC) * iChange;
		}
	}
	{
		const std::vector<BornGreatPersonSpecialistYieldEntry>& vEntries = pEffect->GetBornGreatPersonSpecialistYieldEntries();
		int iEntryCount = (int)vEntries.size();
		for (int iEntryIdx = 0; iEntryIdx < iEntryCount; iEntryIdx++)
		{
			const BornGreatPersonSpecialistYieldEntry& e = vEntries[iEntryIdx];
			BornGreatPersonSpecialistYieldEntry entry = e;
			entry.m_iYieldMod *= iChange;
			m_vBornGreatPersonSpecialistYield.push_back(entry);
		}
	}
	{
		const std::vector<BuildingGreatPersonPointsEntry>& vEntries = pEffect->GetBuildingGreatPersonPointsEntries();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			BuildingGreatPersonPointsEntry entry = vEntries[i];
			entry.m_iPoints *= iChange;
			m_vBuildingGPP.push_back(entry);
		}
	}
	{
		const std::vector<BornGreatPersonAllyInfluenceModEntry>& vInfEntries = pEffect->GetBornAllyInfluenceModEntries();
		for (size_t i = 0; i < vInfEntries.size(); i++)
		{
			const BornGreatPersonAllyInfluenceModEntry& e = vInfEntries[i];
			m_vBornAllyInfluenceMod.push_back(e);
		}
	}
	{
		const std::vector<PurchasedBuildingXPEntry>& vEntries = pEffect->GetPurchasedBuildingXPEntries();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			PurchasedBuildingXPEntry entry = vEntries[i];
			entry.m_iXP *= iChange;
			m_vPurchasedBuildingXP.push_back(entry);
		}
	}
	{
		const std::vector<UnitBornYieldEntry>& vEntries = pEffect->GetUnitBornYieldEntries();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			UnitBornYieldEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vUnitBornYield.push_back(entry);
		}
	}
	// Yerevan: global happiness per worked holy site + literacy / born-great-person / adjacent-improvement sub-tables
	m_iHolySiteHappiness += pEffect->GetHolySiteHappiness() * iChange;
	{
		const std::vector<LiteracyYieldModifierEntry>& vEntries = pEffect->GetLiteracyYieldModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			LiteracyYieldModifierEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vLiteracyYieldModifiers.push_back(entry);
		}
	}
	{
		const std::vector<BornGreatPersonNationwideYieldEntry>& vEntries = pEffect->GetBornGreatPersonYieldModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			BornGreatPersonNationwideYieldEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vBornGreatPersonYieldModifiers.push_back(entry);
		}
	}
	{
		const std::vector<AdjacentImprovementYieldChangeEntry>& vEntries = pEffect->GetAdjacentImprovementYieldChanges();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			AdjacentImprovementYieldChangeEntry entry = vEntries[i];
			entry.m_iYield *= iChange;
			m_vAdjacentImprovementYieldChanges.push_back(entry);
		}
	}
	// Bogota: a city matching a special city type gains a yield % modifier
	{
		const std::vector<SpecialCityYieldModifierEntry>& vEntries = pEffect->GetSpecialCityYieldModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			SpecialCityYieldModifierEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vSpecialCityYieldModifiers.push_back(entry);
		}
	}
	// Bogota: per owned city matching a special city type, ALL cities gain a yield % modifier
	{
		const std::vector<SpecialCityCountYieldModifierEntry>& vEntries = pEffect->GetSpecialCityCountYieldModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			SpecialCityCountYieldModifierEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vSpecialCityCountYieldModifiers.push_back(entry);
		}
	}
	//Kuala Lumpur: per N population living in cities matching a special city type, a nation-wide yield %
	{
		const std::vector<SpecialCityPopulationYieldModifierEntry>& vEntries = pEffect->GetSpecialCityPopulationYieldModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			SpecialCityPopulationYieldModifierEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vSpecialCityPopulationYieldModifiers.push_back(entry);
		}
	}
	//Tyre: a city matching a special city type takes Percent% less damage
	{
		const std::vector<SpecialCityDamageReductionEntry>& vEntries = pEffect->GetSpecialCityDamageReductions();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			SpecialCityDamageReductionEntry entry = vEntries[i];
			entry.m_iPercent *= iChange;
			m_vSpecialCityDamageReductions.push_back(entry);
		}
	}
	//Singapore: each owned building class grants a nation-wide yield % modifier per YieldType
	{
		const std::vector<BuildingClassGlobalYieldModifierEntry>& vEntries = pEffect->GetBuildingClassGlobalYieldModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			BuildingClassGlobalYieldModifierEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			m_vBuildingClassGlobalYieldModifiers.push_back(entry);
		}
	}
	//Singapore: each owned building class lowers the city-count research threshold by TechCostMod points
	{
		const std::vector<BuildingClassTechCostModifierEntry>& vEntries = pEffect->GetBuildingClassTechCostModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			BuildingClassTechCostModifierEntry entry = vEntries[i];
			entry.m_iTechCostMod *= iChange;
			m_vBuildingClassTechCostModifiers.push_back(entry);
		}
	}
	//Milan: each point of luxury happiness grants a nation-wide yield % modifier per YieldType
	{
		const std::vector<LuxuryHappinessYieldModifierEntry>& vEntries = pEffect->GetLuxuryHappinessYieldModifiers();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			LuxuryHappinessYieldModifierEntry entry = vEntries[i];
			entry.m_iYieldMod *= iChange;
			entry.m_iCap *= iChange;
			m_vLuxuryHappinessYieldModifiers.push_back(entry);
		}
	}
	//Milan: a unit takes Percent% less damage when the opposing side lacks the configured tech
	{
		const std::vector<CombatDamageReductionVsNoTechEntry>& vEntries = pEffect->GetCombatDamageReductionVsNoTech();
		for (size_t i = 0; i < vEntries.size(); i++)
		{
			CombatDamageReductionVsNoTechEntry entry = vEntries[i];
			entry.m_iPercent *= iChange;
			m_vCombatDamageReductionVsNoTech.push_back(entry);
		}
	}
}

int CvPlayerCityStateUA::GetFaithPurchaseGreatPeopleCostRiseModifier() const { return m_iFaithPurchaseGreatPeopleCostRiseModifier; }
int CvPlayerCityStateUA::GetFaithPurchaseGreatPeopleCostRiseModifierPerGW() const { return m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW; }
int CvPlayerCityStateUA::GetGreatPersonPoints(int i) const { CvAssertMsg(i < GC.getNumSpecialistInfos(), "Index out of bounds"); CvAssertMsg(i > -1, "Index out of bounds"); return (i >= 0 && i < (int)m_aiGreatPersonPoints.size()) ? m_aiGreatPersonPoints[i] : 0; }
bool CvPlayerCityStateUA::IsFaithPurchaseAllGreatPeople() const { return m_iFaithPurchaseAllGreatPeopleCount > 0; }
bool CvPlayerCityStateUA::IsGPNoDeathAfterGreatWork() const { return m_iGPNoDeathAfterGreatWorkCount > 0; }
int CvPlayerCityStateUA::GetGPConcertTourismRetentionPercent() const { return m_iGPConcertTourismRetentionPercent; }
int CvPlayerCityStateUA::GetGreatMusicianConcertGoldPercent() const { return m_iGreatMusicianConcertGoldPercent; }
int CvPlayerCityStateUA::GetCapitalAndSecondCapitalCultureModifier() const { return m_iCapitalAndSecondCapitalCultureModifier; }
int CvPlayerCityStateUA::GetImmigrationRatePerImmigrant() const { return m_iImmigrationRatePerImmigrant; }
int CvPlayerCityStateUA::GetImmigrationRateMax() const { return m_iImmigrationRateMax; }
int CvPlayerCityStateUA::GetEmigrationRatePerImmigrant() const { return m_iEmigrationRatePerImmigrant; }
int CvPlayerCityStateUA::GetEmigrationRateMax() const { return m_iEmigrationRateMax; }
bool CvPlayerCityStateUA::IsPuppetNoTechCostPenalty() const { return m_iPuppetNoTechCostPenaltyCount > 0; }
int CvPlayerCityStateUA::GetPuppetTechCostPartial() const { return m_iPuppetTechCostPartial; }
bool CvPlayerCityStateUA::IsCanPillageNeutralTradeRoute() const { return m_iCanPillageNeutralTradeRouteCount > 0; }
int CvPlayerCityStateUA::GetPlunderTradeRouteGold() const { return m_iPlunderTradeRouteGold; }
int CvPlayerCityStateUA::GetPlunderTradeRouteXP() const { return m_iPlunderTradeRouteXP; }
int CvPlayerCityStateUA::GetPlunderTradeRouteOpinionPenalty() const { return m_iPlunderTradeRouteOpinionPenalty; }
bool CvPlayerCityStateUA::HasKillMaxHpByPromotion() const { return !m_vKillMaxHpByPromotion.empty(); }
const std::vector<KillMaxHpByPromotionEntry>& CvPlayerCityStateUA::GetKillMaxHpByPromotionEntries() const { return m_vKillMaxHpByPromotion; }
int CvPlayerCityStateUA::GetGarrisonCityDefenseModifier() const { return m_iGarrisonCityDefenseModifier; }

int CvPlayerCityStateUA::GetMilitaryUnitProductionXP() const { return m_iMilitaryUnitProductionXP; }

int CvPlayerCityStateUA::GetZOCRangeBonus() const { return m_iZOCRangeBonus; }
bool CvPlayerCityStateUA::IsLandUnitsImmuneRiverCrossing() const { return m_iLandUnitsImmuneRiverCrossingCount > 0; }
const std::vector<UnitMaintenanceByPromotionEntry>& CvPlayerCityStateUA::GetUnitMaintenanceByPromotionEntries() const { return m_vUnitMaintenanceByPromotion; }
int CvPlayerCityStateUA::GetWoundedFixedDamage() const { return m_iWoundedFixedDamage; }
int CvPlayerCityStateUA::GetEnemyFixedDamageModifierInBorders() const { return m_iEnemyFixedDamageModifierInBorders; }
int CvPlayerCityStateUA::GetCulturePerWarPeace() const { return m_iCulturePerWarPeace; }
int CvPlayerCityStateUA::GetEnemyCombatModifierInBordersPerBeenDoW() const { return m_iEnemyCombatModifierInBordersPerBeenDoW; }
int CvPlayerCityStateUA::GetUnitProductionModifierPerCity() const { return m_iUnitProductionModifierPerCity; }
int CvPlayerCityStateUA::GetCombatBonusPerTechDifference() const { return m_iCombatBonusPerTechDifference; }
const std::vector<ResourcePerCityEntry>& CvPlayerCityStateUA::GetResourcePerCityEntries() const { return m_vResourcePerCity; }
int CvPlayerCityStateUA::GetCityAttackIgnoreBuildingDefensePercent() const { return m_iCityAttackIgnoreBuildingDefensePercent; }
int CvPlayerCityStateUA::GetMilitaryXPPerTurnModifier() const { return m_iMilitaryXPPerTurnModifier; }
int CvPlayerCityStateUA::GetMilitaryXPSeaAir() const { return m_iMilitaryXPSeaAir; }
int CvPlayerCityStateUA::GetCoupChanceModifier() const { return m_iCoupChanceModifier; }
bool CvPlayerCityStateUA::GetCoupFailSpySurvives() const { return m_iCoupFailSpySurvives > 0; }
int CvPlayerCityStateUA::GetStealTechSpeedPerSpy() const { return m_iStealTechSpeedPerSpy; }
int CvPlayerCityStateUA::GetSpyKillChancePerSpy() const { return m_iSpyKillChancePerSpy; }
int CvPlayerCityStateUA::GetReligionSpreadSpeedModifier() const { return m_iReligionSpreadSpeedModifier; }
int CvPlayerCityStateUA::GetPapalRecognitionVotes() const { return m_iPapalRecognitionVotes; }
int CvPlayerCityStateUA::GetPapalRecognitionAllyVotes() const { return m_iPapalRecognitionAllyVotes; }
int CvPlayerCityStateUA::GetHolyCityYieldModifierPerFollowingCity(YieldTypes eYieldType) const { return m_aiHolyCityYieldModifierPerFollowingCity[eYieldType]; }
int CvPlayerCityStateUA::GetReligiousPressureModifierPerHolyCity() const { return m_iReligiousPressureModifierPerHolyCity; }
bool CvPlayerCityStateUA::IsDenounceImmunity() const { return m_iDenounceImmunityCount > 0; }
int CvPlayerCityStateUA::GetCapitalYieldModifierPerFollowingCity(YieldTypes eYieldType) const
{
	return (eYieldType >= 0 && (int)eYieldType < (int)m_aiCapitalYieldModifierPerFollowingCity.size()) ? m_aiCapitalYieldModifierPerFollowingCity[(int)eYieldType] : 0;
}
int CvPlayerCityStateUA::GetLandTradeRouteDistancePerTradeSlot() const { return m_iLandTradeRouteDistancePerTradeSlot; }
int CvPlayerCityStateUA::GetTradeRouteGoldPercentNonNeighbor() const { return m_iTradeRouteGoldPercentNonNeighbor; }
int CvPlayerCityStateUA::GetHappinessPerGoldDonated() const { return m_iHappinessPerGoldDonated; }
int CvPlayerCityStateUA::GetGoldDonationInterval() const { return m_iGoldDonationInterval; }
int CvPlayerCityStateUA::GetWonderProductionPerDonationHappiness() const { return m_iWonderProductionPerDonationHappiness; }
int CvPlayerCityStateUA::GetIdeologyPressurePerDonationHappiness() const { return m_iIdeologyPressurePerDonationHappiness; }
int CvPlayerCityStateUA::GetGoldDonationInfluenceModifierPerSeaRoute() const { return m_iGoldDonationInfluenceModifierPerSeaRoute; }
int CvPlayerCityStateUA::GetFriendCityStateYieldModifier(YieldTypes eYieldType) const
{
	return (eYieldType >= 0 && (int)eYieldType < (int)m_aiFriendCityStateYieldModifiers.size()) ? m_aiFriendCityStateYieldModifiers[(int)eYieldType] : 0;
}
int CvPlayerCityStateUA::GetAllyCityStateYieldModifier(YieldTypes eYieldType) const
{
	return (eYieldType >= 0 && (int)eYieldType < (int)m_aiAllyCityStateYieldModifiers.size()) ? m_aiAllyCityStateYieldModifiers[(int)eYieldType] : 0;
}
int CvPlayerCityStateUA::GetPolicyYieldModifier(YieldTypes eYieldType) const
{
	return (eYieldType >= 0 && (int)eYieldType < (int)m_aiPolicyYieldModifiers.size()) ? m_aiPolicyYieldModifiers[(int)eYieldType] : 0;
}
int CvPlayerCityStateUA::GetGoldenAgeThresholdPerPopulation() const { return m_iGoldenAgeThresholdPerPopulation; }
int CvPlayerCityStateUA::GetLuxuryHappinessModifier() const { return m_iLuxuryHappinessModifier; }
int CvPlayerCityStateUA::GetLocalHappinessCapModifier() const { return m_iLocalHappinessCapModifier; }
int CvPlayerCityStateUA::GetGoldenAgeBuildingMaintenanceMod() const { return m_iGoldenAgeBuildingMaintenanceMod; }
int CvPlayerCityStateUA::GetFoodKeptModifierPerLuxury() const { return m_iFoodKeptModifierPerLuxury; }
int CvPlayerCityStateUA::GetTradeRouteGoldModifierPerLuxuryType() const { return m_iTradeRouteGoldModifierPerLuxuryType; }
int CvPlayerCityStateUA::GetTradeRouteGoldModifierPerDistance() const { return m_iTradeRouteGoldModifierPerDistance; }
int CvPlayerCityStateUA::GetUnhappinessReductionPerCrossContinentRoute() const { return m_iUnhappinessReductionPerCrossContinentRoute; }
int CvPlayerCityStateUA::GetTradeRouteGoldPercentInternational() const { return m_iTradeRouteGoldPercentInternational; }
int CvPlayerCityStateUA::GetTradeRouteGoldModifierPerInternationalRoute() const { return m_iTradeRouteGoldModifierPerInternationalRoute; }
int CvPlayerCityStateUA::GetFoodModifierPerHappyLuxuryType() const { return m_iFoodModifierPerHappyLuxuryType; }
int CvPlayerCityStateUA::GetFoodModifierPerHappyLuxuryCap() const { return m_iFoodModifierPerHappyLuxuryCap; }
bool CvPlayerCityStateUA::HasCityStateTradeRouteYieldModifiersGlobal() const
{
	return !m_vCityStateTradeRouteYieldModifiersGlobal.empty();
}
bool CvPlayerCityStateUA::HasLandTradeRouteGoldModifiers() const
{
	return !m_vLandTradeRouteGoldModifiers.empty();
}
bool CvPlayerCityStateUA::HasInternationalLandTradeRouteYieldPerEra() const
{
	return !m_vInternationalLandTradeRouteYieldPerEra.empty();
}
int CvPlayerCityStateUA::GetResearchAgreementBreakBonusPercent() const { return m_iResearchAgreementBreakBonusPercent; }
int CvPlayerCityStateUA::GetSpecialistYieldFromBornGreatPerson(SpecialistTypes eSpecialist, YieldTypes eYield) const
{
	if (!m_pPlayer) return 0;
	int iResult = 0;
	int iVecSize = (int)m_vBornGreatPersonSpecialistYield.size();
	for (int iIdx = 0; iIdx < iVecSize; iIdx++)
	{
		const BornGreatPersonSpecialistYieldEntry& entry = m_vBornGreatPersonSpecialistYield[iIdx];
		if (entry.m_iSpecialistType != (int)eSpecialist || entry.m_iYieldType != (int)eYield) continue;
		GreatPersonTypes eGP = GetGreatPersonFromUnitClass((UnitClassTypes)entry.m_iUnitClassType);
		if (eGP == NO_GREATPERSON) continue;
		int iBornCount = m_pPlayer->GetBornGreatPersonCount(eGP);
		int iAdd = iBornCount * entry.m_iYieldMod / 100;
		if (iAdd > 0) iResult += iAdd;
	}
	return iResult;
}

int CvPlayerCityStateUA::GetBuildingGreatPersonPointsForCity(const CvCity* pCity, SpecialistTypes eSpecialist) const
{
	if (!pCity) return 0;
	int iTotal = 0;
	for (size_t i = 0; i < m_vBuildingGPP.size(); i++)
	{
		const BuildingGreatPersonPointsEntry& entry = m_vBuildingGPP[i];
		if (entry.m_iSpecialist == (int)eSpecialist && entry.m_iPoints > 0)
		{
			int iCount = pCity->GetNumBuildingClass((BuildingClassTypes)entry.m_iBuildingClass);
			iTotal += entry.m_iPoints * iCount;
		}
	}
	return iTotal;
}

int CvPlayerCityStateUA::GetAllyInfluenceModFromBornGreatPerson() const
{
	if (!m_pPlayer) return 0;
	int iTotal = 0;
	for (size_t i = 0; i < m_vBornAllyInfluenceMod.size(); i++)
	{
		const BornGreatPersonAllyInfluenceModEntry& entry = m_vBornAllyInfluenceMod[i];
		GreatPersonTypes eGP = GetGreatPersonFromUnitClass((UnitClassTypes)entry.m_iUnitClassType);
		if (eGP == NO_GREATPERSON) continue;
		int iBornCount = m_pPlayer->GetBornGreatPersonCount(eGP);
		iTotal += iBornCount * entry.m_iModPerBorn;
	}
	return iTotal;
}

int CvPlayerCityStateUA::GetBuildingClassYieldModifier(BuildingClassTypes eBuildingClass, YieldTypes eYieldType) const
{
	if (!m_ppiBuildingClassYieldModifiers) return 0;
	return m_ppiBuildingClassYieldModifiers[(int)eBuildingClass][(int)eYieldType];
}

bool CvPlayerCityStateUA::HasBuildingClassYieldModifiers() const
{
	return m_iBuildingClassYieldModifierCount > 0;
}

int CvPlayerCityStateUA::GetSpyGarrisonYieldModifier(YieldTypes eYieldType) const
{
	return (eYieldType >= 0 && (int)eYieldType < (int)m_aiSpyGarrisonYieldModifiers.size()) ? m_aiSpyGarrisonYieldModifiers[(int)eYieldType] : 0;
}

bool CvPlayerCityStateUA::HasSpyGarrisonYieldModifiers() const
{
	return m_iSpyGarrisonYieldModifierCount > 0;
}

int CvPlayerCityStateUA::GetSpyKillGainSpyProgress() const
{
	return m_iSpyKillGainSpyProgress;
}

int CvPlayerCityStateUA::GetResourceYieldModifier(ResourceTypes eResource, YieldTypes eYield) const
{
	if (!m_ppiResourceYieldModifiers) return 0;
	return m_ppiResourceYieldModifiers[(int)eResource][(int)eYield];
}

bool CvPlayerCityStateUA::HasResourceYieldModifiers() const
{
	return m_iResourceYieldModifierCount > 0;
}

int CvPlayerCityStateUA::GetCoastalCityGrowthThresholdModifier() const
{
	return m_iCoastalCityGrowthThresholdModifier;
}

int CvPlayerCityStateUA::GetDiplomaticPrestigePerCity() const
{
	return m_iDiplomaticPrestigePerCity;
}

int CvPlayerCityStateUA::GetImprovementYieldModifier(ImprovementTypes eImprovement, YieldTypes eYield) const
{
	if (!m_ppiImprovementYieldModifiers) return 0;
	return m_ppiImprovementYieldModifiers[(int)eImprovement][(int)eYield];
}

bool CvPlayerCityStateUA::HasImprovementYieldModifiers() const
{
	return m_iImprovementYieldModifierCount > 0;
}

int CvPlayerCityStateUA::GetImprovementHappiness(ImprovementTypes eImprovement) const
{
	return (eImprovement >= 0 && (int)eImprovement < (int)m_aiImprovementHappiness.size()) ? m_aiImprovementHappiness[(int)eImprovement] : 0;
}

bool CvPlayerCityStateUA::HasImprovementHappiness() const
{
	return m_iImprovementHappinessCount > 0;
}

int CvPlayerCityStateUA::GetBuildingClassHappiness(BuildingClassTypes eBuildingClass) const
{
	return (eBuildingClass >= 0 && (int)eBuildingClass < (int)m_aiBuildingClassHappiness.size()) ? m_aiBuildingClassHappiness[(int)eBuildingClass] : 0;
}

bool CvPlayerCityStateUA::HasBuildingClassHappiness() const
{
	return m_iBuildingClassHappinessCount > 0;
}

int CvPlayerCityStateUA::GetTradeRouteGoldPerSurplusResource(ResourceTypes eResource) const
{
	return (eResource >= 0 && (int)eResource < (int)m_aiTradeRouteGoldPerSurplusResource.size()) ? m_aiTradeRouteGoldPerSurplusResource[(int)eResource] : 0;
}

bool CvPlayerCityStateUA::HasTradeRouteGoldPerSurplusResource() const
{
	return m_iTradeRouteGoldPerSurplusResourceCount > 0;
}

int CvPlayerCityStateUA::GetSpecialistPointRate(SpecialistTypes eSpecialist) const
{
	return (eSpecialist >= 0 && (int)eSpecialist < (int)m_aiSpecialistPointRate.size()) ? m_aiSpecialistPointRate[(int)eSpecialist] : 0;
}

int CvPlayerCityStateUA::GetGreatWorkGreatPersonPointsForCity(const CvCity* pCity, SpecialistTypes eSpecialist) const
{
	if (!pCity) return 0;
	int iTotal = 0;
	for (size_t i = 0; i < m_vGreatWorkGreatPersonPoints.size(); i++)
	{
		const GreatWorkGreatPersonPointsEntry& entry = m_vGreatWorkGreatPersonPoints[i];
		if (entry.m_iSpecialistType != (int)eSpecialist || entry.m_iRate <= 0) continue;
		if (entry.m_bCapitalOnly && !pCity->isCapital()) continue;
		int iNumGW = pCity->GetCityBuildings()->GetNumGreatWorks((GreatWorkClass)entry.m_iGreatWorkClassType);
		iTotal += iNumGW * entry.m_iRate;
	}
	return iTotal;
}

bool CvPlayerCityStateUA::HasGreatWorkGreatPersonPoints() const
{
	return !m_vGreatWorkGreatPersonPoints.empty();
}

int CvPlayerCityStateUA::GetGreatPersonOneShotModifier(UnitClassTypes eUnitClass) const
{
	return (eUnitClass >= 0 && (int)eUnitClass < (int)m_aiGreatPersonOneShotModifier.size()) ? m_aiGreatPersonOneShotModifier[(int)eUnitClass] : 0;
}

int CvPlayerCityStateUA::GetEnemyCityNoHealBesiegeCount() const { return m_iEnemyCityNoHealBesiegeCount; }
const std::vector<PurchasedBuildingXPEntry>& CvPlayerCityStateUA::GetPurchasedBuildingXPEntries() const { return m_vPurchasedBuildingXP; }
const std::vector<UnitBornYieldEntry>& CvPlayerCityStateUA::GetUnitBornYieldEntries() const { return m_vUnitBornYield; }
int CvPlayerCityStateUA::GetHappinessPerFollowingCity() const { return m_iHappinessPerFollowingCity; }
int CvPlayerCityStateUA::GetFaithInfluencePurchaseCostDivisor() const { return m_iFaithInfluencePurchaseCostDivisor; }
int CvPlayerCityStateUA::GetFaithInfluencePurchasePerTurnLimit() const { return m_iFaithInfluencePurchasePerTurnLimit; }
bool CvPlayerCityStateUA::HasFaithInfluencePurchase() const { return m_iFaithInfluencePurchaseCostDivisor > 0; }
bool CvPlayerCityStateUA::AnyFaithBeliefPurchase() const { return m_iFaithBeliefPurchaseCount > 0; }
bool CvPlayerCityStateUA::IsInquisitorCanUseOnCityState() const { return m_iInquisitorCanUseOnCityStateCount > 0; }
bool CvPlayerCityStateUA::AnyFaithPantheonPurchase() const { return m_iFaithPantheonPurchaseCount > 0; }
int CvPlayerCityStateUA::GetGreatPersonRateModifierPerGreatWork() const { return m_iGreatPersonRateModifierPerGreatWork; }
int CvPlayerCityStateUA::GetFaithRefundPerDonationPercent() const { return m_iFaithRefundPerDonationPercent; }
int CvPlayerCityStateUA::GetDiplomaticPrestigePerMajorityCiv() const { return m_iDiplomaticPrestigePerMajorityCiv; }
int CvPlayerCityStateUA::GetInfluencePerTurnPerFollowCityMod() const { return m_iInfluencePerTurnPerFollowCityMod; }
int CvPlayerCityStateUA::GetFollowingCityDivisor() const { return m_iFollowingCityDivisor; }
// Sydney: per immigrant received yield % modifier (per YieldType, 100 = +1%)
int CvPlayerCityStateUA::GetImmigrantYieldModifier(YieldTypes eYield) const
{
	return (eYield >= 0 && (int)eYield < (int)m_aiImmigrantYieldModifiers.size()) ? m_aiImmigrantYieldModifiers[(int)eYield] : 0;
}
bool CvPlayerCityStateUA::HasImmigrantYieldModifiers() const
{
	for (size_t i = 0; i < m_aiImmigrantYieldModifiers.size(); i++)
		if (m_aiImmigrantYieldModifiers[i] != 0) return true;
	return false;
}
int CvPlayerCityStateUA::GetImmigrantCashPercent() const { return m_iImmigrantCashPercent; }
int CvPlayerCityStateUA::GetImmigrantCashCapBase() const { return m_iImmigrantCashCapBase; }
int CvPlayerCityStateUA::GetCoastalCityHappiness() const { return m_iCoastalCityHappiness; }
int CvPlayerCityStateUA::GetHappinessYieldModifier(YieldTypes eYield) const
{
	return (eYield >= 0 && (int)eYield < (int)m_aiHappinessYieldModifiers.size()) ? m_aiHappinessYieldModifiers[(int)eYield] : 0;
}
int CvPlayerCityStateUA::GetHappinessYieldModifierCap(YieldTypes eYield) const
{
	return (eYield >= 0 && (int)eYield < (int)m_aiHappinessYieldModifierCaps.size()) ? m_aiHappinessYieldModifierCaps[(int)eYield] : 0;
}
int CvPlayerCityStateUA::GetFaithGPClassCostModifier(UnitClassTypes eUnitClass) const
{
	return (eUnitClass >= 0 && (int)eUnitClass < (int)m_aiFaithGPClassCostModifier.size()) ? m_aiFaithGPClassCostModifier[(int)eUnitClass] : 0;
}
int CvPlayerCityStateUA::GetGreatWorkYieldModifier(GreatWorkClass eGreatWorkClass, YieldTypes eYield) const
{
	int iTotal = 0;
	for (size_t i = 0; i < m_vGreatWorkYieldModifiers.size(); i++)
	{
		const GreatWorkYieldModifierEntry& entry = m_vGreatWorkYieldModifiers[i];
		if (entry.m_iGreatWorkClassType == (int)eGreatWorkClass && entry.m_iYieldType == (int)eYield)
			iTotal += entry.m_iYieldMod;
	}
	return iTotal;
}
const std::vector<GreatWorkYieldModifierEntry>& CvPlayerCityStateUA::GetGreatWorkYieldModifierEntries() const { return m_vGreatWorkYieldModifiers; }
bool CvPlayerCityStateUA::HasGreatWorkYieldModifiers() const
{
	return !m_vGreatWorkYieldModifiers.empty();
}
// Ife: cached per-class great-work count (indexed by GreatWorkClass ID). Refreshed once per doTurn.
int CvPlayerCityStateUA::GetCachedGreatWorkCount(GreatWorkClass eGreatWorkClass) const
{
	const int iClass = (int)eGreatWorkClass;
	return (iClass >= 0 && iClass < (int)m_aiCachedGreatWorkCount.size()) ? m_aiCachedGreatWorkCount[iClass] : 0;
}
// Ife: rebuild the cached per-class great-work count from all of the player's cities.
// Called once per doTurn in CvPlayer::RefreshCSAllUAEffects (which already traverses cities),
// so the hot path GetCSUAYieldPercentModifier reads a flat cached int instead of per-city accumulation.
void CvPlayerCityStateUA::CacheGreatWorkCounts()
{
	m_aiCachedGreatWorkCount.clear();
	if (!m_pPlayer) return;
	int iMaxClass = -1;
	// Determine the greatest GreatWorkClass ID actually referenced by any persisted IFE entry.
	for (size_t i = 0; i < m_vGreatWorkYieldModifiers.size(); i++)
		if (m_vGreatWorkYieldModifiers[i].m_iGreatWorkClassType > iMaxClass)
			iMaxClass = m_vGreatWorkYieldModifiers[i].m_iGreatWorkClassType;
	if (iMaxClass < 0) return;
	m_aiCachedGreatWorkCount.assign(iMaxClass + 1, 0);
	// Mark referenced classes first: a class carrying several rows (e.g. GREAT_WORK_MUSIC is
	// shared by Ife, Ur and Buenos Aires) must contribute its real count only once, otherwise
	// the cached count would be multiplied by the number of rows referencing that class.
	std::vector<bool> abReferenced(iMaxClass + 1, false);
	for (size_t i = 0; i < m_vGreatWorkYieldModifiers.size(); i++)
	{
		const int iClass = m_vGreatWorkYieldModifiers[i].m_iGreatWorkClassType;
		if (iClass >= 0 && iClass <= iMaxClass)
			abReferenced[iClass] = true;
	}
	// Accumulate the player's great works of each referenced class across all cities.
	for (int iCityIdx = 0; iCityIdx < m_pPlayer->getNumCities(); iCityIdx++)
	{
		const CvCity* pCity = m_pPlayer->getCity(iCityIdx);
		if (!pCity || !pCity->GetCityBuildings()) continue;
		for (int iClass = 0; iClass <= iMaxClass; iClass++)
		{
			if (abReferenced[iClass])
				m_aiCachedGreatWorkCount[iClass] += pCity->GetCityBuildings()->GetNumGreatWorks((GreatWorkClass)iClass);
		}
	}
}
int CvPlayerCityStateUA::GetHolySiteHappiness() const { return m_iHolySiteHappiness; }
bool CvPlayerCityStateUA::HasLiteracyYieldModifiers() const { return !m_vLiteracyYieldModifiers.empty(); }
bool CvPlayerCityStateUA::HasBornGreatPersonYieldModifiers() const { return !m_vBornGreatPersonYieldModifiers.empty(); }
bool CvPlayerCityStateUA::HasAdjacentImprovementYieldChanges() const { return !m_vAdjacentImprovementYieldChanges.empty(); }
int CvPlayerCityStateUA::GetCachedLiteracyPercent() const { return m_iCachedLiteracyPercent; }
void CvPlayerCityStateUA::ComputeLiteracyPercent()
{
	m_iCachedLiteracyPercent = 0;
	if (!m_pPlayer) return;
	const int iTotal = GC.getNumTechInfos();
	if (iTotal <= 0) return;
	int iKnown = 0;
	for (int iTech = 0; iTech < iTotal; iTech++)
	{
		if (m_pPlayer->HasTech((TechTypes)iTech))
			iKnown++;
	}
	m_iCachedLiteracyPercent = (iKnown * 100) / iTotal;
}
int CvPlayerCityStateUA::GetCachedWorkedHolySites() const { return m_iCachedWorkedHolySites; }
void CvPlayerCityStateUA::CacheWorkedHolySites()
{
	m_iCachedWorkedHolySites = 0;
	if (!m_pPlayer) return;
	const ImprovementTypes eHolySite = (ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_HOLY_SITE");
	if (eHolySite == NO_IMPROVEMENT) return;
	for (int iCityIdx = 0; iCityIdx < m_pPlayer->getNumCities(); iCityIdx++)
	{
		CvCity* pCity = m_pPlayer->getCity(iCityIdx);
		if (pCity)
			m_iCachedWorkedHolySites += pCity->GetNumImprovementWorked(eHolySite);
	}
}
int CvPlayerCityStateUA::GetCachedHappyLuxuryCount() const { return m_iCachedHappyLuxuryCount; }
// Manila: rebuild the cached happy-luxury type count, called once per doTurn in
// CvPlayer::RefreshCSAllUAEffects. The resource scan is skipped unless the player actually holds a
// Manila food effect, so the common case (no Manila ally) costs nothing.
void CvPlayerCityStateUA::CacheHappyLuxuryCount()
{
	m_iCachedHappyLuxuryCount = 0;
	if (!m_pPlayer) return;
	if (m_iFoodModifierPerHappyLuxuryType == 0) return;
	m_iCachedHappyLuxuryCount = m_pPlayer->GetHappyLuxuryTypeCount();
}
bool CvPlayerCityStateUA::HasSpecialCityYieldModifiers() const
{
	return !m_vSpecialCityYieldModifiers.empty();
}
bool CvPlayerCityStateUA::HasSpecialCityCountYieldModifiers() const
{
	return !m_vSpecialCityCountYieldModifiers.empty();
}
bool CvPlayerCityStateUA::HasWorldWonderYieldModifiers() const
{
	return !m_vWorldWonderYieldModifiers.empty();
}
// Bucharest / Ur: cached world-wonder count, refreshed once per doTurn in CvPlayer::RefreshCSAllUAEffects.
// The city scan is skipped unless the player actually holds a world-wonder effect (Bucharest yield % or
// Ur global happiness).
int CvPlayerCityStateUA::GetCachedWorldWonderCount() const { return m_iCachedWorldWonderCount; }
void CvPlayerCityStateUA::CacheWorldWonderCount()
{
	m_iCachedWorldWonderCount = 0;
	if (!m_pPlayer) return;
	if (m_vWorldWonderYieldModifiers.empty() && m_iWorldWonderHappiness == 0) return;
	m_iCachedWorldWonderCount = m_pPlayer->GetNumWorldWonders();
}
// Quebec: when another civilization computes its culture-victory progress against the player, inflate the
// player's lifetime culture by this plain percent. Read by CvPlayerCulture (victory-progress denominators).
int CvPlayerCityStateUA::GetCultureVictoryProgressModifier() const { return m_iCultureVictoryProgressModifier; }
bool CvPlayerCityStateUA::HasUnknownInfluenceYieldModifiers() const
{
	return !m_vUnknownInfluenceYieldModifiers.empty();
}
// Quebec: cached count of met, living major civilizations whose influence level toward the player is
// Unknown (the lowest influence level), refreshed once per doTurn in CvPlayer::RefreshCSAllUAEffects so the
// per-yield hot path (GetCSUAYieldPercentModifier) reads a flat int. The scan is skipped unless the player
// holds a Quebec Unknown-influence effect. Unmet civilizations are NO_INFLUENCE_LEVEL and do not count.
int CvPlayerCityStateUA::GetCachedUnknownInfluenceCount() const { return m_iCachedUnknownInfluenceCount; }
void CvPlayerCityStateUA::CacheUnknownInfluenceCount()
{
	m_iCachedUnknownInfluenceCount = 0;
	if (!m_pPlayer) return;
	if (m_vUnknownInfluenceYieldModifiers.empty()) return;
	for (int iPlayer = 0; iPlayer < MAX_MAJOR_CIVS; iPlayer++)
	{
		if (iPlayer == m_pPlayer->GetID()) continue;
		CvPlayer& kOther = GET_PLAYER((PlayerTypes)iPlayer);
		if (!kOther.isAlive()) continue;
		if (!GET_TEAM(kOther.getTeam()).isHasMet(m_pPlayer->getTeam())) continue;
		if (kOther.GetCulture() != NULL &&
			kOther.GetCulture()->GetInfluenceLevel(m_pPlayer->GetID()) == INFLUENCE_LEVEL_UNKNOWN)
		{
			m_iCachedUnknownInfluenceCount++;
		}
	}
}
bool CvPlayerCityStateUA::HasDiplomatAbroadYieldModifiers() const
{
	return !m_vDiplomatAbroadYieldModifiers.empty();
}
// Bucharest: cached count of diplomats stationed in a foreign MAJOR civilization's city, refreshed
// once per doTurn. Diplomats sent to city-states do not count. The spy scan is skipped unless the
// player actually holds a Bucharest diplomat effect.
int CvPlayerCityStateUA::GetCachedDiplomatAbroadCount() const { return m_iCachedDiplomatAbroadCount; }
void CvPlayerCityStateUA::CacheDiplomatAbroadCount()
{
	m_iCachedDiplomatAbroadCount = 0;
	if (!m_pPlayer) return;
	if (m_vDiplomatAbroadYieldModifiers.empty()) return;
	CvPlayerEspionage* pEspionage = m_pPlayer->GetEspionage();
	if (pEspionage == NULL) return;
	const int iNumSpies = pEspionage->GetNumSpies();
	for (int iSpy = 0; iSpy < iNumSpies; iSpy++)
	{
		if (!pEspionage->IsDiplomat((uint)iSpy)) continue;
		CvCity* pCity = pEspionage->GetCityWithSpy((uint)iSpy);
		if (pCity == NULL) continue;
		const PlayerTypes eOwner = pCity->getOwner();
		if (eOwner == m_pPlayer->GetID()) continue;
		if (GET_PLAYER(eOwner).isMinorCiv()) continue;
		m_iCachedDiplomatAbroadCount++;
	}
}
// Kiev: +X% great-person rate per national wonder the player has completed (plain percent).
int CvPlayerCityStateUA::GetGreatPersonRateModifierPerNationalWonder() const { return m_iGreatPersonRateModifierPerNationalWonder; }
bool CvPlayerCityStateUA::HasNationalWonderGreatPersonModifier() const
{
	return m_iGreatPersonRateModifierPerNationalWonder != 0;
}
// Kiev: cached national-wonder count, refreshed once per doTurn in CvPlayer::RefreshCSAllUAEffects.
// CvCity::getGreatPeopleRateModifier runs per city, so the city scan must stay out of it. The scan is
// skipped unless the player actually holds a Kiev national-wonder effect.
int CvPlayerCityStateUA::GetCachedNationalWonderCount() const { return m_iCachedNationalWonderCount; }
void CvPlayerCityStateUA::CacheNationalWonderCount()
{
	m_iCachedNationalWonderCount = 0;
	if (!m_pPlayer) return;
	if (m_iGreatPersonRateModifierPerNationalWonder == 0) return;
	m_iCachedNationalWonderCount = m_pPlayer->GetNumNationalWonders();
}
// Kiev: League delegate votes granted per Declaration of Friendship (read live; cheap diplomacy count).
int CvPlayerCityStateUA::GetLeagueVotesPerDoF() const { return m_iLeagueVotesPerDoF; }
bool CvPlayerCityStateUA::HasLeagueVotesPerDoF() const { return m_iLeagueVotesPerDoF != 0; }
// Ur: global happiness per world wonder owned by the ally/friend (100 = +1 happiness per world wonder).
int CvPlayerCityStateUA::GetWorldWonderHappiness() const { return m_iWorldWonderHappiness; }
bool CvPlayerCityStateUA::HasLeagueVoteYieldModifiers() const
{
	return !m_vLeagueVoteYieldModifiers.empty();
}
// Kiev: cached League vote count, refreshed once per doTurn. Recomputing the member's starting votes
// walks every civilization (city-state allies, diplomats, religion, ideology), so it must not run in
// the per-yield hot path. The lookup is skipped unless the player holds a Kiev league-vote effect.
// Note: CalculateStartingVotesForMember also rebuilds Member::sVoteSources when no session is running;
// that write is idempotent, so calling it here has no lasting side effect.
int CvPlayerCityStateUA::GetCachedLeagueVotes() const { return m_iCachedLeagueVotes; }
void CvPlayerCityStateUA::CacheLeagueVotes()
{
	m_iCachedLeagueVotes = 0;
	if (!m_pPlayer) return;
	if (m_vLeagueVoteYieldModifiers.empty()) return;
	CvGameLeagues* pLeagues = GC.getGame().GetGameLeagues();
	if (pLeagues == NULL) return;
	CvLeague* pLeague = pLeagues->GetActiveLeague();
	if (pLeague == NULL) return;
	m_iCachedLeagueVotes = pLeague->CalculateStartingVotesForMember(m_pPlayer->GetID());
}
int CvPlayerCityStateUA::GetCachedSpecialCityCount(int iSpecialCityType) const
{
	if (iSpecialCityType < 0 || iSpecialCityType >= (int)m_avCachedSpecialCityIDs.size())
		return 0;
	return (int)m_avCachedSpecialCityIDs[iSpecialCityType].size();
}
bool CvPlayerCityStateUA::IsCachedSpecialCityTypeMatch(int iCityID, int iSpecialCityType) const
{
	if (iSpecialCityType < 0 || iSpecialCityType >= (int)m_avCachedSpecialCityIDs.size())
		return false;

	const std::vector<int>& vCityIDs = m_avCachedSpecialCityIDs[iSpecialCityType];
	for (size_t i = 0; i < vCityIDs.size(); i++)
	{
		if (vCityIDs[i] == iCityID)
			return true;
	}
	return false;
}
void CvPlayerCityStateUA::CacheSpecialCityMatches()
{
	m_avCachedSpecialCityIDs.clear();
	m_aiCachedSpecialCityPopulation.clear();
	if (!m_pPlayer) return;

	// Size the cache to cover every special city type referenced by ANY special-city effect, whether it
	// targets the matching city itself, counts matching cities nation-wide, or counts their population.
	int iMaxType = -1;
	for (size_t i = 0; i < m_vSpecialCityYieldModifiers.size(); i++)
	{
		if (m_vSpecialCityYieldModifiers[i].m_iSpecialCityType > iMaxType)
			iMaxType = m_vSpecialCityYieldModifiers[i].m_iSpecialCityType;
	}
	for (size_t i = 0; i < m_vSpecialCityCountYieldModifiers.size(); i++)
	{
		if (m_vSpecialCityCountYieldModifiers[i].m_iSpecialCityType > iMaxType)
			iMaxType = m_vSpecialCityCountYieldModifiers[i].m_iSpecialCityType;
	}
	for (size_t i = 0; i < m_vSpecialCityPopulationYieldModifiers.size(); i++)
	{
		if (m_vSpecialCityPopulationYieldModifiers[i].m_iSpecialCityType > iMaxType)
			iMaxType = m_vSpecialCityPopulationYieldModifiers[i].m_iSpecialCityType;
	}
	if (iMaxType < 0) return;

	// Mark referenced types so that a type carrying several yield rows still lists each matching city once.
	std::vector<bool> abReferenced(iMaxType + 1, false);
	for (size_t i = 0; i < m_vSpecialCityYieldModifiers.size(); i++)
	{
		const int iType = m_vSpecialCityYieldModifiers[i].m_iSpecialCityType;
		if (iType >= 0 && iType <= iMaxType)
			abReferenced[iType] = true;
	}
	for (size_t i = 0; i < m_vSpecialCityCountYieldModifiers.size(); i++)
	{
		const int iType = m_vSpecialCityCountYieldModifiers[i].m_iSpecialCityType;
		if (iType >= 0 && iType <= iMaxType)
			abReferenced[iType] = true;
	}
	for (size_t i = 0; i < m_vSpecialCityPopulationYieldModifiers.size(); i++)
	{
		const int iType = m_vSpecialCityPopulationYieldModifiers[i].m_iSpecialCityType;
		if (iType >= 0 && iType <= iMaxType)
			abReferenced[iType] = true;
	}

	m_avCachedSpecialCityIDs.resize(iMaxType + 1);
	m_aiCachedSpecialCityPopulation.assign(iMaxType + 1, 0);
	for (int iCityIdx = 0; iCityIdx < m_pPlayer->getNumCities(); iCityIdx++)
	{
		const CvCity* pCity = m_pPlayer->getCity(iCityIdx);
		if (pCity == NULL) continue;

		const int iCityID = pCity->GetID();
		for (int iType = 0; iType <= iMaxType; iType++)
		{
			if (abReferenced[iType] && pCity->IsSpecialCityType(iType))
			{
				m_avCachedSpecialCityIDs[iType].push_back(iCityID);
				m_aiCachedSpecialCityPopulation[iType] += pCity->getPopulation();
			}
		}
	}
}
// Kuala Lumpur: cached population living in cities matching each special city type, refreshed once per
// doTurn (in CvPlayer::RefreshCSAllUAEffects, via CacheSpecialCityMatches). The per-yield hot path
// (CvPlayer::GetCSUAYieldPercentModifier) reads this table instead of re-running the predicate.
bool CvPlayerCityStateUA::HasSpecialCityPopulationYieldModifiers() const
{
	return !m_vSpecialCityPopulationYieldModifiers.empty();
}
bool CvPlayerCityStateUA::HasSpecialCityDamageReduction() const
{
	return !m_vSpecialCityDamageReductions.empty();
}
bool CvPlayerCityStateUA::HasBuildingClassGlobalYieldModifiers() const
{
	return !m_vBuildingClassGlobalYieldModifiers.empty();
}
bool CvPlayerCityStateUA::HasBuildingClassTechCostModifiers() const
{
	return !m_vBuildingClassTechCostModifiers.empty();
}
// Milan: cached luxury happiness total, refreshed once per doTurn in CvPlayer::RefreshCSAllUAEffects.
// GetCSUAYieldPercentModifier is a per-yield hot path, so the scan is skipped unless the player holds a
// Milan luxury-happiness effect.
int CvPlayerCityStateUA::GetCachedLuxuryHappiness() const { return m_iCachedLuxuryHappiness; }
void CvPlayerCityStateUA::CacheLuxuryHappiness()
{
	m_iCachedLuxuryHappiness = 0;
	if (!m_pPlayer) return;
	if (m_vLuxuryHappinessYieldModifiers.empty()) return;
	m_iCachedLuxuryHappiness = m_pPlayer->GetLuxuryHappinessBaseTotal();
}
// Almaty: refresh the cached surplus of each configured resource once per turn. GetMaxHitPoints reads
// the cached value so it never has to walk every city, while the kill count is read live.
void CvPlayerCityStateUA::CacheKillMaxHpSurplus()
{
	for (size_t i = 0; i < m_vKillMaxHpByPromotion.size(); i++)
	{
		m_vKillMaxHpByPromotion[i].m_iCachedSurplus = 0;
	}
	if (!m_pPlayer) return;
	for (size_t i = 0; i < m_vKillMaxHpByPromotion.size(); i++)
	{
		m_vKillMaxHpByPromotion[i].m_iCachedSurplus =
			m_pPlayer->getNumResourceAvailable((ResourceTypes)m_vKillMaxHpByPromotion[i].m_iResource, true);
	}
}
bool CvPlayerCityStateUA::HasLuxuryHappinessYieldModifiers() const
{
	return !m_vLuxuryHappinessYieldModifiers.empty();
}
bool CvPlayerCityStateUA::HasCombatDamageReductionVsNoTech() const
{
	return !m_vCombatDamageReductionVsNoTech.empty();
}
int CvPlayerCityStateUA::GetCachedSpecialCityPopulation(int iSpecialCityType) const
{
	if (iSpecialCityType < 0 || iSpecialCityType >= (int)m_aiCachedSpecialCityPopulation.size())
		return 0;
	return m_aiCachedSpecialCityPopulation[iSpecialCityType];
}
// Kuala Lumpur: puppet count, refreshed once per doTurn. CvPlayerTechs::GetResearchCost reads it on a
// hot path, so the city scan must not run there. The counting rule matches CvPlayer::GetNumPuppetCities
// (every IsPuppet city, regardless of razing/limbo state), which is exactly what GetMaxEffectiveCities
// adds back into the tech-cost city count. Puppet population is not cached here: the per-yield effect
// reads it from m_aiCachedSpecialCityPopulation, filled by CacheSpecialCityMatches.
int CvPlayerCityStateUA::GetCachedPuppetCount() const { return m_iCachedPuppetCount; }
void CvPlayerCityStateUA::CachePuppetStats()
{
	m_iCachedPuppetCount = 0;
	if (!m_pPlayer) return;
	// Only the tech-threshold path reads this count, and it skips the discount entirely unless the
	// player holds one of the two puppet tech-cost effects. Guard on the same condition so the city
	// scan is skipped for every player that has neither (the common case).
	if (!IsPuppetNoTechCostPenalty() && GetPuppetTechCostPartial() <= 0) return;

	int iLoop;
	for (const CvCity* pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		if (pLoopCity->IsPuppet())
		{
			m_iCachedPuppetCount++;
		}
	}
}
int CvPlayerCityStateUA::GetGoldenAgeYieldModifier(YieldTypes eYield) const
{
	return (eYield >= 0 && (int)eYield < (int)m_aiGoldenAgeYieldModifiers.size()) ? m_aiGoldenAgeYieldModifiers[(int)eYield] : 0;
}
bool CvPlayerCityStateUA::HasGoldenAgeYieldModifiers() const
{
	for (size_t i = 0; i < m_aiGoldenAgeYieldModifiers.size(); i++)
		if (m_aiGoldenAgeYieldModifiers[i] != 0) return true;
	return false;
}
bool CvPlayerCityStateUA::HasHappinessYieldModifiers() const
{
	for (size_t i = 0; i < m_aiHappinessYieldModifiers.size(); i++)
		if (m_aiHappinessYieldModifiers[i] != 0) return true;
	return false;
}
int CvPlayerCityStateUA::GetInquisitorRetentionPercent() const { return m_iInquisitorRetentionPercent; }
