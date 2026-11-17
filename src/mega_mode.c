#include "global.h"
#include "mega_mode.h"
#include "item.h"
#include "event_data.h"
#include "pokemon.h"
#include "constants/form_change_types.h"
#include "constants/items.h"
#include "constants/trainers.h"
#include "constants/flags.h"
#include "randomizer.h"
#include "random.h"

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

// Johto badge count is used for the non-gym boss tier.
static u8 GetMegaModeBadgeCount(void)
{
    u8 badges = 0;
    for (u16 flag = FLAG_BADGE01_GET; flag < FLAG_BADGE01_GET + 8; flag++)
        if (FlagGet(flag))
            badges++;
    return badges;
}

enum Type MegaMode_GetTrainerTheme(u16 trainerId)
{
    switch (trainerId)
    {
    case TRAINER_FALKNER_1_HNS:
    case TRAINER_FALKNER_2_HNS:
    case TRAINER_FALKNER_POSTOBC_HNS: return TYPE_FLYING;
    case TRAINER_BUGSY_1_HNS:
    case TRAINER_BUGSY_2_HNS:
    case TRAINER_BUGSY_POSTOBC_HNS: return TYPE_BUG;
    case TRAINER_WHITNEY_1_HNS:
    case TRAINER_WHITNEY_2_HNS:
    case TRAINER_WHITNEY_POSTOBC_HNS: return TYPE_NORMAL;
    case TRAINER_MORTY_1_HNS:
    case TRAINER_MORTY_2_HNS:
    case TRAINER_MORTY_POSTOBC_HNS: return TYPE_GHOST;
    case TRAINER_CHUCK_1_HNS:
    case TRAINER_CHUCK_2_HNS:
    case TRAINER_CHUCK_1_2_HNS:
    case TRAINER_CHUCK_1_3_HNS:
    case TRAINER_CHUCK_POSTOBC_HNS: return TYPE_FIGHTING;
    case TRAINER_JASMINE_1_HNS:
    case TRAINER_JASMINE_2_HNS:
    case TRAINER_JASMINE_1_2_HNS:
    case TRAINER_JASMINE_1_3_HNS:
    case TRAINER_JASMINE_POSTOBC_HNS: return TYPE_STEEL;
    case TRAINER_PRYCE_1_HNS:
    case TRAINER_PRYCE_2_HNS:
    case TRAINER_PRYCE_1_2_HNS:
    case TRAINER_PRYCE_1_3_HNS:
    case TRAINER_PRYCE_POSTOBC_HNS: return TYPE_ICE;
    case TRAINER_CLAIR_1_HNS:
    case TRAINER_CLAIR_2_HNS:
    case TRAINER_CLAIR_3_HNS:
    case TRAINER_CLAIR_POSTOBC_HNS:
    case TRAINER_LANCE_1_HNS:
    case TRAINER_LANCE_2_HNS:
    case TRAINER_LANCE_3_HNS:
    case TRAINER_LANCE_POSTOBC_HNS: return TYPE_DRAGON;
    case TRAINER_WILL_1_HNS:
    case TRAINER_WILL_2_HNS:
    case TRAINER_WILL_POSTOBC_HNS:
    case TRAINER_SABRINA_HNS:
    case TRAINER_SABRINA_POSTOBC_HNS: return TYPE_PSYCHIC;
    case TRAINER_KOGA_1_HNS:
    case TRAINER_KOGA_2_HNS:
    case TRAINER_KOGA_POSTOBC_HNS:
    case TRAINER_JANINE_HNS:
    case TRAINER_JANINE_POSTOBC_HNS: return TYPE_POISON;
    case TRAINER_BRUNO_1_HNS:
    case TRAINER_BRUNO_2_HNS:
    case TRAINER_BRUNO_POSTOBC_HNS: return TYPE_FIGHTING;
    case TRAINER_KAREN_1_HNS:
    case TRAINER_KAREN_2_HNS:
    case TRAINER_KAREN_POSTOBC_HNS: return TYPE_DARK;
    case TRAINER_BROCK_HNS:
    case TRAINER_BROCK_POSTOBC_HNS: return TYPE_ROCK;
    case TRAINER_MISTY_HNS:
    case TRAINER_MISTY_POSTOBC_HNS: return TYPE_WATER;
    case TRAINER_LTSURGE_HNS:
    case TRAINER_LTSURGE_POSTOBC_HNS: return TYPE_ELECTRIC;
    case TRAINER_ERIKA_HNS:
    case TRAINER_ERIKA_POSTOBC_HNS: return TYPE_GRASS;
    case TRAINER_BLAINE_HNS:
    case TRAINER_BLAINE_POSTOBC_HNS: return TYPE_FIRE;
    case TRAINER_GIOVANNI_HNS: return TYPE_GROUND;
    default: return TYPE_NONE; // Blue is deliberately a mixed-type leader.
    }
}

