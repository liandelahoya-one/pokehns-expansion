#ifndef GUARD_MEGA_MODE_H
#define GUARD_MEGA_MODE_H

#include "global.h"
#include "pokemon.h"

// Mega Mode is only set by the randomizer startup menu. The ordinary game
// mechanics are left unchanged when this option is off.
bool32 IsMegaRandomizerModeEnabled(void);

// Convert a direct Mega/Primal species into its battle-ready base form.
u16 MegaMode_NormalizeBattleForm(u16 species);

// Consult form-change tables for Mega Stones and Primal Orbs.
enum Item MegaMode_GetStoneForSpecies(u16 species, enum Type requiredMegaType);
// Recognizes split Mega Stones and both Primal Orbs.
bool32 MegaMode_SpeciesHasStone(u16 species, enum Item stone);
void MegaMode_GrantStonesForSpecies(u16 species);

// Three repeatable, Mega-capable species choices for Elm's laboratory.
// The native script hook updates PLAYER_STARTER_SPECIES (VAR_TEMP_2) to
// ensure the preview, dialogue, and received Pokemon all agree.
u16 MegaMode_GetStarterSpecies(u8 slot);
void MegaMode_OverrideLabStarterSpecies(void);

// Trainer parties are generated using their original size. Only forced Mega
// slots are replaced with a compatible type-themed base species.
enum Type MegaMode_GetTrainerTheme(u16 trainerId);
u8 MegaMode_GetTrainerMegaCount(u16 trainerId, u8 partySize);
u16 MegaMode_SelectThemedSpecies(enum Type theme, u16 trainerId, u8 slot, struct Pokemon *party, u8 previousCount);
u16 MegaMode_RerollOffThemeMega(u16 species, enum Type theme, u16 trainerId, u8 slot);

#endif // GUARD_MEGA_MODE_H
