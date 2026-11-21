/* PokittoLib project settings for RoadTrip (the options the game sets upstream). */
#define PROJ_SCREENMODE MODE13
#define PROJ_FPS 60
#define PROJ_AUD_FREQ 8000

/* Rockbox port: the game's debug printfs go nowhere, and it uses Arduino's
 * boolean. */
#define printf(...) ((void)0)
typedef bool boolean;
#define WHITE 0     /* PokittoLib's GBcompatibility.h values, as upstream */
#define BLACK 1

template<typename T> static inline T max(T a, T b) { return a > b ? a : b; }
template<typename T> static inline T min(T a, T b) { return a < b ? a : b; }
