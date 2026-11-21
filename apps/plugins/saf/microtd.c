/* microTD (drummyfish, CC0) as a Rockbox plugin via the SAF frontend. */
#define SAF_PLATFORM_ROCKBOX 1
#include "plugin.h"
#include "utd.h"

#define SAFRB_HINT "A=build/pick"
#define SAFRB_HELP "microTD tower defense.", "", \
  "Move the cursor with", "the arrows.", "", \
  "SELECT: build a tower,", "upgrade or confirm.", \
  "SEL+LEFT/RIGHT: B", "(cancel, scroll map).", \
  "SEL+DOWN: C.", "", "Hold SEL+UP: menu."
#define SAFRB_ABOUT "microTD", "by drummyfish", "(Miloslav Ciz),", "CC0 1.0.", \
  "codeberg.org/", "drummyfish/SAF"

#include "saf_main.h"
