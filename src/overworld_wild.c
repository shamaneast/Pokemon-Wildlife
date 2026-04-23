#include "global.h"
#include "config/overworld.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "metatile_behavior.h"
#include "overworld_wild.h"
#include "random.h"
#include "script.h"
#include "wild_encounter.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/maps.h"

struct OverworldWildMon
{
    u16 species;
    u8 level;
    u8 localId;
    u8 mapNum;
    u8 mapGroup;
    u16 remainingLifetimeSteps;
    u32 spawnTimeSeconds;
    bool8 active;
};

static EWRAM_DATA struct OverworldWildMon sOverworldWildMons[OW_VISIBLE_WILD_MAX_ACTIVE] = {0};

static bool8 TryGetEncounterData(const struct WildPokemonInfo **wildMonsInfo);
static bool8 TrySpawnVisibleWildMon(void);
static void TryDespawnVisibleWildMon(u8 slot);
static void TryDespawnExpiredVisibleWildMons(void);
static bool8 TryStartBattleWithCollidingWildMon(void);
static u8 CountActiveVisibleWildMons(void);
static u32 GetCurrentPlayTimeSeconds(void);

bool8 OverworldWild_UseVisibleEncounters(void)
{
    s16 playerX;
    s16 playerY;
    u16 tileBehavior;

    if (!OW_VISIBLE_WILD_ENCOUNTERS)
        return FALSE;

    if (FlagGet(OW_FLAG_NO_ENCOUNTER) || MapHasNoEncounterData())
        return FALSE;

    PlayerGetDestCoords(&playerX, &playerY);
    tileBehavior = MapGridGetMetatileBehaviorAt(playerX, playerY);
    if (!MetatileBehavior_IsLandWildEncounter(tileBehavior))
        return FALSE;

    return TRUE;
}

void OverworldWild_ResetState(void)
{
    u8 i;

    for (i = 0; i < OW_VISIBLE_WILD_MAX_ACTIVE; i++)
        TryDespawnVisibleWildMon(i);
}

bool32 OverworldWild_OnStep(void)
{
    if (!OverworldWild_UseVisibleEncounters())
    {
        OverworldWild_ResetState();
        return FALSE;
    }

    TryDespawnExpiredVisibleWildMons();

    if (TryStartBattleWithCollidingWildMon())
        return TRUE;

    if (CountActiveVisibleWildMons() < OW_VISIBLE_WILD_MAX_ACTIVE)
        TrySpawnVisibleWildMon();

    return FALSE;
}

static u32 GetCurrentPlayTimeSeconds(void)
{
    return (gSaveBlock2Ptr->playTimeHours * 3600) + (gSaveBlock2Ptr->playTimeMinutes * 60) + gSaveBlock2Ptr->playTimeSeconds;
}

static u8 CountActiveVisibleWildMons(void)
{
    u8 i;
    u8 count = 0;

    for (i = 0; i < OW_VISIBLE_WILD_MAX_ACTIVE; i++)
    {
        if (sOverworldWildMons[i].active)
            count++;
    }

    return count;
}

static bool8 TryGetEncounterData(const struct WildPokemonInfo **wildMonsInfo)
{
    s16 playerX;
    s16 playerY;
    u16 tileBehavior;
    u32 headerId;
    enum TimeOfDay timeOfDay;

    PlayerGetDestCoords(&playerX, &playerY);
    tileBehavior = MapGridGetMetatileBehaviorAt(playerX, playerY);
    headerId = GetCurrentMapWildMonHeaderId();

    if (headerId == HEADER_NONE)
        return FALSE;

    if (MetatileBehavior_IsLandWildEncounter(tileBehavior))
    {
        timeOfDay = GetTimeOfDayForEncounters(headerId, WILD_AREA_LAND);
        *wildMonsInfo = gWildMonHeaders[headerId].encounterTypes[timeOfDay].landMonsInfo;
    }
    else
    {
        return FALSE;
    }

    return (*wildMonsInfo != NULL && (*wildMonsInfo)->wildPokemon != NULL);
}

