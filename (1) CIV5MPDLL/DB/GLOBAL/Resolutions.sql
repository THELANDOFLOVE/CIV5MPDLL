/*ALTER TABLE Resolutions ADD COLUMN 'OpenDoor' boolean default 0;
ALTER TABLE Resolutions ADD COLUMN 'SphereOfInfluence' boolean default 0;
ALTER TABLE Resolutions ADD COLUMN 'Decolonization' boolean default 0;
ALTER TABLE Resolutions ADD COLUMN 'SpaceshipProductionMod' integer default 0;
ALTER TABLE Resolutions ADD COLUMN 'SpaceshipPurchaseMod' integer default 0;
ALTER TABLE Resolutions ADD COLUMN 'IsWorldWar' integer default 0;*/
ALTER TABLE Resolutions ADD COLUMN 'EmbargoIdeology' boolean default 0;
ALTER TABLE Resolutions ADD COLUMN 'GlobalAttackModifier' integer default 0;
ALTER TABLE Resolutions ADD COLUMN 'GlobalWarCasualtiesChanges' integer default 0;
-- Vassal/Suzerain submission resolution columns (all default 0 for reuse)
ALTER TABLE Resolutions ADD COLUMN 'VassalTaxPercent' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalTechDiscount' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalTaxScience' BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalTaxCulture' BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalTaxFaith'   BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalTaxGold'    BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalForcePeace'  BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalNoDenounce'  BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalGetUC'       BOOLEAN DEFAULT 0;
-- Vassal suzerain condition: whether to count allied city-state / vassal stats when checking the 2x requirement
ALTER TABLE Resolutions ADD COLUMN 'VassalCountPermanentAllyCities'      BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalCountVassalCities'             BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalCountPermanentAllyPopulation'  BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalCountVassalPopulation'         BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalCountVassalMilitaryMight'      BOOLEAN DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalPermanentAllyCitiesPercent'     INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalVassalCitiesPercent'            INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalPermanentAllyPopulationPercent' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalVassalPopulationPercent'        INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalVassalMilitaryMightPercent'     INTEGER DEFAULT 0;
-- International law protection: marks a CITY_CSD resolution that places a city-state under international law protection
ALTER TABLE Resolutions ADD COLUMN 'InternationalLawProtection' BOOLEAN DEFAULT 0;
-- World Bank: treasury interest every 10 turns, plus a per-international-trade-route bonus (basis points)
ALTER TABLE Resolutions ADD COLUMN 'WorldBankInterestBasisPoints' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'WorldBankRouteInterestBasisPoints' INTEGER DEFAULT 0;
-- World Health Organization: percent modifier to per-citizen food consumption, and the surplus
-- percentage a city keeps when it lacks the sanitation building required for its scale
ALTER TABLE Resolutions ADD COLUMN 'WHOFoodConsumptionModifier' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'WHOSanitationFoodKeptPercent' INTEGER DEFAULT 0;
-- International Maritime Organization: extra gold percent on ocean international trade routes,
-- and forbids weak civs (minor/barbarian/below-average score) from plundering them
ALTER TABLE Resolutions ADD COLUMN 'IMOOceanRouteGoldModifier' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'IMOForbidWeakPlunder' BOOLEAN DEFAULT 0;
-- World Bank: the interval in turns between interest payments (0 falls back to 10 in code)
ALTER TABLE Resolutions ADD COLUMN 'WorldBankIntervalTurns' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalDemandCityPopulationPercent' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalDemandGoldPercent' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalDemandGoldPerTurnPercent' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalDemandLuxuryResourcePercent' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalDemandStrategicResourcePercent' INTEGER DEFAULT 0;
ALTER TABLE Resolutions ADD COLUMN 'VassalDemandCooldownTurns' INTEGER DEFAULT 0;
