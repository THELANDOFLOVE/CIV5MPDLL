-- ******************** UNIQUE CITYSTATES：DiplomaticPrestige******************** --
alter table Buildings add DiplomaticPrestige int default 0;
alter table Policies  add DiplomaticPrestige int default 0;
alter table Traits    add DiplomaticPrestige int default 0;
-- League project rewards: one-shot Diplomatic Prestige (e.g. World Trade Organization tier 2)
alter table LeagueProjectRewards add DiplomaticPrestige int default 0;

-- Diplomatic Overextension Penalty Ratios (GlobalDefines, Rule 20)
INSERT INTO Defines (Name, Value) VALUES ('DIPLOMATIC_OVEREXTENSION_DECAY_MODIFIER', 10);
INSERT INTO Defines (Name, Value) VALUES ('DIPLOMATIC_OVEREXTENSION_RISE_MODIFIER', -10);
INSERT INTO Defines (Name, Value) VALUES ('DIPLOMATIC_OVEREXTENSION_UNHAPPINESS_MODIFIER', 3);

-- City-State UA Basic Effects (GlobalDefines, per-ally modifier values)
-- Cultured
INSERT INTO Defines (Name, Value) VALUES ('CS_CULTURED_POLICY_COST_MODIFIER', -2);
INSERT INTO Defines (Name, Value) VALUES ('CS_CULTURED_IMMIGRATION_REGRESSAND_MODIFIER', -2);
-- Militaristic
INSERT INTO Defines (Name, Value) VALUES ('CS_MILITARISTIC_LAND_XP_PER_TURN', 1);
-- Maritime
INSERT INTO Defines (Name, Value) VALUES ('CS_MARITIME_SEA_TRADE_GOLD_PER_ERA', 1);
-- Religious
INSERT INTO Defines (Name, Value) VALUES ('CS_RELIGIOUS_FAITH_COST_MODIFIER', -5);
INSERT INTO Defines (Name, Value) VALUES ('CS_RELIGIOUS_PRESSURE_MODIFIER', 10);
-- Mercantile
INSERT INTO Defines (Name, Value) VALUES ('CS_MERCANTILE_LUXURY_HAPPINESS_MODIFIER', 5);
INSERT INTO Defines (Name, Value) VALUES ('CS_MERCANTILE_TREASURY_INTEREST_RATE', 1);
INSERT INTO Defines (Name, Value) VALUES ('CS_TREASURY_INTEREST_CAP_MULTIPLIER', 100);

-- Economic Aid (Super Power V11): base length of one global aid round, scaled by game speed
INSERT INTO Defines (Name, Value) VALUES ('ECONOMIC_AID_ROUND_LENGTH', 20);


-- MinorCivAlliesThresholdExtra: per-era ally threshold increase (Rule 8)
alter table Eras add column MinorCivAlliesThresholdExtra int default 0;

-- MinorCivAlliesThresholdModifier: per-building/policy/trait threshold modifier
alter table Buildings add column MinorCivAlliesThresholdModifier int default 0;
alter table Policies  add column MinorCivAlliesThresholdModifier int default 0;
alter table Traits    add column MinorCivAlliesThresholdModifier int default 0;

-- GainConqueredCityStateUA: the player PERMANENTLY keeps the ally-tier UA effect of any city-state
-- whose ORIGINAL capital they have ever conquered. The conquest is recorded once (CvPlayer::
-- m_abConqueredCityStateUA, set in acquireCity) and is independent of later city ownership, of the
-- city-state still being alive and of the current diplomatic relationship. Judged on the original
-- capital's coordinates (not bCapital), so a city-state that owns a second city is not misdetected.
-- Only genuine conquest counts; gifts and the Austria/Venice buyout do not trigger it.
alter table Traits add column GainConqueredCityStateUA boolean default 0;

-- GainAnnexedCityStateUA: the player PERMANENTLY keeps the ally-tier UA effect of any city-state it
-- ANNEXES through the gold buyout (the Austria / Diplomatic Marriage UA, CvMinorCivAI::DoBuyout).
-- Recorded once in DoBuyout before DoAcquire runs (the city-state ceases to exist at that point) into
-- CvPlayer::m_abBoughtCityStateAllyUA - deliberately the SAME permanent record the Venice-style unit
-- buyout (GainAllyCityStateUAOnBuy) writes, because both paths mean exactly the same thing: the
-- city-state was bought out, so its ally-tier effect stays ours forever. Independent of the
-- conquest-based GainConqueredCityStateUA.
alter table Traits add column GainAnnexedCityStateUA boolean default 0;

-- GainAllyCityStateUAOnBuy / GainFriendCityStateUAOnBuy: a unit with CanBuyCityState that performs
-- the Venice-style buyout PERMANENTLY keeps the bought city-state's ally/friend-tier UA effect.
-- The buyout is recorded once (CvPlayer::m_abBoughtCityStateAllyUA / m_abBoughtCityStateFriendUA,
-- set in CvUnit::buyCityState) so the effect survives the city-state's disappearance - a bought-out
-- city-state is gone for good, so its effect can never come back through diplomacy. Independent of
-- the trait-based conquest path (GainConqueredCityStateUA).
alter table Units add column GainAllyCityStateUAOnBuy boolean default 0;
alter table Units add column GainFriendCityStateUAOnBuy boolean default 0;

-- ==================== Unique CityState UA System (Rule 1-18) ====================

