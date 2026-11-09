/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CV_PROVINCE_CHRONICLE_H
#define CV_PROVINCE_CHRONICLE_H

// Tiandao Province Chronicle System
// Historical record compilation: periodically compile raw event reports into era summaries

class CvPlayer;

// Event importance levels
enum EventImportance
{
	EVENT_TRIVIAL = 0,     // Trivial (daily routine, not recorded)
	EVENT_MINOR = 1,       // Minor (official appointment, building completion)
	EVENT_NORMAL = 2,      // Normal (barbarian camp destroyed, garrison reinforcement)
	EVENT_IMPORTANT = 3,   // Important (war declared, major construction)
	EVENT_CRITICAL = 4     // Critical (province crisis, major victory)
};

// Flat chronicle storage layout: one entry occupies CHRONICLE_STRIDE ints.
// The storage itself lives in CvPlayer as a serialized vector so the
// compiled history survives save/load.
enum ChronicleLayout
{
	CHRONICLE_FIELD_START_TURN = 0,
	CHRONICLE_FIELD_END_TURN = 1,
	CHRONICLE_FIELD_PROVINCE = 2,
	CHRONICLE_FIELD_ERA = 3,
	CHRONICLE_FIELD_BARBARIANS = 4,
	CHRONICLE_FIELD_WARS = 5,
	CHRONICLE_FIELD_OFFICIALS = 6,
	CHRONICLE_FIELD_BUILDINGS = 7,
	CHRONICLE_FIELD_UNITS = 8,
	CHRONICLE_FIELD_GOLD_START = 9,
	CHRONICLE_FIELD_GOLD_END = 10,
	CHRONICLE_FIELD_POP_START = 11,
	CHRONICLE_FIELD_POP_END = 12,
	CHRONICLE_FIELD_EVENT_COUNT = 13,
	CHRONICLE_FIELD_EVENTS = 14,   // 5 events x 5 ints
	CHRONICLE_MAX_EVENTS = 5,
	CHRONICLE_EVENT_STRIDE = 5,
	CHRONICLE_STRIDE = CHRONICLE_FIELD_EVENTS + CHRONICLE_MAX_EVENTS * CHRONICLE_EVENT_STRIDE
};

// Chronicle entry (statistical summary of 50 turns)
struct ChronicleEntry
{
	int iStartTurn;           // Start turn
	int iEndTurn;             // End turn
	int iProvince;            // Province ID
	int iEra;                 // Era

	// Statistical data
	int iBarbariansKilled;    // Barbarians eliminated
	int iWarsEngaged;         // Wars participated
	int iOfficialsAppointed;  // Officials appointed
	int iBuildingsBuilt;      // Buildings constructed
	int iUnitsProduced;       // Units trained
	int iGoldStart;           // Gold at start
	int iGoldEnd;             // Gold at end
	int iPopulationStart;     // Population at start
	int iPopulationEnd;       // Population at end

	// Major events (up to 5 recorded)
	std::vector<int> vMajorEvents; // Each event: 5 ints (turn, type, a, b, importance)

	ChronicleEntry()
		: iStartTurn(0), iEndTurn(0), iProvince(-1), iEra(0)
		, iBarbariansKilled(0), iWarsEngaged(0), iOfficialsAppointed(0)
		, iBuildingsBuilt(0), iUnitsProduced(0)
		, iGoldStart(0), iGoldEnd(0), iPopulationStart(0), iPopulationEnd(0)
	{
	}
};

class CvProvinceChronicle
{
public:
	CvProvinceChronicle(void);
	~CvProvinceChronicle(void);

	void Init(CvPlayer* pPlayer);
	void Uninit(void);

	// Main entry point: called each turn, checks if compilation is needed
	void DoTurn(void);

	// Compile history: convert raw reports to summaries
	void CompileChronicle(void);

	// Query raw reports (for viewing details)
	std::vector<int> GetRawReports(int iStartTurn, int iEndTurn, int iProvince) const;

private:
	CvPlayer* m_pPlayer;

	// Compilation interval (turns)
	int m_iCompileInterval;

	// Last compilation turn
	int m_iLastCompileTurn;

	// Era seen on the previous turn (per player, not a shared static)
	int m_iLastEra;

	// Helper: collect event statistics for given period
	void CollectEventStatistics(int iStartTurn, int iEndTurn, int iProvince, ChronicleEntry& entry);

	// Helper: filter major events
	void FilterMajorEvents(int iStartTurn, int iEndTurn, int iProvince, ChronicleEntry& entry);

	// Helper: get current era
	int GetCurrentEra() const;
};

#endif // CV_PROVINCE_CHRONICLE_H
