/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.
	All other marks and trademarks are the property of their respective owners.
	All rights reserved.
	------------------------------------------------------------------------------------------------------- */

#include "CvGameCoreDLLPCH.h"
#include "CvNationalMilitaryDispatcher.h"
#include "CvPlayer.h"
#include "CvUnit.h"
#include "CvPlot.h"
#include "CvMap.h"
#include "CvGameCoreUtils.h"
#include "CvTypes.h"

// include this last to avoid macro conflicts
#include "LintFree.h"

//=====================================
// CvNationalMilitaryDispatcher
//=====================================

CvNationalMilitaryDispatcher::CvNationalMilitaryDispatcher(void)
	: m_pPlayer(NULL)
{
}

CvNationalMilitaryDispatcher::~CvNationalMilitaryDispatcher(void)
{
	Uninit();
}

void CvNationalMilitaryDispatcher::Init(CvPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
	m_vIdleUnits.clear();
	m_vTasks.clear();
}

void CvNationalMilitaryDispatcher::Uninit(void)
{
	m_vIdleUnits.clear();
	m_vTasks.clear();
	m_pPlayer = NULL;
}

// Main entry point - called each turn in CvPlayer::DoProvinceTurn()
void CvNationalMilitaryDispatcher::DoTurn(void)
{
	if (m_pPlayer == NULL) return;
	if (!m_pPlayer->isAlive()) return;
	if (!MOD_GLOBAL_SUZERAIN) return;

	// Step 0: units that finished their queued orders are no longer busy
	ReleaseFinishedTasks();

	// Step 1: Collect all idle garrison units
	CollectIdleGarrison();

	// Step 2: Evaluate threats across all provinces
	EvaluateNationalThreats();

	// Step 3: Dispatch units by priority
	DispatchByPriority();

	// Step 4: Idle units drift back to their home province
	ReturnIdleGarrison();
}

// Collect all idle garrison units (no active missions)
void CvNationalMilitaryDispatcher::CollectIdleGarrison(void)
{
	m_vIdleUnits.clear();

	int iLoop = 0;
	for (CvUnit* pUnit = m_pPlayer->firstUnit(&iLoop); pUnit != NULL; pUnit = m_pPlayer->nextUnit(&iLoop))
	{
		if (!IsUnitAvailable(pUnit))
			continue;

		m_vIdleUnits.push_back(pUnit);
	}
}

// Check if unit is available for assignment
bool CvNationalMilitaryDispatcher::IsUnitAvailable(CvUnit* pUnit) const
{
	if (pUnit == NULL) return false;
	if (pUnit->IsDead()) return false;
	if (!pUnit->IsCombatUnit()) return false;
	if (pUnit->getDomainType() != DOMAIN_LAND) return false;
	if (!pUnit->canMove()) return false;

	// Don't touch automated units
	if (pUnit->IsAutomated()) return false;

	// Don't interrupt player-controlled tasks
	int iCurrentTask = GetUnitCurrentTask(pUnit);
	if (iCurrentTask == PROVINCE_TASK_PLAYER) return false;

	// Don't interrupt units with mission queues (they're already doing something)
	if (pUnit->GetLengthMissionQueue() > 0) return false;

	return true;
}

// Evaluate threats across all provinces
void CvNationalMilitaryDispatcher::EvaluateNationalThreats(void)
{
	m_vTasks.clear();

	int iProvinceCount = m_pPlayer->GetProvinceCount();
	for (int iProv = 0; iProv < iProvinceCount; iProv++)
	{
		// Evaluate enemy threats (highest priority)
		EvaluateEnemyThreats(iProv);

		// Evaluate barbarian threats (medium priority)
		EvaluateBarbarianThreats(iProv);
	}

	// Sort tasks by priority (highest first)
	std::sort(m_vTasks.begin(), m_vTasks.end(), TaskPriorityComparator);
}

