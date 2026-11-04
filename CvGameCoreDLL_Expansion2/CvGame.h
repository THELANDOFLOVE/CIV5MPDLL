/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

// game.h

#ifndef CIV5_GAME_H
#define CIV5_GAME_H

#include "CvRandom.h"
#include "FFastVector.h"
#include "FTimer.h"
#include "CvPreGame.h"
#include <CvLocalization.h>
#include <map>

#include "CvDealClasses.h"

class CvPlot;
class CvCity;
class CvReplayMessage;
class CvReplayInfo;
class CvSiteEvaluatorForSettler;
class CvSiteEvaluatorForStart;
class CvStartPositioner;
class CvGameReligions;
class CvGameCulture;
class CvGameLeagues;
class CvGameTrade;
class CvTacticalAnalysisMap;
class CvAdvisorCounsel;
class CvAdvisorRecommender;


class CvGameInitialItemsOverrides
{
public:
	CvGameInitialItemsOverrides();

	std::vector<bool> GrantInitialFreeTechsPerTeam;
	std::vector<bool> GrantInitialGoldPerPlayer;
	std::vector<bool> GrantInitialCulturePerPlayer;
	std::vector<bool> ClearResearchQueuePerPlayer;
	std::vector<bool> GrantInitialUnitsPerPlayer;
};

class CvGame
{
public:
	CvGame();
	~CvGame();

	void init(HandicapTypes eHandicap);
	bool init2();

	void InitPlayers();

	void reset(HandicapTypes eHandicap, bool bConstructorCall = false);
	void uninit();

	void DoGameStarted();

	void update();
	void updateScore(bool bForce = false);

	int GetMapScoreMod() const;

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	int GetNuclearWinterProcess() const;
	void ChangeNuclearWinterProcess(int iChange, bool bUpdate = false, bool bAllowLevelReduce = false);
	int GetNuclearWinterNaturalReduction() const;
	void ChangeNuclearWinterNaturalReduction(int iChange);
	int GetNowNuclearWinterLevelIndex();
	void DoNuclearWinterTurn();
	void UpdateNuclearWinterLevel();
	int GetNuclearWinterYieldMultiplier(YieldTypes eIndex);
	int GetYieldFromNuclearWinter(YieldTypes eIndex);
#endif
#if defined(MOD_GLOBAL_MAX_PLOT_BUILD)
	bool IsPlotExceedMaxBuild(PlayerTypes ePlayer, CvPlot *pPlot) const;
	void IncreasePlotBuildNum(PlayerTypes ePlayer, CvPlot *pPlot);
	void DoPlotBuildNumTurn();
#endif

	void updateCitySight(bool bIncrement);
	void updateSelectionList();
	void updateTestEndTurn();

	CvUnit* getPlotUnit(CvPlot* pPlot, int iIndex);
	void getPlotUnits(CvPlot* pPlot, std::vector<CvUnit*>& plotUnits);

	void cycleCities(bool bForward = true, bool bAdd = false);
	void cycleUnits(bool bClear, bool bForward = true, bool bWorkers = false);
	bool cyclePlotUnits(CvPlot* pPlot, bool bForward = true, bool bAuto = false, int iCount = -1);

	void selectionListMove(CvPlot* pPlot, bool bShift);
	void selectionListGameNetMessage(int eMessage, int iData2 = -1, int iData3 = -1, int iData4 = -1, int iFlags = 0, bool bAlt = false, bool bShift = false);
	void selectedCitiesGameNetMessage(int eMessage, int iData2 = -1, int iData3 = -1, int iData4 = -1, bool bOption = false, bool bAlt = false, bool bShift = false, bool bCtrl = false);
	void cityPushOrder(CvCity* pCity, OrderTypes eOrder, int iData, bool bAlt = false, bool bShift = false, bool bCtrl = false);
	void CityPurchase(CvCity* pCity, UnitTypes eUnitType, BuildingTypes eBuildingType, ProjectTypes eProjectType, YieldTypes ePurchaseYield);

	void selectUnit(CvUnit* pUnit, bool bClear, bool bToggle = false, bool bSound = false);
	void mouseoverUnit(CvUnit* pUnit, bool bEnter);
	void selectGroup(CvUnit* pUnit, bool bShift, bool bCtrl, bool bAlt);
	void selectAll(CvPlot* pPlot);
	void SelectSettler(void);

	bool selectionListIgnoreBuildingDefense();

	bool canHandleAction(int iAction, CvPlot* pPlot = NULL, bool bTestVisible = false);
	void handleAction(int iAction);

	bool canDoControl(ControlTypes eControl);
	void doControl(ControlTypes eControl);

	int getProductionPerPopulation(HurryTypes eHurry);

	int getAdjustedPopulationPercent(VictoryTypes eVictory) const;
	int getAdjustedLandPercent(VictoryTypes eVictory) const;

	int countCivPlayersAlive() const;
	int countCivPlayersEverAlive() const;
	int countCivTeamsAlive() const;
	int countCivTeamsEverAlive() const;
	int countHumanPlayersAlive() const;
	int countHumanPlayersEverAlive() const;
	int countSeqHumanTurnsUntilPlayerTurn(PlayerTypes playerID) const;

	int countMajorCivsAlive() const;
	int countMajorCivsEverAlive() const;
	int countTotalCivPower();
	int countTotalNukeUnits();
	int countKnownTechNumTeams(TechTypes eTech);

	int goldenAgeLength() const;
	int victoryDelay(VictoryTypes eVictory) const;

	int getImprovementUpgradeTimeMod(ImprovementTypes eImprovement, const CvPlot* pPlot = NULL) const;
	int getImprovementUpgradeTime(ImprovementTypes eImprovement, const CvPlot* pPlot = NULL) const;

