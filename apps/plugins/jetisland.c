/* Jet Island -- low-RAM Rockbox port of CubeCoders' Jet tropical-island demo.
 * Copyright (c) 2026 CubeCoders Limited. MIT license reproduced below.
 * C / Rockbox adaptation by OpenAI Codex, at the user's request (2026).
 *
 * SOURCE: https://github.com/CubeCoders/JetExamples
 * esp32-tropical-island/main/Island.hpp: all beach, palm and rock geometry,
 * face colors and island layout. Rockbox v2 adds a dramatic 24-second camera
 * tour: two sweeps, shore-level close passes, an overhead climb and a dive.
 * https://github.com/CubeCoders/Jet src/TriangleSpans.hpp: integer edge walker.
 * This is a reduced scene port, not the complete C++ Jet engine. Procedural
 * geometry is baked into compact C arrays: 464 vertices, 554 land triangles.
 * The 72-triangle ocean becomes a horizon fill. Reflections use mirrored land
 * geometry with scanline ripples instead of Jet's previous-field sampling.
 * The scene is silent. No external art, C++ runtime, heap, FPU or CPU1 needed.
 *
 * INSTALL: copy jetisland.c to apps/plugins; add jetisland.c to SOURCES under
 * #if LCD_WIDTH >= 96 && LCD_HEIGHT >= 64
 * and optionally add jetisland,demos to CATEGORIES. Build normally.
 *
 * DISPLAY: native color, grayscale, or ordered-dither monochrome. Render size
 * scales down on larger LCDs, then expands by integer pixels. A 128x160 YP3
 * renders at full resolution. Default maximum logical dimension is 192.
 * JETISLAND_SCALE overrides scaling; logical size must be 64..192 per axis.
 * JETISLAND_COLOR=0 uses an 8-bit luminance working buffer even on color LCDs.
 * On a monochrome target that mode is automatic. The Rockbox core framebuffer
 * is separate and is not saved by disabling color within this plugin.
 *
 * MEMORY at 128x160: color image 40,960 bytes, gray image 20,480 bytes;
 * transformed vertices 2,784, sort state 2,216, geometry 7,216, plus palette,
 * wave tables, a display row, code and stack. No second image or Z-buffer.
 * Const geometry/code occupy plugin RAM on non-XIP targets. Only CPU0 is used.
 * No assumption about the SL6801's actual Rockbox clock or FPU handoff is made.
 *
 * CONTROLS: Left/Right tap or hold orbits; Up/Down zooms. Manual movement
 * pauses the camera tour at its current pose. Short Select resumes/pauses
 * the tour; resuming after manual movement blends back over 1.6 seconds.
 * Double Left opens menu; Double Right cycles Day/Sunset/Night. Double Up/Down
 * raises/lowers camera height (hold the second press to keep changing height).
 * First-tap movement is restored when a double tap is recognized, within
 * 300ms. Any hold/repeat or intervening action cancels double-tap eligibility.
 * Long Select, its repeat AND its release have no demo action anywhere.
 * Cancel/Exit opens menu. Menus: Up/Down choose, Select/Right confirms,
 * Left/Cancel returns. Help scrolls. Quit is always available in the menu.
 * Wheels move the camera / scroll lists; wheel detents never trigger doubles.
 * YP3_PAD has an explicit five-button mapping. PORT_CENTER_BUTTON can override
 * its center mask. Other devices use Rockbox's standard pluginlib mappings,
 * including remote controls and touchscreen 3x3 zones (restored on exit).
 *
 * MENU: camera tour, three palettes, reflection on/off, wireframe,
 * camera reset, HUD/performance overlay, help, quit. Settings are session-only.
 * Animation is elapsed-time based; slow frames do not speed up the scene.
 * FPS is measured end-to-end, including LCD updates and frame pacing.
 * Painter sorting has the usual limitations for intersecting triangles;
 * wireframe intentionally shows every triangle, including hidden edges.
 * Exact YP3 RAM use and frame rate require your ARM build and real hardware.
 */
#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "lib/helper.h"
#include "lib/playback_control.h"

#if LCD_WIDTH < 96 || LCD_HEIGHT < 64
#error "Jet Island requires a bitmap LCD of at least 96x64"
#endif
#ifndef JETISLAND_SCALE
#define JETISLAND_SCALE ((MAX(LCD_WIDTH,LCD_HEIGHT)+191)/192)
#endif
#define IW (LCD_WIDTH/JETISLAND_SCALE)
#define IH (LCD_HEIGHT/JETISLAND_SCALE)
#if JETISLAND_SCALE < 1 || IW < 64 || IH < 64 || IW > 192 || IH > 192
#error "Choose JETISLAND_SCALE so each logical dimension is 64..192"
#endif
#ifndef JETISLAND_COLOR
#ifdef HAVE_LCD_COLOR
#define JETISLAND_COLOR 1
#else
#define JETISLAND_COLOR 0
#endif
#endif
#if JETISLAND_COLOR && !defined(HAVE_LCD_COLOR)
#error "JETISLAND_COLOR=1 requires a color LCD"
#endif
struct vertex { int16_t x,y,z; };
struct face { uint16_t a,b,c; uint8_t color; };