static bool8 TrySpawnVisibleWildMon(void)
{
    const struct WildPokemonInfo *wildMonsInfo;
    u8 wildMonIndex;
    s16 playerX;
    s16 playerY;
    u8 playerElevation;
    u8 freeSlot = OW_VISIBLE_WILD_MAX_ACTIVE;
    u8 slot;
    u8 i;

    if ((Random() % 100) >= OW_VISIBLE_WILD_SPAWN_CHANCE)
        return FALSE;

    for (slot = 0; slot < OW_VISIBLE_WILD_MAX_ACTIVE; slot++)
    {
        if (!sOverworldWildMons[slot].active)
        {
            freeSlot = slot;
            break;
        }
    }

    if (freeSlot == OW_VISIBLE_WILD_MAX_ACTIVE)
        return FALSE;

    if (!TryGetEncounterData(&wildMonsInfo))
        return FALSE;

    wildMonIndex = ChooseWildMonIndex_Land();

    PlayerGetDestCoords(&playerX, &playerY);
    playerElevation = gObjectEvents[gPlayerAvatar.objectEventId].currentElevation;

    for (i = 0; i < 20; i++)
    {
        s16 x = playerX + ((Random() % (OW_VISIBLE_WILD_SPAWN_RADIUS * 2 + 1)) - OW_VISIBLE_WILD_SPAWN_RADIUS);
        s16 y = playerY + ((Random() % (OW_VISIBLE_WILD_SPAWN_RADIUS * 2 + 1)) - OW_VISIBLE_WILD_SPAWN_RADIUS);
        u16 behavior;

        if (GetMapBorderIdAt(x, y) == CONNECTION_INVALID)
            continue;

        if (GetObjectEventIdByPosition(x, y, playerElevation) != OBJECT_EVENTS_COUNT)
            continue;

        if (MapGridGetCollisionAt(x, y) != 0)
            continue;

        behavior = MapGridGetMetatileBehaviorAt(x, y);
        if (!MetatileBehavior_IsLandWildEncounter(behavior))
            continue;

        if (SpawnSpecialObjectEventParameterized(wildMonsInfo->wildPokemon[wildMonIndex].species + OBJ_EVENT_MON,
                                                 MOVEMENT_TYPE_WANDER_AROUND,
                                                 OW_VISIBLE_WILD_LOCAL_ID_BASE + freeSlot,
                                                 x,
                                                 y,
                                                 playerElevation) >= OBJECT_EVENTS_COUNT)
        {
            return FALSE;
        }

        sOverworldWildMons[freeSlot].active = TRUE;
        sOverworldWildMons[freeSlot].species = wildMonsInfo->wildPokemon[wildMonIndex].species;
        sOverworldWildMons[freeSlot].level = wildMonsInfo->wildPokemon[wildMonIndex].minLevel + Random() % (wildMonsInfo->wildPokemon[wildMonIndex].maxLevel - wildMonsInfo->wildPokemon[wildMonIndex].minLevel + 1);
        sOverworldWildMons[freeSlot].localId = OW_VISIBLE_WILD_LOCAL_ID_BASE + freeSlot;
        sOverworldWildMons[freeSlot].mapNum = gSaveBlock1Ptr->location.mapNum;
        sOverworldWildMons[freeSlot].mapGroup = gSaveBlock1Ptr->location.mapGroup;
        sOverworldWildMons[freeSlot].remainingLifetimeSteps = OW_VISIBLE_WILD_DESPAWN_STEPS;
        sOverworldWildMons[freeSlot].spawnTimeSeconds = GetCurrentPlayTimeSeconds();
        return TRUE;
    }

    return FALSE;
}

static void TryDespawnVisibleWildMon(u8 slot)
{
    if (slot >= OW_VISIBLE_WILD_MAX_ACTIVE)
        return;

    if (sOverworldWildMons[slot].active)
        RemoveObjectEventByLocalIdAndMap(sOverworldWildMons[slot].localId, sOverworldWildMons[slot].mapNum, sOverworldWildMons[slot].mapGroup);

    sOverworldWildMons[slot].active = FALSE;
}

static void TryDespawnExpiredVisibleWildMons(void)
{
    u8 slot;
    u32 now = GetCurrentPlayTimeSeconds();

    for (slot = 0; slot < OW_VISIBLE_WILD_MAX_ACTIVE; slot++)
    {
        u8 objectEventId;

        if (!sOverworldWildMons[slot].active)
            continue;

        if (sOverworldWildMons[slot].mapNum != gSaveBlock1Ptr->location.mapNum || sOverworldWildMons[slot].mapGroup != gSaveBlock1Ptr->location.mapGroup)
        {
            TryDespawnVisibleWildMon(slot);
            continue;
        }

        if (!TryGetObjectEventIdByLocalIdAndMap(sOverworldWildMons[slot].localId, sOverworldWildMons[slot].mapNum, sOverworldWildMons[slot].mapGroup, &objectEventId))
        {
            sOverworldWildMons[slot].active = FALSE;
            continue;
        }

        if (sOverworldWildMons[slot].remainingLifetimeSteps > 0)
            sOverworldWildMons[slot].remainingLifetimeSteps--;

        if (sOverworldWildMons[slot].remainingLifetimeSteps == 0
            || (now - sOverworldWildMons[slot].spawnTimeSeconds) >= OW_VISIBLE_WILD_DESPAWN_SECONDS)
        {
            TryDespawnVisibleWildMon(slot);
        }
    }
}

static bool8 TryStartBattleWithCollidingWildMon(void)
{
    s16 playerX;
    s16 playerY;
    u8 slot;

    PlayerGetDestCoords(&playerX, &playerY);

    for (slot = 0; slot < OW_VISIBLE_WILD_MAX_ACTIVE; slot++)
    {
        u8 objectEventId;
        s16 distanceX;
        s16 distanceY;
        struct ObjectEvent *objectEvent;

        if (!sOverworldWildMons[slot].active)
            continue;

        if (!TryGetObjectEventIdByLocalIdAndMap(sOverworldWildMons[slot].localId, sOverworldWildMons[slot].mapNum, sOverworldWildMons[slot].mapGroup, &objectEventId))
        {
            sOverworldWildMons[slot].active = FALSE;
            continue;
        }

        objectEvent = &gObjectEvents[objectEventId];
        distanceX = playerX - objectEvent->currentCoords.x;
        distanceY = playerY - objectEvent->currentCoords.y;
        if (distanceX < 0)
            distanceX = -distanceX;
        if (distanceY < 0)
            distanceY = -distanceY;

        if ((u16)(distanceX + distanceY) <= 1)
        {
            RemoveObjectEventByLocalIdAndMap(sOverworldWildMons[slot].localId, sOverworldWildMons[slot].mapNum, sOverworldWildMons[slot].mapGroup);
            CreateWildMon(sOverworldWildMons[slot].species, sOverworldWildMons[slot].level);
            sOverworldWildMons[slot].active = FALSE;
            ScriptContext_SetupScript(EventScript_StartDexNavBattle);
            return TRUE;
        }
    }

    return FALSE;
}