	bool canTrainNukes() const;
	EraTypes getCurrentEra() const;
	EraTypes getCurrentEraCached() const;

#if defined(MOD_SP_UNIQUE_CITYSTATE)
	// Economic Aid (Super Power V11) - all city-states share a single global round
	void DoEconomicAidRoundTurn();
	void StartNewEconomicAidRound();
	bool IsEconomicAidActive() const;
	int GetEconomicAidRound() const;
	int GetEconomicAidRoundStartTurn() const;
	int GetEconomicAidWorldEra() const;
	int GetEconomicAidRoundTurnsLeft() const;

	// SP extended quests: the master spy reward for the capture-original-capital quest
	// is unique for the whole game, so the first player to finish it claims it
	bool IsMasterSpyQuestAwarded() const;
	void SetMasterSpyQuestAwarded(bool bValue);
#endif

	TeamTypes getActiveTeam();
	CivilizationTypes getActiveCivilizationType();

#if defined(MOD_ROG_CORE)
	int NumOriginalCapitalModMax() const;
#endif


#if defined(MOD_API_EXTENSIONS)
	bool isReallyNetworkMultiPlayer() const;
#endif
	bool isNetworkMultiPlayer() const;
	bool isGameMultiPlayer() const;
	// True on the single updateMoves() frame that activates the human players right after a load (or a new game).
	// CvPlayer::setTurnActive() uses it to clear a stale "turn complete" the engine restored from the save.
	bool isFirstActivationOfPlayersAfterLoad() const;
	bool isTeamGame() const;

	void ReviveActivePlayer();
	bool CanMoveActivePlayerToObserver();
	void ActivateObserverSlot();

	int getNumHumanPlayers();
	int GetNumMinorCivsEver();
	int getNumHumansInHumanWars(PlayerTypes ignorePlayer = NO_PLAYER);
	int getNumSequentialHumans(PlayerTypes ignorePlayer = NO_PLAYER);

	//int getGameTurn();
	int getGameTurn() const;

	void setGameTurn(int iNewValue);
	void incrementGameTurn();
	int getTurnYear(int iGameTurn);
	int getGameTurnYear();

	int getElapsedGameTurns() const;
	void incrementElapsedGameTurns();

	int getMaxTurns() const;
	void setMaxTurns(int iNewValue);
	void changeMaxTurns(int iChange);

	int getMaxCityElimination() const;
	void setMaxCityElimination(int iNewValue);

	int getNumAdvancedStartPoints() const;
	void setNumAdvancedStartPoints(int iNewValue);

	int getStartTurn() const;
	void setStartTurn(int iNewValue);

	int GetWinningTurn() const;
	void SetWinningTurn(int iNewValue);

	int getStartYear() const;
	void setStartYear(int iNewValue);

	int getEstimateEndTurn() const;
	void setEstimateEndTurn(int iNewValue);
	int getDefaultEstimateEndTurn() const;
	void setDefaultEstimateEndTurn(int iNewValue);

	int getTurnSlice() const;
	int getMinutesPlayed() const;
	void setTurnSlice(int iNewValue);
	void changeTurnSlice(int iChange);

	void resetTurnTimer(bool resetGameTurnStart = true);
	int getMaxTurnLen();

	bool IsStaticTutorialActive() const;
	void SetStaticTutorialActive(bool bStaticTutorialActive);

	bool HasAdvisorMessageBeenSeen(_In_z_ const char* szAdvisorMessageName);
	void SetAdvisorMessageHasBeenSeen(_In_z_ const char* szAdvisorMessageName, bool bSeen);

	bool IsCityScreenBlocked();
	bool CanOpenCityScreen(PlayerTypes eOpener, CvCity* pCity);

	int getTargetScore() const;
	void setTargetScore(int iNewValue);

	int getNumGameTurnActive();
	int countNumHumanGameTurnActive();
	void changeNumGameTurnActive(int iChange, const std::string& why);

	int getNumCities() const;
	int getNumCivCities() const;
	void changeNumCities(int iChange);

	int getTotalPopulation() const;
	void changeTotalPopulation(int iChange);

	int getNoNukesCount() const;
	bool isNoNukes() const;
	void changeNoNukesCount(int iChange);

	int getNukesExploded() const;
	void changeNukesExploded(int iChange);

	int getMaxPopulation() const;
	int getInitPopulation() const;
	int getInitLand() const;
	int getInitTech() const;
	int getInitWonders() const;
	void initScoreCalculation();

	int getAIAutoPlay();
	void setAIAutoPlay(int iNewValue, PlayerTypes eReturnPlayer);
	void changeAIAutoPlay(int iChange);

	unsigned int getInitialTime();
	void setInitialTime(unsigned int uiNewValue);

	bool isScoreDirty() const;
	void setScoreDirty(bool bNewValue);

	bool isCircumnavigated() const;
	void makeCircumnavigated();
	bool circumnavigationAvailable() const;

	void DoFromUIDiploEvent(FromUIDiploEventTypes eEvent, PlayerTypes eAIPlayer, int iArg1, int iArg2);

	// Diplomacy Victory
	void DoInitDiploVictory();
	void DoUpdateDiploVictory();
	int GetVotesNeededForDiploVictory() const;
	void SetVotesNeededForDiploVictory(int iValue);
	bool IsUnitedNationsActive();