// Helper: evaluate enemy threats in a province
void CvNationalMilitaryDispatcher::EvaluateEnemyThreats(int iProv)
{
	if (m_pPlayer->IsCapitalProvince(iProv)) return;
	if (m_pPlayer->GetProvinceAutonomy(iProv) < 1) return;

	// Check if province is in war state
	if (m_pPlayer->GetProvinceWar(iProv) == 0) return;

	int iDefenseRadius = GC.getDefineINT("SUZERAIN_PROVINCE_DEFENSE_RADIUS", false);
	if (iDefenseRadius <= 0) return;

	// Find province cities
	int iCityLoop = 0;
	for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
	{
		if (m_pPlayer->GetCityProvince(pCity->GetID()) != iProv)
			continue;

		int iCityX = pCity->getX();
		int iCityY = pCity->getY();

		// Scan for enemy units near this city
		for (int dx = -iDefenseRadius; dx <= iDefenseRadius; dx++)
		{
			for (int dy = -iDefenseRadius; dy <= iDefenseRadius; dy++)
			{
				CvPlot* pPlot = GC.getMap().plot(iCityX + dx, iCityY + dy);
				if (pPlot == NULL) continue;

				// Check for enemy units
				CvUnit* pEnemyUnit = pPlot->getVisibleEnemyDefender(m_pPlayer->GetID());
				if (pEnemyUnit != NULL)
				{
					// Calculate units needed based on enemy strength (SQL tunable)
					int iMinUnits = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_MIN_UNITS", false);
					if (iMinUnits <= 0) iMinUnits = 3;
					int iPerEnemy = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_UNITS_PER_ENEMY", false);
					if (iPerEnemy <= 0) iPerEnemy = 2;

					int iEnemyCount = pPlot->getNumUnits();
					int iUnitsNeeded = std::max(iMinUnits, iEnemyCount * iPerEnemy);

					int iPriority = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_PRIORITY", false);
					if (iPriority <= 0) iPriority = 10;

					MilitaryTask task;
					task.iType = PROVINCE_TASK_DEFEND;
					task.iPriority = iPriority;
					task.iProvince = iProv;
					task.pTarget = pPlot;
					task.iX = pPlot->getX();
					task.iY = pPlot->getY();
					task.iUnitsNeeded = iUnitsNeeded;
					task.iThreat = iEnemyCount;

					m_vTasks.push_back(task);
					return;  // One defense task per province per turn
				}
			}
		}
	}
}

// Helper: evaluate barbarian threats in a province
void CvNationalMilitaryDispatcher::EvaluateBarbarianThreats(int iProv)
{
	if (m_pPlayer->IsCapitalProvince(iProv)) return;
	if (m_pPlayer->GetProvinceAutonomy(iProv) < 1) return;

	// Check if province has an active raid mission
	int iMissionType = m_pPlayer->GetProvinceMissionField(iProv, 0);
	if (iMissionType != 1) return;  // Not a raid mission

	int iState = m_pPlayer->GetProvinceMissionField(iProv, 4);
	if (iState == 2 || iState == 3) return;  // Already complete or failed

	int iX = m_pPlayer->GetProvinceMissionField(iProv, 1);
	int iY = m_pPlayer->GetProvinceMissionField(iProv, 2);
	int iRadius = GC.getDefineINT("SUZERAIN_PROVINCE_RAID_RADIUS", false);
	if (iRadius <= 0) return;

	// Find nearest barbarian unit or camp
	CvPlot* pTarget = NULL;
	int iBestDistance = INT_MAX;

	CvPlayer& kBarb = GET_PLAYER(BARBARIAN_PLAYER);
	int iBarbLoop = 0;
	for (CvUnit* pBarbUnit = kBarb.firstUnit(&iBarbLoop); pBarbUnit != NULL; pBarbUnit = kBarb.nextUnit(&iBarbLoop))
	{
		if (pBarbUnit->IsDead()) continue;
		int iDistance = plotDistance(iX, iY, pBarbUnit->getX(), pBarbUnit->getY());
		if (iDistance <= iRadius && iDistance < iBestDistance)
		{
			iBestDistance = iDistance;
			pTarget = pBarbUnit->plot();
		}
	}

	// If no barbarian units, look for camps
	if (pTarget == NULL)
	{
		for (int dx = -iRadius; dx <= iRadius && pTarget == NULL; dx++)
		{
			for (int dy = -iRadius; dy <= iRadius; dy++)
			{
				CvPlot* pPlot = GC.getMap().plot(iX + dx, iY + dy);
				if (pPlot == NULL) continue;
				if (pPlot->HasBarbarianCamp())
				{
					pTarget = pPlot;
					break;
				}
			}
		}
	}

	if (pTarget != NULL)
	{
		int iMinUnits = GC.getDefineINT("SUZERAIN_PROVINCE_RAID_MIN_UNITS", false);
		if (iMinUnits <= 0) iMinUnits = 5;

		int iPriority = GC.getDefineINT("SUZERAIN_DISPATCH_RAID_PRIORITY", false);
		if (iPriority <= 0) iPriority = 5;

		MilitaryTask task;
		task.iType = PROVINCE_TASK_RAID;
		task.iPriority = iPriority;
		task.iProvince = iProv;
		task.pTarget = pTarget;
		task.iX = pTarget->getX();
		task.iY = pTarget->getY();
		task.iUnitsNeeded = iMinUnits;
		task.iThreat = iPriority;

		m_vTasks.push_back(task);
	}
	else
	{
		// No barbarians found - mission complete
		m_pPlayer->SetProvinceMissionState(iProv, 2);
		m_pPlayer->AddProvinceReport(iProv,
			CvPlayer::SuzerainReportType("SUZERAIN_REPORT_RAID_DONE", 5), 0, 0);
	}
}

