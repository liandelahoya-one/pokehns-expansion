#include "global.h"
#include "challenge_menu.h"
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

// Battle-only Mega and Primal forms are never directly spawned.
u16 MegaMode_NormalizeBattleForm(u16 species)
{
    if (species > SPECIES_NONE && species < NUM_SPECIES
     && IsSpeciesEnabled(species)
     && (gSpeciesInfo[species].isMegaEvolution || gSpeciesInfo[species].isPrimalReversion))
        return GET_BASE_SPECIES_ID(species);
    return species;
}

// Mega Stones and Red/Blue Orbs all use item-triggered form changes.
static bool32 MegaMode_IsHeldItemTransformation(const struct FormChange *change)
{
    if (change->targetSpecies >= NUM_SPECIES || change->param1 == ITEM_NONE
     || !IsSpeciesEnabled(change->targetSpecies))
        return FALSE;
    return (change->method == FORM_CHANGE_BATTLE_MEGA_EVOLUTION_ITEM
         && gSpeciesInfo[change->targetSpecies].isMegaEvolution)
        || (change->method == FORM_CHANGE_BATTLE_PRIMAL_REVERSION
         && gSpeciesInfo[change->targetSpecies].isPrimalReversion);
}

enum Item MegaMode_GetStoneForSpecies(u16 species, enum Type requiredMegaType)
{
    const struct FormChange *changes;

