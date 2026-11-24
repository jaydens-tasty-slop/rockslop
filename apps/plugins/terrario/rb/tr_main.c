/* Terrario for Rockbox: start menu, game loop, native menus and memory
 * set-up. Replaces upstream's main.c and menu.c (calculator specific). */
#include "tr_gint.h"
#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/plugin_fpu.h"

#include "defs.h"
#include "world.h"
#include "entity.h"
#include "render.h"
#include "menu.h"
#include "save.h"
#include "crafting.h"
#include "update.h"
#include "generate.h"
#include "npc.h"
#include "chat.h"
#include "tr_game.h"

/* Globals upstream defines in main.c */
GYRAM char varBuffer[VAR_BUF_HEIGHT][VAR_BUF_WIDTH];
GYRAM struct SaveData save;
struct World world;
struct Player player;
char versionBuffer[16];
struct GameCompatibilityPresets game;

#define WORLD_HEIGHT_TILES 250
#define MIN_WORLD_WIDTH 240
#define MAX_WORLD_WIDTH 640
#define AUDIO_BUF_RESERVE 2048

static int palette = 1;
static int world_size = 2; /* 0 small, 1 medium, 2 large */
static bool in_game;

/* ------------------------------------------------------------ menus -- */

static void clear_screen(void)
{
	rb->lcd_set_viewport(NULL);
	rb->lcd_set_foreground(LCD_WHITE);
	rb->lcd_set_background(LCD_BLACK);
	rb->lcd_clear_display();
	rb->lcd_update();
}

static bool show_text(char **words, int count)
{
	if (display_text(count, words, NULL, NULL, true))
	{
		tr_usb = true;
		return true;
	}
	return false;
}

static bool help(void)
{
	static char *words[] = {
		"Terrario", "", "LEFT/RIGHT: walk.", "UP: jump (hold = higher).",
		"DOWN tap: game menu", "(inventory, crafting,", "hotbar, housing).",
		"Hold SELECT+UP for 1 s:", "pause menu (save, quit).",
		"DOWN hold: drop through", "platforms.", "SELECT hold: use the", "held item at the cursor",
		"(mine, attack, place);", "pressing it with the", "cursor on a chest, door,", "bed or NPC interacts.",
		"SELECT+direction: move", "the cursor.", "", "In inventory, crafting", "and chest screens:",
		"arrows move, SELECT tap", "= pick up / place /", "craft, SELECT+UP = take", "one, SELECT+LEFT/RIGHT",
		"= change tab, SELECT+", "DOWN = back.", "", "Playing stops music: the",
		"world uses the audio", "buffer."
	};
	return show_text(words, ARRAYLEN(words));
}

static bool about(void)
{
	static char *words[] = {
		"Terrario", "A Terraria rewrite", "by KBD2", "github.com/KBD2/terrario", "Code: MIT License.",
		"Images: CC BY 4.0,", "(c) the Terrario", "authors (KBD2 and", "contributors).",
		"creativecommons.org/", "licenses/by/4.0/", "", "Rockbox port: YP3."
	};
	return show_text(words, ARRAYLEN(words));
}

static void settings(void)
{
	static const struct opt_items sizes[3] = { { "Small", -1 }, { "Medium", -1 }, { "Large", -1 } };
	struct opt_items pal[8];
	MENUITEM_STRINGLIST(menu, "Settings", NULL, "Palette", "New world size");
	int sel = 0;

	for (int i = 0; i < tr_palette_count() && i < 8; i++)
	{
		pal[i].string = tr_palette_name(i);
		pal[i].voice_id = -1;
	}
	while (true)
	{
		int r = rb->do_menu(&menu, &sel, NULL, false);
		if (r < 0)
			return;
		if (r == 0)
		{
			rb->set_option("Palette", &palette, RB_INT, pal, tr_palette_count(), NULL);
			tr_set_palette(palette);
		}
		else
			rb->set_option("New world size", &world_size, RB_INT, sizes, 3, NULL);
	}
}

enum { START_NEW, START_CONTINUE, START_QUIT };

