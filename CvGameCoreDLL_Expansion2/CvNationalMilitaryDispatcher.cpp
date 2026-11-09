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
	m_vIdleShips.clear();
	m_vTasks.clear();
}

void CvNationalMilitaryDispatcher::Uninit(void)
{
	m_vIdleUnits.clear();
	m_vIdleShips.clear();
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
	CollectIdleShips();

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

// Check if a naval unit is available for assignment
bool CvNationalMilitaryDispatcher::IsShipAvailable(CvUnit* pUnit) const
{
	if (pUnit == NULL) return false;
	if (pUnit->IsDead()) return false;
	if (!pUnit->IsCombatUnit()) return false;
	if (pUnit->getDomainType() != DOMAIN_SEA) return false;
	if (!pUnit->canMove()) return false;
	if (pUnit->IsAutomated()) return false;

	int iCurrentTask = GetUnitCurrentTask(pUnit);
	if (iCurrentTask == PROVINCE_TASK_PLAYER) return false;

	if (pUnit->GetLengthMissionQueue() > 0) return false;

	return true;
}

// True when the task is executed by naval units
bool CvNationalMilitaryDispatcher::IsNavalTask(int iType)
{
	return iType == PROVINCE_TASK_DEFEND_SEA;
}

// Collect all idle naval units
void CvNationalMilitaryDispatcher::CollectIdleShips(void)
{
	m_vIdleShips.clear();

	int iLoop = 0;
	for (CvUnit* pUnit = m_pPlayer->firstUnit(&iLoop); pUnit != NULL; pUnit = m_pPlayer->nextUnit(&iLoop))
	{
		if (IsShipAvailable(pUnit))
			m_vIdleShips.push_back(pUnit);
	}
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

		// Evaluate enemy naval threats near coastal cities
		EvaluateNavalThreats(iProv);

		// Evaluate barbarian threats (medium priority)
		EvaluateBarbarianThreats(iProv);
	}

	// Sort tasks by priority (highest first)
	std::sort(m_vTasks.begin(), m_vTasks.end(), TaskPriorityComparator);
}

// Helper: evaluate enemy land threats in a province. Water plots are skipped so
// land garrisons are never ordered into the sea; ships are handled separately.
void CvNationalMilitaryDispatcher::EvaluateEnemyThreats(int iProv)
{
	if (m_pPlayer->IsCapitalProvince(iProv)) return;
	if (m_pPlayer->GetProvinceAutonomy(iProv) < 1) return;

	if (m_pPlayer->GetProvinceWar(iProv) == 0) return;

	int iDefenseRadius = GC.getDefineINT("SUZERAIN_PROVINCE_DEFENSE_RADIUS", false);
	if (iDefenseRadius <= 0) return;

	// 0 (default) means one task per enemy stack, with no cap.
	int iMaxTasks = GC.getDefineINT("SUZERAIN_DISPATCH_MAX_TASKS_PER_PROVINCE", false);

	int iMinUnits = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_MIN_UNITS", false);
	int iPerEnemy = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_UNITS_PER_ENEMY", false);
	int iPriority = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_PRIORITY", false);
	if (iMinUnits <= 0 || iPerEnemy <= 0 || iPriority <= 0)
		return; // not configured = no land dispatch

	// Gather distinct land plots holding a visible enemy near a province city.
	struct ThreatPlot
	{
		CvPlot* pPlot;
		int iDistance;
		int iCount;
	};
	std::vector<ThreatPlot> vThreats;

	int iCityLoop = 0;
	for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
	{
		if (m_pPlayer->GetCityProvince(pCity->GetID()) != iProv)
			continue;

		int iCityX = pCity->getX();
		int iCityY = pCity->getY();

		for (int dx = -iDefenseRadius; dx <= iDefenseRadius; dx++)
		{
			for (int dy = -iDefenseRadius; dy <= iDefenseRadius; dy++)
			{
				CvPlot* pPlot = GC.getMap().plot(iCityX + dx, iCityY + dy);
				if (pPlot == NULL) continue;

				// Land garrisons cannot engage ships; naval threats are separate.
				if (pPlot->isWater()) continue;

				if (pPlot->getVisibleEnemyDefender(m_pPlayer->GetID()) == NULL) continue;

				bool bKnown = false;
				for (size_t t = 0; t < vThreats.size(); t++)
				{
					if (vThreats[t].pPlot == pPlot)
					{
						bKnown = true;
						break;
					}
				}
				if (bKnown) continue;

				ThreatPlot kThreat;
				kThreat.pPlot = pPlot;
				kThreat.iDistance = plotDistance(pCity->getX(), pCity->getY(), pPlot->getX(), pPlot->getY());
				kThreat.iCount = pPlot->getNumUnits();
				vThreats.push_back(kThreat);
			}
		}
	}

	// Nearest threats first, capped by the configured task count.
	std::sort(vThreats.begin(), vThreats.end(),
		[](const ThreatPlot& a, const ThreatPlot& b) { return a.iDistance < b.iDistance; });

	int iTasksMade = 0;
	for (size_t t = 0; t < vThreats.size() && (iMaxTasks <= 0 || iTasksMade < iMaxTasks); t++)
	{
		MilitaryTask task;
		task.iType = PROVINCE_TASK_DEFEND;
		task.iPriority = iPriority;
		task.iProvince = iProv;
		task.pTarget = vThreats[t].pPlot;
		task.iX = vThreats[t].pPlot->getX();
		task.iY = vThreats[t].pPlot->getY();
		task.iUnitsNeeded = std::max(iMinUnits, vThreats[t].iCount * iPerEnemy);
		task.iThreat = vThreats[t].iCount;

		m_vTasks.push_back(task);
		iTasksMade++;
	}
}