-- Effects definition table (internal data, not shown to players): one row per ally or friend effect
-- Columns are grouped and annotated by source city-state; 0 means the effect does not apply
CREATE TABLE CityStateUAEffects (
    ID                                              INTEGER PRIMARY KEY AUTOINCREMENT,
    Type                                            TEXT NOT NULL UNIQUE,
    Help                                            TEXT DEFAULT NULL REFERENCES Language_en_US(Tag),
    -- Florence: reduces the incremental rise of faith-bought great people cost (applies to the increment only, not the full cost)
    FaithPurchaseGreatPeopleCostRiseModifier       integer DEFAULT 0,
    FaithPurchaseGreatPeopleCostRiseModifierPerGW  integer DEFAULT 0,
    FaithPurchaseAllGreatPeople                     boolean DEFAULT 0,
    -- Buenos Aires: great musician does not die after a great work and retains concert tourism
    GPNoDeathAfterGreatWork                         boolean DEFAULT 0,
    GPConcertTourismRetentionPercent                integer DEFAULT 0,
    -- Brussels: special great musician concert modifiers
    GreatMusicianConcertGoldPercent                 integer DEFAULT 0,
    -- Bratislava: capital and second capital
    CapitalAndSecondCapitalCultureModifier          integer DEFAULT 0,
    -- Bucharest: international migration
    ImmigrationRatePerImmigrant                     integer DEFAULT 0,
    ImmigrationRateMax                              integer DEFAULT 0,
    EmigrationRatePerImmigrant                      integer DEFAULT 0,
    EmigrationRateMax                               integer DEFAULT 0,
    -- Kuala Lumpur: puppet city science threshold
    PuppetNoTechCostPenalty                         boolean DEFAULT 0,
    PuppetTechCostPartial                           integer DEFAULT 0,
    -- Almaty: the ally/friend's units may pillage trade routes of players they are NOT at war with
    CanPillageNeutralTradeRoute                     boolean DEFAULT 0,
    -- Almaty: extra gold granted to the ally/friend for plundering ANY trade route, on top of the vanilla
    -- plunder gold. This is the per-era base value (200); the actual amount is multiplied by the unified
    -- era coefficient (设计全局规则 14): GetCurrentEra() + 1 (Ancient x1 ... Future x10).
    PlunderTradeRouteGold                           integer DEFAULT 0,
    -- Almaty: extra XP granted to the ally/friend's unit that plunders a trade route.
    PlunderTradeRouteXP                             integer DEFAULT 0,
    -- Almaty: opinion weight penalty (visible red face) applied to the plundered player per NEUTRAL plunder
    -- only (plundering a player currently at war adds none). The penalty is a flat sum that decays by 1 per
    -- turn; it is never reset by peace or by anything else. 10 = -10 per plunder.
    PlunderTradeRouteOpinionPenalty                 integer DEFAULT 0,
    -- Belgrade: garrison city defense
    GarrisonCityDefenseModifier                     integer DEFAULT 0,
    -- Belgrade: ally-built military units gain XP (purchases do not apply)
    MilitaryUnitProductionXP                        integer DEFAULT 0,
    -- Belgrade: bonus ZOC range for the ally's units (1 = the ally's units exert Zone of Control
    -- at radius 2 instead of 1). Plain integer; 0 = base behavior.
    ZOCRangeBonus                                   integer DEFAULT 0,
    -- Budapest: immune to river crossing penalties
    LandUnitsImmuneRiverCrossing                    boolean DEFAULT 0,
    -- Budapest: the ally's units deal this much extra flat HP damage against a wounded target (both when attacking and when defending)
    WoundedFixedDamage                              integer DEFAULT 0,
    -- Hanoi: fixed damage in borders + peace treaty + being declared war on
    EnemyFixedDamageModifierInBorders               integer DEFAULT 0,
    CulturePerWarPeace                              integer DEFAULT 0,
    EnemyCombatModifierInBordersPerBeenDoW          integer DEFAULT 0,
    -- Mbanza-Kongo: city-count related
    UnitProductionModifierPerCity                   integer DEFAULT 0,
    CombatBonusPerTechDifference                    integer DEFAULT 0,
    -- Sidon: attacker ignores this % of the defended city's building defense (30 ally / 15 friend)
    CityAttackIgnoreBuildingDefensePercent          integer DEFAULT 0,
    -- Sidon: % modifier on the per-turn XP granted by allied militaristic city-states (100 = +100%)
    MilitaryXPPerTurnModifier                       integer DEFAULT 0,
    -- Sidon: when non-zero, the militaristic city-state per-turn XP also applies to sea and air domains
    MilitaryXPSeaAir                                integer DEFAULT 0,
    -- Vatican City: religion spread speed
    ReligionSpreadSpeedModifier                     integer DEFAULT 0,
    -- Vatican City: Papal Recognition - league delegate votes granted to each civilization with a majority of cities following the ally's religion (mainstream votes, 1 = +1 vote)
    PapalRecognitionVotes                           integer DEFAULT 0,
    -- Vatican City: Papal Recognition - league delegate votes granted to the ally per following civilization (including itself, 1 = +1 vote per follower)
    PapalRecognitionAllyVotes                       integer DEFAULT 0,
    -- Jerusalem: +X% religious pressure of the founder's religion per holy city owned by the ally/friend
    ReligiousPressureModifierPerHolyCity            integer DEFAULT 0,
    -- Jerusalem: player who is the ally of this city-state cannot be denounced
    DenounceImmunity                                boolean DEFAULT 0,
    -- Kyzyl: trade route cap -> trade route distance (plain percent per trade-route slot; 10 = +10% per slot)
    LandTradeRouteDistancePerTradeSlot              integer DEFAULT 0,
    -- Kyzyl: trade route gold % when the destination player is not a neighbor (plain percent, 100 = +100%; city-states included)
    TradeRouteGoldPercentNonNeighbor                integer DEFAULT 0,
    -- Dubai: donation counting
    HappinessPerGoldDonated                         integer DEFAULT 0,
    GoldDonationInterval                            integer DEFAULT 0,
    WonderProductionPerDonationHappiness            integer DEFAULT 0,
    IdeologyPressurePerDonationHappiness            integer DEFAULT 0,
    -- Genoa: each sea trade route grants a bonus % to influence gained from gold donations (1 = +1% per sea route)
    GoldDonationInfluenceModifierPerSeaRoute        integer DEFAULT 0,
    -- Malacca: luxury happiness
    LuxuryHappinessModifier                         integer DEFAULT 0,
    -- Malacca: food surplus per happy luxury type (value = per-type food% * 100; 每1=100, 每2=50)
    FoodKeptModifierPerLuxury                           integer DEFAULT 0,
    -- Malacca: trade route gold per luxury type (value = per-type gold% * 100; 每1=100)
    TradeRouteGoldModifierPerLuxuryType             integer DEFAULT 0,
    -- Panama: international trade route gold per distance tile (value = per-tile gold% * 100; 每1=100, 每2=50)
    TradeRouteGoldModifierPerDistance               integer DEFAULT 0,
    -- Panama: population unhappiness reduction per cross-continental trade route (value = per-route % * 100; 每1=100, cap 90)
    UnhappinessReductionPerCrossContinentRoute      integer DEFAULT 0,
    -- Manila: flat gold % on the ally's international trade routes (plain percent, 25 = +25%; ally 25 / friend 10)
    TradeRouteGoldPercentInternational              integer DEFAULT 0,
    -- Manila: gold % on the ally's international trade routes per international route the ally runs
    -- (plain percent, 2 = +2% per route; applies to every international route, city-state destinations included)
    TradeRouteGoldModifierPerInternationalRoute     integer DEFAULT 0,
    -- Manila: nation-wide food % per happy luxury type the ally owns (plain percent, 2 = +2% per luxury type)
    FoodModifierPerHappyLuxuryType                  integer DEFAULT 0,
    -- Manila: cap on the food % above (plain percent, 50 = at most +50%; 0 = uncapped)
    FoodModifierPerHappyLuxuryCap                   integer DEFAULT 0,
    -- Mogadishu: a research agreement broken because the OTHER side declared war grants the non-initiator
    -- this percent of the normal completion bonus (plain percent, 200 = 200% of normal = double)
    ResearchAgreementBreakBonusPercent              integer DEFAULT 0,
    -- Valletta: enemy city besieged by >= this many of our combat units cannot heal
    EnemyCityNoHealBesiegeCount                      integer DEFAULT 0,
    -- Prague: killing an enemy spy grants spy progress toward a new spy (100 = kill 1 gain 1, 20 = kill 5 gain 1)
    SpyKillGainSpyProgress                           integer DEFAULT 0,
    -- Antananarivo: coastal city food growth threshold modifier (ordinary modifier, -20 = -20%)
    CoastalCityGrowthThresholdModifier              integer DEFAULT 0,
    -- Antananarivo: diplomatic prestige per city (value = prestige * 100; 10 = 0.1 prestige per city)
    DiplomaticPrestigePerCity                       integer DEFAULT 0,
    -- Vilnius: Mod-type golden-age threshold change per population (applied before percentage modifiers; negative = lower threshold, -100 = -1 per 1 population)
    GoldenAgeThresholdPerPopulation                 integer DEFAULT 0,
    -- Sofia: coup/espionage spy UA (columns on the effect table, one row per ally/friend effect)
    CoupChanceModifier          integer DEFAULT 0,  -- +% coup success, may exceed the 85% cap (ally 30 / friend 12)
    CoupFailSpySurvives         boolean DEFAULT 0,  -- failed coup keeps the spy alive (ally only)
    StealTechSpeedPerSpy        integer DEFAULT 0,  -- +% steal-tech speed per alive spy (ally only)
    SpyKillChancePerSpy         integer DEFAULT 0,  -- +% chance to catch/kill enemy spies per alive spy (ally only)
    -- Gangtok: per city worldwide following the player's religion, global happiness (100 = +1 happiness per city)
    HappinessPerFollowingCity   integer DEFAULT 0,
    -- Gangtok: ally may buy influence at ANY city-state with faith, at (gold price / divisor) faith
    -- (divisor > 0 also enables the feature; 4 = 1/4 of the gold price)
    FaithInfluencePurchaseCostDivisor  integer DEFAULT 0,
    -- Gangtok: how many faith influence purchases the ally may make per turn (1 = once per turn, globally)
    FaithInfluencePurchasePerTurnLimit integer DEFAULT 0,
    -- Wittenberg: ally may buy a self-chosen belief and add it to the ally-led religion (once per major)
    FaithBeliefPurchase boolean DEFAULT 0,
    -- Wittenberg: when this religion is cleansed from a city-state city by an inquisitor/great prophet, keep this % of the followers
    InquisitorRetentionPercent integer DEFAULT 0,
    -- Wittenberg: the ally's inquisitors may remove heresy in city-state cities (including the adjacent-city check)
    InquisitorCanUseOnCityState boolean DEFAULT 0,
    -- La Venta: ally may buy an idle pantheon belief and add it to the ally-led religion (price doubles per purchase)
    FaithPantheonPurchase boolean DEFAULT 0,
    -- La Venta: +X% great-person rate per masterpiece/artifact the ally owns
    GreatPersonRateModifierPerGreatWork integer DEFAULT 0,
    -- Kathmandu: the first gold donation each turn refunds this % of the amount as faith to the ally
    FaithRefundPerDonationPercent integer DEFAULT 0,
    -- Geneva: diplomatic prestige per major civilization whose majority religion is the ally-led religion
    -- (value = prestige * 100 per civ; 50 = +1 prestige per 2 civs, integer division gives "per 2 civs +1")
    DiplomaticPrestigePerMajorityCiv integer DEFAULT 0,
    -- Geneva: per-turn influence with each met city-state, one unit per FollowingCityDivisor following cities
    -- (value = influence * 100; 100 = +1 influence per unit). Returned via GetFriendshipChangePerTurnTimes100,
    -- so it drives both the real DoFriendship settlement and the Lua influence-trend UI from the same source.
    InfluencePerTurnPerFollowCityMod integer DEFAULT 0,
    -- Geneva: how many cities following the ally-led religion produce one influence unit (3 = one per 3 cities)
    FollowingCityDivisor integer DEFAULT 0,
    -- Vancouver: global happiness per coastal city owned by the ally/friend (100 = +1 happiness per coastal city)
    CoastalCityHappiness integer DEFAULT 0,
    -- Yerevan: global happiness per worked holy-site improvement (IMPROVEMENT_HOLY_SITE), accumulated in GetHappinessFromMinorCivs
    -- (basis points, 100 = +1 global happiness per worked holy site; no local-population cap). Ally = 300.
    HolySiteHappiness integer DEFAULT 0,
    -- Kiev: +X% great-person rate per national wonder the ally/friend has completed (plain percent).
    -- The palace counts as a national wonder, so every player always has at least one.
    GreatPersonRateModifierPerNationalWonder integer DEFAULT 0,
    -- Kiev: extra League delegate votes per civilization the ally has a Declaration of Friendship with
    LeagueVotesPerDoF integer DEFAULT 0,
    -- Ur: global happiness per world wonder owned by the ally/friend, accumulated in GetHappinessFromMinorCivs
    -- (basis points, 100 = +1 global happiness per world wonder). Ally = 200, friend = 100.
    WorldWonderHappiness integer DEFAULT 0,
    -- Quebec: when another civilization computes its culture-victory progress against the ally/friend, the
    -- target's lifetime culture is inflated by this percent (50 = +50%), lowering the computed influence
    -- percentage and thus making culture domination of the target harder. Plain percent; read by
    -- CvPlayerCulture (GetInfluenceLevel and the other victory-progress denominators).
    CultureVictoryProgressModifier integer DEFAULT 0,
    -- Ragusa: percent modifier on a city's local-happiness cap (plain percent, 50 = cap x1.5). The base cap
    -- is the city population; consumed in CvCity::GetLocalHappiness.
    LocalHappinessCapModifier integer DEFAULT 0,
    -- Monaco: percent modifier on the ally's building gold maintenance while the ally is in a golden age
    -- (plain percent, -50 = -50%). Merged into the same pool as Policies.BuildingGoldMaintenanceMod in
    -- CvTreasury::GetBuildingGoldMaintenance; the final maintenance is floored at 0.
    GoldenAgeBuildingMaintenanceMod integer DEFAULT 0,
    -- Optional note appended to this city-state's gold-gift tooltip while the effect is active (e.g. a
    -- first-gift wager or refund). A Language text key; NULL/empty = no note. Keyed to the effect (hence
    -- the city-state), so the UI shows it only on the panel of a city-state that actually grants one.
    GoldGiftTooltip TEXT DEFAULT NULL REFERENCES Language_en_US(Tag)
);