/* Geometry baked from the unmodified upstream beach/palm/rock generators. */
static const struct vertex vertices[]={
{810,1,0},
{787,1,152},
{665,1,277},
{519,1,375},
{364,1,456},
{184,1,498},
{0,1,493},
{-178,1,479},
{-377,1,472},
{-588,1,425},
{-720,1,301},
{-725,1,140},
{-672,1,0},
{-645,1,-125},
{-619,1,-258},
{-529,1,-383},
{-377,1,-472},
{-199,1,-537},
{0,1,-578},
{206,1,-556},
{364,1,-456},
{460,1,-333},
{563,1,-235},
{707,1,-137},
{711,7,0},
{690,7,134},
{583,7,243},
{455,7,329},
{319,7,400},
{162,7,437},
{0,7,433},
{-156,7,421},
{-331,7,414},
{-516,7,373},
{-632,7,264},
{-636,7,123},
{-589,7,0},
{-566,7,-110},
{-543,7,-227},
{-464,7,-336},
{-331,7,-414},
{-175,7,-471},
{0,7,-507},
{181,7,-487},
{319,7,-400},
{404,7,-292},
{494,7,-206},
{620,7,-120},
{583,42,0},
{566,42,110},
{478,42,200},
{373,42,270},
{262,42,328},
{133,42,358},
{0,42,355},
{-128,42,345},
{-271,42,340},
{-423,42,306},
{-518,42,216},
{-521,42,101},
{-483,42,0},
{-464,42,-90},
{-445,42,-186},
{-381,42,-275},
{-271,42,-340},
{-143,42,-386},
{0,42,-416},
{148,42,-400},
{262,42,-328},
{331,42,-239},
{405,42,-169},
{508,42,-99},
{398,73,0},
{387,73,75},
{326,73,136},
{255,73,184},
{179,73,224},
{91,73,245},
{0,73,242},
{-87,73,236},
{-185,73,232},
{-289,73,209},
{-354,73,148},
{-356,73,69},
{-330,73,0},
{-317,73,-61},
{-304,73,-127},
{-260,73,-188},
{-185,73,-232},
{-98,73,-264},
{0,73,-284},
{101,73,-273},
{179,73,-224},
{226,73,-163},
{277,73,-116},
{347,73,-67},
{0,96,0},
{726,3,0},
{720,5,0},
{705,3,137},
{700,5,136},
{596,3,249},
{591,5,247},
{465,3,336},
{461,5,334},
{326,3,408},
{324,5,405},
{165,3,446},
{164,5,443},
{0,3,442},
{0,5,439},
{-159,3,430},
{-158,5,426},
{-338,3,424},
{-336,5,420},
{-527,3,381},
{-523,5,378},
{-646,3,270},
{-641,5,267},
{-650,3,126},
{-645,5,125},
{-602,3,0},
{-598,5,0},
{-578,3,-112},
{-573,5,-111},
{-555,3,-232},
{-550,5,-230},
{-474,3,-343},
{-471,5,-340},
{-338,3,-424},
{-336,5,-420},
{-178,3,-482},
{-177,5,-478},
{0,3,-518},
{0,5,-514},
{185,3,-498},
{183,5,-494},
{326,3,-408},
{324,5,-405},
{413,3,-298},
{409,5,-296},
{505,3,-211},
{501,5,-209},
{634,3,-123},
{629,5,-122},
{-215,72,-40},
{-229,72,-21},
{-251,72,-28},
{-251,72,-52},
{-229,72,-59},
{-209,240,-38},
{-221,240,-21},
{-240,240,-28},
{-240,240,-48},
{-221,240,-54},
{-184,409,-31},
{-194,409,-17},
{-209,409,-22},
{-209,409,-39},
{-194,409,-44},
{-141,577,-19},
{-148,577,-8},
{-161,577,-12},
{-161,577,-25},
{-148,577,-29},
{-152,577,-19},
{-64,599,40},
{-55,622,6},
{-46,599,-28},
{73,501,39},
{-152,577,-19},
{-142,599,93},
{-108,622,78},
{-74,599,63},
{-50,501,207},
{-152,577,-19},
{-238,599,64},
{-203,622,82},
{-168,599,100},
{-272,501,215},
{-152,577,-19},
{-257,599,-33},
{-250,622,1},
{-243,599,35},
{-380,501,27},
{-152,577,-19},
{-209,599,-116},
{-233,622,-87},
{-257,599,-59},
{-341,501,-178},
{-152,577,-19},
{-110,599,-131},
{-149,622,-132},
{-188,599,-133},
{-145,501,-281},
{-152,577,-19},
{-51,599,-51},
{-72,622,-79},
{-93,599,-107},
{33,501,-159},
{125,72,110},
{111,72,129},
{89,72,122},
{89,72,98},
{111,72,91},
{114,272,118},
{102,272,134},
{83,272,128},
{83,272,108},
{102,272,101},
{86,472,141},
{76,472,154},
{60,472,149},
{60,472,132},
{76,472,127},
{41,672,179},
{33,672,189},
{21,672,185},
{21,672,172},
{33,672,168},
{30,672,179},
{-86,703,229},
{-58,726,259},
{-30,703,290},
{-174,582,365},
{30,672,179},
{-88,703,116},
{-95,726,159},
{-102,703,203},
{-261,582,133},
{30,672,179},
{4,703,39},
{-37,726,62},
{-77,703,85},
{-124,582,-92},
{30,672,179},
{112,703,84},
{74,726,69},
{35,703,53},
{132,582,-77},
{30,672,179},
{164,703,185},
{151,726,142},
{138,703,100},
{311,582,94},
{30,672,179},
{114,703,294},
{140,726,255},
{167,703,217},
{286,582,356},
{30,672,179},
{-3,703,300},
{38,726,297},
{79,703,294},
{49,582,454},
{300,72,-70},
{286,72,-51},
{264,72,-58},
{264,72,-82},
{286,72,-89},
{300,204,-63},
{288,204,-47},
{269,204,-53},
{269,204,-73},
{288,204,-79},
{305,335,-42},
{295,335,-29},
{279,335,-34},
{279,335,-50},
{295,335,-55},
{315,467,-7},
{308,467,3},
{295,467,-1},
{295,467,-14},
{308,467,-18},
{304,467,-7},
{307,480,75},
{333,503,65},
{358,480,56},
{370,408,162},
{304,467,-7},
{237,480,50},
{262,503,65},
{288,480,79},
{207,408,160},
{304,467,-7},
{212,480,-25},
{217,503,5},
{221,480,36},
{101,408,23},
{304,467,-7},
{266,480,-81},
{247,503,-61},
{229,480,-41},
{172,408,-131},
{304,467,-7},
{340,480,-88},
{311,503,-90},
{282,480,-93},
{319,408,-200},
{304,467,-7},
{395,480,-31},
{377,503,-57},
{360,480,-82},
{474,408,-123},
{304,467,-7},
{371,480,42},
{379,503,16},
{387,480,-10},
{478,408,47},
{-30,72,-200},
{-44,72,-181},
{-66,72,-188},
{-66,72,-212},
{-44,72,-219},
{-35,212,-208},
{-46,212,-192},
{-65,212,-198},
{-65,212,-218},
{-46,212,-224},
{-43,352,-231},
{-52,352,-218},
{-68,352,-223},
{-68,352,-239},
{-52,352,-244},
{-54,492,-270},
{-62,492,-259},
{-74,492,-263},
{-74,492,-276},
{-62,492,-280},
{-65,492,-270},
{-54,507,-357},
{-83,530,-351},
{-111,507,-345},
{-106,429,-459},
{-65,492,-270},
{15,507,-319},
{-9,530,-338},
{-33,507,-358},
{65,429,-429},
{-65,492,-270},
{28,507,-236},
{29,530,-269},
{29,507,-301},
{153,429,-267},
{-65,492,-270},
{-37,507,-186},
{-14,530,-204},
{9,507,-222},
{54,429,-117},
{-65,492,-270},
{-116,507,-191},
{-86,530,-184},
{-56,507,-177},
{-113,429,-70},
{-65,492,-270},
{-164,507,-260},
{-150,530,-230},
{-136,507,-200},
{-263,429,-177},
{-65,492,-270},
{-127,507,-333},
{-139,530,-307},
{-152,507,-281},
{-238,429,-356},
{-275,72,180},
{-289,72,199},
{-311,72,192},
{-311,72,168},
{-289,72,161},
{-285,199,177},
{-296,199,194},
{-315,199,187},
{-315,199,167},
{-296,199,161},
{-308,325,170},
{-318,325,183},
{-333,325,178},
{-333,325,162},
{-318,325,157},
{-344,452,157},
{-352,452,168},
{-364,452,164},
{-364,452,151},
{-352,452,147},
{-355,452,157},
{-417,463,106},
{-426,486,131},
{-435,463,156},
{-519,395,96},
{-355,452,157},
{-354,463,73},
{-380,486,81},
{-407,463,90},
{-413,395,-20},
{-355,452,157},
{-284,463,103},
{-309,486,86},
{-333,463,70},
{-247,395,-8},
{-355,452,157},
{-278,463,177},
{-281,486,151},
{-283,463,124},
{-181,395,142},
{-355,452,157},
{-320,463,234},
{-300,486,215},
{-280,463,196},
{-227,395,292},
{-355,452,157},
{-396,463,238},
{-367,486,242},
{-337,463,246},
{-382,395,353},
{-355,452,157},
{-433,463,173},
{-420,486,196},
{-407,463,218},
{-505,395,247},
{617,8,100},
{558,8,141},
{488,8,111},
{504,8,52},
{583,8,46},
{593,54,96},
{555,54,122},
{510,54,104},
{521,54,66},
{571,54,62},
{557,70,90},
{639,8,146},
{588,8,156},
{561,8,123},
{594,8,93},
{642,8,108},
{627,35,139},
{594,35,145},
{577,35,123},
{598,35,104},
{629,35,114},
{609,45,125},
{-400,8,-211},
{-485,8,-181},
{-549,8,-232},
{-504,8,-294},
{-412,8,-281},
{-425,64,-222},
{-479,64,-202},
{-521,64,-235},
{-492,64,-275},
{-433,64,-266},
{-462,82,-240},
{-491,8,-215},
{-534,8,-191},
{-579,8,-214},
{-563,8,-253},
{-508,8,-253},
{-507,37,-218},
{-535,37,-203},
{-563,37,-218},
{-553,37,-243},
{-518,37,-243},
{-530,47,-225},
};
static const struct face faces[]={
{0,1,25,0},
{25,24,0,0},
{1,2,26,1},
{26,25,1,1},
{2,3,27,2},
{27,26,2,2},
{3,4,28,3},
{28,27,3,3},
{4,5,29,0},
{29,28,4,0},
{5,6,30,1},
{30,29,5,1},
{6,7,31,2},
{31,30,6,2},
{7,8,32,3},
{32,31,7,3},
{8,9,33,0},
{33,32,8,0},
{9,10,34,1},
{34,33,9,1},
{10,11,35,2},
{35,34,10,2},
{11,12,36,3},
{36,35,11,3},
{12,13,37,0},
{37,36,12,0},
{13,14,38,1},
{38,37,13,1},
{14,15,39,2},
{39,38,14,2},
{15,16,40,3},
{40,39,15,3},
{16,17,41,0},
{41,40,16,0},
{17,18,42,1},
{42,41,17,1},
{18,19,43,2},
{43,42,18,2},
{19,20,44,3},
{44,43,19,3},
{20,21,45,0},
{45,44,20,0},
{21,22,46,1},
{46,45,21,1},
{22,23,47,2},
{47,46,22,2},
{23,0,24,3},
{24,47,23,3},
{24,25,49,1},
{49,48,24,1},
{25,26,50,2},
{50,49,25,2},
{26,27,51,3},
{51,50,26,3},
{27,28,52,0},
{52,51,27,0},
{28,29,53,1},
{53,52,28,1},
{29,30,54,2},
{54,53,29,2},
{30,31,55,3},
{55,54,30,3},
{31,32,56,0},
{56,55,31,0},
{32,33,57,1},
{57,56,32,1},
{33,34,58,2},
{58,57,33,2},
{34,35,59,3},
{59,58,34,3},
{35,36,60,0},
{60,59,35,0},
{36,37,61,1},
{61,60,36,1},
{37,38,62,2},
{62,61,37,2},
{38,39,63,3},
{63,62,38,3},
{39,40,64,0},
{64,63,39,0},
{40,41,65,1},
{65,64,40,1},
{41,42,66,2},
{66,65,41,2},
{42,43,67,3},
{67,66,42,3},
{43,44,68,0},
{68,67,43,0},
{44,45,69,1},
{69,68,44,1},
{45,46,70,2},
{70,69,45,2},
{46,47,71,3},
{71,70,46,3},
{47,24,48,0},
{48,71,47,0},
{48,49,73,2},
{73,72,48,2},
{49,50,74,3},
{74,73,49,3},
{50,51,75,0},
{75,74,50,0},
{51,52,76,1},
{76,75,51,1},
{52,53,77,2},
{77,76,52,2},
{53,54,78,3},
{78,77,53,3},
{54,55,79,0},
{79,78,54,0},
{55,56,80,1},
{80,79,55,1},
{56,57,81,2},
{81,80,56,2},
{57,58,82,3},
{82,81,57,3},
{58,59,83,0},
{83,82,58,0},
{59,60,84,1},
{84,83,59,1},
{60,61,85,2},
{85,84,60,2},
{61,62,86,3},
{86,85,61,3},
{62,63,87,0},
{87,86,62,0},
{63,64,88,1},
{88,87,63,1},
{64,65,89,2},
{89,88,64,2},
{65,66,90,3},
{90,89,65,3},
{66,67,91,0},
{91,90,66,0},
{67,68,92,1},
{92,91,67,1},
{68,69,93,2},
{93,92,68,2},
{69,70,94,3},
{94,93,69,3},
{70,71,95,0},
{95,94,70,0},
{71,48,72,1},
{72,95,71,1},
{72,73,96,4},
{73,74,96,5},
{74,75,96,6},
{75,76,96,4},
{76,77,96,5},
{77,78,96,6},
{78,79,96,4},
{79,80,96,5},
{80,81,96,6},
{81,82,96,4},
{82,83,96,5},
{83,84,96,6},
{84,85,96,4},
{85,86,96,5},
{86,87,96,6},
{87,88,96,4},
{88,89,96,5},
{89,90,96,6},
{90,91,96,4},
{91,92,96,5},
{92,93,96,6},
{93,94,96,4},
{94,95,96,5},
{95,72,96,6},
{99,101,102,7},
{102,100,99,7},
{101,103,104,7},
{104,102,101,7},
{103,105,106,7},
{106,104,103,7},
{107,109,110,7},
{110,108,107,7},
{109,111,112,7},
{112,110,109,7},
{111,113,114,7},
{114,112,111,7},
{115,117,118,7},
{118,116,115,7},
{117,119,120,7},
{120,118,117,7},
{119,121,122,7},
{122,120,119,7},
{123,125,126,7},
{126,124,123,7},
{125,127,128,7},
{128,126,125,7},
{127,129,130,7},
{130,128,127,7},
{131,133,134,7},
{134,132,131,7},
{133,135,136,7},
{136,134,133,7},
{135,137,138,7},
{138,136,135,7},
{139,141,142,7},
{142,140,139,7},
{141,143,144,7},
{144,142,141,7},
{143,97,98,7},
{98,144,143,7},
{145,146,151,8},
{151,150,145,8},
{146,147,152,9},
{152,151,146,9},
{147,148,153,10},
{153,152,147,10},
{148,149,154,11},
{154,153,148,11},
{149,145,150,12},
{150,154,149,12},
{150,151,156,8},
{156,155,150,8},
{151,152,157,9},
{157,156,151,9},
{152,153,158,10},
{158,157,152,10},
{153,154,159,11},
{159,158,153,11},
{154,150,155,12},
{155,159,154,12},
{155,156,161,8},
{161,160,155,8},
{156,157,162,9},
{162,161,156,9},
{157,158,163,10},
{163,162,157,10},
{158,159,164,11},
{164,163,158,11},
{159,155,160,12},
{160,164,159,12},
{165,166,167,13},
{165,167,168,14},
{166,169,167,13},
{167,169,168,14},
{170,171,172,14},
{170,172,173,15},
{171,174,172,14},
{172,174,173,15},
{175,176,177,15},
{175,177,178,16},
{176,179,177,15},
{177,179,178,16},
{180,181,182,16},
{180,182,183,13},
{181,184,182,16},
{182,184,183,13},
{185,186,187,13},
{185,187,188,14},
{186,189,187,13},
{187,189,188,14},
{190,191,192,14},
{190,192,193,15},
{191,194,192,14},
{192,194,193,15},
{195,196,197,15},
{195,197,198,16},
{196,199,197,15},
{197,199,198,16},
{200,201,206,8},
{206,205,200,8},
{201,202,207,9},
{207,206,201,9},
{202,203,208,10},
{208,207,202,10},
{203,204,209,11},
{209,208,203,11},
{204,200,205,12},
{205,209,204,12},
{205,206,211,8},
{211,210,205,8},
{206,207,212,9},
{212,211,206,9},
{207,208,213,10},
{213,212,207,10},
{208,209,214,11},
{214,213,208,11},
{209,205,210,12},
{210,214,209,12},
{210,211,216,8},
{216,215,210,8},
{211,212,217,9},
{217,216,211,9},
{212,213,218,10},
{218,217,212,10},
{213,214,219,11},
{219,218,213,11},
{214,210,215,12},
{215,219,214,12},
{220,221,222,13},
{220,222,223,14},
{221,224,222,13},
{222,224,223,14},
{225,226,227,14},
{225,227,228,15},
{226,229,227,14},
{227,229,228,15},
{230,231,232,15},
{230,232,233,16},
{231,234,232,15},
{232,234,233,16},
{235,236,237,16},
{235,237,238,13},
{236,239,237,16},
{237,239,238,13},
{240,241,242,13},
{240,242,243,14},
{241,244,242,13},
{242,244,243,14},
{245,246,247,14},
{245,247,248,15},
{246,249,247,14},
{247,249,248,15},
{250,251,252,15},
{250,252,253,16},
{251,254,252,15},
{252,254,253,16},
{255,256,261,8},
{261,260,255,8},
{256,257,262,9},
{262,261,256,9},
{257,258,263,10},
{263,262,257,10},
{258,259,264,11},
{264,263,258,11},
{259,255,260,12},
{260,264,259,12},
{260,261,266,8},
{266,265,260,8},
{261,262,267,9},
{267,266,261,9},
{262,263,268,10},
{268,267,262,10},
{263,264,269,11},
{269,268,263,11},
{264,260,265,12},
{265,269,264,12},
{265,266,271,8},
{271,270,265,8},
{266,267,272,9},
{272,271,266,9},
{267,268,273,10},
{273,272,267,10},
{268,269,274,11},
{274,273,268,11},
{269,265,270,12},
{270,274,269,12},
{275,276,277,13},
{275,277,278,14},
{276,279,277,13},
{277,279,278,14},
{280,281,282,14},
{280,282,283,15},
{281,284,282,14},
{282,284,283,15},
{285,286,287,15},
{285,287,288,16},
{286,289,287,15},
{287,289,288,16},
{290,291,292,16},
{290,292,293,13},
{291,294,292,16},
{292,294,293,13},
{295,296,297,13},
{295,297,298,14},
{296,299,297,13},
{297,299,298,14},
{300,301,302,14},
{300,302,303,15},
{301,304,302,14},
{302,304,303,15},
{305,306,307,15},
{305,307,308,16},
{306,309,307,15},
{307,309,308,16},
{310,311,316,8},
{316,315,310,8},
{311,312,317,9},
{317,316,311,9},
{312,313,318,10},
{318,317,312,10},
{313,314,319,11},
{319,318,313,11},
{314,310,315,12},
{315,319,314,12},
{315,316,321,8},
{321,320,315,8},
{316,317,322,9},
{322,321,316,9},
{317,318,323,10},
{323,322,317,10},
{318,319,324,11},
{324,323,318,11},
{319,315,320,12},
{320,324,319,12},
{320,321,326,8},
{326,325,320,8},
{321,322,327,9},
{327,326,321,9},
{322,323,328,10},
{328,327,322,10},
{323,324,329,11},
{329,328,323,11},
{324,320,325,12},
{325,329,324,12},
{330,331,332,13},
{330,332,333,14},
{331,334,332,13},
{332,334,333,14},
{335,336,337,14},
{335,337,338,15},
{336,339,337,14},
{337,339,338,15},
{340,341,342,15},
{340,342,343,16},
{341,344,342,15},
{342,344,343,16},
{345,346,347,16},
{345,347,348,13},
{346,349,347,16},
{347,349,348,13},
{350,351,352,13},
{350,352,353,14},
{351,354,352,13},
{352,354,353,14},
{355,356,357,14},
{355,357,358,15},
{356,359,357,14},
{357,359,358,15},
{360,361,362,15},
{360,362,363,16},
{361,364,362,15},
{362,364,363,16},
{365,366,371,8},
{371,370,365,8},
{366,367,372,9},
{372,371,366,9},
{367,368,373,10},
{373,372,367,10},
{368,369,374,11},
{374,373,368,11},
{369,365,370,12},
{370,374,369,12},
{370,371,376,8},
{376,375,370,8},
{371,372,377,9},
{377,376,371,9},
{372,373,378,10},
{378,377,372,10},
{373,374,379,11},
{379,378,373,11},
{374,370,375,12},
{375,379,374,12},
{375,376,381,8},
{381,380,375,8},
{376,377,382,9},
{382,381,376,9},
{377,378,383,10},
{383,382,377,10},
{378,379,384,11},
{384,383,378,11},
{379,375,380,12},
{380,384,379,12},
{385,386,387,13},
{385,387,388,14},
{386,389,387,13},
{387,389,388,14},
{390,391,392,14},
{390,392,393,15},
{391,394,392,14},
{392,394,393,15},
{395,396,397,15},
{395,397,398,16},
{396,399,397,15},
{397,399,398,16},
{400,401,402,16},
{400,402,403,13},
{401,404,402,16},
{402,404,403,13},
{405,406,407,13},
{405,407,408,14},
{406,409,407,13},
{407,409,408,14},
{410,411,412,14},
{410,412,413,15},
{411,414,412,14},
{412,414,413,15},
{415,416,417,15},
{415,417,418,16},
{416,419,417,15},
{417,419,418,16},
{420,421,426,17},
{426,425,420,17},
{425,426,430,18},
{421,422,427,18},
{427,426,421,18},
{426,427,430,19},
{422,423,428,19},
{428,427,422,19},
{427,428,430,20},
{423,424,429,20},
{429,428,423,20},
{428,429,430,17},
{424,420,425,17},
{425,429,424,17},
{429,425,430,18},
{431,432,437,17},
{437,436,431,17},
{436,437,441,18},
{432,433,438,18},
{438,437,432,18},
{437,438,441,19},
{433,434,439,19},
{439,438,433,19},
{438,439,441,20},
{434,435,440,20},
{440,439,434,20},
{439,440,441,17},
{435,431,436,17},
{436,440,435,17},
{440,436,441,18},
{442,443,448,17},
{448,447,442,17},
{447,448,452,18},
{443,444,449,18},
{449,448,443,18},
{448,449,452,19},
{444,445,450,19},
{450,449,444,19},
{449,450,452,20},
{445,446,451,20},
{451,450,445,20},
{450,451,452,17},
{446,442,447,17},
{447,451,446,17},
{451,447,452,18},
{453,454,459,17},
{459,458,453,17},
{458,459,463,18},
{454,455,460,18},
{460,459,454,18},
{459,460,463,19},
{455,456,461,19},
{461,460,455,19},
{460,461,463,20},
{456,457,462,20},
{462,461,456,20},
{461,462,463,17},
{457,453,458,17},
{458,462,457,17},
{462,458,463,18},
};
static const uint32_t original_palette[]={
0xe6c587,0xffe2a0,0xf4d395,0xfadda0,0x75aa3e,0x85bc43,0x639b32,0xc6f1ce,0x94613b,0xbb8852,0xd09d60,0xaa7844,0x785335,0x228546,0x55b94e,0x8bcf50,0x389d40,0x879b97,0xaabab1,0x6a837f,0xc0c5a8,
};
#define VERTEX_COUNT 464
#define FACE_COUNT 554
#define PALETTE_COUNT 21

