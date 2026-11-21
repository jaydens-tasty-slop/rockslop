/* Terrario save/load on the Rockbox file API: one file holding the header,
 * player, chests, housing and the run-length coded world. Replaces the
 * calculator storage-memory save code of upstream's save.c. */
#include "tr_gint.h"

#include "save.h"
#include "defs.h"
#include "world.h"
#include "npc.h"
#include "tr_game.h"

#define MAGIC 0x31525254 /* "TRR1" */

struct SaveHeader {
	uint32_t magic;
	char version[VERSION_BUFFER_SIZE];
	int32_t width, height;
	int32_t timeTicks;
	int32_t numChests, numMarkers, numHomeless;
};

struct PlayerSave {
	Item items[INVENTORY_SIZE];
	Item accessories[5];
	Item armour[3];
	int health;
	int maxHealth;
	Coords spawn;
	int x, y;
};

struct HousingSave {
	Coords position;
	enum NPCs npc;
};

static int fd;
static uint8_t iobuf[512];
static int iolen;
static bool ioerr;

static void flush(void)
{
	if (iolen && rb->write(fd, iobuf, iolen) != iolen)
		ioerr = true;
	iolen = 0;
}

static void put(const void *data, int n)
{
	const uint8_t *p = data;
	while (n > 0)
	{
		int c = min(n, (int)sizeof iobuf - iolen);
		memcpy(iobuf + iolen, p, c);
		iolen += c;
		p += c;
		n -= c;
		if (iolen == (int)sizeof iobuf)
			flush();
	}
}

static bool get(void *data, int n)
{
	return rb->read(fd, data, n) == n;
}

bool getSave(void)
{
	return rb->file_exists(TR_SAVE_PATH);
}

bool tr_save_peek(int *width, int *height)
{
	struct SaveHeader h;
	fd = rb->open(TR_SAVE_PATH, O_RDONLY);
	if (fd < 0)
		return false;
	bool ok = get(&h, sizeof h) && h.magic == MAGIC;
	rb->close(fd);
	if (ok)
	{
		*width = h.width;
		*height = h.height;
	}
	return ok;
}

void getVersionInfo(void)
{
	extern char versionBuffer[16];
	struct SaveHeader h;
	fd = rb->open(TR_SAVE_PATH, O_RDONLY);
	if (fd < 0 || !get(&h, sizeof h))
		versionBuffer[0] = 0;
	else
		strncpy(versionBuffer, h.version, VERSION_BUFFER_SIZE);
	if (fd >= 0)
		rb->close(fd);
}

void saveGame(void)
{
	struct SaveHeader h = { .magic = MAGIC, .width = game.WORLD_WIDTH, .height = game.WORLD_HEIGHT,
	                        .timeTicks = world.timeTicks, .numChests = world.chests.number,
	                        .numMarkers = world.numMarkers };
	struct PlayerSave ps;
	struct HousingSave hs;

	for (int i = 0; i < world.numNPCs; i++)
		if (world.npcs[i].marker == -1)
			h.numHomeless++;
	strncpy(h.version, VERSION, VERSION_BUFFER_SIZE);

	memset(&ps, 0, sizeof ps);
	ps.health = player.combat.health;
	ps.maxHealth = player.maxHealth;
	memcpy(ps.items, player.inventory.items, sizeof ps.items);
	memcpy(ps.accessories, player.inventory.accessories, sizeof ps.accessories);
	memcpy(ps.armour, player.inventory.armour, sizeof ps.armour);
	ps.spawn = player.spawn;
	ps.x = player.props.x;
	ps.y = player.props.y;

	save.error = -99;
	rb->mkdir(TR_DIR);
	fd = rb->open(TR_SAVE_PATH ".tmp", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0)
	{
		save.error = -3;
		return;
	}
	iolen = 0;
	ioerr = false;
	put(&h, sizeof h);
	put(&ps, sizeof ps);
	put(world.chests.chests, h.numChests * sizeof(struct Chest));
	for (int i = 0; i < world.numMarkers; i++)
	{
		HouseMarker *m = &world.markers[i];
		hs.position = m->position;
		hs.npc = m->occupant != -1 ? world.npcs[m->occupant].id : NPC_NONE;
		put(&hs, sizeof hs);
	}
	for (int i = 0; i < world.numNPCs; i++)
	{
		NPC *n = &world.npcs[i];
		if (n->marker == -1)
		{
			hs.position = (Coords){ n->props.x, n->props.y };
			hs.npc = n->id;
			put(&hs, sizeof hs);
		}
	}

	/* tiles as (id, run length <= 255) pairs */
	int total = game.WORLD_WIDTH * game.WORLD_HEIGHT, i = 0;
	while (i < total)
	{
		uint8_t id = world.tiles[i].id;
		int run = 1;
		while (i + run < total && run < 255 && world.tiles[i + run].id == id)
			run++;
		uint8_t pair[2] = { id, (uint8_t)run };
		put(pair, 2);
		i += run;
	}
	flush();
	if (rb->close(fd) < 0)
		ioerr = true;
	if (ioerr)
	{
		rb->remove(TR_SAVE_PATH ".tmp");
		save.error = -3;
		return;
	}
	rb->remove(TR_SAVE_PATH);
	rb->rename(TR_SAVE_PATH ".tmp", TR_SAVE_PATH);
}