-- UA type table (shown to players): pairs a city-state's ally and friend effects
-- Description = ally effect text  Help = friend effect text, combined in the UI layer
CREATE TABLE CityStateUAs (
    ID INTEGER PRIMARY KEY AUTOINCREMENT,
    Type TEXT NOT NULL UNIQUE,
    Description TEXT DEFAULT NULL REFERENCES Language_en_US(Tag),
    Help TEXT DEFAULT NULL REFERENCES Language_en_US(Tag),
    AllyEffectType TEXT NOT NULL REFERENCES CityStateUAEffects(Type),
    FriendEffectType TEXT NOT NULL REFERENCES CityStateUAEffects(Type)
);

-- AI personality attraction: how much a leader whose personality has the given flavor wants this
-- city-state's UA. Mirrors Belief_ExtraFlavors; the AI score is (Flavor * leader flavor) / 100.
create table CityStateUA_ExtraFlavors (
    CityStateUAType text references CityStateUAs(Type),
    FlavorType text references Flavors(Type),
    Flavor integer default 0 not null
);

-- AI victory preference: extra desirability of this city-state's UA for an AI that is currently
-- pursuing the given grand strategy. Flavor is on the gold-gift value scale (gift threshold is 100).
create table CityStateUA_VictoryFlavors (
    CityStateUAType text references CityStateUAs(Type),
    AIGrandStrategyType text references AIGrandStrategies(Type),
    Flavor integer default 0 not null
);