	// DEPRECATED
	int GetUnitedNationsCountdown() const;
	void SetUnitedNationsCountdown(int iValue);
	void ChangeUnitedNationsCountdown(int iChange);
	int GetNumVictoryVotesTallied() const;
	void SetNumVictoryVotesTallied(int iValue, bool bPreliminaryVote);
	void ChangeNumVictoryVotesTallied(int iChange, bool bPreliminaryVote);
	int GetNumVictoryVotesExpected() const;
	void SetNumVictoryVotesExpected(int iValue);
	TeamTypes GetVoteCast(TeamTypes eVotingTeam) const;
	void SetVoteCast(TeamTypes eVotingTeam, TeamTypes eVote, bool bPreliminaryVote = false);
	TeamTypes GetPreviousVoteCast(TeamTypes eVotingTeam) const;
	void SetPreviousVoteCast(TeamTypes eVotingTeam, TeamTypes eVotingTarget);
	int GetNumVotesForTeam(TeamTypes eTeam) const;
	void SetNumVotesForTeam(TeamTypes eTeam, int iValue);
	void ChangeNumVotesForTeam(TeamTypes eTeam, int iChange);
	// End DEPRECATED

	Localization::String GetDiploResponse(const char* szLeader, const char* szResponse, const Localization::String& strOptionalKey1, const Localization::String& strOptionalKey2);

	bool isDebugMode() const;
	void setDebugMode(bool bDebugMode);
	void toggleDebugMode();
	void updateDebugModeCache();

	void setFOW(bool bMode);
	bool getFOW();

	int getPitbossTurnTime() const;
	void setPitbossTurnTime(int iHours);

	bool isHotSeat() const;
	bool isPbem() const;
	bool isPitboss() const;
	bool isSimultaneousTeamTurns() const;

	bool isFinalInitialized() const;
	void setFinalInitialized(bool bNewValue);

	bool IsWaitingForBlockingInput() const
	{
		return (m_eWaitDiploPlayer != NO_PLAYER);
	};
	void SetWaitingForBlockingInput(PlayerTypes ePlayer)
	{
		m_eWaitDiploPlayer = ePlayer;
	};

	bool getPbemTurnSent() const;
	void setPbemTurnSent(bool bNewValue);

	bool getHotPbemBetweenTurns() const;
	void setHotPbemBetweenTurns(bool bNewValue);

	bool isPlayerOptionsSent() const;
	void sendPlayerOptions(bool bForce = false);

	PlayerTypes getActivePlayer() const;
	void setActivePlayer(PlayerTypes eNewValue, bool bForceHotSeat = false, bool bAutoplaySwitch = false);

	CvHandicapInfo& getHandicapInfo() const;
	HandicapTypes getHandicapType() const;
	void setHandicapType(HandicapTypes eHandicap);

	PlayerTypes getPausePlayer();
	bool isPaused();
	void setPausePlayer(PlayerTypes eNewValue);

	UnitTypes getBestLandUnit();
	int getBestLandUnitCombat();
	void setBestLandUnit(UnitTypes eNewValue);

	int GetFaithCost(CvUnitEntry *pkUnit) const;

	TeamTypes getWinner() const;
	VictoryTypes getVictory() const;
	void setWinner(TeamTypes eNewWinner, VictoryTypes eNewVictory);

	bool isVictoryAvailable(VictoryTypes eVictory) const;
	int GetNextAvailableVictoryCompetitionRank(VictoryTypes eVictory) const;
	void DoPlaceTeamInVictoryCompetition(VictoryTypes eNewVictory, TeamTypes eTeam);
	TeamTypes getTeamVictoryRank(VictoryTypes eNewVictory, int iRank) const;
	void setTeamVictoryRank(VictoryTypes eNewVictory, int iRank, TeamTypes eTeam);

	int GetWorldMilitaryStrengthAverage(PlayerTypes ePlayer, bool bIncludeMe, bool bIncludeOnlyKnown);

	int GetTotalReligionTechCost() const;
	void DoUpdateTotalReligionTechCost();
	int GetCachedWorldReligionTechProgress() const;
	void DoUpdateCachedWorldReligionTechProgress();

	TechTypes GetReligionTech() const;
	void SetReligionTech(TechTypes eTech);
	int GetResearchLeftToReligionTech(TeamTypes eTeam);
	int GetResearchLeftToTech(TeamTypes eTeam, TechTypes eTech);

	GameStateTypes getGameState();
	void setGameState(GameStateTypes eNewValue);

	CvEraInfo& getStartEraInfo() const;
	EraTypes getStartEra() const;

	CalendarTypes getCalendar() const;

	CvGameSpeedInfo& getGameSpeedInfo() const;
	GameSpeedTypes getGameSpeedType() const;

	int getEndTurnMessagesReceived(int iIndex);
	void incrementEndTurnMessagesReceived(int iIndex);

	PlayerTypes getRankPlayer(int iRank);
	void setRankPlayer(int iRank, PlayerTypes ePlayer);

	int getPlayerRank(PlayerTypes ePlayer);
	void setPlayerRank(PlayerTypes ePlayer, int iRank);

	int getPlayerScore(PlayerTypes ePlayer);
	void setPlayerScore(PlayerTypes ePlayer, int iScore);

	TeamTypes getRankTeam(int iRank);
	void setRankTeam(int iRank, TeamTypes eTeam);

	int getTeamRank(TeamTypes eTeam);
	void setTeamRank(TeamTypes eTeam, int iRank);

	int getTeamScore(TeamTypes eTeam) const;
	void setTeamScore(TeamTypes eTeam, int iScore);

	bool isOption(GameOptionTypes eIndex) const;
	bool isOption(const char* pszOption) const;
	void setOption(GameOptionTypes eIndex, bool bEnabled);
	void setOption(const char* pszOption, bool bEnabled);

	bool isMPOption(MultiplayerOptionTypes eIndex) const;
	void setMPOption(MultiplayerOptionTypes eIndex, bool bEnabled);

	int getUnitCreatedCount(UnitTypes eIndex);
	void incrementUnitCreatedCount(UnitTypes eIndex);