    // GetSpeciesFormChanges sanitizes its input and intentionally crashes on
    // disabled entries. The species IDs are sparse, even below NUM_SPECIES.
    if (species <= SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return ITEM_NONE;

    changes = GetSpeciesFormChanges(species);
    if (changes == NULL)
        return ITEM_NONE;

    for (u32 i = 0; changes[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        const struct FormChange *change = &changes[i];

        if (!MegaMode_IsHeldItemTransformation(change))
            continue;

        if (requiredMegaType != TYPE_NONE
         && gSpeciesInfo[change->targetSpecies].types[0] != requiredMegaType
         && gSpeciesInfo[change->targetSpecies].types[1] != requiredMegaType)
            continue;

        return change->param1;
    }
    return ITEM_NONE;
}

// Check all legal Mega stones for a species, not just the first form
// in its table. Boss AI must recognize X/Y and original/Z variants.
bool32 MegaMode_SpeciesHasStone(u16 species, enum Item stone)
{
    const struct FormChange *changes;

    if (stone == ITEM_NONE || species <= SPECIES_NONE
     || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return FALSE;

    changes = GetSpeciesFormChanges(species);
    if (changes == NULL)
        return FALSE;

    for (u32 i = 0; changes[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        if (changes[i].param1 == stone && MegaMode_IsHeldItemTransformation(&changes[i]))
            return TRUE;
    }
    return FALSE;
}

void MegaMode_GrantStonesForSpecies(u16 species)
{
    const struct FormChange *changes;

    if (!IsMegaRandomizerModeEnabled() || species <= SPECIES_NONE
     || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return;

    changes = GetSpeciesFormChanges(species);
    if (changes == NULL)
        return;

    // Grant every split Mega Stone, or Groudon/Kyogre's Primal Orb.
    for (u32 i = 0; changes[i].method != FORM_CHANGE_TERMINATOR; i++)
    {
        enum Item stone = changes[i].param1;
        if (!MegaMode_IsHeldItemTransformation(&changes[i]))
            continue;

        if (CheckBagHasItem(stone, 1) || CheckPCHasItem(stone, 1))
            continue;

        // Never silently replace other items when the bag is full.
        if (!AddBagItem(stone, 1))
            AddPCItem(stone, 1);
    }
}

// Mega Mode starter picks must stay identical across all three Poké Balls,
// including when the player declines an option and checks another one.
#define MEGA_MODE_STARTER_COUNT 3
EWRAM_DATA static u16 sMegaModeStarterChoices[MEGA_MODE_STARTER_COUNT] = {SPECIES_NONE};
EWRAM_DATA static u32 sMegaModeStarterSeed;
EWRAM_DATA static u16 sMegaModeStarterSettings;

u16 MegaMode_GetStarterSpecies(u8 slot)
{
    u32 seed = 0;
    u16 settings = 0;
    u16 chosen[MEGA_MODE_STARTER_COUNT] = {SPECIES_NONE};
    u32 eligibleCount = 0;

    if (!IsMegaRandomizerModeEnabled() || slot >= MEGA_MODE_STARTER_COUNT)
        return SPECIES_NONE;

#if RANDOMIZER_AVAILABLE
    seed = GetRandomizerSeed();
    settings = (gSaveBlock3Ptr->challengeSettings.tx_Random_GenScope << 0)
             | (gSaveBlock3Ptr->challengeSettings.tx_Random_IncludeLegendaries << 1)
             | (gSaveBlock3Ptr->challengeSettings.tx_Random_Type << 2)
             | (gSaveBlock3Ptr->challengeSettings.tx_Challenges_OneTypeChallenge << 3);

    if (sMegaModeStarterChoices[0] != SPECIES_NONE
     && sMegaModeStarterSeed == seed
     && sMegaModeStarterSettings == settings)
        return sMegaModeStarterChoices[slot];

    // The abilities stream is deterministic even when Chaos randomizer is
    // active. This prevents the three choices changing between interactions.
    struct Sfc32State rng = RandomizerRandSeed(RANDOMIZER_REASON_ABILITIES,
                                              0x4D454741, 0x53544152);
#endif

    // Prefer candidates that also satisfy a simultaneous One Type Challenge.
    // If that has no eligible Mega forms, fall back to the Mega-capable pool.
    for (u32 pass = 0; pass < 2; pass++)
    {
        eligibleCount = 0;
        for (u32 species = 1; species < NUM_SPECIES; species++)
        {
            if (!IsSpeciesEnabled(species)
             || gSpeciesInfo[species].isMegaEvolution
             || gSpeciesInfo[species].isPrimalReversion
             || MegaMode_GetStoneForSpecies(species, TYPE_NONE) == ITEM_NONE)
                continue;
#if RANDOMIZER_AVAILABLE
            if (!IsSpeciesInGenScope(species))
                continue;
            if (!gSaveBlock3Ptr->challengeSettings.tx_Random_IncludeLegendaries
             && (gSpeciesInfo[species].isRestrictedLegendary
              || gSpeciesInfo[species].isSubLegendary
              || gSpeciesInfo[species].isMythical))
                continue;
#endif
            if (pass == 0 && IsOneTypeChallengeActive()
             && !DoesSpeciesPassOneTypeChallenge(species))
                continue;

            // Reservoir sampling: each eligible base species has an equal
            // chance to appear, without allocating a large candidate array.
            if (eligibleCount < MEGA_MODE_STARTER_COUNT)
            {
                chosen[eligibleCount] = species;
            }
            else
            {
#if RANDOMIZER_AVAILABLE
                u32 replaceIndex = RandomizerNextRange(&rng, eligibleCount + 1);
#else
                u32 replaceIndex = Random() % (eligibleCount + 1);
#endif
                if (replaceIndex < MEGA_MODE_STARTER_COUNT)
                    chosen[replaceIndex] = species;
            }
            eligibleCount++;
        }
        if (eligibleCount > 0)
            break;
    }

    // Small monotype pools can have only one or two distinct Mega Pokémon;
    // repeating an eligible one is preferable to showing an ineligible starter.
    if (eligibleCount > 0)
    {
        for (u32 i = eligibleCount; i < MEGA_MODE_STARTER_COUNT; i++)
            chosen[i] = chosen[i % eligibleCount];

        for (u32 i = 0; i < MEGA_MODE_STARTER_COUNT; i++)
            sMegaModeStarterChoices[i] = chosen[i];
        sMegaModeStarterSeed = seed;
        sMegaModeStarterSettings = settings;
    }

    return chosen[slot];
}

void MegaMode_OverrideLabStarterSpecies(void)
{
    u16 starter = MegaMode_GetStarterSpecies(VarGet(VAR_STARTER_MON));
    if (starter != SPECIES_NONE)
        VarSet(VAR_TEMP_2, starter);
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
    // Gym trainers are keyed from the actual Johto/Kanto gym map scripts.
    case TRAINER_ROD_HNS:
    case TRAINER_ABE_HNS:
        return TYPE_FLYING;
    case TRAINER_AL_HNS:
    case TRAINER_JOSH_HNS:
    case TRAINER_BENNY_HNS:
    case TRAINER_AMY_AND_MAY_HNS:
        return TYPE_BUG;
    case TRAINER_CARRIE_HNS:
    case TRAINER_BRIDGET_HNS:
    case TRAINER_VICTORIA_HNS:
    case TRAINER_SAMANTHA_HNS:
        return TYPE_NORMAL;
    case TRAINER_JEFFREY_HNS:
    case TRAINER_PING_HNS:
    case TRAINER_MARTHA_HNS:
    case TRAINER_GRACE_HNS:
        return TYPE_GHOST;
    case TRAINER_YOSHI_HNS:
    case TRAINER_LAO_HNS:
    case TRAINER_NOB_HNS:
    case TRAINER_LUNG_HNS:
        return TYPE_FIGHTING;
    case TRAINER_RONALD_HNS:
    case TRAINER_BRAD_HNS:
    case TRAINER_DOUGLAS_HNS:
    case TRAINER_ROXANNE_HNS:
    case TRAINER_CLARISSA_HNS:
        return TYPE_ICE;
    case TRAINER_CODY_HNS:
    case TRAINER_FRAN_HNS:
    case TRAINER_PAUL_HNS:
    case TRAINER_MIKE_HNS:
    case TRAINER_LOLA_HNS:
        return TYPE_DRAGON;
    case TRAINER_JERRY_HNS:
        return TYPE_ROCK;
    case TRAINER_DIANA_HNS:
    case TRAINER_BRIANA_HNS:
    case TRAINER_PARKER_HNS:
        return TYPE_WATER;
    case TRAINER_GREGORY_HNS:
    case TRAINER_VINCENT_HNS:
    case TRAINER_HORTON_HNS:
        return TYPE_ELECTRIC;
    case TRAINER_MICHELLE_HNS:
    case TRAINER_TANYA_HNS:
    case TRAINER_JULIA_HNS:
    case TRAINER_JO_AND_ZOE_HNS:
        return TYPE_GRASS;
    case TRAINER_REBECCA_HNS:
    case TRAINER_FRANKLIN_HNS:
    case TRAINER_DORIS_HNS:
    case TRAINER_JARED_HNS:
        return TYPE_PSYCHIC;
    case TRAINER_ALICE_HNS:
    case TRAINER_LINDA_HNS:
    case TRAINER_CINDY_HNS:
    case TRAINER_BARRY_HNS:
        return TYPE_POISON;
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
    // Every gym leader is a full Mega team from their first encounter.
    // Rival, Rocket executive and other named boss battles follow the same
    // rule; their original party sizes and levels are retained.
    case TRAINER_FALKNER_1_HNS:
    case TRAINER_FALKNER_2_HNS:
    case TRAINER_BUGSY_1_HNS:
    case TRAINER_BUGSY_2_HNS:
    case TRAINER_WHITNEY_1_HNS:
    case TRAINER_WHITNEY_2_HNS:
    case TRAINER_MORTY_1_HNS:
    case TRAINER_MORTY_2_HNS:
    case TRAINER_CHUCK_1_HNS:
    case TRAINER_CHUCK_2_HNS:
    case TRAINER_CHUCK_1_2_HNS:
    case TRAINER_CHUCK_1_3_HNS:
    case TRAINER_JASMINE_1_HNS:
    case TRAINER_JASMINE_2_HNS:
    case TRAINER_JASMINE_1_2_HNS:
    case TRAINER_JASMINE_1_3_HNS:
    case TRAINER_PRYCE_1_HNS:
    case TRAINER_PRYCE_2_HNS:
    case TRAINER_PRYCE_1_2_HNS:
    case TRAINER_PRYCE_1_3_HNS:
    // Story rivals, Rocket executives, Eusine and Giovanni.

    case TRAINER_RIVAL_CHIKORITA_1_HNS:
    case TRAINER_RIVAL_CHIKORITA_2_HNS:
    case TRAINER_RIVAL_CHIKORITA_3_HNS:
    case TRAINER_RIVAL_CHIKORITA_4_HNS:
    case TRAINER_RIVAL_CHIKORITA_5_HNS:
    case TRAINER_RIVAL_CHIKORITA_6_HNS:
    case TRAINER_RIVAL_CHIKORITA_7_HNS:
    case TRAINER_RIVAL_CYNDAQUIL_1_HNS:
    case TRAINER_RIVAL_CYNDAQUIL_2_HNS:
    case TRAINER_RIVAL_CYNDAQUIL_3_HNS:
    case TRAINER_RIVAL_CYNDAQUIL_4_HNS:
    case TRAINER_RIVAL_CYNDAQUIL_5_HNS:
    case TRAINER_RIVAL_CYNDAQUIL_6_HNS:
    case TRAINER_RIVAL_CYNDAQUIL_7_HNS:
    case TRAINER_RIVAL_TOTODILE_1_HNS:
    case TRAINER_RIVAL_TOTODILE_2_HNS:
    case TRAINER_RIVAL_TOTODILE_3_HNS:
    case TRAINER_RIVAL_TOTODILE_4_HNS:
    case TRAINER_RIVAL_TOTODILE_5_HNS:
    case TRAINER_RIVAL_TOTODILE_6_HNS:
    case TRAINER_RIVAL_TOTODILE_7_HNS:
    case TRAINER_PROTON_1_HNS:
    case TRAINER_PROTON_2_HNS:
    case TRAINER_PETREL_1_HNS:
    case TRAINER_PETREL_2_HNS:
    case TRAINER_ARCHER_HNS:
    case TRAINER_ARCHER_1_HNS:
    case TRAINER_ARCHER_4_HNS:
    case TRAINER_ARCHER_5_HNS:
    case TRAINER_ARIANA_1_HNS:
    case TRAINER_ARIANA_2_HNS:
    case TRAINER_EUSINE_HNS:
    case TRAINER_GIOVANNI_HNS:
        count = partySize; break;
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
    case TRAINER_RED_HNS:
    case TRAINER_RED_POSTOBC_HNS:
    // The five Kimono Girls are fixed story-boss encounters.
    case TRAINER_NAOKO_HNS:
    case TRAINER_SAYO_HNS:
    case TRAINER_ZUKI_HNS:
    case TRAINER_KUNI_HNS:
    case TRAINER_MIKI_HNS:
        count = partySize; break;
    // Progressive minimum Mega counts for each gym's actual trainers.
    case TRAINER_ROD_HNS:
    case TRAINER_ABE_HNS:
    case TRAINER_AL_HNS:
    case TRAINER_JOSH_HNS:
    case TRAINER_BENNY_HNS:
    case TRAINER_AMY_AND_MAY_HNS:
    case TRAINER_CARRIE_HNS:
    case TRAINER_BRIDGET_HNS:
    case TRAINER_VICTORIA_HNS:
    case TRAINER_SAMANTHA_HNS:
    case TRAINER_JEFFREY_HNS:
    case TRAINER_PING_HNS:
    case TRAINER_MARTHA_HNS:
    case TRAINER_GRACE_HNS:
        count = 1; break;
    case TRAINER_YOSHI_HNS:
    case TRAINER_LAO_HNS:
    case TRAINER_NOB_HNS:
    case TRAINER_LUNG_HNS:
        count = 2; break;
    case TRAINER_RONALD_HNS:
    case TRAINER_BRAD_HNS:
    case TRAINER_DOUGLAS_HNS:
    case TRAINER_ROXANNE_HNS:
    case TRAINER_CLARISSA_HNS:
        count = 3; break;
    case TRAINER_CODY_HNS:
    case TRAINER_FRAN_HNS:
    case TRAINER_PAUL_HNS:
    case TRAINER_MIKE_HNS:
    case TRAINER_LOLA_HNS:
    case TRAINER_JERRY_HNS:
    case TRAINER_DIANA_HNS:
    case TRAINER_BRIANA_HNS:
    case TRAINER_PARKER_HNS:
    case TRAINER_GREGORY_HNS:
    case TRAINER_VINCENT_HNS:
    case TRAINER_HORTON_HNS:
    case TRAINER_MICHELLE_HNS:
    case TRAINER_TANYA_HNS:
    case TRAINER_JULIA_HNS:
    case TRAINER_JO_AND_ZOE_HNS:
    case TRAINER_REBECCA_HNS:
    case TRAINER_FRANKLIN_HNS:
    case TRAINER_DORIS_HNS:
    case TRAINER_JARED_HNS:
    case TRAINER_ALICE_HNS:
    case TRAINER_LINDA_HNS:
    case TRAINER_CINDY_HNS:
    case TRAINER_BARRY_HNS:
        count = 4; break;
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
            // Avoid even attempting form-table lookups for disabled IDs.
            if (!IsSpeciesEnabled(species)
             || gSpeciesInfo[species].isMegaEvolution
             || gSpeciesInfo[species].isPrimalReversion
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
         && IsSpeciesEnabled(replacement)
         && (MegaMode_GetStoneForSpecies(replacement, TYPE_NONE) == ITEM_NONE
          || MegaMode_GetStoneForSpecies(replacement, theme) != ITEM_NONE))
            return replacement;
    }
#endif
    // Deterministic safe fallback; avoids an endless loop on restricted pools.
    for (u32 candidate = 1; candidate < NUM_SPECIES; candidate++)
        if (IsSpeciesEnabled(candidate)
         && !gSpeciesInfo[candidate].isMegaEvolution
         && !gSpeciesInfo[candidate].isPrimalReversion
         && MegaMode_GetStoneForSpecies(candidate, TYPE_NONE) == ITEM_NONE)
            return candidate;

    return species;
}