static int start_menu(void)
{
	MENUITEM_STRINGLIST(menu, "Terrario", NULL, "New game", "Continue", "Settings", "Help", "About",
	                    "Playback Control", "Quit");
	int sel = getSave() ? 1 : 0;

	clear_screen();
	while (true)
	{
		int r = rb->do_menu(&menu, &sel, NULL, false);
		if (r == MENU_ATTACHED_USB)
		{
			tr_usb = true;
			return START_QUIT;
		}
		switch (r)
		{
			case 0: return START_NEW;
			case 1:
				if (getSave())
					return START_CONTINUE;
				rb->splash(HZ, "No saved world");
				break;
			case 2: settings(); break;
			case 3: if (help()) return START_QUIT; break;
			case 4: if (about()) return START_QUIT; break;
			case 5: playback_control(NULL); break;
			default: return START_QUIT;
		}
	}
}

static void leave_menu(void)
{
	registerHeld();
	registerGhost();
	rb->lcd_set_viewport(NULL);
	rb->lcd_clear_display();
	rb->lcd_update();
	tr_input_reset();
}

/* In-game menu (DOWN tap): the game's own screens. */
void gameMenu(void)
{
	MENUITEM_STRINGLIST(menu, "Terrario", NULL, "Resume", "Inventory", "Crafting", "Hotbar slot",
	                    "Place housing banner");
	MENUITEM_STRINGLIST(slots, "Hotbar slot", NULL, "1", "2", "3", "4", "5");
	int sel = 0;
	bool done = false;

	clear_screen();
	while (!done)
	{
		int r = rb->do_menu(&menu, &sel, NULL, false);
		if (r == MENU_ATTACHED_USB)
		{
			tr_usb = true;
			break;
		}
		switch (r)
		{
			case 1:
				clear_screen();
				inventoryMenu(NULL);
				done = true;
				break;
			case 2:
				clear_screen();
				craftingMenu();
				done = true;
				break;
			case 3:
			{
				int s = player.inventory.hotbarSlot;
				int h = rb->do_menu(&slots, &s, NULL, false);
				if (h >= 0)
				{
					player.inventory.hotbarSlot = h;
					player.inventory.ticksSinceInteracted = 0;
					done = true;
				}
				break;
			}
			case 4:
				if (getTile(player.cursorTile.x, player.cursorTile.y).id == TILE_NOTHING
				    && checkHousingValid(player.cursorTile))
					addMarker(player.cursorTile);
				else
					sendMessage("Can't place a banner there");
				done = true;
				break;
			default: done = true; break;
		}
	}
	leave_menu();
}

/* Pause menu (hold SELECT+UP). Returns 0 to resume, 1 to save and leave, 2 to
 * leave without saving. */
int exitMenu(void)
{
	MENUITEM_STRINGLIST(menu, "Terrario", NULL, "Resume", "Settings", "Help", "About",
	                    "Playback Control", "Save and quit", "Quit without saving");
	int sel = 0, ret = 0;
	bool done = false;

	clear_screen();
	while (!done)
	{
		int r = rb->do_menu(&menu, &sel, NULL, false);
		if (r == MENU_ATTACHED_USB)
		{
			tr_usb = true;
			ret = 2;
			break;
		}
		switch (r)
		{
			case 1: settings(); break;
			case 2: if (help()) { ret = 2; done = true; } break;
			case 3: if (about()) { ret = 2; done = true; } break;
			case 4: playback_control(NULL); break;
			case 5: ret = 1; done = true; break;
			case 6: ret = 2; done = true; break;
			default: done = true; break;
		}
	}
	leave_menu();
	return ret;
}

/* Upstream's debug and error screens. */
void memoryErrorMenu(void)
{
	rb->splash(HZ * 2, "Terrario: out of memory");
	tr_usb = true;
}

void itemMenu(void) {}

/* ------------------------------------------------------------- game -- */

static void setPlayerSpawn(void)
{
	int playerX = game.WORLD_WIDTH / 2;
	int playerY = 0;

	while (tiles[getTile(playerX, playerY).id].physics == PHYS_NON_SOLID && playerY < game.WORLD_HEIGHT)
		playerY++;
	player.spawn.x = playerX << 3;
	player.spawn.y = (playerY << 3) - player.props.height;
}

/* 60 UPS: tick (100 Hz) at which update number n is due */
static unsigned long due_tick(unsigned long base, unsigned long n)
{
	return base + n * 5 / 3;
}