	int getUnitClassCreatedCount(UnitClassTypes eIndex);
	bool isUnitClassMaxedOut(UnitClassTypes eIndex, int iExtra = 0);
	void incrementUnitClassCreatedCount(UnitClassTypes eIndex);

	int getBuildingClassCreatedCount(BuildingClassTypes eIndex);
	bool isBuildingClassMaxedOut(BuildingClassTypes eIndex, int iExtra = 0);
	void incrementBuildingClassCreatedCount(BuildingClassTypes eIndex);

	int getProjectCreatedCount(ProjectTypes eIndex);
	bool isProjectMaxedOut(ProjectTypes eIndex, int iExtra = 0);
	void incrementProjectCreatedCount(ProjectTypes eIndex, int iExtra = 1);

	bool isVictoryValid(VictoryTypes eIndex) const;
	void setVictoryValid(VictoryTypes eIndex, bool bValid);
	bool areNoVictoriesValid() const;

	bool isSpecialUnitValid(SpecialUnitTypes eIndex);
	void makeSpecialUnitValid(SpecialUnitTypes eIndex);

	bool isNukesValid() const;
	void makeNukesValid(bool bValid = true);

	bool isInAdvancedStart() const;

	const CvString& getName();
	void setName(const char* szName);

	bool isDestroyedCityName(CvString& szName) const;
	void addDestroyedCityName(const CvString& szName);

	bool isGreatPersonBorn(CvString& szName) const;
	void addGreatPersonBornName(const CvString& szName);
#if defined(MOD_API_EXTENSIONS)
	void removeGreatPersonBornName(const CvString& szName);
#endif

	CvRandom& getMapRand();
	int getMapRandNum(int iNum, const char* pszLog);

	CvRandom& getJonRand();
	int getJonRandNum(int iNum, const char* pszLog);
	int getJonRandNumVA(int iNum, const char* pszLog, ...);
	int getAsyncRandNum(int iNum, const char* pszLog);

	int GetImmigrationRegressand(PlayerTypes ePlayer = NO_PLAYER) const;


	uint randCore(const CvSeeder& extraSeed) const;
	uint urandLimitExclusive(uint limit, const CvSeeder& extraSeed) const;
	uint urandLimitInclusive(uint limit, const CvSeeder& extraSeed) const;
	uint urandRangeExclusive(uint min, uint max, const CvSeeder& extraSeed) const;
	uint urandRangeInclusive(uint min, uint max, const CvSeeder& extraSeed) const;
	int randRangeExclusive(int min, int max, const CvSeeder& extraSeed) const;
	int randRangeInclusive(int min, int max, const CvSeeder& extraSeed) const;


#if defined(MOD_ROG_CORE)
	//get random number from gamestate without a seed in the generator
	int	getSmallFakeRandNum(int iNum, const CvPlot& input) const;
	int	getSmallFakeRandNum(int iNum, int iExtraSeed) const;
#endif

	int calculateSyncChecksum();
	int calculateOptionsChecksum();

	void addReplayMessage(ReplayMessageTypes eType, PlayerTypes ePlayer, const CvString& pszText, int iPlotX = -1, int iPlotY = -1);
	void clearReplayMessageMap();
	uint getNumReplayMessages() const;
	const CvReplayMessage* getReplayMessage(uint i) const;

	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream) const;
	void ReadSupportingClassData(FDataStream& kStream);
	void WriteSupportingClassData(FDataStream& kStream);

	void writeReplay(FDataStream& kStream);

	// Ported in from old CvGameAI
	int GetCombatValue(UnitTypes eUnit);

	void saveReplay();

	void addPlayer(PlayerTypes eNewPlayer, LeaderHeadTypes eLeader, CivilizationTypes eCiv);

	void testVictory();
	bool testVictory(VictoryTypes eVictory, TeamTypes eTeam, bool* pbEndScore = NULL) const;

	int getPlotExtraYield(int iX, int iY, YieldTypes eYield) const;
	void setPlotExtraYield(int iX, int iY, YieldTypes eYield, int iCost);
	void removePlotExtraYield(int iX, int iY);

	int getPlotExtraCost(int iX, int iY) const;
	void changePlotExtraCost(int iX, int iY, int iCost);
	void removePlotExtraCost(int iX, int iY);

	// Plot naming
	void SetPlotName(int iX, int iY, const char* szName);
	const char* GetPlotName(int iX, int iY) const;
	void RemovePlotName(int iX, int iY);
	const std::map<int, std::string>& GetAllPlotNames() const;

	// Project first completion tracking (like wonder builder display)
	void SetProjectFirstCompletion(ProjectTypes eProject, PlayerTypes ePlayer, const char* szCityName);
	int GetProjectFirstPlayer(ProjectTypes eProject) const;
	const char* GetProjectFirstCityName(ProjectTypes eProject) const;
	const std::map<int, int>& GetAllProjectFirstPlayers() const;
	const std::map<int, std::string>& GetAllProjectFirstCityNames() const;

	bool isCivEverActive(CivilizationTypes eCivilization) const;
	bool isLeaderEverActive(LeaderHeadTypes eLeader) const;
	bool isUnitEverActive(UnitTypes eUnit) const;
	bool isBuildingEverActive(BuildingTypes eBuilding) const;

	RouteTypes GetIndustrialRoute() const;
	void DoUpdateIndustrialRoute();

	ImprovementTypes GetBarbarianCampImprovementType();
	UnitTypes getRandomBarbarianUnitType(CvArea* pArea, UnitAITypes eUnitAI);
	int GetBarbarianReleaseTurn() const
	{
		return m_iEarliestBarbarianReleaseTurn;
	};
	void SetBarbarianReleaseTurn(int iValue);

	UnitTypes GetRandomSpawnUnitType(PlayerTypes ePlayer, bool bIncludeUUs, bool bIncludeRanged);
	UnitTypes GetCompetitiveSpawnUnitType(PlayerTypes ePlayer, bool bIncludeUUs, bool bIncludeRanged, bool bIncludeShips);
