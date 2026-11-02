/*	-------------------------------------------------------------------------------------------------------
	City-State Unique Ability System for Super Power V11 (Pillars of Sovereignty)
	------------------------------------------------------------------------------------------------------- */
#ifndef CVCITYSTATEUACLASSES_H
#define CVCITYSTATEUACLASSES_H

#pragma once

#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreDLLUtil.h"

//======================================================================================================
// CvCityStateUAEffectEntry - mirrors CityStateUAEffects database table
struct BornGreatPersonSpecialistYieldEntry {
	int m_iSpecialistType;
	int m_iUnitClassType;
	int m_iYieldType;
	int m_iYieldMod;
};

struct BuildingGreatPersonPointsEntry {
	int m_iBuildingClass;
	int m_iSpecialist;
	int m_iPoints;
};

struct BornGreatPersonAllyInfluenceModEntry {
	int m_iUnitClassType;
	int m_iModPerBorn;
};

struct BornGreatPersonNationwideYieldEntry {
	int m_iUnitClassType;
	int m_iYieldType;
	int m_iYieldMod;
};

struct LiteracyYieldModifierEntry {
	int m_iYieldType;
	int m_iYieldMod;
};

struct AdjacentImprovementYieldChangeEntry {
	int m_iImprovementType;          // local (affected) improvement type (strict match)
	int m_iAdjacentImprovementType;  // adjacent (neighbor) improvement that triggers the bonus
	int m_iYieldType;
	int m_iYield;
};

struct GreatWorkGreatPersonPointsEntry {
	int m_iGreatWorkClassType;
	int m_iSpecialistType;
	int m_iRate;
	bool m_bCapitalOnly;
};

struct UnitMaintenanceByPromotionEntry {
	int m_iPromotion;
	int m_iChange;
};

// Almaty: a unit holding the given promotion gains extra max HP equal to
// (the owner's total kills) x (the owner's surplus resource) x Percent / 100, evaluated live in
// CvUnit::GetMaxHitPoints. The surplus is cached once per turn (see CacheKillMaxHpSurplus) because
// reading it walks every city; the kill count stays live (a plain member read).
struct KillMaxHpByPromotionEntry {
	int m_iPromotion;
	int m_iResource;
	int m_iPercent;
	int m_iCachedSurplus = 0;
};

struct ResourcePerCityEntry {
	int m_iResource;
	int m_iQuantity;
	int m_iCityScale;
	bool m_bLargerScaleValid;
	bool m_bMustCoastal;
};

struct InternalTRToUCSPerEraYieldEntry {
	int m_iYieldType;
	int m_iYieldValue;
};

struct YieldToYieldViaTRToUCSEntry {
	int m_iInYieldType;
	int m_iOutYieldType;
	int m_iPercent;
	// 1 = the route must go TO this city-state (Colombo/Cape Town); 0 = any international route from the city (Mogadishu)
	bool m_bRequireRouteToThisCS;
};

// Monaco: unconditional yield-to-yield conversion (no trade route requirement)
struct YieldToYieldEntry {
	int m_iInYieldType;
	int m_iOutYieldType;
	int m_iMod;
};

// Monaco: one outcome of the first-gold-donation wager
struct GoldDonationGambleEntry {
	int m_iWeight;
	int m_iMultiplier;
};

struct PurchasedBuildingXPEntry {
	int m_iBuildingClass;
	int m_iDomain;
	int m_iXP;
};

struct UnitBornYieldEntry {
	int m_iMinorCivType;
	int m_iUnitClass;
	int m_iYieldType;
	int m_iYieldMod;
};

struct GreatWorkYieldModifierEntry {
	int m_iGreatWorkClassType;
	int m_iYieldType;
	int m_iYieldMod;
};

// Bucharest: each world wonder owned by the player grants a yield % modifier per YieldType, nation-wide.
// YieldMod is a PLAIN PERCENT (4 = +4% per world wonder), NOT basis points; Cap is a plain percent
// cap (0 = uncapped).
struct WorldWonderYieldModifierEntry {
	int m_iYieldType;
	int m_iYieldMod;
	int m_iCap;
};

// Quebec: for each met major civilization whose influence level toward the player is Unknown, a nation-wide
// yield % modifier per YieldType. YieldMod is a PLAIN PERCENT (4 = +4% per such civilization), NOT basis
// points; Cap is a plain percent cap on the accumulated sum (40 = +40% maximum), 0 = uncapped.
struct UnknownInfluenceYieldModifierEntry {
	int m_iYieldType;
	int m_iYieldMod;
	int m_iCap;
};

// Mogadishu: each international trade route the ally runs TO a city-state grants a yield % modifier per
// YieldType, nation-wide. Mirrors the building effect Building_CityStateTradeRouteYieldModifiersGlobal.
// YieldMod is a PLAIN PERCENT (5 = +5% per route), NOT basis points.
struct CityStateTradeRouteYieldModifierGlobalEntry {
	int m_iYieldType;
	int m_iYieldMod;
};

// Kabul: each international LAND trade route the ally runs grants trade-route gold %. YieldMod is a PLAIN
// PERCENT (50 = +50%); when the origin city sits on hills, HillsBonus is added on top (50 = an extra +50%).
struct LandTradeRouteGoldModifierEntry {
	int m_iYieldMod;
	int m_iHillsBonus;
};

// Kabul: each international LAND trade route the ally runs to ANY other player, city-states included,
// grants a nation-wide yield % modifier per YieldType, scaled by era
// (value * (currentEra + 1)). YieldMod is a PLAIN PERCENT (1 = +1% per route in the ancient era).
struct InternationalLandTradeRouteYieldEntry {
	int m_iYieldType;
	int m_iYieldMod;
};

// Bucharest: each diplomat stationed in a foreign MAJOR civilization's city grants a yield % modifier
// per YieldType, nation-wide. Diplomats sent to city-states do NOT count. YieldMod is a PLAIN PERCENT
// (5 = +5% per diplomat); Cap 0 = uncapped.
struct DiplomatAbroadYieldModifierEntry {
	int m_iYieldType;
	int m_iYieldMod;
	int m_iCap;
};

// Kiev: each League vote the player holds grants a yield % modifier per YieldType, nation-wide.
// YieldMod is in BASIS POINTS (100 = +1% per vote), matching Ife's GreatWorkYieldModifierEntry.
struct LeagueVoteYieldModifierEntry {
	int m_iYieldType;
	int m_iYieldMod;
};

//======================================================================================================
// Special city type - a named boolean predicate over a CvCity, described by data rows
// (CityStateUAEffect_SpecialCityTypeConditionsOr / ...And) and referenced by effect rows.
class CvCity;

// Condition kind for a special city type; unknown kinds are ignored (fail-safe).
// Add new branches in CvSpecialCityTypeEntry::EvaluateCondition when a future city-state needs them.
enum SpecialCityConditionTypes {
	SPECIAL_CITY_CONDITION_NONE = 0,
	SPECIAL_CITY_CONDITION_HAS_RESOURCE,   // Value = resource type (must be developed/improved)
	SPECIAL_CITY_CONDITION_HAS_FEATURE,    // Value = feature type (inside the city's territory)
	SPECIAL_CITY_CONDITION_IS_RIVER,       // Boolean, no Value: the city center sits on a river
	SPECIAL_CITY_CONDITION_IS_COASTAL,     // Boolean, no Value: the city center borders the sea (lakes excluded)
	SPECIAL_CITY_CONDITION_IS_PUPPET,      // Boolean, no Value: the city is a puppet (annexed cities do not qualify)
	SPECIAL_CITY_CONDITION_IS_OTHER_CONTINENT, // Boolean, no Value: the city sits on a different landmass than the owner's original capital
	SPECIAL_CITY_CONDITION_HAS_LAND_AND_SEA_INTERNATIONAL_TR, // Boolean, no Value: the city is the origin of both an international land route and an international sea route
	SPECIAL_CITY_CONDITION_NO_INTERNATIONAL_TR, // Boolean, no Value: the city is neither the origin nor the destination of any international trade route
	NUM_SPECIAL_CITY_CONDITION_TYPES
};

struct SpecialCityConditionEntry {
	SpecialCityConditionTypes m_eConditionType;
	int m_iValue;   // resource/feature ID, -1 for boolean conditions
};

struct SpecialCityYieldModifierEntry {
	int m_iSpecialCityType;
	int m_iYieldType;
	int m_iYieldMod;
};

struct SpecialCityCountYieldModifierEntry {
	int m_iSpecialCityType;
	int m_iYieldType;
	int m_iYieldMod;
};

// Kuala Lumpur: per N population living in cities matching a special city type, a nation-wide yield %
// modifier per YieldType. YieldMod is a PLAIN PERCENT (2 = +2% per N population), NOT basis points;
// PerPopulation is the population step (5 = count one step per 5 population).
struct SpecialCityPopulationYieldModifierEntry {
	int m_iSpecialCityType;
	int m_iPerPopulation;
	int m_iYieldType;
	int m_iYieldMod;
};

// Tyre: cities matching a special city type take Percent% less damage. Percent is a PLAIN PERCENT
// (40 = -40% damage taken); matching rows sum and the total is clamped to 90. Only damage routed through
// CvCity::changeDamage is reduced; nuclear explosions set city damage directly in
// CvUnitCombat::ApplyNuclearExplosionDamage and therefore bypass this reduction.
struct SpecialCityDamageReductionEntry {
	int m_iSpecialCityType;
	int m_iPercent;
};

// Singapore / Tyre: each owned building class grants a nation-wide yield % modifier per YieldType. The
// count comes from CvPlayer::getBuildingClassCount (maintained live by the building system). For
// one-per-city dummy buildings (city scale / corruption tiers) this equals the number of cities of that
// tier. Real buildings are also allowed and are counted per building, so a single city holding two
// counted building classes (e.g. Tyre's Wood Dock + Shipyard) contributes twice. YieldMod is a PLAIN
// PERCENT (10 = +10% per owned building class).
struct BuildingClassGlobalYieldModifierEntry {
	int m_iBuildingClass;
	int m_iYieldType;
	int m_iYieldMod;
};