static bool game_loop(void)
{
	int respawnCounter = 0;
	int frames = 0;
	enum UpdateReturnCodes updateRet;
	bool renderThisFrame = true;
	unsigned long base = *rb->current_tick, n = 0;

	tr_input_reset();
	while (true)
	{
		tr_input_frame();
		if (tr_usb)
			return false;
		updateRet = keyboardUpdate();
		if (tr_usb)
			return false;
		if (!respawnCounter) playerUpdate(frames);
		if (updateRet == UPDATE_EXIT) return true;
		else if (updateRet == UPDATE_EXIT_NOSAVE) return false;

		if (frames & 1 && world.explosion.numParticles > 0) updateExplosion();
		if (frames % 8 == 0) worldUpdate();
		if (frames % 3600 == 0) doMarkerChecks();

		doEntityCycle(frames);
		doSpawningCycle();
		npcUpdate(frames);
		if (player.combat.health <= 0)
		{
			if (respawnCounter == 1)
			{
				player.props.x = player.spawn.x;
				player.props.y = player.spawn.y;
				player.combat.health = player.maxHealth;
				player.props.xVel = 0;
				player.props.yVel = 0;
				player.props.dropping = false;
				respawnCounter = 0;
				player.combat.currImmuneFrames = player.combat.immuneFrames;
				player.breath = 200;
			}
			else if (respawnCounter > 0) respawnCounter--;
			else
			{
				createExplosion(player.props.x + (player.props.width >> 1), player.props.y + (player.props.height >> 1));
				respawnCounter = 300;
			}
		}
		if (renderThisFrame)
		{
			render(true);
			dupdate();
			renderThisFrame = false;
		}
		else renderThisFrame = true;

		updateChat();

		frames++;
		world.timeTicks++;
		if (world.timeTicks >= DAY_TICKS) world.timeTicks = 0;

		/* pace to 60 updates per second; if we are far behind, give up catching up */
		n++;
		unsigned long due = due_tick(base, n), now = *rb->current_tick;
		if (now + 30 < due || now > due + 30)
		{
			base = now;
			n = 0;
		}
		else
			while (*rb->current_tick < due)
				rb->yield();
	}
}

static void free_game(void)
{
	free(world.entities);
	free(world.explosion.particles);
	if (lastMessage != NULL) clearMessage(lastMessage);
	free(world.chests.chests);
	if (world.npcs != NULL) free(world.npcs);
	if (world.markers != NULL) free(world.markers);
	world.npcs = NULL;
	world.markers = NULL;
	lastMessage = NULL;
}