static const int16_t sin_quarter[257]={
0,101,201,302,402,503,603,704,804,904,1005,1105,1205,1306,1406,1506,1606,1706,1806,1906,2006,2105,2205,2305,2404,2503,2603,2702,2801,2900,2999,3098,3196,3295,3393,3492,3590,3688,3786,3883,3981,4078,4176,4273,4370,4467,4563,4660,4756,4852,4948,5044,5139,5235,5330,5425,5520,5614,5708,5803,5897,5990,6084,6177,6270,6363,6455,6547,6639,6731,6823,6914,7005,7096,7186,7276,7366,7456,7545,7635,7723,7812,7900,7988,8076,8163,8250,8337,8423,8509,8595,8680,8765,8850,8935,9019,9102,9186,9269,9352,9434,9516,9598,9679,9760,9841,9921,10001,10080,10159,10238,10316,10394,10471,10549,10625,10702,10778,10853,10928,11003,11077,11151,11224,11297,11370,11442,11514,11585,11656,11727,11797,11866,11935,12004,12072,12140,12207,12274,12340,12406,12472,12537,12601,12665,12729,12792,12854,12916,12978,13039,13100,13160,13219,13279,13337,13395,13453,13510,13567,13623,13678,13733,13788,13842,13896,13949,14001,14053,14104,14155,14206,14256,14305,14354,14402,14449,14497,14543,14589,14635,14680,14724,14768,14811,14854,14896,14937,14978,15019,15059,15098,15137,15175,15213,15250,15286,15322,15357,15392,15426,15460,15493,15525,15557,15588,15619,15649,15679,15707,15736,15763,15791,15817,15843,15868,15893,15917,15941,15964,15986,16008,16029,16049,16069,16088,16107,16125,16143,16160,16176,16192,16207,16221,16235,16248,16261,16273,16284,16295,16305,16315,16324,16332,16340,16347,16353,16359,16364,16369,16373,16376,16379,16381,16383,16384,16384
};