// Singapore: each owned building class lowers the city-count research threshold
// (CvPlayerTechs::GetResearchCost) by TechCostMod threshold points, nation-wide. TechCostMod is a plain
// number of threshold points, NOT a percent of the threshold (2 = -2 per owned building class). The
// consumer subtracts the sum from the modifier and floors it at 0.
struct BuildingClassTechCostModifierEntry {
	int m_iBuildingClass;
	int m_iTechCostMod;
};

// Milan: each point of luxury happiness the player has grants a nation-wide yield % modifier per
// YieldType. YieldMod is in BASIS POINTS per point of luxury happiness (50 = +0.5% per point, i.e.
// every 2 points +1%); Cap is a plain percent cap on the accumulated sum (0 = uncapped). The luxury
// happiness total is cached once per doTurn (CvPlayerCityStateUA::CacheLuxuryHappiness).
struct LuxuryHappinessYieldModifierEntry {
	int m_iYieldType;
	int m_iYieldMod;
	int m_iCap;
};

// Milan: a unit takes Percent% less damage when the opposing side's team has NOT researched TechType
// (Percent is a PLAIN PERCENT, 25 = -25% damage taken). Applies in both directions: when the unit is
// the defender being hit and when it is the attacker taking the counter-blow. Consumed live by
// CvUnit::GetCSUADamageTakenScale, which is shared by the real resolution and the UI combat panel.
struct CombatDamageReductionVsNoTechEntry {
	int m_iTech;
	int m_iPercent;
};

// A city type matches when: EVERY And-row matches AND (the Or table is empty OR at least one Or-row matches).
class CvSpecialCityTypeEntry : public CvBaseInfo
{
public:
	CvSpecialCityTypeEntry(void);
	~CvSpecialCityTypeEntry(void);

	bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

	bool IsCityMatch(const CvCity* pCity) const;

private:
	bool EvaluateCondition(const SpecialCityConditionEntry& kCondition, const CvCity* pCity) const;

	std::vector<SpecialCityConditionEntry> m_vConditionsOr;
	std::vector<SpecialCityConditionEntry> m_vConditionsAnd;
	// Set when a condition table has rows but none of them parsed. IsCityMatch then fails closed:
	// without this an all-unparsable Or table would be read as "no restriction" and match every city.
	bool m_bConditionsInvalid;
};

//======================================================================================================
// CvCityStateUASpecialCityTypeXMLEntries - container for all CityStateUAEffect_SpecialCityTypes entries
//======================================================================================================
class CvCityStateUASpecialCityTypeXMLEntries
{
public:
	CvCityStateUASpecialCityTypeXMLEntries(void);
	~CvCityStateUASpecialCityTypeXMLEntries(void);

	std::vector<CvSpecialCityTypeEntry*>& GetEntries();
	int GetNumEntries() const;
	CvSpecialCityTypeEntry* GetEntry(int index) const;
	void DeleteArray();

private:
	std::vector<CvSpecialCityTypeEntry*> m_paEntries;
};

//======================================================================================================
class CvCityStateUAEffectEntry : public CvBaseInfo
{
public:
	CvCityStateUAEffectEntry(void);
	~CvCityStateUAEffectEntry(void);

	bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

