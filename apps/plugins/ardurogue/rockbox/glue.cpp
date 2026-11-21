/* ArduRogue platform glue (game side). The Rockbox API lives in platform.c. */
#include "../src/game.hpp"

extern "C" {
enum { AR_NONE, AR_UP, AR_DOWN, AR_LEFT, AR_RIGHT, AR_A, AR_B, AR_SAVEQUIT, AR_ABANDON };
int ar_wait_btn(void);
void ar_wait(void);
void ar_paint(const uint8_t* b, int offset);
unsigned ar_seed(void);
void ar_file_load(uint8_t* p, int n);
void ar_file_store(const uint8_t* p, int n);
void ar_quit(void);
void ar_load_save(void);
void ar_run(void) { run(); }
}

static uint8_t persistent_data[1024];

extern "C" void ar_load_save(void)
{
    ar_file_load(persistent_data, sizeof(persistent_data));
}

uint8_t read_persistent(uint16_t addr) { return persistent_data[addr % 1024]; }
void update_persistent(uint16_t addr, uint8_t data) { persistent_data[addr % 1024] = data; }
void flush_persistent() { ar_file_store(persistent_data, sizeof(persistent_data)); }

void seed()
{
    rand_seed = (uint16_t)ar_seed();
    if(rand_seed == 0) rand_seed = 0xbabe;
}

void wait() { ar_wait(); }

uint8_t wait_btn()
{
    for(;;)
    {
        switch(ar_wait_btn())
        {
        case AR_UP:    return BTN_UP;
        case AR_DOWN:  return BTN_DOWN;
        case AR_LEFT:  return BTN_LEFT;
        case AR_RIGHT: return BTN_RIGHT;
        case AR_A:     return BTN_A;
        case AR_B:     return BTN_B;
        case AR_SAVEQUIT:
            /* a run exists only once the player entity is alive */
            if(ents[0].type == entity::PLAYER)
                save();
            ar_quit();
            break;
        case AR_ABANDON:
            ar_quit();
            break;
        }
    }
}

void paint_offset(uint8_t x, bool clear)
{
    ar_paint(buf.data(), x);
    if(clear)
        memzero(buf.data(), sizeof(buf));
}