// Helper: evaluate enemy naval threats near a province's coastal cities.
void CvNationalMilitaryDispatcher::EvaluateNavalThreats(int iProv)
{
	if (m_pPlayer->IsCapitalProvince(iProv)) return;
	if (m_pPlayer->GetProvinceAutonomy(iProv) < 1) return;

	if (m_pPlayer->GetProvinceWar(iProv) == 0) return;

	int iRadius = GC.getDefineINT("SUZERAIN_NAVAL_DEFENSE_RADIUS", false);
	if (iRadius <= 0) return; // not configured = no naval defence

	// 0 (default) means one task per enemy stack, with no cap.
	int iMaxTasks = GC.getDefineINT("SUZERAIN_DISPATCH_MAX_TASKS_PER_PROVINCE", false);

	int iMinUnits = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_MIN_UNITS", false);
	int iPerEnemy = GC.getDefineINT("SUZERAIN_DISPATCH_DEFEND_UNITS_PER_ENEMY", false);
	int iPriority = GC.getDefineINT("SUZERAIN_NAVAL_DISPATCH_PRIORITY", false);
	if (iMinUnits <= 0 || iPerEnemy <= 0 || iPriority <= 0)
		return; // not configured = no naval dispatch

	struct SeaThreat
	{
		CvPlot* pPlot;
		int iDistance;
		int iCount;
	};
	std::vector<SeaThreat> vThreats;

	int iCityLoop = 0;
	for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
	{
		if (m_pPlayer->GetCityProvince(pCity->GetID()) != iProv)
			continue;
		if (!pCity->isCoastal()) continue;

		for (int dx = -iRadius; dx <= iRadius; dx++)
		{
			for (int dy = -iRadius; dy <= iRadius; dy++)
			{
				CvPlot* pPlot = GC.getMap().plot(pCity->getX() + dx, pCity->getY() + dy);
				if (pPlot == NULL) continue;
				if (!pPlot->isWater()) continue;

				CvUnit* pEnemy = pPlot->getVisibleEnemyDefender(m_pPlayer->GetID());
				if (pEnemy == NULL) continue;

				bool bKnown = false;
				for (size_t t = 0; t < vThreats.size(); t++)
				{
					if (vThreats[t].pPlot == pPlot)
					{
						bKnown = true;
						break;
					}
				}
				if (bKnown) continue;

				SeaThreat kThreat;
				kThreat.pPlot = pPlot;
				kThreat.iDistance = plotDistance(pCity->getX(), pCity->getY(), pPlot->getX(), pPlot->getY());
				kThreat.iCount = pPlot->getNumUnits();
				vThreats.push_back(kThreat);
			}
		}
	}

	std::sort(vThreats.begin(), vThreats.end(),
		[](const SeaThreat& a, const SeaThreat& b) { return a.iDistance < b.iDistance; });

	int iTasksMade = 0;
	for (size_t t = 0; t < vThreats.size() && (iMaxTasks <= 0 || iTasksMade < iMaxTasks); t++)
	{
		MilitaryTask task;
		task.iType = PROVINCE_TASK_DEFEND_SEA;
		task.iPriority = iPriority;
		task.iProvince = iProv;
		task.pTarget = vThreats[t].pPlot;
		task.iX = vThreats[t].pPlot->getX();
		task.iY = vThreats[t].pPlot->getY();
		task.iUnitsNeeded = std::max(iMinUnits, vThreats[t].iCount * iPerEnemy);
		task.iThreat = vThreats[t].iCount;

		m_vTasks.push_back(task);
		iTasksMade++;
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
		int iPriority = GC.getDefineINT("SUZERAIN_DISPATCH_RAID_PRIORITY", false);
		if (iMinUnits <= 0 || iPriority <= 0)
			return; // not configured = no raid dispatch

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

		// Land tasks draw from the land pool, naval tasks from the ship pool.
		bool bNaval = IsNavalTask(task.iType);
		std::vector<CvUnit*>* pPool = bNaval ? &m_vIdleShips : &m_vIdleUnits;

		std::vector<CvUnit*> vAssigned;
		for (int j = 0; j < task.iUnitsNeeded && !pPool->empty(); j++)
		{
			CvUnit* pBest = FindNearestUnit(*pPool, task.iX, task.iY);
			if (pBest == NULL) break;

			AssignUnitToTask(pBest, task);
			vAssigned.push_back(pBest);

			pPool->erase(std::remove(pPool->begin(), pPool->end(), pBest), pPool->end());
		}

		// Log dispatch only when units actually moved, so the report log does
		// not fill with empty decisions.
		if (!vAssigned.empty())
		{
			int iLogType;
			if (bNaval)
				iLogType = CvPlayer::SuzerainReportType("SUZERAIN_REPORT_DISPATCH_SEA", 16);
			else if (task.iType == PROVINCE_TASK_DEFEND)
				iLogType = CvPlayer::SuzerainReportType("SUZERAIN_REPORT_DISPATCH_DEFEND", 10);
			else
				iLogType = CvPlayer::SuzerainReportType("SUZERAIN_REPORT_DISPATCH_RAID", 11);
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
	if (iThreshold <= 0) return; // not configured = units stay where they are

	int iLoop = 0;
	for (CvUnit* pUnit = m_pPlayer->firstUnit(&iLoop); pUnit != NULL; pUnit = m_pPlayer->nextUnit(&iLoop))
	{
		if (pUnit->IsDead()) continue;
		if (!pUnit->IsCombatUnit()) continue;

		int iDomain = pUnit->getDomainType();
		if (iDomain != DOMAIN_LAND && iDomain != DOMAIN_SEA) continue;
		bool bNaval = (iDomain == DOMAIN_SEA);

		if (!pUnit->canMove()) continue;
		if (pUnit->IsAutomated()) continue;
		if (pUnit->GetLengthMissionQueue() > 0) continue;

		int iTask = GetUnitCurrentTask(pUnit);
		if (iTask != PROVINCE_TASK_NONE) continue;

		int iHome = pUnit->GetHomeProvince();
		if (iHome < 0) continue;

		// Home city: most populous city of the home province; ships need a
		// coastal one.
		CvCity* pHomeCity = NULL;
		int iBestPop = -1;
		int iCityLoop = 0;
		for (CvCity* pCity = m_pPlayer->firstCity(&iCityLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iCityLoop))
		{
			if (m_pPlayer->GetCityProvince(pCity->GetID()) != iHome) continue;
			if (bNaval && !pCity->isCoastal()) continue;
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

// Static comparator: higher priority first, then the bigger threat first so
// a large enemy stack is reinforced before a lone scout.
bool CvNationalMilitaryDispatcher::TaskPriorityComparator(const MilitaryTask& a, const MilitaryTask& b)
{
	if (a.iPriority != b.iPriority)
		return a.iPriority > b.iPriority;
	return a.iThreat > b.iThreat;
}