	// Florence (Fulolunsi)
	int GetFaithPurchaseGreatPeopleCostRiseModifier() const;
	int GetFaithPurchaseGreatPeopleCostRiseModifierPerGW() const;
	bool IsFaithPurchaseAllGreatPeople() const;
	int GetGreatPersonPoints(int i) const;
	// Buenos Aires (BuYiNuoSiAiLiSi)
	bool IsGPNoDeathAfterGreatWork() const;
	int GetGPConcertTourismRetentionPercent() const;
	// Brussels (BuLuSaiEr)
	int GetGreatMusicianConcertGoldPercent() const;
	// Bratislava (BuLaDiSiLaFa)
	int GetCapitalAndSecondCapitalCultureModifier() const;
	// Bucharest (BuJiaLeSiTe)
	int GetImmigrationRatePerImmigrant() const;
	int GetImmigrationRateMax() const;
	int GetEmigrationRatePerImmigrant() const;
	int GetEmigrationRateMax() const;
	// Kuala Lumpur (JiLongPo)
	bool IsPuppetNoTechCostPenalty() const;
	int GetPuppetTechCostPartial() const;
	// Almaty (ALaMuTu)
	bool IsCanPillageNeutralTradeRoute() const;
	int GetPlunderTradeRouteGold() const;
	int GetPlunderTradeRouteXP() const;
	int GetPlunderTradeRouteOpinionPenalty() const;
	// Almaty: extra max HP per kill per surplus resource while holding a promotion (live in GetMaxHitPoints)
	const std::vector<KillMaxHpByPromotionEntry>& GetKillMaxHpByPromotionEntries() const { return m_vKillMaxHpByPromotion; }
	// Belgrade (BeiErGeLaiDe)
	int GetGarrisonCityDefenseModifier() const;
	int GetMilitaryUnitProductionXP() const;
	int GetZOCRangeBonus() const;
	// Budapest (BuDaPeiSi)
	bool IsLandUnitsImmuneRiverCrossing() const;
	int GetWoundedFixedDamage() const;
	// Ha Noi (HeNei)
	int GetEnemyFixedDamageModifierInBorders() const;
	int GetCulturePerWarPeace() const;
	int GetEnemyCombatModifierInBordersPerBeenDoW() const;
	// Mbanza Kongo (MuBanZhaGangGuo)
	int GetUnitProductionModifierPerCity() const;
	int GetCombatBonusPerTechDifference() const;
	// Mbanza Kongo: each owned city provides the listed resource (see CityStateUAEffect_ResourcePerCity)
	const std::vector<ResourcePerCityEntry>& GetResourcePerCityEntries() const { return m_vResourcePerCity; }
	// Sidon (XiDun)
	int GetCityAttackIgnoreBuildingDefensePercent() const;
	int GetMilitaryXPPerTurnModifier() const;
	int GetMilitaryXPSeaAir() const;
	// Sofia (SuoFeiYa) spy/coup UA
	int GetCoupChanceModifier() const;
	bool GetCoupFailSpySurvives() const;
	int GetStealTechSpeedPerSpy() const;
	int GetSpyKillChancePerSpy() const;
	// Vatican (FanDiGang)
	int GetReligionSpreadSpeedModifier() const;
	// Vatican: Papal Recognition league delegate votes granted to each following civilization (mainstream votes)
	int GetPapalRecognitionVotes() const;
	// Vatican: Papal Recognition league delegate votes granted to the ally per following civilization (including itself)
	int GetPapalRecognitionAllyVotes() const;
	// Vatican: per following city, the holy city gains +Modifier% of the yield (per YieldType, 100 = +1%)
	int GetHolyCityYieldModifierPerFollowingCity(int i) const;
	// Jerusalem (YeLuSaLeng): +X% religious pressure per holy city owned by the player (applies to the founder's religion)
	int GetReligiousPressureModifierPerHolyCity() const;
	// Jerusalem (YeLuSaLeng): player who is the ally of this city-state cannot be denounced
	bool IsDenounceImmunity() const;
	// Jerusalem / Wittenberg: per city following the player's religion, capital gains +Modifier% of the yield (per YieldType, 100 = +1%)
	int GetCapitalYieldModifierPerFollowingCity(int i) const;
	// Kyzyl (KeZiLe)
	int GetLandTradeRouteDistancePerTradeSlot() const;
	int GetTradeRouteGoldPercentNonNeighbor() const;
	// Dubai (DiBai)
	int GetHappinessPerGoldDonated() const;
	int GetGoldDonationInterval() const;
	int GetWonderProductionPerDonationHappiness() const;
	int GetIdeologyPressurePerDonationHappiness() const;
	// Genoa (ReNaYa): gold-gift influence +% per sea trade route
	int GetGoldDonationInfluenceModifierPerSeaRoute() const;
	// Genoa / Vilnius: per-unit yield % modifiers, keyed by YieldType (Rate=100 => +1% per friend/ally city-state or unlocked policy)
	int GetFriendCityStateYieldModifier(YieldTypes eYieldType) const;
	int GetAllyCityStateYieldModifier(YieldTypes eYieldType) const;
	int GetPolicyYieldModifier(YieldTypes eYieldType) const;
	// Vilnius (WeiErNiWuSi): fixed GA threshold reduction per population (before percentage modifier)
	int GetGoldenAgeThresholdPerPopulation() const;
	// Malacca (MaLiuJia)
	int GetLuxuryHappinessModifier() const;
	// Ragusa: percent modifier on the city's local-happiness cap
	int GetLocalHappinessCapModifier() const;
	// Monaco: golden-age building maintenance modifier for the ally
	int GetGoldenAgeBuildingMaintenanceMod() const;
	int GetFoodKeptModifierPerLuxury() const;
	int GetTradeRouteGoldModifierPerLuxuryType() const;
	// Panama (BaNaMa)
	int GetTradeRouteGoldModifierPerDistance() const;
	int GetUnhappinessReductionPerCrossContinentRoute() const;
	// Manila (MaNiLa)
	int GetTradeRouteGoldPercentInternational() const;
	int GetTradeRouteGoldModifierPerInternationalRoute() const;
	int GetFoodModifierPerHappyLuxuryType() const;
	int GetFoodModifierPerHappyLuxuryCap() const;
	// Mogadishu (MoJiaDiSha)
	const std::vector<CityStateTradeRouteYieldModifierGlobalEntry>& GetCityStateTradeRouteYieldModifiersGlobal() const { return m_vCityStateTradeRouteYieldModifiersGlobal; }
	// Kabul (KaBuEr): each international land trade route grants trade-route gold %, plus an extra bonus
	// when the origin city sits on hills
	const std::vector<LandTradeRouteGoldModifierEntry>& GetLandTradeRouteGoldModifiers() const { return m_vLandTradeRouteGoldModifiers; }
	// Kabul: per international land trade route to any other player (city-states included), a nation-wide yield % per era
	const std::vector<InternationalLandTradeRouteYieldEntry>& GetInternationalLandTradeRouteYieldPerEra() const { return m_vInternationalLandTradeRouteYieldPerEra; }
	int GetResearchAgreementBreakBonusPercent() const;
	const std::vector<BornGreatPersonSpecialistYieldEntry>& GetBornGreatPersonSpecialistYieldEntries() const { return m_vBornGreatPersonSpecialistYield; }
	const std::vector<BuildingGreatPersonPointsEntry>& GetBuildingGreatPersonPointsEntries() const { return m_vBuildingGPP; }
	const std::vector<BornGreatPersonAllyInfluenceModEntry>& GetBornAllyInfluenceModEntries() const { return m_vBornAllyInfluenceMod; }
	// Budapest: per owned unit holding a promotion, changes the player's total unit maintenance
	const std::vector<UnitMaintenanceByPromotionEntry>& GetUnitMaintenanceByPromotionEntries() const { return m_vUnitMaintenanceByPromotion; }
	// Prague (BuLaGe) / Yerevan (AiLiWen): building-class yield percentage modifiers
	int GetBuildingClassYieldModifiers(int i, int j) const;
	// Brussels (BuLuSaiEr): specialist great person point accumulation rate (%)
	int GetSpecialistPointRate(int i) const;
	// Brussels: each great work of a class grants great person points to a specialist (Rate=100 => 1 great work = 1 point)
	const std::vector<GreatWorkGreatPersonPointsEntry>& GetGreatWorkGreatPersonPointsEntries() const { return m_vGreatWorkGreatPersonPoints; }
	// Brussels: specified unit class's one-shot great person output modifier (%)
	int GetGreatPersonOneShotModifier(int i) const;
	// Colombo: flat per-era yield on the international trade route (InternalTR) to this city-state (UCS)
	const std::vector<InternalTRToUCSPerEraYieldEntry>& GetInternalTRToUCSPerEraYieldEntries() const { return m_vInternalTRToUCSPerEraYield; }
	int GetInternalTRToUCSPerEraYield(int eYield) const;
	// Colombo: in cities with a trade route to this city-state (UCS), a percentage of the input yield is granted as extra output yield
	int GetYieldToYieldViaTRToUCS(int eInYield, int eOutYield) const;
	// Mogadishu: whether the YieldToYieldViaTRToUCS entry for (eInYield -> eOutYield) requires a trade
	// route TO this city-state (true) or any international trade route originating from the city (false)
	bool YieldToYieldViaTRToUCSRequiresRouteToThisCS(int eInYield, int eOutYield) const;
	// Monaco: unconditional conversion, Mod% of the city's eInYield output is granted as extra eOutYield output
	int GetYieldToYield(int eInYield, int eOutYield) const;
	// Colombo / Cape Town / Mogadishu / Monaco: the raw conversion rows carried by this effect.
	// Exposed so ApplyEffect can maintain a player-level "has any conversion" flag, which keeps the
	// per-city per-yield CvCity::GetYieldRateFromUCSConversion out of the city-state scan.
	const std::vector<YieldToYieldViaTRToUCSEntry>& GetYieldToYieldViaTRToUCSEntries() const { return m_vYieldToYieldViaTRToUCS; }
	const std::vector<YieldToYieldEntry>& GetYieldToYieldEntries() const { return m_vYieldToYield; }
	// Monaco: the outcomes of the first-gold-donation wager (empty = effect not present)
	const std::vector<GoldDonationGambleEntry>& GetGoldDonationGambleEntries() const { return m_vGoldDonationGamble; }
	// Valletta: enemy city besieged by >= this many of our combat units cannot heal
	int GetEnemyCityNoHealBesiegeCount() const;
	// Valletta: buying the specified building class grants all units of the specified domain XP
	const std::vector<PurchasedBuildingXPEntry>& GetPurchasedBuildingXPEntries() const { return m_vPurchasedBuildingXP; }
	// Valletta: born unit of the specified unit class grants a configurable yield = YieldMod% of influence with MinorCivType
	const std::vector<UnitBornYieldEntry>& GetUnitBornYieldEntries() const { return m_vUnitBornYield; }
	// Prague: a city with our own spy garrisoned in it grants yield percentage modifiers (per YieldType)
	int GetSpyGarrisonYieldModifiers(int i) const;
	// Prague: killing an enemy spy grants spy progress toward a new spy (100 = kill 1 gain 1)
	int GetSpyKillGainSpyProgress() const;
	// Melbourne: a city that owns the specified improved resource grants yield percentage modifiers (per YieldType)
	int GetResourceYieldModifiers(int i, int j) const;
	// Antananarivo
	int GetCoastalCityGrowthThresholdModifier() const;
	int GetDiplomaticPrestigePerCity() const;
	int GetImprovementYieldModifiers(int i, int j) const;
	// Zanzibar: each worked plot holding the specified improvement grants flat local happiness
	int GetImprovementHappiness(int i) const;
	// Ragusa: each owned building of the specified class grants flat local happiness
	int GetBuildingClassHappiness(int i) const;
	// Hormuz: each unit of surplus strategic resource grants trade-route gold %
	int GetTradeRouteGoldPerSurplusResource(int i) const;
	// Load-time "is this sub-table present" flags for the fixed-size sub-tables above. The backing
	// arrays are always allocated (Initialize2DArray / PopulateArrayByValue), so a NULL check cannot tell
	// whether an effect actually carries data; ApplyEffect uses these flags to skip the whole-dimension
	// scan for effects that do not (the common case: most effects carry none of these).
	bool HasBuildingClassYieldModifiers() const { return m_bHasBuildingClassYieldModifiers; }
	bool HasResourceYieldModifiers() const { return m_bHasResourceYieldModifiers; }
	bool HasImprovementYieldModifiers() const { return m_bHasImprovementYieldModifiers; }
	bool HasImprovementHappiness() const { return m_bHasImprovementHappiness; }
	bool HasBuildingClassHappiness() const { return m_bHasBuildingClassHappiness; }
	bool HasTradeRouteGoldPerSurplusResource() const { return m_bHasTradeRouteGoldPerSurplusResource; }
	bool HasSpecialistPointRate() const { return m_bHasSpecialistPointRate; }
	bool HasGreatPersonOneShotModifier() const { return m_bHasGreatPersonOneShotModifier; }
	bool HasFaithGPClassCostModifier() const { return m_bHasFaithGPClassCostModifier; }
	bool HasGreatPersonPoints() const { return m_bHasGreatPersonPoints; }
	// Gangtok: per city worldwide following the player's religion, global happiness (100 = +1 happiness per city)
	int GetHappinessPerFollowingCity() const;
	// Gangtok: buy influence at any city-state with faith at (gold price / divisor) faith (divisor > 0 enables the feature)
	int GetFaithInfluencePurchaseCostDivisor() const;
	// Gangtok: how many faith influence purchases the ally may make per turn (globally)
	int GetFaithInfluencePurchasePerTurnLimit() const;
	// Wittenberg: the ally may spend faith to add one belief to the city-state's religion (true = enabled)
	bool GetFaithBeliefPurchase() const;
	// Wittenberg: keep this % of the followers when an inquisitor clears the city-state's religion
	int GetInquisitorRetentionPercent() const;
	// Wittenberg: the ally's inquisitors may remove heresy in city-state cities
	bool GetInquisitorCanUseOnCityState() const;
	// La Venta: the ally may spend faith to add an idle pantheon belief to the religion the ally leads (true = enabled)
	bool GetFaithPantheonPurchase() const;
	// La Venta: +X% great-person rate per masterpiece/artifact the ally owns
	int GetGreatPersonRateModifierPerGreatWork() const;
	// Kiev: +X% great-person rate per national wonder the ally/friend has completed (plain percent)
	int GetGreatPersonRateModifierPerNationalWonder() const;
	// Kiev: League delegate votes granted per civilization the ally has a Declaration of Friendship with
	int GetLeagueVotesPerDoF() const;
	// Ur: global happiness per world wonder owned by the ally/friend (100 = +1 happiness per world wonder)
	int GetWorldWonderHappiness() const;
	// Kathmandu: the first gold donation each turn refunds a % of the amount as faith to the ally
	int GetFaithRefundPerDonationPercent() const;
	// Geneva: diplomatic prestige per major civilization whose majority religion is the ally-led religion
	int GetDiplomaticPrestigePerMajorityCiv() const;
	// Geneva: per-turn influence with each met city-state, one unit per FollowingCityDivisor following cities (x100)
	int GetInfluencePerTurnPerFollowCityMod() const;
	// Geneva: how many cities following the ally-led religion produce one per-turn influence unit
	int GetFollowingCityDivisor() const;
	// Vancouver: global happiness per coastal city owned by the ally/friend (100 = +1 happiness per coastal city)
	int GetCoastalCityHappiness() const;
	// Vancouver: per point of the player's net happiness, a yield % modifier per YieldType (YieldMod=100 => +1% per happiness)
	int GetHappinessYieldModifier(int i) const;
	// Vancouver: per-point yield % modifier cap per YieldType (in percent, 50 = +50% maximum)
	int GetHappinessYieldModifierCap(int i) const;
	// Ife: per-unitclass discount on the FAITH cost of buying great people (CostRiseModifier in percent, negative = discount)
	int GetFaithGPClassCostModifier(int i) const;
	// Ife: each great work / artifact of a GreatWorkClassType grants a yield % modifier per YieldType (YieldMod basis points, 100 = +1%)
	const std::vector<GreatWorkYieldModifierEntry>& GetGreatWorkYieldModifiers() const { return m_vGreatWorkYieldModifiers; }
	// Ife: while the player is in a golden age, grant a yield % modifier per YieldType (YieldMod in percent, 25 = +25%)
	int GetGoldenAgeYieldModifier(int i) const;
	// Sydney: per immigrant received, a yield % modifier per YieldType (Modifier=100 => +1%)
	int GetImmigrantYieldModifier(int i) const;
	bool HasImmigrantYieldModifiers() const;
	// Sydney: each immigrant received grants cash (CashPercent% of treasury, capped by CashCapBase x era x game speed)
	int GetImmigrantCashPercent() const;
	int GetImmigrantCashCapBase() const;
	// Yerevan: global happiness per worked holy-site improvement (100 = +1, no local cap)
	int GetHolySiteHappiness() const;
	const std::vector<LiteracyYieldModifierEntry>& GetLiteracyYieldModifiers() const { return m_vLiteracyYieldModifiers; }
	const std::vector<BornGreatPersonNationwideYieldEntry>& GetBornGreatPersonYieldModifiers() const { return m_vBornGreatPersonYieldModifiers; }
	const std::vector<AdjacentImprovementYieldChangeEntry>& GetAdjacentImprovementYieldChanges() const { return m_vAdjacentImprovementYieldChanges; }
	// Bogota: cities matching the special city type gain a yield % modifier; per owned matching city, all cities do
	const std::vector<SpecialCityYieldModifierEntry>& GetSpecialCityYieldModifiers() const { return m_vSpecialCityYieldModifiers; }
	const std::vector<SpecialCityCountYieldModifierEntry>& GetSpecialCityCountYieldModifiers() const { return m_vSpecialCityCountYieldModifiers; }
	// Bucharest
	const std::vector<WorldWonderYieldModifierEntry>& GetWorldWonderYieldModifiers() const { return m_vWorldWonderYieldModifiers; }
	const std::vector<DiplomatAbroadYieldModifierEntry>& GetDiplomatAbroadYieldModifiers() const { return m_vDiplomatAbroadYieldModifiers; }
	// Quebec
	int GetCultureVictoryProgressModifier() const { return m_iCultureVictoryProgressModifier; }
	const std::vector<UnknownInfluenceYieldModifierEntry>& GetUnknownInfluenceYieldModifiers() const { return m_vUnknownInfluenceYieldModifiers; }
	// Kiev
	const std::vector<LeagueVoteYieldModifierEntry>& GetLeagueVoteYieldModifiers() const { return m_vLeagueVoteYieldModifiers; }
	// Kuala Lumpur
	const std::vector<SpecialCityPopulationYieldModifierEntry>& GetSpecialCityPopulationYieldModifiers() const { return m_vSpecialCityPopulationYieldModifiers; }
	// Tyre
	const std::vector<SpecialCityDamageReductionEntry>& GetSpecialCityDamageReductions() const { return m_vSpecialCityDamageReductions; }
	// Singapore
	const std::vector<BuildingClassGlobalYieldModifierEntry>& GetBuildingClassGlobalYieldModifiers() const { return m_vBuildingClassGlobalYieldModifiers; }
	const std::vector<BuildingClassTechCostModifierEntry>& GetBuildingClassTechCostModifiers() const { return m_vBuildingClassTechCostModifiers; }
	// Milan
	const std::vector<LuxuryHappinessYieldModifierEntry>& GetLuxuryHappinessYieldModifiers() const { return m_vLuxuryHappinessYieldModifiers; }
	const std::vector<CombatDamageReductionVsNoTechEntry>& GetCombatDamageReductionVsNoTech() const { return m_vCombatDamageReductionVsNoTech; }

private:
	// Florence
	int m_iFaithPurchaseGreatPeopleCostRiseModifier;
	int m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW;
	int* m_piGreatPersonPoints;
	bool m_bFaithPurchaseAllGreatPeople;
	// Buenos Aires
	bool m_bGPNoDeathAfterGreatWork;
	int m_iGPConcertTourismRetentionPercent;
	// Brussels
	int m_iGreatMusicianConcertGoldPercent;
	// Bratislava
	int m_iCapitalAndSecondCapitalCultureModifier;
	// Bucharest
	int m_iImmigrationRatePerImmigrant;
	int m_iImmigrationRateMax;
	int m_iEmigrationRatePerImmigrant;
	int m_iEmigrationRateMax;
	// Kuala Lumpur
	bool m_bPuppetNoTechCostPenalty;
	int m_iPuppetTechCostPartial;
	// Almaty
	bool m_bCanPillageNeutralTradeRoute;
	int m_iPlunderTradeRouteGold;
	int m_iPlunderTradeRouteXP;
	int m_iPlunderTradeRouteOpinionPenalty;
	std::vector<KillMaxHpByPromotionEntry> m_vKillMaxHpByPromotion;
	// Belgrade
	int m_iGarrisonCityDefenseModifier;
	int m_iMilitaryUnitProductionXP;
	int m_iZOCRangeBonus;
	// Budapest
	bool m_bLandUnitsImmuneRiverCrossing;
	int m_iWoundedFixedDamage;
	// Ha Noi
	int m_iEnemyFixedDamageModifierInBorders;
	int m_iCulturePerWarPeace;
	int m_iEnemyCombatModifierInBordersPerBeenDoW;
	// Mbanza Kongo
	int m_iUnitProductionModifierPerCity;
	int m_iCombatBonusPerTechDifference;
	std::vector<ResourcePerCityEntry> m_vResourcePerCity;
	// Sidon
	int m_iCityAttackIgnoreBuildingDefensePercent;
	int m_iMilitaryXPPerTurnModifier;
	int m_iMilitaryXPSeaAir;
	// Sofia (spy/coup UA)
	int m_iCoupChanceModifier;
	bool m_bCoupFailSpySurvives;
	int m_iStealTechSpeedPerSpy;
	int m_iSpyKillChancePerSpy;
	// Vatican
	int m_iReligionSpreadSpeedModifier;
	int m_iPapalRecognitionVotes;
	int m_iPapalRecognitionAllyVotes;
	int* m_piHolyCityYieldModifierPerFollowingCity;
	// Jerusalem
	int m_iReligiousPressureModifierPerHolyCity;
	bool m_bDenounceImmunity;
	int* m_piCapitalYieldModifierPerFollowingCity;
	// Kyzyl
	int m_iLandTradeRouteDistancePerTradeSlot;
	int m_iTradeRouteGoldPercentNonNeighbor;
	// Dubai
	int m_iHappinessPerGoldDonated;
	int m_iGoldDonationInterval;
	int m_iWonderProductionPerDonationHappiness;
	int m_iIdeologyPressurePerDonationHappiness;
	// Genoa
	int m_iGoldDonationInfluenceModifierPerSeaRoute;
	// Genoa / Vilnius (per YieldType arrays)
	int* m_piFriendCityStateYieldModifiers;
	int* m_piAllyCityStateYieldModifiers;
	int* m_piPolicyYieldModifiers;
	// Vilnius
	int m_iGoldenAgeThresholdPerPopulation;
	// Malacca
	int m_iLuxuryHappinessModifier;
	int m_iFoodKeptModifierPerLuxury;
	int m_iTradeRouteGoldModifierPerLuxuryType;
	// Ragusa
	int m_iLocalHappinessCapModifier;
	// Monaco
	int m_iGoldenAgeBuildingMaintenanceMod;
	// Panama
	int m_iTradeRouteGoldModifierPerDistance;
	int m_iUnhappinessReductionPerCrossContinentRoute;
	// Manila
	int m_iTradeRouteGoldPercentInternational;
	int m_iTradeRouteGoldModifierPerInternationalRoute;
	int m_iFoodModifierPerHappyLuxuryType;
	int m_iFoodModifierPerHappyLuxuryCap;
	// Mogadishu
	std::vector<CityStateTradeRouteYieldModifierGlobalEntry> m_vCityStateTradeRouteYieldModifiersGlobal;
	// Kabul
	std::vector<LandTradeRouteGoldModifierEntry> m_vLandTradeRouteGoldModifiers;
	std::vector<InternationalLandTradeRouteYieldEntry> m_vInternationalLandTradeRouteYieldPerEra;
	int m_iResearchAgreementBreakBonusPercent;
	// Prague / Yerevan
	int** m_ppiBuildingClassYieldModifiers;
	// Brussels
	int* m_piSpecialistPointRate;
	std::vector<GreatWorkGreatPersonPointsEntry> m_vGreatWorkGreatPersonPoints;
	int* m_piGreatPersonOneShotModifier;
	std::vector<BornGreatPersonSpecialistYieldEntry> m_vBornGreatPersonSpecialistYield;
	std::vector<BuildingGreatPersonPointsEntry> m_vBuildingGPP;
	std::vector<BornGreatPersonAllyInfluenceModEntry> m_vBornAllyInfluenceMod;
	std::vector<UnitMaintenanceByPromotionEntry> m_vUnitMaintenanceByPromotion;
	std::vector<InternalTRToUCSPerEraYieldEntry> m_vInternalTRToUCSPerEraYield;
	std::vector<YieldToYieldViaTRToUCSEntry> m_vYieldToYieldViaTRToUCS;
	// Monaco
	std::vector<YieldToYieldEntry> m_vYieldToYield;
	std::vector<GoldDonationGambleEntry> m_vGoldDonationGamble;
	// Valletta
	int m_iEnemyCityNoHealBesiegeCount;
	std::vector<PurchasedBuildingXPEntry> m_vPurchasedBuildingXP;
	std::vector<UnitBornYieldEntry> m_vUnitBornYield;
	// Prague
	int* m_piSpyGarrisonYieldModifiers;
	int m_iSpyKillGainSpyProgress;
	// Melbourne
	int** m_ppiResourceYieldModifiers;
	// Antananarivo
	int m_iCoastalCityGrowthThresholdModifier;
	int m_iDiplomaticPrestigePerCity;
	int** m_ppiImprovementYieldModifiers;
	// Zanzibar
	int* m_piImprovementHappiness;
	// Ragusa
	int* m_piBuildingClassHappiness;
	// Hormuz
	int* m_piTradeRouteGoldPerSurplusResource;
	// Load-time presence flags for the fixed-size sub-tables above (see the matching Has* getters).
	// The backing arrays are always allocated, so ApplyEffect needs these to tell whether an effect
	// actually carries data before scanning the whole dimension.
	bool m_bHasBuildingClassYieldModifiers;
	bool m_bHasResourceYieldModifiers;
	bool m_bHasImprovementYieldModifiers;
	bool m_bHasImprovementHappiness;
	bool m_bHasBuildingClassHappiness;
	bool m_bHasTradeRouteGoldPerSurplusResource;
	bool m_bHasSpecialistPointRate;
	bool m_bHasGreatPersonOneShotModifier;
	bool m_bHasFaithGPClassCostModifier;
	bool m_bHasGreatPersonPoints;
	// Gangtok
	int m_iHappinessPerFollowingCity;
	int m_iFaithInfluencePurchaseCostDivisor;
	int m_iFaithInfluencePurchasePerTurnLimit;
	// Wittenberg
	bool m_bFaithBeliefPurchase;
	int m_iInquisitorRetentionPercent;
	bool m_bInquisitorCanUseOnCityState;
	// La Venta
	bool m_bFaithPantheonPurchase;
	int m_iGreatPersonRateModifierPerGreatWork;
	// Kathmandu
	int m_iFaithRefundPerDonationPercent;
	// Geneva
	int m_iDiplomaticPrestigePerMajorityCiv;
	int m_iInfluencePerTurnPerFollowCityMod;
	int m_iFollowingCityDivisor;
	// Sydney
	int* m_piImmigrantYieldModifiers;
	int m_iImmigrantCashPercent;
	int m_iImmigrantCashCapBase;
	// Vancouver
	int m_iCoastalCityHappiness;
	int* m_piHappinessYieldModifiers;
	int* m_piHappinessYieldModifierCaps;
	// Ife
	int* m_piFaithGPClassCostModifier;
	std::vector<GreatWorkYieldModifierEntry> m_vGreatWorkYieldModifiers;
	int* m_piGoldenAgeYieldModifiers;
	// Yerevan
	int m_iHolySiteHappiness;
	std::vector<LiteracyYieldModifierEntry> m_vLiteracyYieldModifiers;
	std::vector<BornGreatPersonNationwideYieldEntry> m_vBornGreatPersonYieldModifiers;
	std::vector<AdjacentImprovementYieldChangeEntry> m_vAdjacentImprovementYieldChanges;
	// Bogota
	std::vector<SpecialCityYieldModifierEntry> m_vSpecialCityYieldModifiers;
	std::vector<SpecialCityCountYieldModifierEntry> m_vSpecialCityCountYieldModifiers;
	// Bucharest
	std::vector<WorldWonderYieldModifierEntry> m_vWorldWonderYieldModifiers;
	std::vector<DiplomatAbroadYieldModifierEntry> m_vDiplomatAbroadYieldModifiers;
	// Quebec
	int m_iCultureVictoryProgressModifier;
	std::vector<UnknownInfluenceYieldModifierEntry> m_vUnknownInfluenceYieldModifiers;
	// Kiev
	int m_iGreatPersonRateModifierPerNationalWonder;
	int m_iLeagueVotesPerDoF;
	std::vector<LeagueVoteYieldModifierEntry> m_vLeagueVoteYieldModifiers;
	// Ur
	int m_iWorldWonderHappiness;
	// Kuala Lumpur
	std::vector<SpecialCityPopulationYieldModifierEntry> m_vSpecialCityPopulationYieldModifiers;
	// Tyre
	std::vector<SpecialCityDamageReductionEntry> m_vSpecialCityDamageReductions;
	// Singapore
	std::vector<BuildingClassGlobalYieldModifierEntry> m_vBuildingClassGlobalYieldModifiers;
	std::vector<BuildingClassTechCostModifierEntry> m_vBuildingClassTechCostModifiers;
	// Milan
	std::vector<LuxuryHappinessYieldModifierEntry> m_vLuxuryHappinessYieldModifiers;
	std::vector<CombatDamageReductionVsNoTechEntry> m_vCombatDamageReductionVsNoTech;
};