-- MinorCivilizations reference UAType directly (no mapping table needed)
alter table MinorCivilizations add column UAType text default null references CityStateUAs(Type);

-- Kathmandu CS UA: special buildings (e.g. Everest Camp) declare a prereq effect id that gates construction
-- (must run before any mod XML inserts rows into Buildings with this column)
ALTER TABLE Buildings ADD 'PrereqEffect' TEXT DEFAULT NULL;
-- CityState UA: adds per-turn Great Person Points (per SpecialistType) to all cities
create table CityStateUAEffect_GreatPersonPoints (
    EffectType text references CityStateUAEffects(Type),
    SpecialistType text references Specialists(Type),
    Points integer default 0
);

-- CityState UA (Budapest): each owned unit holding PromotionType changes the player's total unit
-- maintenance by MaintenanceChange gold (negative = cheaper, positive = more expensive). One unit may
-- match several rows and all matches add up; consumed in CvTreasury::CalculateUnitCost.
create table CityStateUAEffect_UnitMaintenanceByPromotion (
    EffectType text references CityStateUAEffects(Type),
    PromotionType text references UnitPromotions(Type),
    MaintenanceChange integer default 0
);

-- CityState UA (Almaty): a unit holding PromotionType gains extra max HP equal to
-- (the owner's total kills) x (the owner's surplus ResourceType) x Percent / 100. It is evaluated live in
-- CvUnit::GetMaxHitPoints (like the promotion per-kill bonus), NOT cached at kill time, so the value
-- follows kills and resource changes automatically. Surplus = getNumResourceAvailable (the count shown in
-- the top UI), clamped at 0 so a resource deficit never reduces HP. Percent is a plain percent (20 = 20%).
-- One unit may match several rows and all matches add up.
create table CityStateUAEffect_KillMaxHpByPromotion (
    EffectType text references CityStateUAEffects(Type),
    PromotionType text references UnitPromotions(Type),
    ResourceType text references Resources(Type),
    Percent integer default 0
);

-- CityState UA: each owned city provides Quantity of ResourceType (added in CvPlayer::getNumResourceTotal
-- before the strategic resource modifier is applied). Mirrors Policy_CityResources: the optional
-- CityScaleType / LargerScaleValid / MustCoastal conditions narrow which cities contribute.
create table CityStateUAEffect_ResourcePerCity (
    EffectType text references CityStateUAEffects(Type),
    ResourceType text references Resources(Type),
    Quantity integer not null default 0,

    -- optional conditions
    CityScaleType text null references CityScales(Type),
    LargerScaleValid boolean not null default 0,
    MustCoastal boolean not null default 0
);