static void run_game(int choice)
{
	size_t abuf_size;
	void *abuf = rb->plugin_get_audio_buffer(&abuf_size);
	int width = MAX_WORLD_WIDTH;

	if (!abuf || abuf_size < AUDIO_BUF_RESERVE + MIN_WORLD_WIDTH * WORLD_HEIGHT_TILES)
	{
		rb->splash(HZ * 2, "Not enough memory");
		return;
	}
	if (!tr_sprites_open(TR_DATA_PATH))
	{
		rb->splash(HZ * 2, "terrario.dat missing");
		rb->plugin_release_audio_buffer();
		return;
	}

	int cap = (int)((abuf_size - AUDIO_BUF_RESERVE) / WORLD_HEIGHT_TILES) & ~3;
	if (choice == START_CONTINUE)
	{
		int h;
		if (!tr_save_peek(&width, &h) || h != WORLD_HEIGHT_TILES || width < MIN_WORLD_WIDTH
		    || width > MAX_WORLD_WIDTH || width > cap)
		{
			rb->splash(HZ * 2, "Saved world unreadable");
			goto out;
		}
	}
	else
	{
		static const int sizes[3] = { 240, 400, MAX_WORLD_WIDTH };
		width = min(sizes[world_size], cap);
	}

	game = (struct GameCompatibilityPresets) {
		.HWMODE = MODE_RAM,
		.RAM_START = abuf,
		.WORLD_WIDTH = width,
		.WORLD_HEIGHT = WORLD_HEIGHT_TILES,
		.WORLDGEN_MULTIPLIER = width / 1000.0f
	};
	save = (struct SaveData) {
		.tileDataSize = game.WORLD_HEIGHT * game.WORLD_WIDTH * sizeof(Tile),
		.regionsX = game.WORLD_WIDTH / REGION_SIZE + 1,
		.regionsY = game.WORLD_HEIGHT / REGION_SIZE + 1,
		.tileData = abuf,
		.error = -99,
	};

	srand(*rb->current_tick * 2654435761u + 1);
	tr_set_palette(palette);

	player = (struct Player) {
		.props = { .width = 12, .height = 21 },
		.combat = { .health = 100, .alignment = ALIGN_NEUTRAL, .immuneFrames = 40 },
		.cursor = { 75, 32 },
		.inventory = {
			.getFirstFreeSlot = &getFirstFreeSlot,
			.removeItem = &removeItem,
			.stackItem = &stackItem,
			.tallyItem = &tallyItem,
			.findSlot = &findSlot,
			.getSelected = &getSelected
		},
		.tool = { TOOL_TYPE_NONE },
		.breath = 200
	};
	for (int slot = 0; slot < INVENTORY_SIZE; slot++)
		player.inventory.items[slot] = (Item){ ITEM_NULL, PREFIX_NONE, 0 };

	world = (struct World) {
		.tiles = (Tile *)save.tileData,
		.entities = malloc(MAX_ENTITIES * sizeof(Entity)),
		.explosion = { .particles = malloc(EXPLOSION_NUM_PARTICLES * sizeof(Particle)) },
		.chests = {
			.chests = malloc(MAX_CHESTS * sizeof(struct Chest)),
			.addChest = &addChest,
			.removeChest = &removeChest,
			.findChest = &findChest
		},
		.placeTile = &placeTile,
		.removeTile = &removeTile,
		.spawnEntity = &spawnEntity,
		.removeEntity = &removeEntity,
		.checkFreeEntitySlot = &checkFreeEntitySlot
	};
	if (!world.entities || !world.explosion.particles || !world.chests.chests)
	{
		memoryErrorMenu();
		free_game();
		goto out;
	}
	for (int idx = 0; idx < MAX_ENTITIES; idx++)
		world.entities[idx] = (Entity){ -1 };

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
	rb->cpu_boost(true);
#endif
	if (choice == START_NEW)
	{
		generateWorld();
		memset(save.regionData, 1, save.regionsX * save.regionsY);
		player.inventory.items[0] = (Item){ ITEM_COPPER_SWORD, rand() % PREFIX_COUNT, 1 };
		player.inventory.items[1] = (Item){ ITEM_COPPER_PICK, rand() % PREFIX_COUNT, 1 };
		player.maxHealth = 100;
		world.timeTicks = timeToTicks(8, 15);
		setPlayerSpawn();
		addNPC(NPC_GUIDE);
		player.props.x = player.spawn.x;
		player.props.y = player.spawn.y;
	}
	else
	{
		dclear(C_WHITE);
		dtext_opt(64, 32, C_BLACK, C_WHITE, DTEXT_CENTER, DTEXT_MIDDLE, "Loading World...");
		dupdate();
		loadSave();
		if (save.error != -99)
		{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
			rb->cpu_boost(false);
#endif
			rb->splash(HZ * 2, "Saved world unreadable");
			free_game();
			goto out;
		}
	}

	dclear(C_WHITE);
	dupdate();
	fillVarBuffer(0, 0, VAR_BUF_WIDTH, VAR_BUF_HEIGHT);
	registerEquipment();
	registerHeld();

	clear_screen();
	in_game = true;
	bool doSave = game_loop();
	in_game = false;

	if (doSave && !tr_usb)
	{
		dclear(C_WHITE);
		dtext_opt(64, 32, C_BLACK, C_WHITE, DTEXT_CENTER, DTEXT_MIDDLE, "Saving World...");
		dupdate();
		saveGame();
		if (save.error != -99)
			rb->splash(HZ * 2, "Saving failed");
	}
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
	rb->cpu_boost(false);
#endif
	free_game();
out:
	tr_sprites_close();
	rb->plugin_release_audio_buffer();
}

/* Everything with FP code runs in here: plugin_start must have none, or
 * its prologue saves FP registers before access is on (fpucheck.py). */
static void __attribute__((noinline)) tr_menu_loop(void)
{
	while (!tr_usb)
	{
		int choice = start_menu();
		if (choice == START_QUIT)
			break;
		run_game(choice);
	}
}

enum plugin_status plugin_start(const void *parameter)
{
	(void)parameter;

	backlight_ignore_timeout();
	playback_control_init(NULL);
	tr_set_palette(palette);
	plugin_fpu_enable(true);
	tr_menu_loop();
	plugin_fpu_enable(false);
	backlight_use_settings();
	return tr_usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