static const char glyph_chars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:/>+-; ";
static const uint16_t glyph_bits[]={0x2bed,0x6bae,0x3923,0x6b6e,0x79a7,0x79a4,0x396b,0x5bed,0x7497,0x126a,0x5bad,0x4927,0x5fed,0x5ffd,0x2b6a,0x6ba4,0x2b59,0x6bad,0x388e,0x7492,0x5b6f,0x5b6a,0x5bfd,0x5aad,0x5a92,0x72a7,0x7b6f,0x2c97,0x62a7,0x628e,0x5bc9,0x798e,0x39aa,0x7292,0x2aaa,0x2ace,0x2,0x410,0x12a4,0x4454,0x5d0,0x1c0,0x414,0x0,0};


/* RGB565 is an internal arithmetic format; scanout uses Rockbox's native
 * FB_RGBPACK, so RGB/BGR, swapped-endian and 24-bit LCDs are supported. */
static uint16_t rgb(int r,int g,int b)
{ return ((r>>3)<<11)|((g>>2)<<5)|(b>>3); }
static int red(uint16_t c) { return ((c>>11)&31)*255/31; }
static int green(uint16_t c) { return ((c>>5)&63)*255/63; }
static int blue(uint16_t c) { return (c&31)*255/31; }
static uint16_t blend(uint16_t a,uint16_t b,int weight)
{
    return rgb((red(a)*weight+red(b)*(256-weight))>>8,
               (green(a)*weight+green(b)*(256-weight))>>8,
               (blue(a)*weight+blue(b)*(256-weight))>>8);
}
#if JETISLAND_COLOR
typedef uint16_t pixel_t;
static pixel_t ink(uint16_t c) { return c; }
#else
typedef uint8_t pixel_t;
static pixel_t ink(uint16_t c) { return (red(c)*77+green(c)*150+blue(c)*29)>>8; }
#endif
static pixel_t frame[IH][IW];
static struct vertex projected[VERTEX_COUNT];
static struct { uint16_t index,depth; } sorted[FACE_COUNT];
static uint16_t palette[PALETTE_COUNT];
static int8_t ripple[IH];
static int orbit, radius=1800, elevation, theme;
static bool automatic=true, reflections=true, wireframe, hud=true, usb;
static uint32_t animation_ms, tour_ms;
static unsigned resume_ms;
static int resume_orbit,resume_radius,resume_elevation;
static unsigned frames,fps_tenths;
static int triangles, horizon, focal;
static int sun_x,sun_y;
static pixel_t sun_pixel;
static int clampi(int n,int lo,int hi) {return n<lo?lo:n>hi?hi:n;}
static int isin(int a)
{
    unsigned q=(unsigned)a&1023, i=q&255;
    int n=sin_quarter[(q&256)?256-i:i];
    return q&512 ? -n:n;
}
static int icos(int a) { return isin(a+256); }
static unsigned isqrt(unsigned n)
{
    unsigned r=0,bit=1u<<30;
    while(bit>n)bit>>=2;
    while(bit){if(n>=r+bit){n-=r+bit;r=(r>>1)+bit;}else r>>=1;bit>>=2;}
    return r;
}
static uint16_t themed(uint16_t c)
{
    if(theme==1)return rgb(MIN(255,red(c)*9/8+14),green(c)*3/4,blue(c)*2/3+15);
    if(theme==2)return rgb(red(c)/4,green(c)*2/5,MIN(255,blue(c)/2+26));
    return c;
}
static uint16_t sky_top(void) {return theme==0?rgb(30,130,210):theme==1?rgb(52,32,91):rgb(5,12,36);}
static uint16_t sky_bottom(void) {return theme==0?rgb(185,228,240):theme==1?rgb(255,169,101):rgb(45,68,104);}
static uint16_t sea_color(void) {return theme==0?rgb(8,140,156):theme==1?rgb(73,74,105):rgb(7,38,60);}
static void dot(int x,int y,pixel_t c)
{if((unsigned)x<IW && (unsigned)y<IH)frame[y][x]=c;}
static void line(int x,int y,int x1,int y1,pixel_t color,int clip_y)
{
    int dx=x1>x?x1-x:x-x1, sx=x<x1?1:-1;
    int dy=y1>y?y-y1:y1-y, sy=y<y1?1:-1, error=dx+dy;
    for(;;) {
        if(y>=clip_y)dot(x,y,color);
        if(x==x1 && y==y1)break;
        int e=2*error;
        if(e>=dy){error+=dy;x+=sx;}
        if(e<=dx){error+=dx;y+=sy;}
    }
}

