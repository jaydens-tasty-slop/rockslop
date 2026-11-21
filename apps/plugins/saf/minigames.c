/* SAF Minigames (drummyfish, CC0) as a Rockbox plugin. */
#define SAF_PLATFORM_ROCKBOX 1
#include "plugin.h"
#include "minigames.h"

#define SAFRB_HINT "A/B=game keys"
#define SAFRB_HELP "Minigames: snake,", "minesweeper, 2048,", "blocks and runner.", "", \
  "Arrows move/steer.", "SELECT: A (confirm).", \
  "SEL+LEFT/RIGHT: B,", "SEL+DOWN: C.", "", "Hold SEL+UP: menu."
#define SAFRB_ABOUT "Minigames", "by drummyfish", "(Miloslav Ciz),", "CC0 1.0.", \
  "codeberg.org/", "drummyfish/SAF"

#include "saf_main.h"
