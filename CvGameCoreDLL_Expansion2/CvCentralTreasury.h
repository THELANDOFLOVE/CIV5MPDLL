/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CV_CENTRAL_TREASURY_H
#define CV_CENTRAL_TREASURY_H

// Tiandao Central Treasury
// Manages national-level resource allocation across provinces
// Rich provinces automatically support poor provinces

class CvPlayer;

class CvCentralTreasury
{
public:
	CvCentralTreasury(void);
	~CvCentralTreasury(void);

	void Init(CvPlayer* pPlayer);
	void Uninit(void);

	// Main entry point - called each turn
	void DoTurn(void);

private:
	CvPlayer* m_pPlayer;

	// Allocate budgets to each province based on needs
	void AllocateBudgets(void);

	// Handle emergency transfers between provinces
	void HandleEmergencyTransfers(void);

	// Get province priority modifier in percent (war, border, etc.)
	int GetProvincePriorityModifier(int iProv) const;
};

#endif // CV_CENTRAL_TREASURY_H
