/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */

#include "CvGameCoreDLLPCH.h"
#include "CvCentralTreasury.h"
#include "CvPlayer.h"
#include "CvCity.h"
#include "CvGameCoreUtils.h"

#include "LintFree.h"

// Missing or non-positive values disable the related treasury behaviour
// instead of falling back to a number baked into the code.
static int SuzerainTreasuryDefine(const char* szName)
{
	return GC.getDefineINT(szName, false);
}

CvCentralTreasury::CvCentralTreasury(void)
	: m_pPlayer(NULL)
{
}

CvCentralTreasury::~CvCentralTreasury(void)
{
	Uninit();
}

void CvCentralTreasury::Init(CvPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

void CvCentralTreasury::Uninit(void)
{
	m_pPlayer = NULL;
}

// Main entry point - called each turn
void CvCentralTreasury::DoTurn(void)
{
	if (m_pPlayer == NULL) return;
	if (!m_pPlayer->isAlive()) return;
	if (!MOD_GLOBAL_SUZERAIN) return;

	// Step 1: Collect revenue (already handled by DoProvinceTreasuryTurn)
	// Step 2: Allocate budgets based on needs
	AllocateBudgets();

	// Step 3: Handle emergency transfers between provinces
	HandleEmergencyTransfers();
}

// Allocate budgets to each province based on needs
void CvCentralTreasury::AllocateBudgets(void)
{
	if (m_pPlayer == NULL) return;

	int iProvinceCount = m_pPlayer->GetProvinceCount();
	if (iProvinceCount <= 0) return;

	// Calculate total cities (excluding capital province)
	int iTotalCities = 0;
	for (int iProv = 0; iProv < iProvinceCount; iProv++)
	{
		if (m_pPlayer->IsCapitalProvince(iProv)) continue;
		iTotalCities += m_pPlayer->GetProvinceCityCount(iProv);
	}

	if (iTotalCities <= 0) return;

	// Total national treasury (player's gold)
	int iTotalRevenue = m_pPlayer->GetTreasury()->GetGold();

	// Each province gets budget proportional to its needs
	for (int iProv = 0; iProv < iProvinceCount; iProv++)
	{
		if (m_pPlayer->IsCapitalProvince(iProv)) continue;

		int iBaseBudget = CalculateBaseBudget(iProv, iTotalRevenue, iTotalCities);
		int iModifierPercent = GetProvincePriorityModifier(iProv);
		int iFinalBudget = iBaseBudget * iModifierPercent / 100;

		// Budget is informational - provinces accumulate their own treasury
		// This system ensures rich provinces help poor ones through emergency transfers
	}
}

// Calculate base budget for a province
int CvCentralTreasury::CalculateBaseBudget(int iProv, int iTotalRevenue, int iTotalCities)
{
	if (m_pPlayer == NULL) return 0;

	int iCities = m_pPlayer->GetProvinceCityCount(iProv);
	if (iCities <= 0 || iTotalCities <= 0) return 0;

	// Base budget: proportional to city count. The share of the national
	// treasury available for distribution comes from SQL.
	int iDistributePercent = SuzerainTreasuryDefine("SUZERAIN_TREASURY_DISTRIBUTE_PERCENT");
	if (iDistributePercent <= 0) return 0; // distribution not configured
	int iDistributable = iTotalRevenue * iDistributePercent / 100;
	return (iDistributable * iCities) / iTotalCities;
}

// Get province priority modifier in percent (war, border, etc.)
int CvCentralTreasury::GetProvincePriorityModifier(int iProv) const
{
	if (m_pPlayer == NULL) return 100;

	int iPercent = 100;

	// War provinces need more resources (bonus only when configured)
	int iWarBonus = SuzerainTreasuryDefine("SUZERAIN_TREASURY_WAR_BONUS_PERCENT");
	if (iWarBonus > 0 && m_pPlayer->GetProvinceWar(iProv) > 0)
	{
		iPercent = iPercent * iWarBonus / 100;
	}

	// Border provinces need defensive preparations. A missing radius simply
	// turns the border check off.
	int iBorderRadius = SuzerainTreasuryDefine("SUZERAIN_TREASURY_BORDER_RADIUS");
	bool bBorder = false;
	if (iBorderRadius > 0)
	{
		int iCityLoop = 0;
		for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
		{
			if (m_pPlayer->GetCityProvince(pCity->GetID()) != iProv)
				continue;

			for (int iRange = 1; iRange <= iBorderRadius && !bBorder; iRange++)
			{
				for (int dx = -iRange; dx <= iRange && !bBorder; dx++)
				{
					for (int dy = -iRange; dy <= iRange; dy++)
					{
						CvPlot* pPlot = GC.getMap().plot(pCity->getX() + dx, pCity->getY() + dy);
						if (pPlot != NULL && pPlot->getOwner() != NO_PLAYER && pPlot->getOwner() != m_pPlayer->GetID())
						{
							bBorder = true;
							break;
						}
					}
				}
			}
		}
	}

	int iBorderBonus = SuzerainTreasuryDefine("SUZERAIN_TREASURY_BORDER_BONUS_PERCENT");
	if (bBorder && iBorderBonus > 0)
	{
		iPercent = iPercent * iBorderBonus / 100;
	}

	return iPercent;
}

// Handle emergency transfers between provinces
void CvCentralTreasury::HandleEmergencyTransfers(void)
{
	if (m_pPlayer == NULL) return;

	int iProvinceCount = m_pPlayer->GetProvinceCount();
	if (iProvinceCount <= 1) return;

	int iRichPercent = SuzerainTreasuryDefine("SUZERAIN_TREASURY_RICH_PERCENT");
	int iPoorPercent = SuzerainTreasuryDefine("SUZERAIN_TREASURY_POOR_PERCENT");
	if (iRichPercent <= 0 || iPoorPercent <= 0)
		return; // transfer thresholds not configured

	// Build list of rich and poor provinces
	std::vector<int> vRichProvinces;
	std::vector<int> vPoorProvinces;

	for (int iProv = 0; iProv < iProvinceCount; iProv++)
	{
		if (m_pPlayer->IsCapitalProvince(iProv)) continue;

		int iTreasury = m_pPlayer->GetProvinceTreasury(iProv);
		int iCap = m_pPlayer->GetProvinceTreasuryCap(iProv);

		// Rich: above the rich threshold; poor: below the poor threshold
		if (iTreasury > (iCap * iRichPercent / 100))
		{
			vRichProvinces.push_back(iProv);
		}
		else if (iTreasury < (iCap * iPoorPercent / 100))
		{
			vPoorProvinces.push_back(iProv);
		}
	}

	// Transfer from rich to poor
	for (size_t i = 0; i < vRichProvinces.size() && !vPoorProvinces.empty(); i++)
	{
		int iFrom = vRichProvinces[i];
		int iFromTreasury = m_pPlayer->GetProvinceTreasury(iFrom);
		int iFromCap = m_pPlayer->GetProvinceTreasuryCap(iFrom);
		int iSurplus = iFromTreasury - iFromCap;

		if (iSurplus <= 0) continue;

		// Find poorest province
		int iBestTo = -1;
		int iBestDeficit = 0;
		for (size_t j = 0; j < vPoorProvinces.size(); j++)
		{
			int iTo = vPoorProvinces[j];
			int iToTreasury = m_pPlayer->GetProvinceTreasury(iTo);
			int iToCap = m_pPlayer->GetProvinceTreasuryCap(iTo);
			int iDeficit = iToCap - iToTreasury;

			if (iDeficit > iBestDeficit)
			{
				iBestDeficit = iDeficit;
				iBestTo = iTo;
			}
		}

		int iTransferPercent = SuzerainTreasuryDefine("SUZERAIN_TREASURY_TRANSFER_PERCENT");
		if (iTransferPercent <= 0) continue; // transfers not configured

		if (iBestTo >= 0 && iBestDeficit > 0)
		{
			// Transfer amount: a configured share of the surplus, capped by the deficit
			int iTransfer = std::min(iSurplus * iTransferPercent / 100, iBestDeficit);

			if (iTransfer > 0)
			{
				m_pPlayer->ChangeProvinceTreasury(iFrom, -iTransfer);
				m_pPlayer->ChangeProvinceTreasury(iBestTo, iTransfer);

				// Add report to both provinces; transfer types are distinct
				// from raid/buy so the report UI does not misread them.
				m_pPlayer->AddProvinceReport(iFrom,
					CvPlayer::SuzerainReportType("SUZERAIN_REPORT_TRANSFER_OUT", 8), iBestTo, iTransfer);
				m_pPlayer->AddProvinceReport(iBestTo,
					CvPlayer::SuzerainReportType("SUZERAIN_REPORT_TRANSFER_IN", 9), iFrom, iTransfer);

				// Remove from poor list if now satisfied
				int iNewBalance = m_pPlayer->GetProvinceTreasury(iBestTo);
				int iNewCap = m_pPlayer->GetProvinceTreasuryCap(iBestTo);
				if (iNewBalance >= (iNewCap * iPoorPercent / 100))
				{
					vPoorProvinces.erase(std::remove(vPoorProvinces.begin(), vPoorProvinces.end(), iBestTo), vPoorProvinces.end());
				}
			}
		}
	}
}
