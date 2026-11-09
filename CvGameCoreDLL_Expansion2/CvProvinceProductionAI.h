/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CV_PROVINCE_PRODUCTION_AI_H
#define CV_PROVINCE_PRODUCTION_AI_H

// Tiandao Province Production AI
// Smart production choices for autonomous provinces
// Considers: national strategy, province characteristics, official wisdom

class CvPlayer;
class CvCity;

// National strategy types
enum NationalStrategyType
{
	NATIONAL_STRATEGY_BALANCED = 0,
	NATIONAL_STRATEGY_MILITARY = 1,
	NATIONAL_STRATEGY_ECONOMIC = 2,
	NATIONAL_STRATEGY_CULTURAL = 3,
	NATIONAL_STRATEGY_SCIENTIFIC = 4
};

class CvProvinceProductionAI
{
public:
	CvProvinceProductionAI(void);
	~CvProvinceProductionAI(void);

	void Init(CvPlayer* pPlayer);
	void Uninit(void);

	// Main entry point - choose what this city should produce
	void ChooseProduction(int iProv, CvCity* pCity);

private:
	CvPlayer* m_pPlayer;

	// Helper: does province need more workers?
	bool NeedWorkers(int iProv, CvCity* pCity);

	// Helper: does province need more garrison units?
	bool NeedGarrison(int iProv, CvCity* pCity);

	// Helper: is this a border province?
	bool IsProvinceBorder(int iProv) const;

	// Helper: is province at war?
	bool IsProvinceAtWar(int iProv) const;

	// Helper: choose building for long-term development (high wisdom)
	void ChooseLongTermBuilding(int iProv, CvCity* pCity);

	// Helper: choose quick-return production (low wisdom)
	void ChooseQuickReturns(int iProv, CvCity* pCity);

	// Helper: select best military unit for current tech level
	UnitTypes SelectBestMilitaryUnit(CvCity* pCity, bool bDefensive);

	// Helper: select best economic building
	BuildingTypes SelectEconomicBuilding(CvCity* pCity);

	// Helper: does city need basic infrastructure?
	bool NeedBasicInfrastructure(CvCity* pCity);
};

#endif // CV_PROVINCE_PRODUCTION_AI_H