-- CityState UA: born great person grants extra specialist yield (per SpecialistType)
create table CityStateUAEffect_BornGreatPersonSpecialistYield (
    EffectType text references CityStateUAEffects(Type),
    SpecialistType text references Specialists(Type),
    UnitClassType text references UnitClasses(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

create table CityStateUAEffect_BuildingGreatPersonPoints (
    EffectType text references CityStateUAEffects(Type),
    BuildingClassType text references BuildingClasses(Type),
    SpecialistType text references Specialists(Type),
    Points integer default 0
);

create table CityStateUAEffect_BornGreatPersonAllyInfluenceMod (
    EffectType text references CityStateUAEffects(Type),
    UnitClassType text references UnitClasses(Type),
    ModPerBorn integer default 0
);

create table CityStateUAEffect_BuildingClassYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    BuildingClassType text references BuildingClasses(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Singapore): each owned building class grants a nation-wide yield % modifier per YieldType.
-- The count comes from CvPlayer::getBuildingClassCount, which the building system maintains live. For
-- one-per-city dummy buildings (city scale / corruption tiers) this equals the number of cities of that
-- tier; for stackable normal buildings it would count total copies, so only use it with dummy building
-- classes. YieldMod is a PLAIN PERCENT (10 = +10% per owned building class), NOT basis points.
create table CityStateUAEffect_BuildingClassGlobalYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    BuildingClassType text references BuildingClasses(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Singapore): each owned building class lowers the city-count research threshold
-- (CvPlayerTechs::GetResearchCost) by TechCostMod percent, nation-wide. TechCostMod is a PLAIN PERCENT
-- (2 = -2% of the city-count threshold per owned building class); the sum across all rows is clamped at
-- 100 so the threshold can never go negative. See BuildingClassGlobalYieldModifiers for the dummy-building
-- caveat on the count.
create table CityStateUAEffect_BuildingClassTechCostModifiers (
    EffectType text references CityStateUAEffects(Type),
    BuildingClassType text references BuildingClasses(Type),
    TechCostMod integer default 0
);

-- CityState UA (Brussels): specified specialist's great person point accumulation rate (%)
create table CityStateUAEffect_SpecialistPointRate (
    EffectType text references CityStateUAEffects(Type),
    SpecialistType text references Specialists(Type),
    Rate integer default 0
);

-- CityState UA (Brussels): each great work of a class (literature/art/music) grants great person points (Rate=100 => 1 great work = 1 point)
create table CityStateUAEffect_GreatWorkGreatPersonPoints (
    EffectType text references CityStateUAEffects(Type),
    GreatWorkClassType text references GreatWorkClasses(Type),
    SpecialistType text references Specialists(Type),
    Rate integer default 0,
    CapitalOnly boolean default 0
);

-- CityState UA (Brussels): specified unit class's one-shot great person output modifier (%)
create table CityStateUAEffect_GreatPersonOneShotModifier (
    EffectType text references CityStateUAEffects(Type),
    UnitClassType text references UnitClasses(Type),
    Modifier integer default 0
);

-- CityState UA (Colombo): the international trade route (InternalTR) ending at this city-state (UCS)
-- grants the origin player a flat per-era yield (Era+1 times YieldValue)
create table CityStateUAEffect_InternalTRToUCSPerEraYield (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldValue integer default 0
);

-- City State UA (Colombo): for cities running a trade route to this city-state (UCS), a percentage of
-- the input yield (InYieldType) is granted as extra output yield (OutYieldType); the original input yield is unchanged.
-- RequireRouteToThisCS: 1 = the route must go TO this city-state (Colombo/Cape Town); 0 = any international
-- trade route originating from the city counts, city-state destinations included (Mogadishu).
create table CityStateUAEffect_YieldToYieldViaTRToUCS (
    EffectType text references CityStateUAEffects(Type),
    InYieldType text references Yields(Type),
    OutYieldType text references Yields(Type),
    Percent integer default 0,
    RequireRouteToThisCS integer default 1
);

-- City State UA (Monaco): a percentage (Mod, plain percent) of the city's output in InYieldType is granted
-- as extra OutYieldType output for every ally/friend city. Unlike _YieldToYieldViaTRToUCS this conversion
-- requires no trade route. For InYieldType = YIELD_TOURISM the source is the city's total tourism
-- (CvCity::GetBaseTourism, incl. great works); other inputs use the standard base yield rate.
create table CityStateUAEffect_YieldToYield (
    EffectType text references CityStateUAEffects(Type),
    InYieldType text references Yields(Type),
    OutYieldType text references Yields(Type),
    Mod integer default 0
);

-- City State UA (Monaco): the first gold donation to this city-state each turn is a wager. Each row is one
-- outcome; an outcome is picked with probability Weight / max(sum(Weight), 10000). Any probability mass
-- below 10000 not covered by the rows is a "no payout" outcome. Multiplier is the percent of the gifted
-- gold refunded to the donor (100 = full refund, 200 = double, 0 = nothing). A non-empty table = enabled.
create table CityStateUAEffect_GoldDonationGamble (
    EffectType text references CityStateUAEffects(Type),
    Weight integer default 0,
    Multiplier integer default 0
);

-- CityState UA (Valletta): buying the specified building class grants all units of the specified domain XP
create table CityStateUAEffect_PurchasedBuildingXP (
    EffectType text references CityStateUAEffects(Type),
    BuildingClassType text references BuildingClasses(Type),
    DomainType text references Domains(Type),
    XP integer default 0
);

-- CityState UA (Valletta): born unit of the specified unit class grants a configurable yield equal to
-- YieldMod% of the player's influence with the specified city-state (MinorCivType)
create table CityStateUAEffect_UnitBornYield (
    EffectType text references CityStateUAEffects(Type),
    MinorCivType text references MinorCivilizations(Type),
    UnitClassType text references UnitClasses(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- MinorCivilizations free building classes (analogous to Civilization_FreeBuildingClasses):
-- grants the building class to the city-state's first city, also at game start
create table MinorCivilization_FreeBuildingClasses (
    MinorCivType text references MinorCivilizations(Type),
    BuildingClassType text references BuildingClasses(Type)
);

-- MinorCivilizations purchasable building classes (a single global list shared by every
-- city-state, no per-city-state preference): each turn a city-state spends its own gold to
-- buy one of these in the first of its cities that still lacks it, as long as the building
-- is already unlocked by the city-state's techs and is not already under construction
create table MinorCivilization_PurchaseBuildingClasses (
    BuildingClassType text references BuildingClasses(Type)
);

-- CityState UA (Prague): a city with our own spy garrisoned in it grants a configurable
-- yield percentage modifier (per YieldType) to that city, e.g. YIELD_SCIENCE / 20 = +20% science
create table CityStateUAEffect_SpyGarrisonYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Melbourne): a city that owns the specified resource (must be improved with its
-- standard improvement) grants a yield percentage modifier per YieldType, e.g. RESOURCE_GOLD / YIELD_GOLD / 50 = +50% gold
create table CityStateUAEffect_ResourceYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    ResourceType text references Resources(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Bogota): named special-city predicates, reused by several effects.
-- The ID column is REQUIRED: the C++ side loads this table through PrefetchCollection,
-- which issues "SELECT * FROM <table> WHERE ID > -1 ORDER BY ID" and FNEWs one entry per row.
create table CityStateUAEffect_SpecialCityTypes (
    ID   INTEGER PRIMARY KEY AUTOINCREMENT,
    Type text not null unique
);

-- A city type matches when: EVERY And-row matches AND (the Or table is empty OR at least one Or-row matches).
-- Both are CHILD tables (read via "where SpecialCityType = ?"), so neither needs an ID column.
-- ConditionType is a fixed enum parsed in CvSpecialCityTypeEntry::CacheResults; Value holds the
-- resource/feature type name for value-carrying conditions (NULL for boolean ones).
-- Currently HAS_RESOURCE / HAS_FEATURE / IS_RIVER / IS_COASTAL / IS_PUPPET are implemented; add enum
-- branches for future kinds (CAPITAL / HILLS / HAS_BUILDINGCLASS / trade-route kinds / continent kinds).
create table CityStateUAEffect_SpecialCityTypeConditionsOr (
    SpecialCityType text references CityStateUAEffect_SpecialCityTypes(Type),
    ConditionType   text,
    Value           text
);

create table CityStateUAEffect_SpecialCityTypeConditionsAnd (
    SpecialCityType text references CityStateUAEffect_SpecialCityTypes(Type),
    ConditionType   text,
    Value           text
);

-- A city matching the special city type grants a yield percentage modifier to that city.
-- SPECIAL_CITY_BOGOTA_LUXURY / YIELD_CULTURE / 35 = +35% culture for cities of that type.
-- Modifiers ACCUMULATE: when one city matches several special city types, every matching row of the
-- same YieldType is summed (CvCity::GetBaseYieldRateModifier adds without breaking). Example -- Riga's
-- river+coastal city matches RIVER and RIVER_COASTAL, so gold/food get +15% + 15% = +30%.
-- NOTE: YieldMod here is a PLAIN PERCENT (35 means +35%), NOT basis points -- the opposite convention
-- from CityStateUAEffect_BornGreatPersonYieldModifiers (100 = +1%). CvCity::GetBaseYieldRateModifier
-- adds this value straight to its percent total, whereas the born-great-person table is divided by 100.
create table CityStateUAEffect_SpecialCityYieldModifiers (
    EffectType      text references CityStateUAEffects(Type),
    SpecialCityType text references CityStateUAEffect_SpecialCityTypes(Type),
    YieldType       text references Yields(Type),
    YieldMod        integer default 0
);

-- For each owned city matching the special city type, ALL cities gain a yield percentage modifier.
-- SPECIAL_CITY_BOGOTA_LUXURY / YIELD_TOURISM / 5 = +5% tourism per qualifying city
-- NOTE: YieldMod here is a PLAIN PERCENT (5 means +5% per qualifying city), NOT basis points.
-- CvPlayer::GetCSUAYieldPercentModifier multiplies it by 100 because that function accumulates basis
-- points and divides by 100 at the end; do not "pre-convert" the stored value.
create table CityStateUAEffect_SpecialCityCountYieldModifiers (
    EffectType      text references CityStateUAEffects(Type),
    SpecialCityType text references CityStateUAEffect_SpecialCityTypes(Type),
    YieldType       text references Yields(Type),
    YieldMod        integer default 0
);

-- For each N population living in cities matching the special city type, ALL cities gain a yield
-- percentage modifier. Kuala Lumpur: SPECIAL_CITY_PUPPET / 5 / YIELD_CULTURE / 2 = +2% culture per 5
-- population living in puppet cities. PerPopulation is the population step (>0; rows with 0 are ignored).
-- NOTE: YieldMod here is a PLAIN PERCENT (2 means +2% per step), NOT basis points.
-- CvPlayer::GetCSUAYieldPercentModifier multiplies it by 100 because that function accumulates basis
-- points and divides by 100 at the end; do not "pre-convert" the stored value.
create table CityStateUAEffect_SpecialCityPopulationYieldModifiers (
    EffectType      text references CityStateUAEffects(Type),
    SpecialCityType text references CityStateUAEffect_SpecialCityTypes(Type),
    PerPopulation   integer default 0,
    YieldType       text references Yields(Type),
    YieldMod        integer default 0
);

-- CityState UA (Tyre): cities matching the special city type take Percent% less damage.
-- Percent is a PLAIN PERCENT (40 = -40% damage taken). Applied in CvCity::changeDamage for damage-dealing
-- (positive) changes only, so healing is unaffected. Multiple matching rows sum and the total is clamped
-- to 90. Unlike the count/population tables above, this is per-city, not nation-wide.
-- NOTE: only damage routed through CvCity::changeDamage is reduced. Nuclear explosions set city damage
-- directly in CvUnitCombat::ApplyNuclearExplosionDamage and therefore BYPASS this reduction.
create table CityStateUAEffect_SpecialCityDamageReduction (
    EffectType      text references CityStateUAEffects(Type),
    SpecialCityType text references CityStateUAEffect_SpecialCityTypes(Type),
    Percent         integer default 0
);

-- CityState UA (Bucharest): each world wonder owned by the ally/friend grants a yield percentage
-- modifier per YieldType, nation-wide. YieldMod is a PLAIN PERCENT (4 = +4% per world wonder) and Cap
-- is a plain percent cap (0 = uncapped). CvPlayer::GetCSUAYieldPercentModifier multiplies it by 100
-- because that function accumulates basis points and divides by 100 at the end; do not pre-convert.
create table CityStateUAEffect_WorldWonderYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType  text references Yields(Type),
    YieldMod   integer default 0,
    Cap        integer default 0
);

-- CityState UA (Quebec): for each met, living major civilization whose influence level toward the ally is
-- Unknown (the lowest influence level), grant a nation-wide yield percentage modifier per YieldType.
-- YieldMod is a PLAIN PERCENT (4 = +4% per such civilization). Cap is a plain percent cap on the
-- accumulated sum (40 = +40% maximum); 0 = uncapped. The count is cached once per turn
-- (CvPlayerCityStateUA::CacheUnknownInfluenceCount) because CvPlayer::GetCSUAYieldPercentModifier is a
-- per-yield hot path.
create table CityStateUAEffect_UnknownInfluenceYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType  text references Yields(Type),
    YieldMod   integer default 0,
    Cap        integer default 0
);

-- CityState UA (Mogadishu): each international trade route the ally runs TO a city-state grants a
-- nation-wide yield percentage modifier per YieldType (e.g. YIELD_GOLD / 5 = +5% gold per route).
-- Routes to major civilizations do NOT count, only city-state destinations. This mirrors the existing
-- building effect Building_CityStateTradeRouteYieldModifiersGlobal, which likewise multiplies its
-- stored value by CvPlayerTrade::GetNumberOfCityStateTradeRoutes(). YieldMod is a PLAIN PERCENT
-- (5 = +5% per route); CvPlayer::GetCSUAYieldPercentModifier multiplies it by 100 because that
-- function accumulates basis points and divides by 100 at the end; do not pre-convert.
create table CityStateUAEffect_CityStateTradeRouteYieldModifiersGlobal (
    EffectType text references CityStateUAEffects(Type),
    YieldType  text references Yields(Type),
    YieldMod   integer default 0
);

-- CityState UA (Bucharest): each diplomat stationed in a foreign MAJOR civilization's city grants a
-- yield percentage modifier per YieldType, nation-wide. Diplomats sent to city-states do NOT count.
-- YieldMod is a PLAIN PERCENT (5 = +5% per diplomat); Cap 0 = uncapped.
create table CityStateUAEffect_DiplomatAbroadYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType  text references Yields(Type),
    YieldMod   integer default 0,
    Cap        integer default 0
);