#if defined(MOD_GLOBAL_CS_GIFTS)
	UnitTypes GetCsGiftSpawnUnitType(PlayerTypes ePlayer, bool bIncludeShips);
#endif
	UnitTypes GetRandomUniqueUnitType(bool bIncludeCivsInGame, bool bIncludeStartEra, bool bIncludeOldEras, bool bIncludeRanged);

	CvSiteEvaluatorForSettler* GetSettlerSiteEvaluator();
	CvSiteEvaluatorForStart* GetStartSiteEvaluator();
	CvStartPositioner* GetStartPositioner();
	CvGameDeals* GetGameDeals();
	CvGameReligions* GetGameReligions();
	CvGameCulture* GetGameCulture();
	CvGameLeagues* GetGameLeagues();
	CvGameTrade* GetGameTrade();
	CvTacticalAnalysisMap* GetTacticalAnalysisMap();

#if defined(MOD_API_LUA_EXTENSIONS)
	CvString getDllGuid() const;
#endif

	int GetAction(int iKeyStroke, bool bAlt, bool bShift, bool bCtrl);
	int IsAction(int iKeyStroke, bool bAlt, bool bShift, bool bCtrl);

	void endTurnTimerSemaphoreIncrement();
	void endTurnTimerSemaphoreDecrement();
	void endTurnTimerReset();
	bool AreUnitsSameType(UnitTypes eFirstUnitType, UnitTypes eSecondUnitType);

	void DoMinorPledgeProtection(PlayerTypes eMajor, PlayerTypes eMinor, bool bProtect, bool bPledgeNowBroken = false);
	void DoMinorGiftGold(PlayerTypes eMinor, int iNumGold);
	void DoMinorFaithGift(PlayerTypes eMinor, int iEquivalentGold);
	// Gangtok CS UA: network-synced variant taking an explicit eMajor (multiplayer-safe)
	void DoMinorFaithGiftFromMajor(PlayerTypes eMajor, PlayerTypes eMinor, int iEquivalentGold);
	// Wittenberg CS UA: faith-purchase a belief for the active player into this city-state's religion
	bool DoCityStateFaithBeliefPurchase(PlayerTypes eMinor, BeliefTypes eBelief);
	// Wittenberg CS UA: network-synced variant taking an explicit eMajor (multiplayer-safe)
	bool DoCityStateFaithBeliefPurchaseFromMajor(PlayerTypes eMajor, PlayerTypes eMinor, BeliefTypes eBelief);
	// Wittenberg CS UA: faith cost for the active player to purchase a belief at this city-state (0 = not available)
	int GetCityStateFaithBeliefPurchaseCost(PlayerTypes eMinor);
	// Wittenberg CS UA: has the active player already faith-purchased a belief at this city-state?
	bool IsCityStateFaithBeliefPurchased(PlayerTypes eMinor);
	// La Venta CS UA: faith-purchase an idle pantheon belief for the active player into this city-state's religion
	bool DoCityStateFaithPantheonPurchase(PlayerTypes eMinor, BeliefTypes eBelief);
	// La Venta CS UA: network-synced variant taking an explicit eMajor (multiplayer-safe)
	bool DoCityStateFaithPantheonPurchaseFromMajor(PlayerTypes eMajor, PlayerTypes eMinor, BeliefTypes eBelief);
	// La Venta CS UA: faith cost for the active player to purchase an idle pantheon belief at this city-state (0 = not available)
	int GetCityStateFaithPantheonPurchaseCost(PlayerTypes eMinor);
	void DoMinorGiftTileImprovement(PlayerTypes eMajor, PlayerTypes eMinor, int iPlotX, int iPlotY);
	void DoMinorBullyGold(PlayerTypes eBully, PlayerTypes eMinor);
	void DoMinorBullyUnit(PlayerTypes eBully, PlayerTypes eMinor);
	void DoMinorBuyout(PlayerTypes eMajor, PlayerTypes eMinor);

	// Economic Aid point spending (Super Power V11)
	// Network-synced variants taking an explicit eMajor (multiplayer-safe). Costs are configured via
	// gCustomMods options and charged against the (major, city-state) aid point pool.
	bool DoMinorRerollQuestsFromMajor(PlayerTypes eMajor, PlayerTypes eMinor);
	void DoMinorBullyGoldWithAidPointsFromMajor(PlayerTypes eMajor, PlayerTypes eMinor, int iGold);
	bool DoSetNextGreatPersonChoiceFromMajor(PlayerTypes eMajor, UnitTypes eUnit, PlayerTypes eCostMinor);
	bool DoSetPendingGiftUnitFromMajor(PlayerTypes eMajor, PlayerTypes eMinor, UnitTypes eUnit);
	// Pull the next World Congress / United Nations session forward by a fixed number of turns.
	// The League countdown is global (shared by every player), so this is a network-synced action.
	bool DoHastenCongressWithAidPointsFromMajor(PlayerTypes eMajor, PlayerTypes eCostMinor);
	// Aid point costs for the spending actions above (0 = disabled)
	int GetAidPointCostRerollQuests();
	int GetAidPointCostNextGreatPerson();
	int GetAidPointCostPendingGiftUnit();
	int GetAidPointCostHastenCongress();
	// Extra aid points (on top of the gold) the Austria UA pays to buy a city-state out (0 = disabled).
	// Charged from the (major, city-state) aid point pool by CvMinorCivAI::DoBuyout.
	int GetAidPointCostAnnexCityState();
	// Turns the League countdown is pulled forward per hasten purchase (0 = disabled)
	int GetAidPointHastenCongressTurns();
	// Share of a city-state treasury seized when a major bullies gold (percent, 0 = disabled)
	int GetBullyTreasurySharePercent();
	// Every unit type a city-state may gift to ePlayer (same filter as GetCompetitiveSpawnUnitType,
	// but returns all valid candidates for the aid-point unit picker UI)
	void GetMinorGiftUnitChoices(PlayerTypes ePlayer, std::vector<UnitTypes>& veChoices);
	// How many city-states currently reserve a pending gift unit of eUnitClass for eMajor. A limited
	// (national) unit class can only be delivered once, so these reservations share its slots.
	int GetPendingGiftUnitCountForClass(PlayerTypes eMajor, UnitClassTypes eUnitClass) const;

	void DoResearchAgreementNotification(TeamTypes eTeam1, TeamTypes eTeam2);

	int GetResearchAgreementCost(PlayerTypes ePlayer1, PlayerTypes ePlayer2);

	// Victory Stuff
	void DoTestConquestVictory();

	PlayerTypes GetBestWondersPlayer();
	void SetBestWondersPlayer(PlayerTypes ePlayer, int iWonderCount);
	PlayerTypes GetBestPoliciesPlayer();
	void SetBestPoliciesPlayer(PlayerTypes ePlayer, int iPolicyCount);
	PlayerTypes GetBestGreatPeoplePlayer();
	void SetBestGreatPeoplePlayer(PlayerTypes ePlayer, int iGreatPeopleCount);

	bool IsEndGameTechResearched() const;
	void SetEndGameTechResearched(bool bValue);
	// End Victory Stuff

	// Returns true if the tuner has ever been connected at any point during this game.
	bool TunerEverConnected() const;

	// slewis - Tutorial stuff
	bool IsEverAttackedTutorial() const;
	void SetEverAttackedTutorial(bool bValue);
	bool IsEverRightClickMoved() const;
	void SetEverRightClickMoved(bool bValue);
	bool IsCombatWarned() const;
	void SetCombatWarned(bool bValue);
	// end Tutorial stuff

	CvAdvisorCounsel* GetAdvisorCounsel();
	CvAdvisorRecommender* GetAdvisorRecommender();

	int GetTurnsBetweenMinorCivElections();
	int GetTurnsUntilMinorCivElection();

	void LogGameState(bool bLogHeaders = false);
	void unitIsMoving();

	void BuildProdModHelpText(CvString* toolTipSink, const char* strTextKey, int iMod, const char* strExtraKey = "") const;
	void BuildCannotPerformActionHelpText(CvString* toolTipSink, const char* strTextKey, const char* strExtraKey1 = "", const char* strExtraKey2 = "", int iValue = -666) const;

	bool allUnitAIProcessed() const;

	void updateTurnTimer();
	bool hasTurnTimerExpired(PlayerTypes playerID);
	void TurnTimerSync(float fCurTurnTime, float fTurnStartTime);
	void GetTurnTimerData(float& fCurTurnTime, float& fTurnStartTime);

	int GetDealDuration();
	int GetPeaceDuration();

	CombatPredictionTypes GetCombatPrediction(const CvUnit* pAttackingUnit, const CvUnit* pDefendingUnit);

	void NetMessageStaticsReset();
	void SetLastTurnAICivsProcessed();

	void TriggerArchaeologySiteCreation(bool bCheckInitialized);
	bool IsArchaeologyTriggered() const;
	int GetNumArchaeologySites() const;
	int GetNumHiddenArchaeologySites() const;