//======================================================================================================
// CvCityStateUAEffectXMLEntries - container for all CityStateUAEffects entries
//======================================================================================================
class CvCityStateUAEffectXMLEntries
{
public:
	CvCityStateUAEffectXMLEntries(void);
	~CvCityStateUAEffectXMLEntries(void);

	std::vector<CvCityStateUAEffectEntry*>& GetEffectEntries();
	int GetNumEffects() const;
	CvCityStateUAEffectEntry* GetEntry(int index) const;
	void DeleteArray();

private:
	std::vector<CvCityStateUAEffectEntry*> m_paEffectEntries;
};

//======================================================================================================
// CvCityStateUAEntry - mirrors CityStateUAs database table
//======================================================================================================
class CvCityStateUAEntry : public CvBaseInfo
{
public:
	CvCityStateUAEntry(void);
	~CvCityStateUAEntry(void);

	bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

	int GetAllyEffectID() const;
	int GetFriendEffectID() const;

	// AI attraction tables (CityStateUA_ExtraFlavors / CityStateUA_VictoryFlavors). Both return 0
	// when the entry has no row for the given index.
	int GetExtraFlavorValue(int i) const;
	int GetVictoryFlavorValue(int i) const;

private:
	int m_iAllyEffectID;
	int m_iFriendEffectID;
	int* m_piExtraFlavorValue;
	int* m_piVictoryFlavorValue;
};