// Dispatch units to tasks by priority
void CvNationalMilitaryDispatcher::DispatchByPriority(void)
{
	for (size_t i = 0; i < m_vTasks.size(); i++)
	{
		const MilitaryTask& task = m_vTasks[i];

		// Assign nearest units to this task
		std::vector<CvUnit*> vAssigned;
		for (int j = 0; j < task.iUnitsNeeded && !m_vIdleUnits.empty(); j++)
		{
			CvUnit* pBest = FindNearestUnit(m_vIdleUnits, task.iX, task.iY);
			if (pBest == NULL) break;

			// Assign unit to task
			AssignUnitToTask(pBest, task);
			vAssigned.push_back(pBest);

			// Remove from idle pool
			m_vIdleUnits.erase(std::remove(m_vIdleUnits.begin(), m_vIdleUnits.end(), pBest), m_vIdleUnits.end());
		}

		// Log dispatch only when units actually moved, so the report log does
		// not fill with empty decisions.
		if (!vAssigned.empty())
		{
			int iLogType = (task.iType == PROVINCE_TASK_DEFEND)
				? CvPlayer::SuzerainReportType("SUZERAIN_REPORT_DISPATCH_DEFEND", 10)
				: CvPlayer::SuzerainReportType("SUZERAIN_REPORT_DISPATCH_RAID", 11);
			m_pPlayer->AddProvinceReport(task.iProvince, iLogType, task.iThreat, (int)vAssigned.size());

			// Update mission state if this is a raid
			if (task.iType == PROVINCE_TASK_RAID)
			{
				m_pPlayer->SetProvinceMissionState(task.iProvince, 1);  // Mission in progress
			}
		}
	}
}

// Find nearest available unit to a target
CvUnit* CvNationalMilitaryDispatcher::FindNearestUnit(const std::vector<CvUnit*>& vUnits, CvPlot* pTarget) const
{
	if (pTarget == NULL) return NULL;
	return FindNearestUnit(vUnits, pTarget->getX(), pTarget->getY());
}

CvUnit* CvNationalMilitaryDispatcher::FindNearestUnit(const std::vector<CvUnit*>& vUnits, int iX, int iY) const
{
	CvUnit* pBest = NULL;
	int iBestDistance = INT_MAX;

	for (size_t i = 0; i < vUnits.size(); i++)
	{
		CvUnit* pUnit = vUnits[i];
		if (pUnit == NULL) continue;

		int iDistance = plotDistance(pUnit->getX(), pUnit->getY(), iX, iY);
		if (iDistance < iBestDistance)
		{
			iBestDistance = iDistance;
			pBest = pUnit;
		}
	}

	return pBest;
}

// Assign a unit to a task
void CvNationalMilitaryDispatcher::AssignUnitToTask(CvUnit* pUnit, const MilitaryTask& task)
{
	if (pUnit == NULL) return;

	// Remember where this unit came from so it can return after the task.
	if (pUnit->GetHomeProvince() < 0)
	{
		CvPlot* pPlot = pUnit->plot();
		if (pPlot != NULL)
		{
			CvCity* pNearest = m_pPlayer->findClosestCity(*pPlot);
			if (pNearest != NULL)
			{
				int iHome = m_pPlayer->GetCityProvince(pNearest->GetID());
				if (iHome >= 0) pUnit->SetHomeProvince(iHome);
			}
		}
	}

	// Set task metadata
	SetUnitCurrentTask(pUnit, task.iType);
	SetUnitTaskProvince(pUnit, task.iProvince);
	SetUnitTaskPriority(pUnit, task.iPriority);

	// Push mission
	pUnit->PushMission(CvTypes::getMISSION_MOVE_TO(), task.iX, task.iY);
}