#if defined(MOD_API_EXTENSIONS)
	bool AnyoneHasBelief(BeliefTypes iBeliefType) const;
	bool AnyoneHasBuilding(BuildingTypes iBuildingType) const;
	bool AnyoneHasBuildingClass(BuildingClassTypes iBuildingClassType) const;
	bool AnyoneHasAnyWonder() const;
	bool AnyoneHasWonder(BuildingTypes iBuildingType) const;
	int GetCivilizationPlayer(CivilizationTypes iCivilizationType) const;
	bool AnyoneIsInEra(EraTypes iEraType) const;
	bool AnyoneHasReachedEra(EraTypes iEraType) const;
	bool AnyoneHasAnyNaturalWonder() const;
	bool AnyoneHasNaturalWonder(FeatureTypes iFeatureType) const;
	bool AnyoneHasPolicy(PolicyTypes iPolicyType) const;
	bool AnyoneHasTenet(PolicyTypes iPolicyType) const;
	bool AnyoneHasPolicyBranch(PolicyBranchTypes iPolicyBranchType) const;
	bool AnyoneHasIdeology(PolicyBranchTypes iPolicyBranchType) const;
	bool AnyoneHasProject(ProjectTypes iProjectType) const;
	bool AnyoneHasPantheon() const;
	bool AnyoneHasAnyReligion() const;
	bool AnyoneHasReligion(ReligionTypes iReligionType) const;
	bool IsResolutionPassed(ResolutionTypes iResolutionType, int iChoice) const;
	bool AnyoneHasTech(TechTypes iTechType) const;
	bool AnyoneHasUnit(UnitTypes iUnitType) const;
	bool AnyoneHasUnitClass(UnitClassTypes iUnitClassType) const;
#endif
public:

	//Function to determine city size from city population
	unsigned int GetVariableCitySizeFromPopulation(unsigned int nPopulation);