-- CityState UA (Kiev): each League delegate vote the ally holds grants a yield percentage modifier per
-- YieldType, nation-wide. YieldMod is in BASIS POINTS (100 = +1% per vote), matching
-- GreatWorkYieldModifierEntry, so CvPlayer::GetCSUAYieldPercentModifier does NOT multiply it by 100.
-- The vote count is recomputed and cached once per turn (CvPlayerCityStateUA::CacheLeagueVotes).
create table CityStateUAEffect_LeagueVoteYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType  text references Yields(Type),
    YieldMod   integer default 0
);

-- CityState UA (Antananarivo): each worked plot holding the specified improvement
-- grants a yield percentage modifier to the city (e.g. MINE / YIELD_GOLD / 3 = +3% gold per worked mine)
create table CityStateUAEffect_ImprovementYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    ImprovementType text references Improvements(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Zanzibar): each worked plot holding the specified improvement
-- grants flat local happiness to the city (e.g. PLANTATION / 2 = +2 local happiness per worked plantation)
create table CityStateUAEffect_ImprovementHappiness (
    EffectType text references CityStateUAEffects(Type),
    ImprovementType text references Improvements(Type),
    Happiness integer default 0
);

-- CityState UA (Ragusa): each owned building of the specified class grants flat local happiness to the city
-- (e.g. HOSPITAL / 2 = +2 local happiness per owned hospital). Consumed in CvCity::GetLocalHappiness.
create table CityStateUAEffect_BuildingClassHappiness (
    EffectType text references CityStateUAEffects(Type),
    BuildingClassType text references BuildingClasses(Type),
    Happiness integer default 0
);