//======================================================================================================
// CvCityStateUAXMLEntries - container for all CityStateUAs entries
//======================================================================================================
class CvCityStateUAXMLEntries
{
public:
	CvCityStateUAXMLEntries(void);
	~CvCityStateUAXMLEntries(void);

	std::vector<CvCityStateUAEntry*>& GetUAEntries();
	int GetNumUAs() const;
	CvCityStateUAEntry* GetEntry(int index) const;
	// Linear lookup returning the row index (or -1). Only used at load time, by ResolveMinorCivUAIndexes.
	int GetEntryIndexByType(const char* szType) const;
	// Resolve every city-state's UAType to its row index once, after the CityStateUAs table loads, so
	// every CSUA query site reads CvMinorCivInfo::GetCityStateUAIndex() (O(1)) instead of re-running a
	// linear strcmp lookup on each call.
	void ResolveMinorCivUAIndexes();
	void DeleteArray();

private:
	std::vector<CvCityStateUAEntry*> m_paUAEntries;
};

//======================================================================================================
// CvPlayerCityStateUA - per-player cache of active city-state UA effects
//======================================================================================================
class CvPlayer;
class CvCity;
class CvPlayerCityStateUA
{
public:
	CvPlayerCityStateUA();
	~CvPlayerCityStateUA();

	void Init(CvPlayer* pPlayer);
	void Uninit();

	// Add an effect. iChange is always +1: dropping an effect is done by Reset() followed by a full
	// re-apply in CvPlayer::RefreshCSAllUAEffects, never by passing -1 here.
	void ApplyEffect(int iEffectID, int iChange);

