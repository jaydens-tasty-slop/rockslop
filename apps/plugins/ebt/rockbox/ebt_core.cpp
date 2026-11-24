/* EBT Tracker core for Rockbox: the upstream synth and player (unmodified,
 * ../upstream, WTFPL, (c) shiru8bit) with a song loader following the
 * upstream .ebt file parser. No editor. */
#include "ebt_types.h"

static song_struct song_data;
static song_struct *song = &song_data;
static unsigned mute_state;

/* Editor state the (unused here) pattern test functions of the player refer to. */
static int cur_ins = 1;
static struct { int ptn_sound_len; } config;

#include "../upstream/ebt_synth.h"
#include "../upstream/ebt_player.h"

#define SIGNATURE_SONG "ebtsongv1"

static int parse_hex_char(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

static bool parse_tag(const char *line, const char *tag)
{
	while (*tag) {
		if (*line++ != *tag++) return false;
	}
	return true;
}

static uint8_t parse_hex4(const char *line)
{
	return parse_hex_char(line[0]);
}

static uint8_t parse_hex8(const char *line)
{
	return (parse_hex_char(line[0]) << 4) | parse_hex_char(line[1]);
}

static void parse_str(const char *line, char *str, int size)
{
	for (int i = 0; i < size; i++) {
		char c = line[i];
		if (c < ' ') break;
		str[i] = c;
	}
}

static void song_clear(void)
{
	memset(song, 0, sizeof(*song));
	song->speed_even = DEFAULT_SONG_SPEED;
	song->speed_odd = DEFAULT_SONG_SPEED;
	song->speed_interleave = 1;
	song->order.loop_start = MAX_ORDER_LEN - 1;
	song->order.loop_end = MAX_ORDER_LEN - 1;

	for (int p = 0; p < MAX_PATTERNS; p++) {
		song->ptns[p].len = DEFAULT_PATTERN_LEN;
		song->ptns[p].loop = DEFAULT_PATTERN_LOOP;
	}
	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		song->ins[i].volume = DEFAULT_VOLUME;
		song->ins[i].base_note = DEFAULT_BASE_NOTE;
	}
}

static void instrument_parse(int ins)
{
	instrument_struct *is = &song->ins[ins];

	for (;;) {
		const char *line = ebt_file_get_line();

		if (!line || parse_tag(line, "ie")) break;

		uint8_t param = parse_hex8(line + 2);

		if (parse_tag(line, "wa")) is->wave = param;
		if (parse_tag(line, "vo")) is->volume = param;
		if (parse_tag(line, "oc")) is->offset = (int8_t)param * 12;	//v1.1 and earlier
		if (parse_tag(line, "of")) is->offset = (int8_t)param;
		if (parse_tag(line, "dt")) is->detune = (int8_t)param;
		if (parse_tag(line, "sl")) is->slide = (int8_t)param;
		if (parse_tag(line, "md")) is->mod_delay = param;
		if (parse_tag(line, "ms")) is->mod_speed = param;
		if (parse_tag(line, "me")) is->mod_depth = param;
		if (parse_tag(line, "ct")) is->cut_time = param;
		if (parse_tag(line, "fp")) is->fixed_pitch = param;
		if (parse_tag(line, "bn")) is->base_note = param;
		if (parse_tag(line, "xi")) is->aux_id = (int8_t)param;
		if (parse_tag(line, "xm")) is->aux_mix = param;
		if (parse_tag(line, "nm")) {
			memset(is->name, 0, sizeof(is->name));
			for (int i = 0; i < MAX_INSTRUMENT_NAME_LEN; i++) {
				char c = line[2 + i];
				if (c < ' ') break;
				is->name[i] = c;
			}
		}
	}
}

static void main_parse(void)
{
	for (;;) {
		const char *line = ebt_file_get_line();

		if (!line || parse_tag(line, "me")) break;

		uint8_t param = parse_hex8(line + 2);

		if (parse_tag(line, "se")) song->speed_even = param;
		if (parse_tag(line, "so")) song->speed_odd = param;
		if (parse_tag(line, "si")) song->speed_interleave = param;
		if (parse_tag(line, "sn")) parse_str(line + 2, song->name, sizeof(song->name));
		if (parse_tag(line, "sa")) parse_str(line + 2, song->author, sizeof(song->author));
		if (parse_tag(line, "dp")) {
			for (int ch = 0; ch < MAX_CHANNELS; ch++)
				song->pan_default[ch] = (int8_t)parse_hex8(line + 2 + ch * 2);
		}
	}
}