void loadSave(void)
{
	struct SaveHeader h;
	struct PlayerSave ps;
	struct HousingSave hs;
	NPC *npc;

	save.error = -1;
	fd = rb->open(TR_SAVE_PATH, O_RDONLY);
	if (fd < 0)
		return;
	if (!get(&h, sizeof h) || h.magic != MAGIC || h.width != game.WORLD_WIDTH || h.height != game.WORLD_HEIGHT
	    || h.numChests < 0 || h.numChests > MAX_CHESTS || !get(&ps, sizeof ps)
	    || !get(world.chests.chests, h.numChests * sizeof(struct Chest)))
	{
		rb->close(fd);
		return;
	}
	world.chests.number = h.numChests;
	world.timeTicks = save.timeTicks = h.timeTicks;
	player.combat.health = ps.health;
	player.maxHealth = ps.maxHealth;
	memcpy(player.inventory.items, ps.items, sizeof ps.items);
	memcpy(player.inventory.accessories, ps.accessories, sizeof ps.accessories);
	memcpy(player.inventory.armour, ps.armour, sizeof ps.armour);
	player.spawn = ps.spawn;

	/* housing is read before the tiles are, so keep it until they are in */
	int numHousing = h.numMarkers + h.numHomeless;
	struct HousingSave *housing = malloc(numHousing * sizeof(struct HousingSave) + 1);
	if (!housing || h.numMarkers < 0 || h.numHomeless < 0 || !get(housing, numHousing * sizeof(struct HousingSave)))
	{
		free(housing);
		rb->close(fd);
		return;
	}

	int total = game.WORLD_WIDTH * game.WORLD_HEIGHT, i = 0;
	uint8_t buf[256];
	while (i < total)
	{
		int n = rb->read(fd, buf, sizeof buf);
		if (n < 2)
		{
			free(housing);
			rb->close(fd);
			return;
		}
		for (int j = 0; j + 1 < n && i < total; j += 2)
			for (int c = 0; c < buf[j + 1] && i < total; c++)
				world.tiles[i++].id = buf[j];
	}
	rb->close(fd);

	for (int idx = 0; idx < h.numMarkers; idx++)
	{
		hs = housing[idx];
		if (!checkHousingValid(hs.position))
		{
			if (hs.npc != NPC_NONE)
			{
				addNPC(hs.npc);
				npc = &world.npcs[world.numNPCs - 1];
				npc->props.x = hs.position.x << 3;
				npc->props.y = hs.position.y << 3;
				continue;
			}
		}
		addMarker(hs.position);
		if (hs.npc != NPC_NONE)
		{
			HouseMarker *marker = &world.markers[world.numMarkers - 1];
			addNPC(hs.npc);
			npc = &world.npcs[world.numNPCs - 1];
			npc->props.x = hs.position.x << 3;
			npc->props.y = hs.position.y << 3;
			npc->marker = idx;
			marker->occupant = world.numNPCs - 1;
		}
	}
	for (int idx = 0; idx < h.numHomeless; idx++)
	{
		hs = housing[h.numMarkers + idx];
		addNPC(hs.npc);
		npc = &world.npcs[world.numNPCs - 1];
		npc->props.x = hs.position.x;
		npc->props.y = hs.position.y;
	}
	free(housing);

	player.props.x = ps.x;
	player.props.y = ps.y;
	save.error = -99;
}