// Clear task tags of units whose queued orders have finished, so they can be
// reassigned and later return home instead of staying tagged forever.
void CvNationalMilitaryDispatcher::ReleaseFinishedTasks(void)
{
	if (m_pPlayer == NULL) return;

	int iLoop = 0;
	for (CvUnit* pUnit = m_pPlayer->firstUnit(&iLoop); pUnit != NULL; pUnit = m_pPlayer->nextUnit(&iLoop))
	{
		if (pUnit->IsDead()) continue;

		int iTask = GetUnitCurrentTask(pUnit);
		if (iTask == PROVINCE_TASK_NONE || iTask == PROVINCE_TASK_PLAYER) continue;

		// A queued order means the unit is still executing the task.
		if (pUnit->GetLengthMissionQueue() > 0) continue;

		SetUnitCurrentTask(pUnit, PROVINCE_TASK_NONE);
		SetUnitTaskProvince(pUnit, -1);
		SetUnitTaskPriority(pUnit, 0);
	}
}

// Send idle garrisons back toward their home province when they are far away.
void CvNationalMilitaryDispatcher::ReturnIdleGarrison(void)
{
	if (m_pPlayer == NULL) return;

	int iThreshold = GC.getDefineINT("SUZERAIN_RETURN_DISTANCE", false);
	if (iThreshold <= 0) iThreshold = 3;

	int iLoop = 0;
	for (CvUnit* pUnit = m_pPlayer->firstUnit(&iLoop); pUnit != NULL; pUnit = m_pPlayer->nextUnit(&iLoop))
	{
		if (pUnit->IsDead()) continue;
		if (!pUnit->IsCombatUnit()) continue;
		if (pUnit->getDomainType() != DOMAIN_LAND) continue;
		if (!pUnit->canMove()) continue;
		if (pUnit->IsAutomated()) continue;
		if (pUnit->GetLengthMissionQueue() > 0) continue;

		int iTask = GetUnitCurrentTask(pUnit);
		if (iTask != PROVINCE_TASK_NONE) continue;

		int iHome = pUnit->GetHomeProvince();
		if (iHome < 0) continue;

		// Home city: most populous city of the home province.
		CvCity* pHomeCity = NULL;
		int iBestPop = -1;
		int iCityLoop = 0;
		for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
		{
			if (m_pPlayer->GetCityProvince(pCity->GetID()) != iHome) continue;
			if (pCity->getPopulation() > iBestPop)
			{
				iBestPop = pCity->getPopulation();
				pHomeCity = pCity;
			}
		}
		if (pHomeCity == NULL) continue;

		if (plotDistance(pUnit->getX(), pUnit->getY(), pHomeCity->getX(), pHomeCity->getY()) <= iThreshold)
			continue;

		pUnit->PushMission(CvTypes::getMISSION_MOVE_TO(), pHomeCity->getX(), pHomeCity->getY());
	}
}

// Cancel unit's current task
void CvNationalMilitaryDispatcher::CancelUnitTask(CvUnit* pUnit)
{
	if (pUnit == NULL) return;

	SetUnitCurrentTask(pUnit, PROVINCE_TASK_NONE);
	SetUnitTaskProvince(pUnit, -1);
	SetUnitTaskPriority(pUnit, 0);

	pUnit->ClearMissionQueue();
	pUnit->SetActivityType(ACTIVITY_AWAKE);
}

// Get/Set unit task data
int CvNationalMilitaryDispatcher::GetUnitCurrentTask(CvUnit* pUnit) const
{
	if (pUnit == NULL) return PROVINCE_TASK_NONE;
	return pUnit->GetProvinceTask();
}

void CvNationalMilitaryDispatcher::SetUnitCurrentTask(CvUnit* pUnit, int iTask)
{
	if (pUnit == NULL) return;
	pUnit->SetProvinceTask(iTask);
}

int CvNationalMilitaryDispatcher::GetUnitTaskProvince(CvUnit* pUnit) const
{
	if (pUnit == NULL) return -1;
	return pUnit->GetTaskProvince();
}

void CvNationalMilitaryDispatcher::SetUnitTaskProvince(CvUnit* pUnit, int iProv)
{
	if (pUnit == NULL) return;
	pUnit->SetTaskProvince(iProv);
}

int CvNationalMilitaryDispatcher::GetUnitTaskPriority(CvUnit* pUnit) const
{
	if (pUnit == NULL) return 0;
	return pUnit->GetTaskPriority();
}

void CvNationalMilitaryDispatcher::SetUnitTaskPriority(CvUnit* pUnit, int iPriority)
{
	if (pUnit == NULL) return;
	pUnit->SetTaskPriority(iPriority);
}

// Static comparator for sorting tasks by priority
bool CvNationalMilitaryDispatcher::TaskPriorityComparator(const MilitaryTask& a, const MilitaryTask& b)
{
	return a.iPriority > b.iPriority;
}