static void order_parse(void)
{
	for (;;) {
		const char *line = ebt_file_get_line();

		if (!line || parse_tag(line, "oe")) break;

		if (parse_tag(line, "ls")) {
			song->order.loop_start = parse_hex8(line + 2);
			continue;
		}
		if (parse_tag(line, "le")) {
			song->order.loop_end = parse_hex8(line + 2);
			continue;
		}

		uint8_t pos = parse_hex8(line);

		if (pos >= MAX_ORDER_LEN) continue;
		for (int ch = 0; ch < MAX_CHANNELS; ch++) {
			song->order.pos[pos].ptn[ch] = parse_hex8(line + 2 + ch * 4);
			song->order.pos[pos].trans[ch] = parse_hex8(line + 4 + ch * 4);
		}
	}
}

static void pattern_parse(int ptn)
{
	pattern_struct *ps = &song->ptns[ptn];

	for (;;) {
		const char *line = ebt_file_get_line();

		if (!line || parse_tag(line, "pe")) break;

		if (parse_tag(line, "ln")) {
			ps->len = parse_hex8(line + 2);
			if (ps->len > MAX_PATTERN_LEN) ps->len = MAX_PATTERN_LEN;
			continue;
		}
		if (parse_tag(line, "lp")) {
			ps->loop = parse_hex8(line + 2);
			continue;
		}

		uint8_t row = parse_hex8(line);

		if (row >= MAX_PATTERN_LEN) continue;

		pattern_row_struct *rs = &ps->rows[row];

		rs->note = parse_hex8(line + 2);
		rs->ins = parse_hex8(line + 4);

		for (int e = 0; e < MAX_EFFECTS_PER_ROW; e++) {
			int ptr = 6 + e * 3;

			if (line[ptr] < ' ') break;

			rs->effect[e].type = line[ptr] == '.' ? 0 : parse_hex4(line + ptr) + 1;
			rs->effect[e].param = parse_hex8(line + ptr + 1);
		}
	}
}

bool ebt_core_load(void)
{
	const char *sig = ebt_file_get_line();

	if (!sig || !parse_tag(sig, SIGNATURE_SONG))
		return false;

	song_clear();

	for (;;) {
		const char *chunk = ebt_file_get_line();

		if (!chunk) break;

		if (parse_tag(chunk, "mn")) main_parse();
		if (parse_tag(chunk, "or")) order_parse();
		if (parse_tag(chunk, "pt")) {
			int ptn = parse_hex8(chunk + 2);
			if (ptn < MAX_PATTERNS) pattern_parse(ptn);
		}
		if (parse_tag(chunk, "in")) {
			int ins = parse_hex8(chunk + 2);
			if (ins < MAX_INSTRUMENTS) instrument_parse(ins);
		}
	}
	return true;
}

void ebt_core_start(int sample_rate, unsigned mute)
{
	mute_state = mute;
	ebt_player_init(sample_rate);
	ebt_player_start_order(0);
}

void ebt_core_stop(void)
{
	ebt_player_stop();
	ebt_synth_reset();
}

void ebt_core_set_mute(unsigned mute)
{
	mute_state = mute;
}

void ebt_core_status(struct ebt_status *st)
{
	memset(st, 0, sizeof(*st));
	memcpy(st->name, song->name, sizeof(song->name));
	memcpy(st->author, song->author, sizeof(song->author));
	st->order = ebt_player_report_order_position();
	st->row = ebt_player_report_row_position();
	for (int ch = 0; ch < MAX_CHANNELS; ch++) {
		if (st->order >= 0 && st->row >= 0) {
			int ptn = song->order.pos[st->order].ptn[ch];
			st->note[ch] = ptn ? song->ptns[ptn].rows[st->row].note : 0;
		}
		const synth_channel_struct *c = &synth.chn[ch];
		st->level[ch] = c->running ? c->volume : 0;
	}
}

int ebt_core_note(int order, int ch, int row)
{
	if (order < 0 || order >= MAX_ORDER_LEN || row < 0)
		return 0xff;
	int ptn = song->order.pos[order].ptn[ch];
	if (!ptn || row >= song->ptns[ptn].len)
		return 0xff;
	return song->ptns[ptn].rows[row].note;
}

/* The synth output is unsigned (0..16 per side): remove the DC offset. */
void ebt_core_render(int16_t *out, int frames)
{
	static int32_t dc_l, dc_r;

	for (int i = 0; i < frames; i++) {
		stereo_sample_struct s = ebt_player_render_sample();
		int32_t l = s.l * 1800, r = s.r * 1800;
		dc_l += (l - dc_l) >> 9;
		dc_r += (r - dc_r) >> 9;
		out[2 * i] = l - dc_l;
		out[2 * i + 1] = r - dc_r;
	}
}