-- CityState UA (Genoa): each friendly city-state grants a yield percentage modifier per YieldType
-- (e.g. YIELD_SCIENCE / 100 = +1% science per friendly city-state; Rate=100 => +1%)
create table CityStateUAEffect_FriendCityStateYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Genoa): each allied city-state grants a yield percentage modifier per YieldType
-- (e.g. YIELD_SCIENCE / 200 = +2% science per allied city-state; Rate=100 => +1%)
create table CityStateUAEffect_AllyCityStateYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Vilnius): each unlocked social policy grants a yield percentage modifier per YieldType
-- (e.g. YIELD_GOLD / 200 = +2% gold per unlocked policy; Rate=100 => +1%)
create table CityStateUAEffect_PolicyYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Jerusalem/Wittenberg): for each city worldwide following the player's religion,
-- the player's capital gains +Modifier% of YieldType (Modifier=100 => +1% per following city)
create table CityStateUAEffect_CapitalYieldModifierPerFollowingCity (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    Modifier integer default 0
);

-- CityState UA (Vatican): for each city worldwide following the player's religion,
-- the player's religion's holy city gains +Modifier% of YieldType (Modifier=100 => +1% per following city, e.g. YIELD_TOURISM / 100 = +1% tourism per city)
create table CityStateUAEffect_HolyCityYieldModifierPerFollowingCity (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    Modifier integer default 0
);

-- CityState UA (Sydney): each immigrant received grants a yield percentage modifier per YieldType
-- (Modifier=100 => +1% per immigrant received, e.g. YIELD_CULTURE / 400 = +4% culture per immigrant)
create table CityStateUAEffect_ImmigrantYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    Modifier integer default 0
);

-- CityState UA (Sydney): each immigrant received grants cash equal to CashPercent% of the treasury,
-- capped at CashCapBase x (current era + 1) x game-speed culture percent / 100
-- (e.g. CashPercent=1 / CashCapBase=100 => +1% of treasury, cap = 100 x era x speed)
create table CityStateUAEffect_ImmigrantCashReward (
    EffectType text references CityStateUAEffects(Type),
    CashPercent integer default 0,
    CashCapBase integer default 0
);

-- CityState UA (Hormuz): each unit of surplus strategic resource grants trade-route gold %
-- (per ResourceType; Modifier in basis points (percent x 100), e.g. Modifier 400 = +4% gold per surplus oil;
--  surplus = max(0, getNumResourceAvailable), never negative)
create table CityStateUAEffect_TradeRouteGoldPerSurplusResource (
    EffectType text references CityStateUAEffects(Type),
    ResourceType text references Resources(Type),
    Modifier integer default 0
);

-- CityState UA (Vancouver): each point of the player's net happiness grants a yield percentage
-- modifier per YieldType, capped per yield. YieldMod is in basis points (100 = +1% per happiness);
-- Cap is in percent (50 = +50% maximum). Applies to e.g. YIELD_FOOD and YIELD_TOURISM.
create table CityStateUAEffect_HappinessYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0,
    Cap integer default 0
);

