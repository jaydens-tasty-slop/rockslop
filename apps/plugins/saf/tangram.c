/* SAF Tangram (drummyfish, CC0) as a Rockbox plugin. */
#define SAF_PLATFORM_ROCKBOX 1
#include "plugin.h"
#include "tangram.h"

#define SAFRB_HINT "A/B=pick/rotate"
#define SAFRB_HELP "Tangram puzzle.", "", "Arrows move the", "cursor or piece.", \
  "SELECT: A (pick/drop).", "SEL+LEFT/RIGHT: B", "(rotate), SEL+DOWN: C", \
  "(flip).", "", "Hold SEL+UP: menu."
#define SAFRB_ABOUT "Tangram", "by drummyfish", "(Miloslav Ciz),", "CC0 1.0.", \
  "codeberg.org/", "drummyfish/SAF"

#include "saf_main.h"