/* C translation of Jet's exact-remainder ScanEdge / TriangleSpans. Integer
 * projected coordinates are bounded by the constrained orbit camera. */
struct edge {int x,step,remainder,step_remainder,divisor;};
static void edge_reset(struct edge *e,struct vertex a,struct vertex b,int y)
{
    int dx=b.x-a.x,dy=b.y-a.y;
    e->divisor=dy;e->step=dx/dy;e->step_remainder=dx-e->step*dy;
    if(e->step_remainder<0){--e->step;e->step_remainder+=dy;}
    int n=dx*(y-a.y),q=n/dy,r=n-q*dy;
    if(r<0){--q;r+=dy;}e->x=a.x+q;e->remainder=r;
}
static void edge_advance(struct edge *e)
{
    e->x+=e->step;e->remainder+=e->step_remainder;
    if(e->remainder>=e->divisor){++e->x;e->remainder-=e->divisor;}
}
static void triangle(struct vertex a,struct vertex b,struct vertex c,uint16_t color,bool reflected)
{
    int area=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
    if(!area)return;
    if(a.y>b.y){struct vertex t=a;a=b;b=t;}
    if(b.y>c.y){struct vertex t=b;b=c;c=t;}
    if(a.y>b.y){struct vertex t=a;a=b;b=t;}
    int low=MAX(reflected?horizon:0,a.y),high=MIN(IH-1,c.y);
    if(low>high || c.y==a.y)return;
    ++triangles;
    if(wireframe && !reflected) {
        pixel_t w=ink(theme==2?rgb(90,240,207):rgb(20,63,77));
        line(a.x,a.y,b.x,b.y,w,0);line(b.x,b.y,c.x,c.y,w,0);line(c.x,c.y,a.x,a.y,w,0);return;
    }
    bool short_left=(b.x-a.x)*(c.y-a.y)<(c.x-a.x)*(b.y-a.y);
    struct edge long_edge,short_edge;
    edge_reset(&long_edge,a,c,low);
    int switch_y=IH;
    if(low<b.y || b.y==c.y) {
        edge_reset(&short_edge,a,b,low);if(b.y<c.y)switch_y=b.y;
    } else edge_reset(&short_edge,b,c,low);
    struct edge *left=short_left?&short_edge:&long_edge;
    struct edge *right=short_left?&long_edge:&short_edge;
    pixel_t col=ink(color);
    for(int y=low;y<=high;++y) {
        if(y>=switch_y){edge_reset(&short_edge,b,c,y);switch_y=IH;}
        int shift=reflected?ripple[y]:0;
        int x0=MAX(0,left->x+(left->remainder!=0)+shift),x1=MIN(IW-1,right->x+shift);
        if(reflected) col=ink(blend(color,sea_color(),MAX(70,225-(y-horizon)*128/IH)));
        for(int x=x0;x<=x1;++x)frame[y][x]=col;
        edge_advance(&long_edge);edge_advance(&short_edge);
    }
}

