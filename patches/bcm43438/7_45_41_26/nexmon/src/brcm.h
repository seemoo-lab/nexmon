#pragma once

#include "../include/types.h"
#define WL_CHANSPEC_CHAN_MASK       0x00ff
#define CHSPEC_CHANNEL(chspec)  ((uint8_t)((chspec) & WL_CHANSPEC_CHAN_MASK))
