#pragma once

#include "Game.h"
#include "Pokitto.h"
#include "Strings.h"

using PC = Pokitto::Core;
using PD = Pokitto::Display;
using PS = Pokitto::Sound;

const char nitem0[] = "gold";

const char nitem1[] = "food";
const char nitem2[] = "mold";

const char nitem3[] = "mace";
const char nitem4[] = "axe";
const char nitem5[] = "sword";
const char nitem6[] = "bow";
const char nitem7[] = "dart";
const char nitem8[] = "arrow";
const char nitem9[] = "shuriken";
const char nitem10[] = "dagger";

const char nitem11[] = "leather";
const char nitem12[] = "ring";
const char nitem13[] = "scale";
const char nitem14[] = "chain";
const char nitem15[] = "splint";
const char nitem16[] = "banded";
const char nitem17[] = "plate";

const char nitem18[] = "amber";
const char nitem19[] = "black";
const char nitem20[] = "blue";
const char nitem21[] = "brown";
const char nitem22[] = "clear";
const char nitem23[] = "cyan";
const char nitem24[] = "orange";
const char nitem25[] = "green";
const char nitem26[] = "grey";
const char nitem27[] = "pink";
const char nitem28[] = "red";
const char nitem29[] = "violet";
const char nitem30[] = "white";
const char nitem31[] = "yellow";

const char nitem32[] = "ZELGO";
const char nitem33[] = "JUYED";
const char nitem34[] = "NR 9";
const char nitem35[] = "XIXAXA";
const char nitem36[] = "DAIYEN";
const char nitem37[] = "ELBIB";
const char nitem38[] = "THARR";
const char nitem39[] = "YUM YUM";
const char nitem40[] = "ANDOVA";
const char nitem41[] = "FOOBIE";
const char nitem42[] = "TEMOV";
const char nitem43[] = "READ ME";

const char nitem44[] = "agate";
const char nitem45[] = "amethyst";
const char nitem46[] = "diamond";
const char nitem47[] = "emerald";
const char nitem48[] = "garnet";
const char nitem49[] = "onyx";
const char nitem50[] = "opal";
const char nitem51[] = "pearl";
const char nitem52[] = "ruby";
const char nitem53[] = "sapphire";
const char nitem54[] = "topaz";

const char nitem55[] = "brass";
const char nitem56[] = "bronze";
const char nitem57[] = "copper";
const char nitem58[] = "iron";
const char nitem59[] = "lead";
const char nitem60[] = "nickel";
const char nitem61[] = "steel";
const char nitem62[] = "silver";
const char nitem63[] = "tin";
const char nitem64[] = "zinc";

const char nitem65[] = "Yendor";

const char nitem66[] = "power";
const char nitem67[] = "restore";
const char nitem68[] = "heal";
const char nitem69[] = "e'heal";
const char nitem70[] = "poison";
const char nitem71[] = "level";
const char nitem72[] = "blind";
const char nitem73[] = "hallu";
const char nitem74[] = "m'detect";
const char nitem75[] = "i'detect";
const char nitem76[] = "confuse";
const char nitem77[] = "levitate";
const char nitem78[] = "speed";
const char nitem79[] = "look";

const char nitem80[] = "protect";
const char nitem81[] = "hold";
const char nitem82[] = "weapon";
const char nitem83[] = "armor";
const char nitem84[] = "identify";
const char nitem85[] = "teleport";
const char nitem86[] = "sleep";
const char nitem87[] = "scare";
const char nitem88[] = "bless";
const char nitem89[] = "create";
const char nitem90[] = "anger";
const char nitem91[] = "map";

const char nitem92[] = "stealth";
const char nitem93[] = "teleport";
const char nitem94[] = "life";
const char nitem95[] = "food";
const char nitem96[] = "damage";
const char nitem97[] = "strong";
const char nitem98[] = "dex";
const char nitem99[] = "adorn";
const char nitem100[] = "see";
const char nitem101[] = "armor";
const char nitem102[] = "search";