static int cyaw,syaw,cpitch,spitch,camera_y,render_radius;
static struct vertex project(int x,int y,int z)
{
    int vx=(cyaw*x+syaw*z)/16384;
    int vz=render_radius+(-syaw*x+cyaw*z)/16384;
    int vy=y-camera_y;
    int depth=(vz*cpitch+vy*spitch)/16384;
    int height=(vy*cpitch-vz*spitch)/16384;
    /* Legal camera poses keep every scene vertex in front of the near plane.
     * Clamp only as a final defense if custom geometry is substituted. */
    depth=MAX(depth,64);
    return (struct vertex){clampi(IW/2+vx*focal/depth,-2048,2048),
                          clampi(IH/2-height*focal/depth,-2048,2048),depth};
}
static void prepare_camera(void)
{
    if(automatic) {
        /* A 24-second, two-revolution flight: skim the shore, pull away while
         * climbing, tighten into an overhead pass, then dive to the water.
         * All waves close exactly at the loop boundary. No path allocation. */
        int phase=(tour_ms%24000u)*1024u/24000u;
        int a=(phase*2+55*isin(phase)/16384)&1023;
        int r=1400-400*icos(phase*2)/16384;
        int e=-50+(16384-icos(phase))*1050/16384;
        if(resume_ms) {
            /* Rejoin the moving tour smoothly after manual exploration. */
            int t=256-MIN(resume_ms,1600u)*256/1600;
            int w=t*t*(768-2*t)/65536;
            int da=((a-resume_orbit+512)&1023)-512;
            a=(resume_orbit+da*w/256)&1023;
            r=resume_radius+(r-resume_radius)*w/256;
            e=resume_elevation+(e-resume_elevation)*w/256;
        }
        orbit=a;radius=r;elevation=e;
    }
    int angle=orbit;
    render_radius=radius;
    camera_y=180+elevation;
    syaw=isin(angle);cyaw=icos(angle);
    int dy=160-camera_y;
    int len=isqrt(render_radius*render_radius+dy*dy);
    cpitch=render_radius*16384/len;spitch=dy*16384/len;
    focal=MIN(IW*4/5,IH*5/4);
    horizon=clampi(IH/2+spitch*focal/MAX(cpitch,1),0,IH);
    for(int i=0;i<PALETTE_COUNT;++i)palette[i]=themed(rgb(original_palette[i]>>16,(original_palette[i]>>8)&255,original_palette[i]&255));
    for(int y=0;y<IH;++y) {
        int phase=(int)(animation_ms/13u)+y*29;
        ripple[y]=isin(phase)*MAX(1,(y-horizon)*4/IH+1)/16384;
    }
}
static void geometry(bool reflected)
{
    for(int i=0;i<VERTEX_COUNT;++i) {
        const struct vertex *v=&vertices[i];
        projected[i]=project(v->x,reflected?-v->y:v->y,v->z);
    }
    for(int i=0;i<FACE_COUNT;++i) {
        const struct face *f=&faces[i];sorted[i].index=i;
        sorted[i].depth=(projected[f->a].z+projected[f->b].z+projected[f->c].z)/3;
    }
    /* Shell sort: bounded storage, no recursion, no libc or heap. */
    for(int gap=FACE_COUNT/2;gap>0;gap/=2)for(int i=gap;i<FACE_COUNT;++i) {
        unsigned index=sorted[i].index,depth=sorted[i].depth;int j=i;
        while(j>=gap && sorted[j-gap].depth<depth){sorted[j]=sorted[j-gap];j-=gap;}
        sorted[j].index=index;sorted[j].depth=depth;
    }
    for(int i=0;i<FACE_COUNT;++i) {
        const struct face *f=&faces[sorted[i].index];
        uint16_t color=palette[f->color];
        if(reflected)color=blend(color,sea_color(),115);
        triangle(projected[f->a],projected[f->b],projected[f->c],color,reflected);
    }
}
static void disc(int cx,int cy,int radius_,uint16_t color,int strength,bool ring)
{
    if(radius_<1)return;
    for(int y=MAX(0,cy-radius_);y<=MIN(IH-1,cy+radius_);++y)
    for(int x=MAX(0,cx-radius_);x<=MIN(IW-1,cx+radius_);++x) {
        int d=(x-cx)*(x-cx)+(y-cy)*(y-cy),r2=radius_*radius_;
        if(d>=r2 || (ring && d<r2*3/5))continue;
        int alpha=ring?strength:strength*(r2-d)/r2;
#if JETISLAND_COLOR
        frame[y][x]=blend(color,frame[y][x],alpha);
#else
        frame[y][x]=(ink(color)*alpha+frame[y][x]*(256-alpha))>>8;
#endif
    }
}
static void draw_background(void)
{
    uint16_t top=sky_top(),bottom=sky_bottom(),sea=sea_color();
    for(int y=0;y<IH;++y) {
        uint16_t c=y<horizon ? blend(bottom,top,clampi(y*256/MAX(horizon,1),0,256)) :
            blend(sea,bottom,170+MIN(86,(y-horizon)*86/MAX(IH-horizon,1)));
        uint16_t light=blend(c,bottom,224);
        pixel_t normal=ink(c),shine=ink(light);
        int phase=(int)(animation_ms/47u)+ripple[y]*6;
        for(int x=0;x<IW;++x)frame[y][x]=y>horizon && ((x+y*31+phase)&63)<2 ? shine:normal;
    }
    if(theme==2) {
        /* Deterministic stars: no evolving RNG or allocation. */
        for(unsigned i=0;i<47;++i) {
            unsigned seed=i*2654435761u+12345;
            int x=(seed>>9)%IW,y=(seed>>21)%MAX(1,horizon);
            dot(x,y,ink(rgb(100+(i%3)*60,140+(i%3)*40,215)));
        }
    }
    /* Upstream sun direction (.20,.18,.963), projected with integer math. */
    int x=(cyaw*200+syaw*963)/16384,z=(-syaw*200+cyaw*963)/16384;
    int y=(180*cpitch-z*spitch)/16384,d=(z*cpitch+180*spitch)/16384;
    sun_x=sun_y=-1000;
    if(d>200) {
        sun_x=IW/2+x*focal/d;sun_y=IH/2-y*focal/d;
        uint16_t color=theme==2?rgb(225,239,252):theme==1?rgb(255,216,137):rgb(255,250,215);
        int size=MAX(3,MIN(IW,IH)/18);
        disc(sun_x,sun_y,size*3,color,80,false);
        disc(sun_x,sun_y,size,color,256,false);
        dot(sun_x,sun_y,ink(color));sun_pixel=ink(color);
    }
}
static void render(void)
{
    prepare_camera();triangles=0;draw_background();
    if(reflections && !wireframe)geometry(true);
    geometry(false);
    if(theme!=2 && (unsigned)sun_x<IW && (unsigned)sun_y<IH && frame[sun_y][sun_x]==sun_pixel) {
        int dx=IW/2-sun_x,dy=IH/2-sun_y;
        disc(sun_x+dx*3/4,sun_y+dy*3/4,MAX(2,IW/25),rgb(151,231,204),45,true);
        disc(sun_x+dx*3/2,sun_y+dy*3/2,MAX(2,IW/18),rgb(242,194,151),30,false);
    }
}

/* YP3: only four directions and center. Long-center is device-reserved.
 * Override PORT_CENTER_BUTTON if the board header uses a different name. */