	// Query accumulated modifier values for each effect type
	// Sofia (spy/coup UA)
	int GetCoupChanceModifier() const;
	bool GetCoupFailSpySurvives() const;
	int GetStealTechSpeedPerSpy() const;
	int GetSpyKillChancePerSpy() const;
	// Florence
	int GetFaithPurchaseGreatPeopleCostRiseModifier() const;
	int GetFaithPurchaseGreatPeopleCostRiseModifierPerGW() const;
	bool IsFaithPurchaseAllGreatPeople() const;
	int GetGreatPersonPoints(int i) const;
	// Buenos Aires
	bool IsGPNoDeathAfterGreatWork() const;
	int GetGPConcertTourismRetentionPercent() const;
	// Brussels
	int GetGreatMusicianConcertGoldPercent() const;
	// Bratislava
	int GetCapitalAndSecondCapitalCultureModifier() const;
	// Bucharest
	int GetImmigrationRatePerImmigrant() const;
	int GetImmigrationRateMax() const;
	int GetEmigrationRatePerImmigrant() const;
	int GetEmigrationRateMax() const;
	// Kuala Lumpur
	bool IsPuppetNoTechCostPenalty() const;
	int GetPuppetTechCostPartial() const;
	// Almaty
	bool IsCanPillageNeutralTradeRoute() const;
	int GetPlunderTradeRouteGold() const;
	int GetPlunderTradeRouteXP() const;
	int GetPlunderTradeRouteOpinionPenalty() const;
	bool HasKillMaxHpByPromotion() const;
	const std::vector<KillMaxHpByPromotionEntry>& GetKillMaxHpByPromotionEntries() const;
	// Belgrade
	int GetGarrisonCityDefenseModifier() const;
	int GetMilitaryUnitProductionXP() const;
	int GetZOCRangeBonus() const;
	// Budapest
	bool IsLandUnitsImmuneRiverCrossing() const;
	int GetWoundedFixedDamage() const;
	// Ha Noi
	int GetEnemyFixedDamageModifierInBorders() const;
	int GetCulturePerWarPeace() const;
	int GetEnemyCombatModifierInBordersPerBeenDoW() const;
	// Mbanza Kongo
	int GetUnitProductionModifierPerCity() const;
	int GetCombatBonusPerTechDifference() const;
	const std::vector<ResourcePerCityEntry>& GetResourcePerCityEntries() const;
	// Sidon
	int GetCityAttackIgnoreBuildingDefensePercent() const;
	int GetMilitaryXPPerTurnModifier() const;
	int GetMilitaryXPSeaAir() const;
	// Vatican
	int GetReligionSpreadSpeedModifier() const;
	int GetPapalRecognitionVotes() const;
	int GetPapalRecognitionAllyVotes() const;
	int GetHolyCityYieldModifierPerFollowingCity(YieldTypes eYieldType) const;
	// Jerusalem
	int GetReligiousPressureModifierPerHolyCity() const;
	bool IsDenounceImmunity() const;
	int GetCapitalYieldModifierPerFollowingCity(YieldTypes eYieldType) const;
	// Kyzyl
	int GetLandTradeRouteDistancePerTradeSlot() const;
	int GetTradeRouteGoldPercentNonNeighbor() const;
	// Dubai
	int GetHappinessPerGoldDonated() const;
	int GetGoldDonationInterval() const;
	int GetWonderProductionPerDonationHappiness() const;
	int GetIdeologyPressurePerDonationHappiness() const;
	// Genoa
	int GetGoldDonationInfluenceModifierPerSeaRoute() const;
	// Genoa / Vilnius: per-unit yield % modifiers, keyed by YieldType
	int GetFriendCityStateYieldModifier(YieldTypes eYieldType) const;
	int GetAllyCityStateYieldModifier(YieldTypes eYieldType) const;
	int GetPolicyYieldModifier(YieldTypes eYieldType) const;
	// Vilnius
	int GetGoldenAgeThresholdPerPopulation() const;
	// Malacca
	int GetLuxuryHappinessModifier() const;
	// Ragusa
	int GetLocalHappinessCapModifier() const;
	int GetFoodKeptModifierPerLuxury() const;
	int GetTradeRouteGoldModifierPerLuxuryType() const;
	// Monaco
	int GetGoldenAgeBuildingMaintenanceMod() const;
	bool HasGoldDonationGamble() const { return !m_vGoldDonationGamble.empty(); }
	const std::vector<GoldDonationGambleEntry>& GetGoldDonationGambleEntries() const { return m_vGoldDonationGamble; }
	// Yield-to-yield conversion (Colombo / Cape Town / Mogadishu / Monaco): true when any active effect
	// carries a conversion row. Read at the top of CvCity::GetYieldRateFromUCSConversion, a per-city
	// per-yield hot path whose city-state scan costs a linear string lookup each; players holding no
	// conversion effect at all (the common case) must not pay for it.
	bool HasUCSYieldConversion() const { return m_iUCSYieldConversionCount > 0; }
	// Panama
	int GetTradeRouteGoldModifierPerDistance() const;
	int GetUnhappinessReductionPerCrossContinentRoute() const;
	// Manila
	int GetTradeRouteGoldPercentInternational() const;
	int GetTradeRouteGoldModifierPerInternationalRoute() const;
	int GetFoodModifierPerHappyLuxuryType() const;
	int GetFoodModifierPerHappyLuxuryCap() const;
	// Mogadishu
	bool HasCityStateTradeRouteYieldModifiersGlobal() const;
	const std::vector<CityStateTradeRouteYieldModifierGlobalEntry>& GetCityStateTradeRouteYieldModifiersGlobal() const { return m_vCityStateTradeRouteYieldModifiersGlobal; }
	// Kabul: each international land trade route grants trade-route gold %, plus an extra bonus on a hills origin
	bool HasLandTradeRouteGoldModifiers() const;
	const std::vector<LandTradeRouteGoldModifierEntry>& GetLandTradeRouteGoldModifiers() const { return m_vLandTradeRouteGoldModifiers; }
	// Kabul: per international land trade route to any other player (city-states included), a nation-wide yield % per era
	bool HasInternationalLandTradeRouteYieldPerEra() const;
	const std::vector<InternationalLandTradeRouteYieldEntry>& GetInternationalLandTradeRouteYieldPerEra() const { return m_vInternationalLandTradeRouteYieldPerEra; }
	int GetResearchAgreementBreakBonusPercent() const;
	int GetSpecialistYieldFromBornGreatPerson(SpecialistTypes eSpecialist, YieldTypes eYield) const;
	int GetBuildingGreatPersonPointsForCity(const CvCity* pCity, SpecialistTypes eSpecialist) const;
	int GetAllyInfluenceModFromBornGreatPerson() const;
	// Prague / Yerevan: building-class yield percentage modifiers
	int GetBuildingClassYieldModifier(BuildingClassTypes eBuildingClass, YieldTypes eYieldType) const;
	bool HasBuildingClassYieldModifiers() const;
	// Brussels
	int GetSpecialistPointRate(SpecialistTypes eSpecialist) const;
	int GetGreatWorkGreatPersonPointsForCity(const CvCity* pCity, SpecialistTypes eSpecialist) const;
	bool HasGreatWorkGreatPersonPoints() const;
	int GetGreatPersonOneShotModifier(UnitClassTypes eUnitClass) const;
	// Valletta
	int GetEnemyCityNoHealBesiegeCount() const;
	const std::vector<PurchasedBuildingXPEntry>& GetPurchasedBuildingXPEntries() const;
	const std::vector<UnitBornYieldEntry>& GetUnitBornYieldEntries() const;
	const std::vector<UnitMaintenanceByPromotionEntry>& GetUnitMaintenanceByPromotionEntries() const;
	int GetSpyGarrisonYieldModifier(YieldTypes eYieldType) const;
	bool HasSpyGarrisonYieldModifiers() const;
	int GetSpyKillGainSpyProgress() const;
	int GetResourceYieldModifier(ResourceTypes eResource, YieldTypes eYield) const;
	bool HasResourceYieldModifiers() const;
	// Antananarivo
	int GetCoastalCityGrowthThresholdModifier() const;
	int GetDiplomaticPrestigePerCity() const;
	int GetImprovementYieldModifier(ImprovementTypes eImprovement, YieldTypes eYield) const;
	bool HasImprovementYieldModifiers() const;
	// Zanzibar
	int GetImprovementHappiness(ImprovementTypes eImprovement) const;
	bool HasImprovementHappiness() const;
	// Ragusa
	int GetBuildingClassHappiness(BuildingClassTypes eBuildingClass) const;
	bool HasBuildingClassHappiness() const;
	// Hormuz
	int GetTradeRouteGoldPerSurplusResource(ResourceTypes eResource) const;
	bool HasTradeRouteGoldPerSurplusResource() const;
	// Gangtok
	int GetHappinessPerFollowingCity() const;
	int GetFaithInfluencePurchaseCostDivisor() const;
	int GetFaithInfluencePurchasePerTurnLimit() const;
	bool HasFaithInfluencePurchase() const;
	// Wittenberg
	bool AnyFaithBeliefPurchase() const;
	int GetInquisitorRetentionPercent() const;
	bool IsInquisitorCanUseOnCityState() const;
	// La Venta
	bool AnyFaithPantheonPurchase() const;
	int GetGreatPersonRateModifierPerGreatWork() const;
	// Kathmandu
	int GetFaithRefundPerDonationPercent() const;
	// Geneva
	int GetDiplomaticPrestigePerMajorityCiv() const;
	int GetInfluencePerTurnPerFollowCityMod() const;
	int GetFollowingCityDivisor() const;
	// Vancouver: accumulated global happiness per coastal city (basis points, 100 = +1 happiness per coastal city)
	int GetCoastalCityHappiness() const;
	// Vancouver: accumulated per-happiness yield % modifier per YieldType (basis points, 100 = +1% per happiness)
	int GetHappinessYieldModifier(YieldTypes eYield) const;
	int GetHappinessYieldModifierCap(YieldTypes eYield) const;
	bool HasHappinessYieldModifiers() const;
	// Sydney
	int GetImmigrantYieldModifier(YieldTypes eYield) const;
	bool HasImmigrantYieldModifiers() const;
	int GetImmigrantCashPercent() const;
	int GetImmigrantCashCapBase() const;
	// Ife
	int GetFaithGPClassCostModifier(UnitClassTypes eUnitClass) const;
	int GetGreatWorkYieldModifier(GreatWorkClass eGreatWorkClass, YieldTypes eYield) const;
	bool HasGreatWorkYieldModifiers() const;
	int GetGoldenAgeYieldModifier(YieldTypes eYield) const;
	bool HasGoldenAgeYieldModifiers() const;
	const std::vector<GreatWorkYieldModifierEntry>& GetGreatWorkYieldModifierEntries() const;
	// Ife: return the cached count of great works of the given class (refreshed once per doTurn, not per-yield query)
	int GetCachedGreatWorkCount(GreatWorkClass eGreatWorkClass) const;
	// Ife: clear + repopulate the cached per-class great-work count from the player's cities (called in CvPlayer::RefreshCSAllUAEffects)
	void CacheGreatWorkCounts();

	// Yerevan: global happiness per worked holy-site improvement (100 = +1, no local cap)
	int GetHolySiteHappiness() const;
	const std::vector<LiteracyYieldModifierEntry>& GetLiteracyYieldModifiers() const { return m_vLiteracyYieldModifiers; }
	const std::vector<BornGreatPersonNationwideYieldEntry>& GetBornGreatPersonYieldModifiers() const { return m_vBornGreatPersonYieldModifiers; }
	const std::vector<AdjacentImprovementYieldChangeEntry>& GetAdjacentImprovementYieldChanges() const { return m_vAdjacentImprovementYieldChanges; }
	bool HasLiteracyYieldModifiers() const;
	bool HasBornGreatPersonYieldModifiers() const;
	bool HasAdjacentImprovementYieldChanges() const;
	// Yerevan: cached literacy percent points + worked holy-site count, refreshed once per doTurn
	// in CvPlayer::RefreshCSAllUAEffects so the hot paths read flat ints instead of re-scanning.
	int GetCachedLiteracyPercent() const;
	void ComputeLiteracyPercent();
	int GetCachedWorkedHolySites() const;
	void CacheWorkedHolySites();

	// Bogota: cities matching a special city type gain a yield % modifier; per owned matching city,
	// all cities gain a yield % modifier.
	const std::vector<SpecialCityYieldModifierEntry>& GetSpecialCityYieldModifiers() const { return m_vSpecialCityYieldModifiers; }
	bool HasSpecialCityYieldModifiers() const;
	const std::vector<SpecialCityCountYieldModifierEntry>& GetSpecialCityCountYieldModifiers() const { return m_vSpecialCityCountYieldModifiers; }
	bool HasSpecialCityCountYieldModifiers() const;
	// Bogota: per special city type, the IDs of owned cities matching it, refreshed once per doTurn in
	// CvPlayer::RefreshCSAllUAEffects. The hot path (CvCity::GetBaseYieldRateModifier) reads this table
	// instead of re-running the predicate, which scans every plot of the city for each yield and type.
	int GetCachedSpecialCityCount(int iSpecialCityType) const;
	bool IsCachedSpecialCityTypeMatch(int iCityID, int iSpecialCityType) const;
	void CacheSpecialCityMatches();