const char nitem103[] = "teleport";
const char nitem104[] = "slow";
const char nitem105[] = "confuse";
const char nitem106[] = "clear";
const char nitem107[] = "change";
const char nitem108[] = "fast";
const char nitem109[] = "sleep";
const char nitem110[] = "missile";
const char nitem111[] = "cancel";
const char nitem112[] = "nothing";

const char * const nitem[113] =
{
	nitem0,		//gold

	nitem1,		//food
	nitem2,

	nitem3,		//weapon
	nitem4,
	nitem5,
	nitem6,
	nitem7,
	nitem8,
	nitem9,
	nitem10,

	nitem11,		//armor
	nitem12,
	nitem13,
	nitem14,
	nitem15,
	nitem16,
	nitem17,

	nitem18,		//color
	nitem19,
	nitem20,
	nitem21,
	nitem22,
	nitem23,
	nitem24,
	nitem25,
	nitem26,
	nitem27,
	nitem28,
	nitem29,
	nitem30,
	nitem31,

	nitem32,		//label
	nitem33,
	nitem34,
	nitem35,
	nitem36,
	nitem37,
	nitem38,
	nitem39,
	nitem40,
	nitem41,
	nitem42,
	nitem43,

	nitem44,		//stone
	nitem45,
	nitem46,
	nitem47,
	nitem48,
	nitem49,
	nitem50,
	nitem51,
	nitem52,
	nitem53,
	nitem54,

	nitem55,		//metal
	nitem56,
	nitem57,
	nitem58,
	nitem59,
	nitem60,
	nitem61,
	nitem62,
	nitem63,
	nitem64,

	nitem65,		//amulet

	nitem66,
	nitem67,
	nitem68,
	nitem69,
	nitem70,
	nitem71,
	nitem72,
	nitem73,
	nitem74,
	nitem75,
	nitem76,
	nitem77,
	nitem78,
	nitem79,

	nitem80,
	nitem81,
	nitem82,
	nitem83,
	nitem84,
	nitem85,
	nitem86,
	nitem87,
	nitem88,
	nitem89,
	nitem90,
	nitem91,

	nitem92,
	nitem93,
	nitem94,
	nitem95,
	nitem96,
	nitem97,
	nitem98,
	nitem99,
	nitem100,
	nitem101,
	nitem102,

	nitem103,
	nitem104,
	nitem105,
	nitem106,
	nitem107,
	nitem108,
	nitem109,
	nitem110,
	nitem111,
	nitem112
};


const char message0[] = "Welcome`";
const char message1[] = "I cannot";
const char message2[] = "nothing happens";
const char message3[] = "something here";
const char message4[] = "no slot";
const char message5[] = "dir}";
const char message6[] = "level up";
const char message7[] = "oops'''";
const char message8[] = "yummy";
const char message9[] = "it is cursed";
const char message10[] = "removed";
const char message11[] = "Equipped";
const char message12[] = "already wield";
const char message13[] = "frozen";
const char message14[] = "poisoned";
const char message15[] = "stolen";
const char message16[] = "rusted";
const char message17[] = "held";
const char message18[] = "graced";
const char message19[] = "burned";
const char message20[] = "level down";
const char message21[] = "drained";
const char message22[] = "zzz'''";

const char message23[] = "trapdoor";
const char message24[] = "arrow trap";
const char message25[] = "sleep gas";
const char message26[] = "teleport trap";
const char message27[] = "poison trap";
const char message28[] = "rust trap";

const char message29[] = "";

const char message30[] = "Danger`";

const char * const messages[31] =
{
	message0,
	message1,
	message2,
	message3,
	message4,
	message5,
	message6,
	message7,
	message8,
	message9,
	message10,
	message11,
	message12,
	message13,
	message14,
	message15,
	message16,
	message17,
	message18,
	message19,
	message20,
	message21,
	message22,
  message23,
  message24,
  message25,
  message26,
  message27,
  message28,
  message29,
  message30
};