#ifdef MOD_API_MP_PLOT_SIGNAL
	uint64 GetLastMPSignalInvokeTime() { return m_uiLastMPSignalInvokeTime; }
	void SetLastMPSignalInvokeTime(uint64 uiNewValue) { m_uiLastMPSignalInvokeTime = uiNewValue; }
	void GenerateMPSignalNotification(PlayerTypes iFromPlayer, int iPlotX, int iPlotY);
#endif // MOD_API_MP_PLOT_SIGNAL

	// Transient suppression window for the whole of CvPlayer::acquireCity() so the
	// empire-wide happiness/religion/corruption publishes (DoUpdateHappiness /
	// UpdateReligion / UpdateCorruption) are deferred to the end of the acquisition.
	// Lives on the singleton game object so it is shared across every player touched
	// during the acquisition (teardown of the old owner's city included).
	// Single-threaded game logic, so no locking is needed. Not serialized.
	// Reference-counted so nested windows (e.g. a Lua hook capturing another city
	// mid-acquisition) compose safely instead of collapsing the outer window.
	// The window is opened/closed exclusively via the RAII SuppressHappinessUpdateGuard
	// nested class declared at the end of this class.
	bool IsSuppressingHappinessUpdate() const { return m_iSuppressHappinessUpdate > 0; }


	//------------------------------------------------------------
	//------------------------------------------------------------
	//------------------------------------------------------------

private:
	//------------------------------------------------------------
	// Convert from city population to discrete size
	//------------------------------------------------------------
	const static unsigned int ms_aiSizes[10];

protected:

	static int m_iSuppressHappinessUpdate; // transient acquireCity suppression counter (see IsSuppressingHappinessUpdate). CvGame is a singleton, so a static counter matches its lifetime; defined & reset in CvGame.cpp
	int m_iEndTurnMessagesSent;
	int m_iElapsedGameTurns;
	int m_iStartTurn;
#if defined(MOD_SP_UNIQUE_CITYSTATE)
	int m_iEconomicAidRound;
	int m_iEconomicAidRoundStartTurn;
	int m_iEconomicAidWorldEra;
	bool m_bEconomicAidActive;
	bool m_bMasterSpyQuestAwarded;
#endif
	mutable EraTypes m_eCurrentEraCache;
	mutable int m_iCurrentEraCacheTurn;
	int m_iWinningTurn;
	int m_iStartYear;
	int m_iEstimateEndTurn;
	int m_iDefaultEstimateEndTurn;
	int m_iTurnSlice;
	int m_iCutoffSlice;
	int m_iNumCities;
	int m_iTotalPopulation;
	int m_iNoNukesCount;
	int m_iNukesExploded;
	int m_iMaxPopulation;
	int m_iUnused1;  //unused
	int m_iUnused2;  //unused
	int m_iUnused3;  //unused
	int m_iInitPopulation;
	int m_iInitLand;
	int m_iInitTech;
	int m_iInitWonders;
	int m_iAIAutoPlay;
	int m_iTotalReligionTechCost;
	int m_iCachedWorldReligionTechProgress;
	int m_iUnitedNationsCountdown;
	int m_iNumVictoryVotesTallied;
	int m_iNumVictoryVotesExpected;
	int m_iVotesNeededForDiploVictory;
	int m_iMapScoreMod;

#if defined(MOD_NUCLEAR_WINTER_FOR_SP)
	int m_iNuclearWinterProcess;
	int m_iNuclearWinterNaturalReduction;
	int m_iNuclearWinterLevelIndex;
#endif
#if defined(MOD_GLOBAL_MAX_PLOT_BUILD)
	std::tr1::unordered_map<int, int> m_mapPlotBuildNum;
#endif

	unsigned int m_uiInitialTime;

	bool m_bScoreDirty;
	bool m_bCircumnavigated;
	bool m_bDebugMode;
	bool m_bDebugModeCache;
	bool m_bFinalInitialized;
	bool m_bPolicyFreeBuildingsRebuilt;
	bool m_bPbemTurnSent;
	bool m_bHotPbemBetweenTurns;
	bool m_bPlayerOptionsSent;
	bool m_bNukesValid;
	bool m_bEndGameTechResearched;
	bool m_bTunerEverConnected;
	bool m_bDynamicTurnsSimultMode;		//if playing dynamic turn mode, are we currently running simultaneous turns?
	PlayerTypes m_eWaitDiploPlayer;

	bool m_bFOW;

	// slewis - tutorial values
	bool m_bStaticTutorialActive;
	bool m_bTutorialEverAttacked;
	bool m_bEverRightClickMoved;
	bool m_bCombatWarned;
	std::tr1::unordered_set<std::string> m_AdvisorMessagesViewed;
	// slewis - tutorial values

	HandicapTypes m_eHandicap;
	PlayerTypes m_ePausePlayer;
	PlayerTypes m_eAIAutoPlayReturnPlayer;
	UnitTypes m_eBestLandUnit;
	TeamTypes m_eWinner;
	VictoryTypes m_eVictory;
	GameStateTypes m_eGameState;
	PlayerTypes m_eEventPlayer;
	PlayerTypes m_eBestWondersPlayer;
	PlayerTypes m_eBestPoliciesPlayer;
	PlayerTypes m_eBestGreatPeoplePlayer;
	TechTypes m_eReligionTech;
	RouteTypes m_eIndustrialRoute;
	bool m_bArchaeologyTriggered;

	CvString m_strScriptData;

	int* m_aiEndTurnMessagesReceived;
	int* m_aiRankPlayer;        // Ordered by rank...
	int* m_aiPlayerRank;        // Ordered by player ID...
	int* m_aiPlayerScore;       // Ordered by player ID...
	int* m_aiRankTeam;						// Ordered by rank...
	int* m_aiTeamRank;						// Ordered by team ID...
	int* m_aiTeamScore;						// Ordered by team ID...

	int* m_paiUnitCreatedCount;
	int* m_paiUnitClassCreatedCount;
	int* m_paiBuildingClassCreatedCount;
	int* m_paiProjectCreatedCount;
	PlayerVoteTypes* m_paiVoteOutcome;
	int* m_aiSecretaryGeneralTimer;
	int* m_aiVoteTimer;
	int* m_aiDiploVote;
	int* m_aiVotesCast;
	int* m_aiPreviousVotesCast;
	int* m_aiNumVotesForTeam;

	int* m_aiTeamCompetitionWinnersScratchPad;

	bool* m_pabSpecialUnitValid;

	int** m_apaiPlayerVote;
	int** m_ppaaiTeamVictoryRank;