	// Kuala Lumpur: per N population living in cities matching a special city type, a nation-wide yield %
	// modifier per YieldType (YieldMod is a plain percent).
	const std::vector<SpecialCityPopulationYieldModifierEntry>& GetSpecialCityPopulationYieldModifiers() const { return m_vSpecialCityPopulationYieldModifiers; }
	bool HasSpecialCityPopulationYieldModifiers() const;
	// Kuala Lumpur: cached population living in cities matching each special city type, refreshed once
	// per doTurn by CacheSpecialCityMatches.
	int GetCachedSpecialCityPopulation(int iSpecialCityType) const;
	// Kuala Lumpur: cached puppet count, refreshed once per doTurn. Read by CvPlayerTechs::
	// GetResearchCost, which is a hot path, so the city scan must not run there. Kept out of
	// CacheSpecialCityMatches, which returns early when the player holds no special-city-type effects.
	int GetCachedPuppetCount() const;
	void CachePuppetStats();

	// Tyre: cities matching a special city type take Percent% less damage (Percent is a plain percent).
	// Evaluated live in CvCity::changeDamage, which is a low-frequency path, so no per-turn cache is needed.
	const std::vector<SpecialCityDamageReductionEntry>& GetSpecialCityDamageReductions() const { return m_vSpecialCityDamageReductions; }
	bool HasSpecialCityDamageReduction() const;

	// Singapore: each owned building class grants a nation-wide yield % modifier (YieldMod is a plain
	// percent). The count is read live from CvPlayer::getBuildingClassCount, so no per-turn cache is needed.
	const std::vector<BuildingClassGlobalYieldModifierEntry>& GetBuildingClassGlobalYieldModifiers() const { return m_vBuildingClassGlobalYieldModifiers; }
	bool HasBuildingClassGlobalYieldModifiers() const;
	// Singapore: each owned building class lowers the city-count research threshold by TechCostMod points
	// (a plain subtraction, not a percent of the threshold).
	const std::vector<BuildingClassTechCostModifierEntry>& GetBuildingClassTechCostModifiers() const { return m_vBuildingClassTechCostModifiers; }
	bool HasBuildingClassTechCostModifiers() const;

	// Manila: cached count of happy luxury types owned by the player, refreshed once per doTurn in
	// CvPlayer::RefreshCSAllUAEffects so the per-yield hot path (GetCSUAYieldPercentModifier) reads a
	// flat int instead of re-scanning every resource for every city.
	int GetCachedHappyLuxuryCount() const;
	void CacheHappyLuxuryCount();

	// Bucharest: each world wonder owned grants a yield % modifier per YieldType, nation-wide.
	const std::vector<WorldWonderYieldModifierEntry>& GetWorldWonderYieldModifiers() const { return m_vWorldWonderYieldModifiers; }
	bool HasWorldWonderYieldModifiers() const;
	// Bucharest: cached world-wonder count, refreshed once per doTurn in CvPlayer::RefreshCSAllUAEffects
	// so the per-yield hot path (GetCSUAYieldPercentModifier) reads a flat int.
	int GetCachedWorldWonderCount() const;
	void CacheWorldWonderCount();

	// Bucharest: each diplomat stationed in a foreign MAJOR civilization's city grants a yield % modifier
	// per YieldType, nation-wide (diplomats sent to city-states do not count).
	const std::vector<DiplomatAbroadYieldModifierEntry>& GetDiplomatAbroadYieldModifiers() const { return m_vDiplomatAbroadYieldModifiers; }
	bool HasDiplomatAbroadYieldModifiers() const;
	// Bucharest: cached count of diplomats stationed abroad, refreshed once per doTurn.
	int GetCachedDiplomatAbroadCount() const;
	void CacheDiplomatAbroadCount();

	// Quebec: when another civilization computes its culture-victory progress against the player, inflate the
	// player's lifetime culture by this plain percent (50 = +50%). Read by CvPlayerCulture.
	int GetCultureVictoryProgressModifier() const;
	// Quebec: for each met major civilization whose influence level toward the player is Unknown, a
	// nation-wide yield % modifier per YieldType (YieldMod is a plain percent, capped by Cap percent).
	const std::vector<UnknownInfluenceYieldModifierEntry>& GetUnknownInfluenceYieldModifiers() const { return m_vUnknownInfluenceYieldModifiers; }
	bool HasUnknownInfluenceYieldModifiers() const;
	// Quebec: cached count of met major civilizations at Unknown influence toward the player, refreshed once
	// per doTurn in CvPlayer::RefreshCSAllUAEffects so the per-yield hot path reads a flat int.
	int GetCachedUnknownInfluenceCount() const;
	void CacheUnknownInfluenceCount();

	// Kiev: +X% great-person rate per national wonder the player has completed, nation-wide
	// (GreatPersonRateModifierPerNationalWonder is a plain percent; ally 2 = +2% per national wonder).
	int GetGreatPersonRateModifierPerNationalWonder() const;
	bool HasNationalWonderGreatPersonModifier() const;
	// Kiev: cached national-wonder count, refreshed once per doTurn in CvPlayer::RefreshCSAllUAEffects.
	// CvCity::getGreatPeopleRateModifier is a per-city hot path, so the city scan must not run there.
	int GetCachedNationalWonderCount() const;
	void CacheNationalWonderCount();

	// Kiev: League delegate votes granted per civilization the player has a Declaration of Friendship with.
	int GetLeagueVotesPerDoF() const;
	bool HasLeagueVotesPerDoF() const;

	// Kiev: each League vote held grants a yield % modifier per YieldType, nation-wide
	// (YieldMod is basis points, 100 = +1% per vote).
	const std::vector<LeagueVoteYieldModifierEntry>& GetLeagueVoteYieldModifiers() const { return m_vLeagueVoteYieldModifiers; }
	bool HasLeagueVoteYieldModifiers() const;
	// Kiev: cached League vote count, refreshed once per doTurn. The league lookup and vote recomputation
	// are far too heavy for the per-yield hot path (GetCSUAYieldPercentModifier).
	int GetCachedLeagueVotes() const;
	void CacheLeagueVotes();

	// Ur: global happiness per world wonder owned by the ally/friend (100 = +1 happiness per world wonder).
	// Shares the world-wonder count cached for Bucharest (GetCachedWorldWonderCount).
	int GetWorldWonderHappiness() const;

	// Milan: each point of luxury happiness the player has grants a nation-wide yield % modifier per
	// YieldType (YieldMod is basis points per point of luxury happiness, 50 = +0.5% per point).
	const std::vector<LuxuryHappinessYieldModifierEntry>& GetLuxuryHappinessYieldModifiers() const { return m_vLuxuryHappinessYieldModifiers; }
	bool HasLuxuryHappinessYieldModifiers() const;
	// Milan: cached luxury happiness total, refreshed once per doTurn. GetCSUAYieldPercentModifier is a
	// per-yield hot path, so the scan is not run there.
	int GetCachedLuxuryHappiness() const;
	void CacheLuxuryHappiness();

	// Almaty: cache the surplus of each configured resource once per turn so CvUnit::GetMaxHitPoints
	// (an extremely hot path) does not scan every city on each call. The kill count is read live.
	void CacheKillMaxHpSurplus();

	// Milan: a unit takes Percent% less damage when the opposing side's team lacks the configured tech.
	// Evaluated live in the combat path (low frequency), so no per-turn cache is needed.
	const std::vector<CombatDamageReductionVsNoTechEntry>& GetCombatDamageReductionVsNoTech() const { return m_vCombatDamageReductionVsNoTech; }
	bool HasCombatDamageReductionVsNoTech() const;

	void Reset();

protected:
	CvPlayer* m_pPlayer;

