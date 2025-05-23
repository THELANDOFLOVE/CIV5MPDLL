/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CIV5_PROJECT_CLASSES_H
#define CIV5_PROJECT_CLASSES_H

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvProjectEntry
//!  \brief		A single project available in the game
//
//!  Key Attributes:
//!  - Used to be called CvProjectInfo
//!  - Populated from XML\GameInfo\CIV5ProjectInfo.xml
//!  - Array of these contained in CvProjectXMLEntries class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvProjectEntry: public CvBaseInfo
{
public:
	CvProjectEntry(void);
	~CvProjectEntry(void);

	virtual bool CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility);

	int GetVictoryPrereq() const;
	int GetTechPrereq() const;
	int GetPolicyBranchPrereq() const;
	int GetAnyoneProjectPrereq() const;
	void SetAnyoneProjectPrereq(int i);
	int GetMaxGlobalInstances() const;
	int CityMaxNum() const;
	int GetMaxTeamInstances() const;
	int GetProductionCost() const;
	int GetNukeInterception() const;
	int GetCultureBranchesRequired() const;
	int GetTechShare() const;
	int GetEveryoneSpecialUnit() const;
	int GetVictoryDelayPercent() const;
	int GetFlavorValue(int i) const;
	bool IsSpaceship() const;
	bool IsAllowsNukes() const;
	bool IsNoBroadcast() const;
	int CostScalerNumberOfRepeats() const;
	int GetGoldMaintenance() const;
	int CostScalerEra() const;
	const char* GetMovieArtDef() const;

	const char* GetCreateSound() const;
	void SetCreateSound(const char* szVal);
	int GetFreePromotion() const;

	// Arrays
	int GetResourceQuantityRequirement(int i) const;
	int GetVictoryThreshold(int i) const;
	int GetVictoryMinThreshold(int i) const;
	int GetProjectsNeeded(int i) const;

	int GetYieldChange(int i) const;
	int* GetYieldChangeArray() const;
	int GetYieldModifier(int i) const;;
	int* GetYieldModifierArray() const;
	const std::vector<PolicyTypes>& GetPolicyNeeded() const;

protected:
	int m_iVictoryPrereq;
	int m_iTechPrereq;
	int m_iPolicyBranchPrereq = NO_POLICY_BRANCH_TYPE;
	int m_iAnyoneProjectPrereq;
	int m_iMaxGlobalInstances;
	int m_iCityMaxNum;
	int m_iMaxTeamInstances;
	int m_iProductionCost;
	int m_iNukeInterception;
	int m_iCultureBranchesRequired;
	int m_iTechShare;
	int m_iEveryoneSpecialUnit;
	int m_iVictoryDelayPercent;

	bool m_bSpaceship;
	bool m_bAllowsNukes;
	bool m_bNoBroadcast;
	int m_iGoldMaintenance;
	int m_iCostScalerEra;
	int m_iCostScalerNumRepeats;
	CvString m_strCreateSound;
	CvString m_strMovieArtDef;
	int m_iFreePromotion;

	// Arrays
	int* m_piResourceQuantityRequirements;
	int* m_piVictoryThreshold;
	int* m_piVictoryMinThreshold;
	int* m_piProjectsNeeded;
	int* m_piFlavorValue;

	int* m_piYieldChange;
	int* m_piYieldModifier;
	std::vector<PolicyTypes> m_vePolicyNeeded;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvProjectXMLEntries
//!  \brief		Game-wide information about projects
//
//! Key Attributes:
//! - Plan is it will be contained in CvGameRules object within CvGame class
//! - Populated from XML\GameInfo\CIV5ProjectInfo.xml
//! - Contains an array of CvProjectEntry from the above XML file
//! - One instance for the entire game
//! - Accessed heavily by the [what stores info on projects built?] class
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvProjectXMLEntries
{
public:
	CvProjectXMLEntries(void);
	~CvProjectXMLEntries(void);

	// Accessor functions
	std::vector<CvProjectEntry*>& GetProjectEntries();
	int GetNumProjects();
	_Ret_maybenull_ CvProjectEntry* GetEntry(int index);

	void DeleteArray();

private:
	std::vector<CvProjectEntry*> m_paProjectEntries;
};

#endif //CIV5_PROJECT_CLASSES_H