u8 MegaMode_GetTrainerMegaCount(u16 trainerId, u8 partySize)
{
    u8 count;
    if (!IsMegaRandomizerModeEnabled())
        return 0;

    switch (trainerId)
    {
    case TRAINER_FALKNER_1_HNS:
    case TRAINER_FALKNER_2_HNS:
    case TRAINER_BUGSY_1_HNS:
    case TRAINER_BUGSY_2_HNS:
        count = 1; break;
    case TRAINER_WHITNEY_1_HNS:
    case TRAINER_WHITNEY_2_HNS:
    case TRAINER_MORTY_1_HNS:
    case TRAINER_MORTY_2_HNS:
        count = 2; break;
    case TRAINER_CHUCK_1_HNS:
    case TRAINER_CHUCK_2_HNS:
    case TRAINER_CHUCK_1_2_HNS:
    case TRAINER_CHUCK_1_3_HNS:
    case TRAINER_JASMINE_1_HNS:
    case TRAINER_JASMINE_2_HNS:
    case TRAINER_JASMINE_1_2_HNS:
    case TRAINER_JASMINE_1_3_HNS:
        count = 3; break;
    case TRAINER_PRYCE_1_HNS:
    case TRAINER_PRYCE_2_HNS:
    case TRAINER_PRYCE_1_2_HNS:
    case TRAINER_PRYCE_1_3_HNS:
        count = 4; break;
    case TRAINER_GIOVANNI_HNS:
    {
        u8 badges = GetMegaModeBadgeCount();
        count = badges < 2 ? 1 : badges < 4 ? 2 : badges < 6 ? 3 : badges < 8 ? 4 : partySize;
        break;
    }
    // Clair, the Elite Four, champion, Kanto leaders and post-game rematches
    // use as many Mega-capable slots as the ORIGINAL party already contains.
    case TRAINER_CLAIR_1_HNS: case TRAINER_CLAIR_2_HNS:
    case TRAINER_CLAIR_3_HNS:
    case TRAINER_WILL_1_HNS: case TRAINER_WILL_2_HNS:
    case TRAINER_KOGA_1_HNS: case TRAINER_KOGA_2_HNS:
    case TRAINER_BRUNO_1_HNS: case TRAINER_BRUNO_2_HNS:
    case TRAINER_KAREN_1_HNS: case TRAINER_KAREN_2_HNS:
    case TRAINER_LANCE_1_HNS: case TRAINER_LANCE_2_HNS:
    case TRAINER_LANCE_3_HNS:
    case TRAINER_BROCK_HNS: case TRAINER_MISTY_HNS:
    case TRAINER_LTSURGE_HNS: case TRAINER_ERIKA_HNS:
    case TRAINER_SABRINA_HNS: case TRAINER_JANINE_HNS:
    case TRAINER_BLAINE_HNS: case TRAINER_BLUE_HNS:
    case TRAINER_BLUE_2_HNS:
    case TRAINER_FALKNER_POSTOBC_HNS:
    case TRAINER_BUGSY_POSTOBC_HNS:
    case TRAINER_WHITNEY_POSTOBC_HNS:
    case TRAINER_MORTY_POSTOBC_HNS:
    case TRAINER_CHUCK_POSTOBC_HNS:
    case TRAINER_JASMINE_POSTOBC_HNS:
    case TRAINER_PRYCE_POSTOBC_HNS:
    case TRAINER_CLAIR_POSTOBC_HNS:
    case TRAINER_BROCK_POSTOBC_HNS:
    case TRAINER_MISTY_POSTOBC_HNS:
    case TRAINER_LTSURGE_POSTOBC_HNS:
    case TRAINER_ERIKA_POSTOBC_HNS:
    case TRAINER_JANINE_POSTOBC_HNS:
    case TRAINER_SABRINA_POSTOBC_HNS:
    case TRAINER_BLAINE_POSTOBC_HNS:
    case TRAINER_BLUE_POSTOBC_HNS:
    case TRAINER_WILL_POSTOBC_HNS:
    case TRAINER_KOGA_POSTOBC_HNS:
    case TRAINER_BRUNO_POSTOBC_HNS:
    case TRAINER_KAREN_POSTOBC_HNS:
    case TRAINER_LANCE_POSTOBC_HNS:
        count = partySize; break;
    default:
        count = 0; break;
    }
    return count < partySize ? count : partySize;
}

