#ifndef GUARD_MEGA_MODE_H
#define GUARD_MEGA_MODE_H

#include "global.h"
#include "pokemon.h"

// Mega Mode is only set by the randomizer startup menu. The ordinary game
// mechanics are left unchanged when this option is off.
bool32 IsMegaRandomizerModeEnabled(void);

// Consult actual form-change tables, not a hard-coded list of stones.
enum Item MegaMode_GetStoneForSpecies(u16 species, enum Type requiredMegaType);
void MegaMode_GrantStonesForSpecies(u16 species);

#endif // GUARD_MEGA_MODE_H
