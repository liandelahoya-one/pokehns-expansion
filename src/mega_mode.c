#include "global.h"
#include "mega_mode.h"
#include "item.h"
#include "pokemon.h"
#include "constants/form_change_types.h"
#include "constants/items.h"

bool32 IsMegaRandomizerModeEnabled(void)
{
    return gSaveBlock3Ptr != NULL && gSaveBlock3Ptr->challengeSettings.tx_Random_MegaMode;
}

enum Item MegaMode_GetStoneForSpecies(u16 species, enum Type requiredMegaType)
{
    const struct FormChange *changes;

    if (species <= SPECIES_NONE || species >= NUM_SPECIES)
        return ITEM_NONE;

    changes = GetSpeciesFormChanges(species);
    if (changes == NULL)
        return ITEM_NONE;

    for (u32 i = 0; changes[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        const struct FormChange *change = &changes[i];

        if (change->method != FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM
         || change->targetSpecies >= NUM_SPECIES
         || !gSpeciesInfo[change->targetSpecies].isMegaEvolution
         || change->param1 == ITEM_NONE)
            continue;

        if (requiredMegaType != TYPE_NONE
         && GetSpeciesType(change->targetSpecies, 0) != requiredMegaType
         && GetSpeciesType(change->targetSpecies, 1) != requiredMegaType)
            continue;

        return change->param1;
    }
    return ITEM_NONE;
}

void MegaMode_GrantStonesForSpecies(u16 species)
{
    const struct FormChange *changes;

    if (!IsMegaRandomizerModeEnabled() || species <= SPECIES_NONE || species >= NUM_SPECIES)
        return;

    changes = GetSpeciesFormChanges(species);
    if (changes == NULL)
        return;

    // Multi-Mega Pokemon (e.g. Charizard) can require two distinct stones.
    for (u32 i = 0; changes[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        enum Item stone = changes[i].param1;
        if (changes[i].method != FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM
         || changes[i].targetSpecies >= NUM_SPECIES
         || !gSpeciesInfo[changes[i].targetSpecies].isMegaEvolution
         || stone == ITEM_NONE)
            continue;

        if (CheckBagHasItem(stone, 1) || CheckPCHasItem(stone, 1))
            continue;

        // Never silently replace other items when the bag is full.
        if (!AddBagItem(stone, 1))
            AddPCItem(stone, 1);
    }
}
