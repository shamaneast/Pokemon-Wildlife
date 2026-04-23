#include "global.h"
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

#define OW_VISIBLE_WILD_ENCOUNTERS TRUE
#define OW_VISIBLE_WILD_SPAWN_CHANCE 20
#define OW_VISIBLE_WILD_SPAWN_RADIUS 5
#define OW_VISIBLE_WILD_DESPAWN_DISTANCE 12
#define OW_VISIBLE_WILD_LOCAL_ID 241

struct OverworldWildMon
{
    u16 species;
    u8 level;
    u8 mapNum;
    u8 mapGroup;
    bool8 active;
};

static EWRAM_DATA struct OverworldWildMon sOverworldWildMon = {0};

static bool8 TryGetEncounterData(bool8 *isWater, const struct WildPokemonInfo **wildMonsInfo);
static bool8 TrySpawnVisibleWildMon(void);
static void TryDespawnVisibleWildMon(void);

bool8 OverworldWild_UseVisibleEncounters(void)
{
    if (!OW_VISIBLE_WILD_ENCOUNTERS)
        return FALSE;

    if (FlagGet(OW_FLAG_NO_ENCOUNTER) || MapHasNoEncounterData())
        return FALSE;

    return TRUE;
}

void OverworldWild_ResetState(void)
{
    TryDespawnVisibleWildMon();
    sOverworldWildMon.active = FALSE;
}

bool32 OverworldWild_OnStep(void)
{
    s16 playerX;
    s16 playerY;
    u8 objectEventId;
    struct ObjectEvent *objectEvent;

    if (!OverworldWild_UseVisibleEncounters())
    {
        OverworldWild_ResetState();
        return FALSE;
    }

    if (sOverworldWildMon.active
        && (sOverworldWildMon.mapNum != gSaveBlock1Ptr->location.mapNum || sOverworldWildMon.mapGroup != gSaveBlock1Ptr->location.mapGroup))
    {
        OverworldWild_ResetState();
    }

    if (!sOverworldWildMon.active)
    {
        TrySpawnVisibleWildMon();
        return FALSE;
    }

    if (!TryGetObjectEventIdByLocalIdAndMap(OW_VISIBLE_WILD_LOCAL_ID, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &objectEventId))
    {
        sOverworldWildMon.active = FALSE;
        return FALSE;
    }

    objectEvent = &gObjectEvents[objectEventId];
    PlayerGetDestCoords(&playerX, &playerY);

    s16 distanceX = playerX - objectEvent->currentCoords.x;
    s16 distanceY = playerY - objectEvent->currentCoords.y;
    if (distanceX < 0)
        distanceX = -distanceX;
    if (distanceY < 0)
        distanceY = -distanceY;

    if ((u16)(distanceX + distanceY) <= 1)
    {
        RemoveObjectEventByLocalIdAndMap(OW_VISIBLE_WILD_LOCAL_ID, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
        CreateWildMon(sOverworldWildMon.species, sOverworldWildMon.level);
        sOverworldWildMon.active = FALSE;
        ScriptContext_SetupScript(EventScript_StartDexNavBattle);
        return TRUE;
    }

    if ((u16)(distanceX + distanceY) >= OW_VISIBLE_WILD_DESPAWN_DISTANCE)
    {
        OverworldWild_ResetState();
    }

    return FALSE;
}

static bool8 TryGetEncounterData(bool8 *isWater, const struct WildPokemonInfo **wildMonsInfo)
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

    if (MetatileBehavior_IsWaterWildEncounter(tileBehavior))
    {
        *isWater = TRUE;
        timeOfDay = GetTimeOfDayForEncounters(headerId, WILD_AREA_WATER);
        *wildMonsInfo = gWildMonHeaders[headerId].encounterTypes[timeOfDay].waterMonsInfo;
    }
    else if (MetatileBehavior_IsLandWildEncounter(tileBehavior) || MetatileBehavior_IsIndoorEncounter(tileBehavior))
    {
        *isWater = FALSE;
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
    bool8 isWater;
    const struct WildPokemonInfo *wildMonsInfo;
    u8 wildMonIndex;
    s16 playerX;
    s16 playerY;
    u8 playerElevation;
    u8 i;

    if ((Random() % 100) >= OW_VISIBLE_WILD_SPAWN_CHANCE)
        return FALSE;

    if (!TryGetEncounterData(&isWater, &wildMonsInfo))
        return FALSE;

    if (isWater)
        wildMonIndex = ChooseWildMonIndex_Water();
    else
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
        if (isWater && !MetatileBehavior_IsWaterWildEncounter(behavior))
            continue;

        if (!isWater && !MetatileBehavior_IsLandWildEncounter(behavior) && !MetatileBehavior_IsIndoorEncounter(behavior))
            continue;

        if (SpawnSpecialObjectEventParameterized(wildMonsInfo->wildPokemon[wildMonIndex].species + OBJ_EVENT_MON,
                                                 MOVEMENT_TYPE_WANDER_AROUND,
                                                 OW_VISIBLE_WILD_LOCAL_ID,
                                                 x,
                                                 y,
                                                 playerElevation) >= OBJECT_EVENTS_COUNT)
        {
            return FALSE;
        }

        sOverworldWildMon.active = TRUE;
        sOverworldWildMon.species = wildMonsInfo->wildPokemon[wildMonIndex].species;
        sOverworldWildMon.level = wildMonsInfo->wildPokemon[wildMonIndex].minLevel + Random() % (wildMonsInfo->wildPokemon[wildMonIndex].maxLevel - wildMonsInfo->wildPokemon[wildMonIndex].minLevel + 1);
        sOverworldWildMon.mapNum = gSaveBlock1Ptr->location.mapNum;
        sOverworldWildMon.mapGroup = gSaveBlock1Ptr->location.mapGroup;
        return TRUE;
    }

    return FALSE;
}

static void TryDespawnVisibleWildMon(void)
{
    if (sOverworldWildMon.active)
        RemoveObjectEventByLocalIdAndMap(OW_VISIBLE_WILD_LOCAL_ID, sOverworldWildMon.mapNum, sOverworldWildMon.mapGroup);
}