	// Accumulated modifier values
	int m_iFaithPurchaseGreatPeopleCostRiseModifier;
	int m_iFaithPurchaseGreatPeopleCostRiseModifierPerGW;
	std::vector<int> m_aiGreatPersonPoints;
	int m_iFaithPurchaseAllGreatPeopleCount;
	int m_iGPNoDeathAfterGreatWorkCount;
	int m_iGPConcertTourismRetentionPercent;
	int m_iGreatMusicianConcertGoldPercent;
	int m_iCapitalAndSecondCapitalCultureModifier;
	int m_iImmigrationRatePerImmigrant;
	int m_iImmigrationRateMax;
	int m_iEmigrationRatePerImmigrant;
	int m_iEmigrationRateMax;
	int m_iPuppetNoTechCostPenaltyCount;
	int m_iPuppetTechCostPartial;
	int m_iCanPillageNeutralTradeRouteCount;
	int m_iPlunderTradeRouteGold;
	int m_iPlunderTradeRouteXP;
	int m_iPlunderTradeRouteOpinionPenalty;
	std::vector<KillMaxHpByPromotionEntry> m_vKillMaxHpByPromotion;
	int m_iGarrisonCityDefenseModifier;
	int m_iMilitaryUnitProductionXP;
	int m_iZOCRangeBonus;
	int m_iLandUnitsImmuneRiverCrossingCount;
	int m_iWoundedFixedDamage;
	int m_iEnemyFixedDamageModifierInBorders;
	int m_iCulturePerWarPeace;
	int m_iEnemyCombatModifierInBordersPerBeenDoW;
	int m_iUnitProductionModifierPerCity;
	int m_iCombatBonusPerTechDifference;
	std::vector<ResourcePerCityEntry> m_vResourcePerCity;
	int m_iCityAttackIgnoreBuildingDefensePercent;
	int m_iMilitaryXPPerTurnModifier;
	int m_iMilitaryXPSeaAir;
	// Sofia (spy/coup UA, main-table columns)
	int m_iCoupChanceModifier;
	int m_iCoupFailSpySurvives;
	int m_iStealTechSpeedPerSpy;
	int m_iSpyKillChancePerSpy;
	int m_iReligionSpreadSpeedModifier;
	int m_iPapalRecognitionVotes;
	int m_iPapalRecognitionAllyVotes;
	std::vector<int> m_aiHolyCityYieldModifierPerFollowingCity;
	int m_iReligiousPressureModifierPerHolyCity;
	int m_iDenounceImmunityCount;
	std::vector<int> m_aiCapitalYieldModifierPerFollowingCity;
	int m_iLandTradeRouteDistancePerTradeSlot;
	int m_iTradeRouteGoldPercentNonNeighbor;
	int m_iHappinessPerGoldDonated;
	int m_iGoldDonationInterval;
	int m_iWonderProductionPerDonationHappiness;
	int m_iIdeologyPressurePerDonationHappiness;
	// Genoa
	int m_iGoldDonationInfluenceModifierPerSeaRoute;
	// Genoa / Vilnius (per YieldType accumulated)
	std::vector<int> m_aiFriendCityStateYieldModifiers;
	std::vector<int> m_aiAllyCityStateYieldModifiers;
	std::vector<int> m_aiPolicyYieldModifiers;
	// Vilnius
	int m_iGoldenAgeThresholdPerPopulation;
	int m_iLuxuryHappinessModifier;
	int m_iFoodKeptModifierPerLuxury;
	int m_iTradeRouteGoldModifierPerLuxuryType;
	// Ragusa
	int m_iLocalHappinessCapModifier;
	// Monaco
	int m_iGoldenAgeBuildingMaintenanceMod;
	std::vector<GoldDonationGambleEntry> m_vGoldDonationGamble;
	// Colombo / Cape Town / Mogadishu / Monaco: count of active effects carrying a yield-to-yield
	// conversion. Maintained by ApplyEffect, read by HasUCSYieldConversion.
	int m_iUCSYieldConversionCount;
	int m_iTradeRouteGoldModifierPerDistance;
	int m_iUnhappinessReductionPerCrossContinentRoute;
	// Manila
	int m_iTradeRouteGoldPercentInternational;
	int m_iTradeRouteGoldModifierPerInternationalRoute;
	int m_iFoodModifierPerHappyLuxuryType;
	int m_iFoodModifierPerHappyLuxuryCap;
	// Mogadishu
	std::vector<CityStateTradeRouteYieldModifierGlobalEntry> m_vCityStateTradeRouteYieldModifiersGlobal;
	// Kabul
	std::vector<LandTradeRouteGoldModifierEntry> m_vLandTradeRouteGoldModifiers;
	std::vector<InternationalLandTradeRouteYieldEntry> m_vInternationalLandTradeRouteYieldPerEra;
	int m_iResearchAgreementBreakBonusPercent;
	// Prague / Yerevan
	int** m_ppiBuildingClassYieldModifiers;
	int m_iBuildingClassYieldModifierCount;
	// Brussels
	std::vector<int> m_aiSpecialistPointRate;
	std::vector<GreatWorkGreatPersonPointsEntry> m_vGreatWorkGreatPersonPoints;
	std::vector<int> m_aiGreatPersonOneShotModifier;
	std::vector<BornGreatPersonSpecialistYieldEntry> m_vBornGreatPersonSpecialistYield;
	std::vector<BuildingGreatPersonPointsEntry> m_vBuildingGPP;
	std::vector<BornGreatPersonAllyInfluenceModEntry> m_vBornAllyInfluenceMod;
	std::vector<UnitMaintenanceByPromotionEntry> m_vUnitMaintenanceByPromotion;
	// Valletta
	int m_iEnemyCityNoHealBesiegeCount;
	std::vector<PurchasedBuildingXPEntry> m_vPurchasedBuildingXP;
	std::vector<UnitBornYieldEntry> m_vUnitBornYield;
	// Prague
	std::vector<int> m_aiSpyGarrisonYieldModifiers;
	int m_iSpyGarrisonYieldModifierCount;
	int m_iSpyKillGainSpyProgress;
	// Melbourne
	int** m_ppiResourceYieldModifiers;
	int m_iResourceYieldModifierCount;
	// Antananarivo
	int m_iCoastalCityGrowthThresholdModifier;
	int m_iDiplomaticPrestigePerCity;
	int** m_ppiImprovementYieldModifiers;
	int m_iImprovementYieldModifierCount;
	// Zanzibar
	std::vector<int> m_aiImprovementHappiness;
	int m_iImprovementHappinessCount;
	// Ragusa
	std::vector<int> m_aiBuildingClassHappiness;
	int m_iBuildingClassHappinessCount;
	// Hormuz
	std::vector<int> m_aiTradeRouteGoldPerSurplusResource;
	int m_iTradeRouteGoldPerSurplusResourceCount;
	// Gangtok
	int m_iHappinessPerFollowingCity;
	int m_iFaithInfluencePurchaseCostDivisor;
	int m_iFaithInfluencePurchasePerTurnLimit;
	// Wittenberg
	int m_iFaithBeliefPurchaseCount;
	int m_iInquisitorRetentionPercent;
	int m_iInquisitorCanUseOnCityStateCount;
	// La Venta
	int m_iFaithPantheonPurchaseCount;
	int m_iGreatPersonRateModifierPerGreatWork;
	// Kathmandu
	int m_iFaithRefundPerDonationPercent;
	// Geneva
	int m_iDiplomaticPrestigePerMajorityCiv;
	int m_iInfluencePerTurnPerFollowCityMod;
	int m_iFollowingCityDivisor;
	// Sydney
	std::vector<int> m_aiImmigrantYieldModifiers;
	int m_iImmigrantCashPercent;
	int m_iImmigrantCashCapBase;
	// Vancouver
	int m_iCoastalCityHappiness;
	std::vector<int> m_aiHappinessYieldModifiers;
	std::vector<int> m_aiHappinessYieldModifierCaps;
	// Ife
	std::vector<int> m_aiFaithGPClassCostModifier;
	std::vector<GreatWorkYieldModifierEntry> m_vGreatWorkYieldModifiers;
	std::vector<int> m_aiGoldenAgeYieldModifiers;
	// Cached per-GreatWorkClass great-work count, refreshed once per doTurn (in CvPlayer::RefreshCSAllUAEffects)
	std::vector<int> m_aiCachedGreatWorkCount;
	// Yerevan
	int m_iHolySiteHappiness;
	std::vector<LiteracyYieldModifierEntry> m_vLiteracyYieldModifiers;
	std::vector<BornGreatPersonNationwideYieldEntry> m_vBornGreatPersonYieldModifiers;
	std::vector<AdjacentImprovementYieldChangeEntry> m_vAdjacentImprovementYieldChanges;
	int m_iCachedLiteracyPercent;
	int m_iCachedWorkedHolySites;
	// Bogota
	std::vector<SpecialCityYieldModifierEntry> m_vSpecialCityYieldModifiers;
	std::vector<SpecialCityCountYieldModifierEntry> m_vSpecialCityCountYieldModifiers;
	// Cached IDs of owned cities matching each special city type, indexed by special city type ID,
	// refreshed once per doTurn (in CvPlayer::RefreshCSAllUAEffects)
	std::vector< std::vector<int> > m_avCachedSpecialCityIDs;
	// Manila: cached happy-luxury type count, refreshed once per doTurn (in CvPlayer::RefreshCSAllUAEffects)
	int m_iCachedHappyLuxuryCount;
	// Bucharest
	std::vector<WorldWonderYieldModifierEntry> m_vWorldWonderYieldModifiers;
	std::vector<DiplomatAbroadYieldModifierEntry> m_vDiplomatAbroadYieldModifiers;
	// Cached counts for the two Bucharest effects above, refreshed once per doTurn
	int m_iCachedWorldWonderCount;
	int m_iCachedDiplomatAbroadCount;
	// Quebec
	int m_iCultureVictoryProgressModifier;
	std::vector<UnknownInfluenceYieldModifierEntry> m_vUnknownInfluenceYieldModifiers;
	// Quebec: cached Unknown-influence major-civ count, refreshed once per doTurn
	int m_iCachedUnknownInfluenceCount;
	// Kiev
	int m_iGreatPersonRateModifierPerNationalWonder;
	int m_iLeagueVotesPerDoF;
	std::vector<LeagueVoteYieldModifierEntry> m_vLeagueVoteYieldModifiers;
	// Cached counts for the two Kiev effects above, refreshed once per doTurn
	int m_iCachedNationalWonderCount;
	int m_iCachedLeagueVotes;
	// Ur: global happiness per world wonder owned by the ally/friend (100 = +1 happiness per world wonder)
	int m_iWorldWonderHappiness;
	// Kuala Lumpur
	std::vector<SpecialCityPopulationYieldModifierEntry> m_vSpecialCityPopulationYieldModifiers;
	// Cached population per special city type, plus the puppet count, refreshed once per doTurn
	// (in CvPlayer::RefreshCSAllUAEffects)
	std::vector<int> m_aiCachedSpecialCityPopulation;
	int m_iCachedPuppetCount;
	// Tyre
	std::vector<SpecialCityDamageReductionEntry> m_vSpecialCityDamageReductions;
	// Singapore
	std::vector<BuildingClassGlobalYieldModifierEntry> m_vBuildingClassGlobalYieldModifiers;
	std::vector<BuildingClassTechCostModifierEntry> m_vBuildingClassTechCostModifiers;
	// Milan
	std::vector<LuxuryHappinessYieldModifierEntry> m_vLuxuryHappinessYieldModifiers;
	// Milan: cached luxury happiness total, refreshed once per doTurn (in CvPlayer::RefreshCSAllUAEffects)
	int m_iCachedLuxuryHappiness;
	std::vector<CombatDamageReductionVsNoTechEntry> m_vCombatDamageReductionVsNoTech;
};

#endif // CVCITYSTATEUACLASSES_H
