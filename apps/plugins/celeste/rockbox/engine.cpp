/* Original fixed-point C++ gameplay, with fixed storage and portable saves.
 * MIT; see ../LICENSE. No STL, heap, exceptions, RTTI or floating-point math. */
#define CE_ROCKBOX 1
#include "../src/classic.cpp"

struct alignas(void *) Cell { uint8_t bytes[256]; };
static Cell cells[CE_OBJECTS];
/* Deleted slots are quarantined until the end of the step: room changes and
 * self-deleting objects cannot reuse a live method's 'this' address. */
static uint8_t used[CE_OBJECTS];
static unsigned pool_peak;
static unsigned game_keys;
static uint32_t rng;
int ce_fault;
static_assert(sizeof(Player)<=sizeof(Cell),"Player exceeds object slot");
static_assert(sizeof(BigChest)<=sizeof(Cell),"Chest exceeds object slot");
static_assert(sizeof(Flag)<=sizeof(Cell),"Flag exceeds object slot");
void *Object::operator new(size_t size) noexcept
{
    if(size<=sizeof(Cell))for(unsigned i=0;i<CE_OBJECTS;++i)if(!used[i]) {
        used[i]=1;ce_zero(cells[i].bytes,sizeof(Cell));
        unsigned n=0;for(unsigned j=0;j<CE_OBJECTS;++j)n+=used[j]!=0;
        if(n>pool_peak)pool_peak=n;
        return cells[i].bytes;
    }
    ce_fault=1;return nullptr;
}
void Object::operator delete(void *ptr) noexcept
{
    if(!ptr)return;
    for(unsigned i=0;i<CE_OBJECTS;++i)if(ptr==cells[i].bytes){used[i]=2;return;}
    ce_fault=1;
}
static void collect(void){for(unsigned i=0;i<CE_OBJECTS;++i)if(used[i]==2)used[i]=0;}
int rand(){rng=rng*1664525u+1013904223u;return (rng>>8)&0x7fffff;}
int rnd(int n){return n>0?rand()%(n+1):0;}
int min(int a,int b){return a<b?a:b;}
int max(int a,int b){return a>b?a:b;}
bool btn(uint8_t key){return key<6 && (game_keys&(1u<<key));}
int fast_sin(unsigned angle)
{
    unsigned a=(angle&0xffffffu)>>16;
    if(a<=64)return ce_sine[a];
    if(a<=128)return ce_sine[128-a];
    if(a<=192)return -ce_sine[a-128];
    return -ce_sine[256-a];
}
bool practice_mode;
static int current_time,best_times[30];
void practice_on_load(){current_time=0;}
void practice_on_complete()
{
    int l=level_index();if(l<30 && (!best_times[l]||current_time<best_times[l]))best_times[l]=current_time;
    load_room(room.x,room.y);
}
int practice_get_total_time()
{
    unsigned n=0;for(int t:best_times){if(!t)return 0;n+=(unsigned)t;}
    return n>0x7fffffffu?0x7fffffff:(int)n;
}
void load_save(uint8_t handle){(void)handle;} /* calculator-only API */
void ce_shutdown(void){while(!objects.empty())delete objects[0];collect();}
void ce_init(void)
{
    ce_shutdown();ce_zero(used,sizeof(used));pool_peak=0;ce_fault=0;
    rng=0xc311e57u;practice_mode=false;ce_zero(best_times,sizeof(best_times));
    freeze=shake=delay_restart=seconds=minutes=0;will_restart=false;
    pause_player=flash_bg=new_bg=false;music_timer=sfx_timer=0;game_keys=0;
    camera();pal();_init(0);_draw();collect();
}
int ce_step(unsigned keys)
{
    game_keys=keys;bool consumed=freeze<=0 && !pause_player;
    _update();
    if(practice_mode && !is_title() && level_index()<30)++current_time;
    /* _draw contains chest/orb/particle simulation, so it runs every tick. */
    _draw();collect();return consumed;
}
void ce_status(int *l,int *d,int *f,int *j,int *t)
{
    *l=level_index();*d=deaths;*f=0;for(bool b:got_fruit)*f+=b;
    *j=player?player->djump:0;*t=practice_mode?current_time:(minutes*60+seconds)*30+frames;
}
void ce_new_game(int practice)
{
    ce_init();practice_mode=practice!=0;begin_game();collect();
}
void ce_change_room(int delta)
{
    if(!practice_mode)return;
    int n=clamp(level_index()+delta,0,30);
    max_djump=n>21?2:1;pause_player=flash_bg=false;new_bg=n>21;freeze=0;
    load_room(n%8,n/8);collect();
}
void ce_retry(void)
{
    if(is_title())return;
    pause_player=flash_bg=will_restart=false;freeze=shake=0;
    load_room(room.x,room.y);collect();
}
int ce_practice(void){return practice_mode;}
int ce_best_time(void){int n=level_index();return n<30?best_times[n]:0;}
int ce_title(void){return is_title();}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=v>>(8*i);}
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
unsigned ce_save_export(uint8_t *out)
{
    if(is_title())return 0;
    ce_zero(out,160);out[0]=level_index();out[1]=max_djump;out[2]=practice_mode;out[3]=new_bg;
    put32(out+4,deaths);put32(out+8,minutes);out[12]=seconds;out[13]=frames;
    uint32_t fruits=0;for(unsigned i=0;i<30;++i)if(got_fruit[i])fruits|=1u<<i;
    put32(out+16,fruits);for(unsigned i=0;i<30;++i)put32(out+20+4*i,best_times[i]);
    return 140;
}
int ce_save_import(const uint8_t *in,unsigned size)
{
    if(size!=140||in[0]>30||in[1]<1||in[1]>2||in[2]>1||in[3]>1||
       in[12]>59||in[13]>29||get32(in+4)>10000000||get32(in+8)>100000||
       (get32(in+16)&0xc0000000u))return 0;
    for(unsigned i=0;i<30;++i)if(get32(in+20+4*i)>100000000)return 0;
    ce_init();max_djump=in[1];practice_mode=in[2];new_bg=in[3];
    deaths=get32(in+4);minutes=get32(in+8);seconds=in[12];frames=in[13];
    for(unsigned i=0;i<30;++i){got_fruit[i]=(get32(in+16)>>i)&1;best_times[i]=get32(in+20+4*i);}
    load_room(in[0]%8,in[0]/8);collect();return 1;
}