#ifdef MOD_API_MP_PLOT_SIGNAL
	uint64 m_uiLastMPSignalInvokeTime;
#endif // MOD_API_MP_PLOT_SIGNAL


	Database::Results* m_pDiploResponseQuery;

	std::vector<CvString> m_aszDestroyedCities;
	std::vector<CvString> m_aszGreatPeopleBorn;

	FFreeListTrashArray<VoteSelectionData> m_voteSelections;
	FFreeListTrashArray<VoteTriggeredData> m_votesTriggered;

	CvRandom m_mapRand;
	CvRandom m_jonRand;

	typedef std::vector<CvReplayMessage> ReplayMessageList;
	ReplayMessageList m_listReplayMessages;

	int m_iNumSessions;

	std::vector<PlotExtraYield> m_aPlotExtraYields;
	std::vector<PlotExtraCost> m_aPlotExtraCosts;
	std::map<int, std::string> m_mapPlotNames;

	// Project first completion: ProjectType -> PlayerType, ProjectType -> CityName
	std::map<int, int> m_mapProjectFirstPlayer;
	std::map<int, std::string> m_mapProjectFirstCityName;

	int m_iEarliestBarbarianReleaseTurn;

	CvSiteEvaluatorForSettler* m_pSettlerSiteEvaluator;
	CvSiteEvaluatorForStart*   m_pStartSiteEvaluator;
	CvStartPositioner*         m_pStartPositioner;
	CvGameReligions*           m_pGameReligions;
	CvGameCulture*             m_pGameCulture;
	CvGameLeagues*             m_pGameLeagues;
	CvGameTrade*               m_pGameTrade;
	CvTacticalAnalysisMap*     m_pTacticalMap;

	CvAdvisorCounsel*          m_pAdvisorCounsel;
	CvAdvisorRecommender*      m_pAdvisorRecommender;

	CvGameDeals                m_kGameDeals;

	//necessary because we only want to hide the mouseover of the most recently moused over unit -KS
	int                        m_iLastMouseoverUnitID;

	// CACHE: cache frequently used values

	FTimer  m_endTurnTimer;
	int     m_endTurnTimerSemaphore;
	int     m_lastTurnAICivsProcessed;
	bool    m_firstActivationOfPlayersAfterLoad;
	FTimer  m_curTurnTimer;
	FTimer  m_timeSinceGameTurnStart;		//time since game turn started for human players
	float	m_fCurrentTurnTimerPauseDelta;	//
	bool    m_sentAutoMoves;
	bool	m_bForceEndingTurn;

	void initDiplomacy();
	void initFreeState(CvGameInitialItemsOverrides& kOverrides);
	void initFreeUnits(CvGameInitialItemsOverrides& kOverrides);

	bool InitMap(CvGameInitialItemsOverrides& kGameInitialItemsOverrides);

	void assignStartingPlots();

	void setInitialItems(CvGameInitialItemsOverrides& kOverrides);
	void CheckGenerateArchaeology();
	void regenerateMap();

	void testExtendedGame();

	bool IsForceEndingTurn() const;
	void SetForceEndingTurn(bool bValue);

	void DoCacheMapScoreMod();

	void doTurn();

	// Rebuilds the Policy_FreeBuildingClass* grants for every City once per load or new
	// game. Policies are restored from the save without going through processPolicies,
	// so the free slots have to be reconciled explicitly.
	void RebuildPolicyFreeBuildings();

	void updateWar();
	void updateMoves();
	void updateTimers();
	void UpdatePlayers();
	void testAlive();

	void showEndGameSequence();

	CvPlot* normalizeFindLakePlot(PlayerTypes ePlayer);

	void doUpdateCacheOnTurn();

	void CheckPlayerTurnDeactivate();

	void PopulateDigSite(CvPlot& kPlot, EraTypes eEra, GreatWorkArtifactClass eArtifact);
	void SpawnArchaeologySitesHistorically();


	// RAII scope guard for the suppression window - the ONLY way to open/close a
	// window. Constructing it opens a window; the destructor always closes it, so the
	// counter can never be left > 0 on any exit path. Call Exit() explicitly right
	// before the deferred publish at the end of the outermost window; the destructor
	// is then a no-op. Declared at the end of the class so m_iSuppressHappinessUpdate
	// is already visible (nested classes can touch outer-class members, but only ones
	// declared before them).
public:
	class SuppressHappinessUpdateGuard
	{
	public:
		SuppressHappinessUpdateGuard() : m_bExited(false) { ++m_iSuppressHappinessUpdate; }
		~SuppressHappinessUpdateGuard() { Exit(); }
		void Exit()
		{
			if (!m_bExited)
			{
				m_bExited = true;
				if (m_iSuppressHappinessUpdate > 0) --m_iSuppressHappinessUpdate;
			}
		}
		SuppressHappinessUpdateGuard(const SuppressHappinessUpdateGuard&) = delete;
		SuppressHappinessUpdateGuard& operator=(const SuppressHappinessUpdateGuard&) = delete;
	private:
		bool m_bExited;
	};

};

#endif