static bool32 MegaMode_PreviouslyUsedSpecies(u16 species, struct Pokemon *party, u8 count)
{
    for (u32 i = 0; i < count; i++)
        if (GetMonData(&party[i], MON_DATA_SPECIES) == species)
            return TRUE;
    return FALSE;
}

u16 MegaMode_SelectThemedSpecies(enum Type theme, u16 trainerId, u8 slot, struct Pokemon *party, u8 previousCount)
{
    u16 chosen = SPECIES_NONE;
    u32 matching = 0;

    // First prefer distinct base species; permit duplicates when a theme has
    // fewer legal Megas than the trainer has original party slots.
    for (u32 allowDuplicate = 0; allowDuplicate < 2; allowDuplicate++)
    {
        matching = 0;
#if RANDOMIZER_AVAILABLE
        struct Sfc32State rng = RandomizerRandSeed(RANDOMIZER_REASON_TRAINER_PARTY,
                                                   ((u32)trainerId << 8) | slot,
                                                   0x4D454741 + allowDuplicate);
#endif
        for (u32 species = 1; species < NUM_SPECIES; species++)
        {
            if (gSpeciesInfo[species].isMegaEvolution
             || MegaMode_GetStoneForSpecies(species, theme) == ITEM_NONE
             || (!allowDuplicate && MegaMode_PreviouslyUsedSpecies(species, party, previousCount)))
                continue;

            matching++;
#if RANDOMIZER_AVAILABLE
            if (RandomizerNextRange(&rng, matching) == 0)
#else
            if (Random() % matching == 0)
#endif
                chosen = species;
        }
        if (chosen != SPECIES_NONE)
            return chosen;
    }
    return SPECIES_NONE;
}

u16 MegaMode_RerollOffThemeMega(u16 species, enum Type theme, u16 trainerId, u8 slot)
{
    // Random off-theme slots may be ANY species, except Mega-capable ones with
    // a nonmatching Mega type, which would violate the boss battle theme.
    if (theme == TYPE_NONE
     || MegaMode_GetStoneForSpecies(species, TYPE_NONE) == ITEM_NONE
     || MegaMode_GetStoneForSpecies(species, theme) != ITEM_NONE)
        return species;

#if RANDOMIZER_AVAILABLE
    for (u32 attempt = 0; attempt < 32; attempt++)
    {
        u16 replacement = RandomizeMon(RANDOMIZER_REASON_TRAINER_PARTY,
                                      GetRandomizerOption(RANDOMIZER_OPTION_SPECIES_MODE),
                                      ((u32)trainerId << 16) | ((u32)slot << 8) | attempt,
                                      species);
        if (replacement > SPECIES_NONE && replacement < NUM_SPECIES
         && (MegaMode_GetStoneForSpecies(replacement, TYPE_NONE) == ITEM_NONE
          || MegaMode_GetStoneForSpecies(replacement, theme) != ITEM_NONE))
            return replacement;
    }
#endif
    // Deterministic safe fallback; avoids an endless loop on restricted pools.
    for (u32 candidate = 1; candidate < NUM_SPECIES; candidate++)
        if (!gSpeciesInfo[candidate].isMegaEvolution
         && MegaMode_GetStoneForSpecies(candidate, TYPE_NONE) == ITEM_NONE)
            return candidate;

    return species;
}