#if defined(YP3_PAD) && CONFIG_KEYPAD == YP3_PAD
#ifndef PORT_CENTER_BUTTON
#if defined(BUTTON_SELECT)
#define PORT_CENTER_BUTTON BUTTON_SELECT
#elif defined(BUTTON_PLAY)
#define PORT_CENTER_BUTTON BUTTON_PLAY
#elif defined(BUTTON_CENTER)
#define PORT_CENTER_BUTTON BUTTON_CENTER
#elif defined(BUTTON_OK)
#define PORT_CENTER_BUTTON BUTTON_OK
#elif defined(BUTTON_PLAYPAUSE)
#define PORT_CENTER_BUTTON BUTTON_PLAYPAUSE
#else
#error "Define PORT_CENTER_BUTTON to the YP3 center button raw mask"
#endif
#endif
static const struct button_mapping yp3_context[]={
    {PLA_UP,BUTTON_UP,BUTTON_NONE}, {PLA_DOWN,BUTTON_DOWN,BUTTON_NONE},
    {PLA_LEFT,BUTTON_LEFT,BUTTON_NONE}, {PLA_RIGHT,BUTTON_RIGHT,BUTTON_NONE},
    {PLA_UP_REPEAT,BUTTON_UP|BUTTON_REPEAT,BUTTON_NONE},
    {PLA_DOWN_REPEAT,BUTTON_DOWN|BUTTON_REPEAT,BUTTON_NONE},
    {PLA_LEFT_REPEAT,BUTTON_LEFT|BUTTON_REPEAT,BUTTON_NONE},
    {PLA_RIGHT_REPEAT,BUTTON_RIGHT|BUTTON_REPEAT,BUTTON_NONE},
    {PLA_SELECT,PORT_CENTER_BUTTON,BUTTON_NONE},
    {PLA_SELECT_REL,PORT_CENTER_BUTTON|BUTTON_REL,BUTTON_NONE},
    {PLA_SELECT_REPEAT,PORT_CENTER_BUTTON|BUTTON_REPEAT,BUTTON_NONE},
    LAST_ITEM_IN_LIST
};
#define PORT_MAIN_CONTEXT yp3_context
#else
#define PORT_MAIN_CONTEXT pla_main_ctx
#endif

#ifdef HAVE_BLUETOOTH_HID
/* Game controller: A toggles the camera, B, Options and PS leave. */
static const struct button_mapping pad_context[]={
    {PLA_SELECT,BUTTON_PAD_A,BUTTON_NONE},
    {PLA_SELECT_REL,BUTTON_PAD_A|BUTTON_REL,BUTTON_NONE},
    {PLA_SELECT_REPEAT,BUTTON_PAD_A|BUTTON_REPEAT,BUTTON_NONE},
    {PLA_CANCEL,BUTTON_PAD_B,BUTTON_NONE},
    {PLA_EXIT,BUTTON_PAD_MENU,BUTTON_NONE},
    {PLA_EXIT,BUTTON_PAD_HOME,BUTTON_NONE},
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_PLUGIN)
};
#endif



static void text_at(int x,int y,const char *s,int scale,uint16_t color)
{
    pixel_t c=ink(color);
    for(;*s;++s,x+=4*scale) {
        char ch=*s;if(ch>='a' && ch<='z')ch-=32;
        int n=0;while(glyph_chars[n] && glyph_chars[n]!=ch)++n;
        uint16_t bits=glyph_bits[n];
        for(int row=0;row<5;++row)for(int col=0;col<3;++col)
            if(bits&(1u<<(14-row*3-col)))
                for(int j=0;j<scale;++j)for(int i=0;i<scale;++i)
                    dot(x+col*scale+i,y+row*scale+j,c);
    }
}
static void present(void)
{
#ifdef HAVE_LCD_COLOR
    static fb_data row[IW*JETISLAND_SCALE];
#elif LCD_DEPTH == 1
    static uint8_t row[IW*JETISLAND_SCALE];
    static const uint8_t bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
#endif
    const int ox=(LCD_WIDTH-IW*JETISLAND_SCALE)/2,oy=(LCD_HEIGHT-IH*JETISLAND_SCALE)/2;
    for(int y=0;y<IH;++y) {
        for(int x=0;x<IW;++x) {
#if JETISLAND_COLOR
            uint16_t c=frame[y][x];int r=red(c),g=green(c),b=blue(c);
#else
            int r=frame[y][x];
#ifdef HAVE_LCD_COLOR
            int g=r,b=r;
#endif
#endif
#ifdef HAVE_LCD_COLOR
            fb_data c_out=FB_RGBPACK(r,g,b);
            for(int i=0;i<JETISLAND_SCALE;++i)row[x*JETISLAND_SCALE+i]=c_out;
#elif LCD_DEPTH == 1
            uint8_t c_out=r>(bayer[y&3][x&3]*16+8)?0:1;
            for(int i=0;i<JETISLAND_SCALE;++i)row[x*JETISLAND_SCALE+i]=c_out;
#else
            rb->lcd_set_foreground(LCD_BRIGHTNESS(r));
            rb->lcd_fillrect(ox+x*JETISLAND_SCALE,oy+y*JETISLAND_SCALE,JETISLAND_SCALE,JETISLAND_SCALE);
#endif
        }
        for(int j=0;j<JETISLAND_SCALE;++j) {
#ifdef HAVE_LCD_COLOR
            rb->lcd_bitmap(row,ox,oy+y*JETISLAND_SCALE+j,IW*JETISLAND_SCALE,1);
#elif LCD_DEPTH == 1
            rb->lcd_mono_bitmap(row,ox,oy+y*JETISLAND_SCALE+j,IW*JETISLAND_SCALE,1);
#endif
        }
        if((y&15)==0)rb->yield();
    }
    rb->lcd_update();
}
static void overlay(void)
{
    if(!hud)return;
    char s[32];
    for(int y=IH-9;y<IH;++y)for(int x=0;x<IW;++x)frame[y][x]=ink(rgb(5,15,28));
    text_at(2,IH-7,"JET ISLAND",1,0xffff);
    rb->snprintf(s,sizeof(s),"%u.%u FPS",fps_tenths/10,fps_tenths%10);
    text_at(MAX(46,IW-(int)rb->strlen(s)*4-2),IH-7,s,1,0xffff);
    if(!automatic)text_at(3,3,"MANUAL",1,0xffff);
}