-- CityState UA (Ife): per-unitclass discount on the FAITH cost of buying great people. CostRiseModifier
-- is in percent applied to the final price (negative = discount), keyed by UnitClassType so it affects
-- only specified great people classes (e.g. UNITCLASS_ARTIST/WRITER/MUSICIAN). -30 = -30% final faith cost.
create table CityStateUAEffect_FaithGPClassCostModifier (
    EffectType text references CityStateUAEffects(Type),
    UnitClassType text references UnitClasses(Type),
    CostRiseModifier integer default 0
);

-- CityState UA (Ife): each great work / artifact of the specified GreatWorkClassType grants a yield
-- percentage modifier per YieldType, nation-wide. YieldMod in basis points (100 = +1% per great work);
-- e.g. GREAT_WORK_ARTIFACT / YIELD_FAITH / 200 = +2% faith nation-wide per artifact owned.
create table CityStateUAEffect_GreatWorkYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    GreatWorkClassType text references GreatWorkClasses(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Ife): while the player is in a golden age, grant a yield percentage modifier per YieldType
-- nation-wide. YieldMod is a plain percent (25 = +25% faith during golden age), not basis points.
create table CityStateUAEffect_GoldenAgeYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Yerevan): literacy rate (owned techs / total techs x 100, integer percent points)
-- grants a nation-wide yield percentage modifier, keyed by YieldType so it can apply to any yield
-- (YieldMod is basis points per literacy point; ally 100 = +1% production per point, friend 50 = +1% per 2 points).
create table CityStateUAEffect_LiteracyYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Yerevan): each born great person of the specified UnitClassType grants a nation-wide
-- yield percentage modifier per YieldType (YieldMod is basis points per born great person;
-- ally prophet: UNITCLASS_PROPHET / YIELD_FAITH / 500 = +5% faith per born prophet).
create table CityStateUAEffect_BornGreatPersonYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    UnitClassType text references UnitClasses(Type),
    YieldType text references Yields(Type),
    YieldMod integer default 0
);

-- CityState UA (Yerevan): if this plot is an improvement and an adjacent plot's improvement is
-- AdjacentImprovementType, this plot gains +Yield of YieldType. Direction matches the vanilla
-- *_AdjacentImprovementYieldChanges family (neighbor improvement -> this improvement).
-- ImprovementType is the LOCAL (affected) improvement and MUST be a concrete type; rows are meant to be
-- fully enumerated for every improvement via SP SQL (INSERT...SELECT FROM Improvements + an AFTER INSERT
-- trigger, same approach as SP_AdjacentImprovementYieldChangesForNewImproments). No wildcard supported.
-- Ally: each <ImprovementType> / IMPROVEMENT_HOLY_SITE / YIELD_CULTURE / 1 = +1 culture to that improved
-- plot when it borders a holy site. Mirrors the Policy/Trait/Building adjacent-improvement family.
create table CityStateUAEffect_AdjacentImprovementYieldChanges (
    EffectType text references CityStateUAEffects(Type),
    ImprovementType text references Improvements(Type),
    AdjacentImprovementType text references Improvements(Type),
    YieldType text references Yields(Type),
    Yield integer default 0
);

-- CityState UA (Kabul): each international LAND trade route the ally runs grants trade-route gold %.
-- YieldMod is a PLAIN PERCENT (50 = +50%); when the origin city sits on hills, HillsBonus is added on
-- top (50 = an extra +50%, so a hills origin gets +100%). Only international land routes are affected
-- (the consumer CvPlayer::GetCSUATradeRouteGoldModifier already returns early for non-international
-- connections), and the modifier is applied per connection from the origin city's owner.
create table CityStateUAEffect_LandTradeRouteGoldModifier (
    EffectType text references CityStateUAEffects(Type),
    YieldMod   integer default 0,
    HillsBonus integer default 0
);

-- CityState UA (Kabul): each international LAND trade route the ally runs to ANY other player
-- (city-state destinations included) grants a nation-wide yield percentage modifier per YieldType,
-- scaled by era: value * (currentEra + 1). YieldMod is a PLAIN PERCENT (1 = +1% per route in the
-- ancient era, +2% in the classical era, ...); CvPlayer::GetCSUAYieldPercentModifier multiplies it by
-- 100 because that function accumulates basis points and divides by 100 at the end; do not pre-convert.
create table CityStateUAEffect_InternationalLandTradeRouteYieldPerEra (
    EffectType text references CityStateUAEffects(Type),
    YieldType  text references Yields(Type),
    YieldMod   integer default 0
);

-- CityState UA (Milan): each point of luxury happiness the ally/friend has grants a nation-wide yield
-- percentage modifier per YieldType. YieldMod is in BASIS POINTS per point of luxury happiness
-- (ally 50 = +0.5% per point, i.e. every 2 points +1%; friend 25 = every 4 points +1%). Cap is a plain
-- percent cap on the accumulated sum (0 = uncapped). The luxury happiness total is cached once per
-- doTurn (CvPlayerCityStateUA::CacheLuxuryHappiness) because CvPlayer::GetCSUAYieldPercentModifier is a
-- per-yield hot path.
create table CityStateUAEffect_LuxuryHappinessYieldModifiers (
    EffectType text references CityStateUAEffects(Type),
    YieldType  text references Yields(Type),
    YieldMod   integer default 0,
    Cap        integer default 0
);

-- CityState UA (Milan): a unit takes Percent% less damage when the opposing side's team has NOT
-- researched TechType (e.g. TECH_RIFLING / 25 = -25% damage taken). Applies in both directions: when
-- the unit is the defender being hit and when it is the attacker taking the counter-blow. Percent is a
-- PLAIN PERCENT. Consumed by CvUnit::GetCSUADamageTakenScale, which is shared by the real resolution
-- (CvUnitCombat::InterveneInflictDamage) and the UI combat panel (CvLuaUnit / EnemyUnitPanel.lua) so
-- the preview matches the real result.
create table CityStateUAEffect_CombatDamageReductionVsNoTech (
    EffectType text references CityStateUAEffects(Type),
    TechType   text references Technologies(Type),
    Percent    integer default 0
);
