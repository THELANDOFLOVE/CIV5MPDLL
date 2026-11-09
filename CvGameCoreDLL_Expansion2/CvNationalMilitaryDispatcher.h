/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CV_NATIONAL_MILITARY_DISPATCHER_H
#define CV_NATIONAL_MILITARY_DISPATCHER_H

// Tiandao National Military Dispatcher
// Unified command system for all garrison units across provinces
// Units are no longer bound by province borders - they serve the nation

class CvPlayer;
class CvUnit;
class CvPlot;

// Task types for garrison units
enum ProvinceTaskType
{
	PROVINCE_TASK_NONE = 0,
	PROVINCE_TASK_DEFEND = 1,      // Defending against enemy invasion (highest priority)
	PROVINCE_TASK_RAID = 2,        // Raiding barbarians (medium priority)
	PROVINCE_TASK_PATROL = 3,      // Routine patrol (low priority)
	PROVINCE_TASK_PLAYER = 4       // Player direct control (never auto-assigned)
};

// Military task descriptor
struct MilitaryTask
{
	int iType;              // ProvinceTaskType
	int iPriority;          // 1-10, higher = more urgent
	int iProvince;          // Related province (-1 = national level)
	CvPlot* pTarget;        // Target location
	int iUnitsNeeded;       // How many units needed
	int iThreat;            // Threat metric for the report log (enemy count / priority)
	int iX;                 // Target X coordinate
	int iY;                 // Target Y coordinate

	MilitaryTask() : iType(0), iPriority(0), iProvince(-1), pTarget(NULL), iUnitsNeeded(0), iThreat(0), iX(-1), iY(-1) {}
};

class CvNationalMilitaryDispatcher
{
public:
	CvNationalMilitaryDispatcher(void);
	~CvNationalMilitaryDispatcher(void);

	void Init(CvPlayer* pPlayer);
	void Uninit(void);

	// Main entry point - called each turn
	void DoTurn(void);

	// Collect all idle garrison units (no active missions)
	void CollectIdleGarrison(void);

	// Evaluate threats across all provinces
	void EvaluateNationalThreats(void);

	// Dispatch units to tasks by priority
	void DispatchByPriority(void);

	// Send idle garrisons back toward their home province when far away
	void ReturnIdleGarrison(void);

	// Clear task tags of units whose queued orders have finished
	void ReleaseFinishedTasks(void);

	// Find nearest available unit to a target
	CvUnit* FindNearestUnit(const std::vector<CvUnit*>& vUnits, CvPlot* pTarget) const;
	CvUnit* FindNearestUnit(const std::vector<CvUnit*>& vUnits, int iX, int iY) const;

	// Assign a unit to a task
	void AssignUnitToTask(CvUnit* pUnit, const MilitaryTask& task);

	// Check if unit is available for assignment
	bool IsUnitAvailable(CvUnit* pUnit) const;

	// Cancel unit's current task and return it to idle pool
	void CancelUnitTask(CvUnit* pUnit);

	// Get/Set unit task data
	int GetUnitCurrentTask(CvUnit* pUnit) const;
	void SetUnitCurrentTask(CvUnit* pUnit, int iTask);
	int GetUnitTaskProvince(CvUnit* pUnit) const;
	void SetUnitTaskProvince(CvUnit* pUnit, int iProv);
	int GetUnitTaskPriority(CvUnit* pUnit) const;
	void SetUnitTaskPriority(CvUnit* pUnit, int iPriority);

private:
	CvPlayer* m_pPlayer;
	std::vector<CvUnit*> m_vIdleUnits;
	std::vector<MilitaryTask> m_vTasks;

	// Helper: evaluate enemy threats in a province
	void EvaluateEnemyThreats(int iProv);

	// Helper: evaluate barbarian threats in a province
	void EvaluateBarbarianThreats(int iProv);

	// Helper: sort tasks by priority
	static bool TaskPriorityComparator(const MilitaryTask& a, const MilitaryTask& b);
};

#endif // CV_NATIONAL_MILITARY_DISPATCHER_H