/* YP3 context is supplied just above this platform layer. */
#ifdef HAVE_BLUETOOTH_HID
static const struct button_mapping *contexts[]={pad_context, PORT_MAIN_CONTEXT,
#else
static const struct button_mapping *contexts[]={PORT_MAIN_CONTEXT,
#endif
#ifdef HAVE_REMOTE_LCD
    pla_remote_ctx,
#endif
};
enum { I_NONE, I_UP,I_DOWN,I_LEFT,I_RIGHT,I_SELECT,I_MENU,
       I_DOUBLE_UP,I_DOUBLE_DOWN,I_DOUBLE_LEFT,I_DOUBLE_RIGHT,I_REPEAT=256 };
static bool select_armed,select_long;
static unsigned long select_since;
static int suppress;
static int read_action(int timeout)
{
#ifdef PORT_CENTER_BUTTON
    /* Common pause gesture: hold Select+Up for 0.5 s; the release is no click. */
    static unsigned long chord_since;static bool chord_done;
    if((rb->button_status()&(PORT_CENTER_BUTTON|BUTTON_UP))==(PORT_CENTER_BUTTON|BUTTON_UP)) {
        if(!chord_since)chord_since=*rb->current_tick|1;
        else if(!chord_done && *rb->current_tick-chord_since>=(unsigned long)(HZ/2)) {
            chord_done=true;select_armed=false;select_long=true;suppress=I_UP;return I_MENU;
        }
    } else {chord_since=0;chord_done=false;}
#endif
    int a=pluginlib_getaction(timeout,contexts,ARRAYLEN(contexts));
    if(rb->default_event_handler(a)==SYS_USB_CONNECTED){usb=true;return I_NONE;}
    if(a==PLA_SELECT){select_armed=true;select_long=false;select_since=*rb->current_tick;suppress=0;return I_NONE;}
    if(a==PLA_SELECT_REPEAT){select_armed=false;select_long=true;return I_NONE;}
    if(a==PLA_SELECT_REL) {
        bool click=select_armed && !select_long && (unsigned long)*rb->current_tick-select_since<(unsigned)(HZ*3/5);
        select_armed=false;return click?I_SELECT:I_NONE;
    }
    if(a==PLA_CANCEL || a==PLA_EXIT){select_armed=false;return I_MENU;}
    int d=0,repeat=0;
    switch(a) {
    case PLA_UP:d=I_UP;break;case PLA_DOWN:d=I_DOWN;break;
    case PLA_LEFT:d=I_LEFT;break;case PLA_RIGHT:d=I_RIGHT;break;
    case PLA_UP_REPEAT:d=I_UP;repeat=I_REPEAT;break;
    case PLA_DOWN_REPEAT:d=I_DOWN;repeat=I_REPEAT;break;
    case PLA_LEFT_REPEAT:d=I_LEFT;repeat=I_REPEAT;break;
    case PLA_RIGHT_REPEAT:d=I_RIGHT;repeat=I_REPEAT;break;
#ifdef HAVE_SCROLLWHEEL
    case PLA_SCROLL_BACK:case PLA_SCROLL_BACK_REPEAT:d=I_UP;repeat=I_REPEAT;break;
    case PLA_SCROLL_FWD:case PLA_SCROLL_FWD_REPEAT:d=I_DOWN;repeat=I_REPEAT;break;
#endif
    }
    if(!d)return I_NONE;
    select_armed=false;
    if(repeat && suppress==d)return I_NONE;
    if(!repeat)suppress=0;
    return d|repeat;
}
static struct {
    int direction,phase,orbit,radius,elevation;
    bool automatic;
    unsigned long when;
} gesture;
static int gesture_action(int a)
{
    int d=a&~I_REPEAT;unsigned long now=*rb->current_tick;
    if(d<I_UP || d>I_RIGHT){if(a)gesture.phase=0;return a;}
    if(a&I_REPEAT) {
        if(gesture.phase==2 && gesture.direction==d)return I_DOUBLE_UP+d-I_UP;
        gesture.phase=0;return d;
    }
    if(gesture.phase==1 && gesture.direction==d && now-gesture.when<=(unsigned)(HZ*3/10)) {
        orbit=gesture.orbit;radius=gesture.radius;elevation=gesture.elevation;automatic=gesture.automatic;
        gesture.phase=d<=I_DOWN?2:0;gesture.direction=d;
        if(d>I_DOWN)suppress=d;
        return I_DOUBLE_UP+d-I_UP;
    }
    gesture.direction=d;gesture.phase=1;gesture.when=now;
    gesture.orbit=orbit;gesture.radius=radius;gesture.elevation=elevation;gesture.automatic=automatic;
    return d;
}
static void reset_camera(void)
{
    tour_ms=resume_ms=0;automatic=true;gesture.phase=0;
    prepare_camera();
}
static void toggle_camera(void)
{
    if(!automatic) {
        resume_orbit=orbit;resume_radius=radius;resume_elevation=elevation;
        resume_ms=1600;
    }
    automatic=!automatic;
}
static bool apply_action(int a)
{
    switch(a) {
    case I_LEFT:orbit=(orbit-10)&1023;automatic=false;break;
    case I_RIGHT:orbit=(orbit+10)&1023;automatic=false;break;
    case I_UP:radius=MAX(1000,radius-50);automatic=false;break;
    case I_DOWN:radius=MIN(3200,radius+50);automatic=false;break;
    case I_DOUBLE_UP:elevation=MIN(2050,elevation+40);automatic=false;break;
    case I_DOUBLE_DOWN:elevation=MAX(-90,elevation-40);automatic=false;break;
    case I_SELECT:toggle_camera();break;
    case I_DOUBLE_RIGHT:theme=(theme+1)%3;break;
    case I_DOUBLE_LEFT:case I_MENU:return true;
    }
    return false;
}
static bool menu(void)
{
    static const struct opt_items themes[]={{"Day",-1},{"Sunset",-1},{"Night",-1}};
    MENUITEM_STRINGLIST(m,"Jet Island",NULL,"Resume","Camera Tour","Palette",
        "Reflections","Wireframe","Reset Camera","Show FPS","Playback Control","Quit");
    int sel=0,th=theme;
    bool quit=false,go=false;
    while(!go&&!quit) {
        switch(rb->do_menu(&m,&sel,NULL,false)) {
        case 0:go=true;break;
        case 1:{bool t=automatic;rb->set_bool("Camera Tour",&t);if(t!=automatic)toggle_camera();break;}
        case 2:rb->set_option("Palette",&th,RB_INT,themes,3,NULL);theme=th;break;
        case 3:rb->set_bool("Reflections",&reflections);break;
        case 4:rb->set_bool("Wireframe",&wireframe);break;
        case 5:reset_camera();go=true;break;
        case 6:rb->set_bool("Show FPS",&hud);break;
        case 7:playback_control(NULL);break;
        case MENU_ATTACHED_USB:usb=true;/* fall through */
        default:quit=true;break;
        }
    }
    gesture.phase=0;
    rb->lcd_set_viewport(NULL);
#if LCD_DEPTH>1
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    return quit;
}
enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;
    int old_mode=rb->lcd_get_drawmode();
#if LCD_DEPTH>1
    unsigned old_fg=rb->lcd_get_foreground(),old_bg=rb->lcd_get_background();
#endif
#if LCD_DEPTH>=16
    fb_data *old_backdrop=rb->lcd_get_backdrop();rb->lcd_set_backdrop(NULL);
#endif
#ifdef HAVE_TOUCHSCREEN
    int old_touch=rb->touchscreen_get_mode();rb->touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif
    rb->lcd_set_viewport(NULL);rb->lcd_set_drawmode(DRMODE_SOLID);
#if LCD_DEPTH>1
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();backlight_ignore_timeout();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    reset_camera();theme=0;reflections=true;wireframe=false;hud=true;usb=false;
    select_armed=select_long=false;suppress=0;animation_ms=0;frames=fps_tenths=0;
    if(menu())goto done;
    unsigned long last=*rb->current_tick,last_fps=last,next=last;
    while(!usb) {
        unsigned long now=*rb->current_tick;
        int timeout=TIME_BEFORE(now,next)?next-now:0;
        int a=gesture_action(read_action(timeout));
        if(usb)break;
        if(apply_action(a)) {
            if(menu())break;
            last=last_fps=*rb->current_tick;frames=0;next=last;
        }
        now=*rb->current_tick;
        if(TIME_BEFORE(now,next))continue;
        unsigned dt=MIN((now-last)*1000u/HZ,1000u);last=now;
        animation_ms+=dt;
        if(automatic) {
            tour_ms=(tour_ms+dt)%24000u;
            resume_ms=resume_ms>dt?resume_ms-dt:0;
        }
        render();overlay();present();++frames;
        now=*rb->current_tick;
        if(now-last_fps>=(unsigned)HZ) {fps_tenths=frames*10u*HZ/MAX(1u,now-last_fps);last_fps=now;frames=0;}
        next=last+MAX(1,HZ/25);
        /* Let Rockbox run even when rendering is slower than the target rate. */
        rb->yield();
    }
done:
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_use_settings();rb->lcd_set_drawmode(old_mode);
#if LCD_DEPTH>1
    rb->lcd_set_foreground(old_fg);rb->lcd_set_background(old_bg);
#endif
#if LCD_DEPTH>=16
    rb->lcd_set_backdrop(old_backdrop);
#endif
#ifdef HAVE_TOUCHSCREEN
    rb->touchscreen_set_mode(old_touch);
#endif
    return usb?PLUGIN_USB_CONNECTED:PLUGIN_OK;
}

/* Pinned source revisions:
jet-upstream: c56dfc0c012ba7a09a6e49b4123340ec24981441
jet-examples-upstream: 57b05a240f5bcaa02a18d62a1206ce7053dc09dc
*/

/*
MIT License

Copyright (c) 2026 CubeCoders Limited

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
