/* Rockbox display, controls, menu and checkpoints. MIT; see ../LICENSE. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/pluginlib_actions.h"
#include "api.h"
#ifndef CELESTE_COLOR
#ifdef HAVE_LCD_COLOR
#define CELESTE_COLOR 1
#else
#define CELESTE_COLOR 0
#endif
#endif
#if LCD_WIDTH < 64 || LCD_HEIGHT < 64
#error "CEleste requires a bitmap LCD of at least 64x64"
#endif
#define SAVE_PATH PLUGIN_GAMES_DATA_DIR "/celeste.sav"
#define TEMP_PATH PLUGIN_GAMES_DATA_DIR "/celeste.tmp"
#define SIDE MIN(LCD_WIDTH,LCD_HEIGHT)
#define VIEW_SIZE (SIDE>=128 ? (SIDE/128)*128 : SIDE)
void ce_zero(void *p,size_t n){rb->memset(p,0,n);}
/* Use the shared Rockbox maps for physical keys, wheel and touchscreen.
 * Read raw releases too: PLA intentionally omits many direction releases. */
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
#error "Define PORT_CENTER_BUTTON to the YP3 center key"
#endif
#endif
static const struct button_mapping yp3_context[]={
    {PLA_UP,BUTTON_UP,BUTTON_NONE}, {PLA_DOWN,BUTTON_DOWN,BUTTON_NONE},
    {PLA_LEFT,BUTTON_LEFT,BUTTON_NONE}, {PLA_RIGHT,BUTTON_RIGHT,BUTTON_NONE},
    {PLA_SELECT,PORT_CENTER_BUTTON,BUTTON_NONE}, LAST_ITEM_IN_LIST
};
#define MAIN_CONTEXT yp3_context
#else
#define MAIN_CONTEXT pla_main_ctx
#endif
static const struct button_mapping *const maps[]={MAIN_CONTEXT,
#ifdef HAVE_REMOTE_LCD
    pla_remote_ctx,
#endif
};
static int decode(unsigned raw)
{
    unsigned base=raw&~(BUTTON_REL|BUTTON_REPEAT);
    for(unsigned m=0;m<ARRAYLEN(maps);++m)
        for(const struct button_mapping *b=maps[m];b->button_code!=BUTTON_NONE;++b) {
            int a=b->action_code;
            if((unsigned)b->button_code==base &&
               (a==PLA_UP || a==PLA_DOWN || a==PLA_LEFT || a==PLA_RIGHT || a==PLA_SELECT))
                return a;
#ifdef HAVE_SCROLLWHEEL
            if((unsigned)b->button_code==base && (a==PLA_SCROLL_BACK || a==PLA_SCROLL_FWD))return a;
#endif
        }
    /* Dedicated exit/back keys may use a release or repeat mapping. */
    for(unsigned m=0;m<ARRAYLEN(maps);++m)
        for(const struct button_mapping *b=maps[m];b->button_code!=BUTTON_NONE;++b)
            if((unsigned)b->button_code==raw && (b->action_code==PLA_CANCEL || b->action_code==PLA_EXIT))
                return PLA_CANCEL;
    return ACTION_NONE;
}
static bool wheel_button(unsigned base)
{
#ifdef BUTTON_SCROLL_BACK
    if(base==BUTTON_SCROLL_BACK || base==BUTTON_SCROLL_FWD)return true;
#endif
#ifdef BUTTON_SCROLL_UP
    if(base==BUTTON_SCROLL_UP || base==BUTTON_SCROLL_DOWN)return true;
#endif
    (void)base;return false;
}
static bool usb,menu_requested,center_armed,center_used,dash_pending,diagonal_memory=true;
static unsigned long dash_at,tap_at,last_at[4];
static unsigned held[4],pulses,dash_keys,tap_direction;
static unsigned pulse_age;
static unsigned last_h,last_v;
static void clear_input(void)
{
    rb->memset(held,0,sizeof(held));rb->memset(last_at,0,sizeof(last_at));
    pulses=dash_keys=tap_direction=last_h=last_v=pulse_age=0;
    center_armed=center_used=dash_pending=false;
}
static void input_event(int raw)
{
    if(raw==BUTTON_NONE)return;
    if(rb->default_event_handler(raw)==SYS_USB_CONNECTED){usb=true;return;}
    unsigned base=(unsigned)raw&~(BUTTON_REL|BUTTON_REPEAT);
    bool rel=raw&BUTTON_REL,repeat=raw&BUTTON_REPEAT;
    unsigned long now=*rb->current_tick;
#if defined(YP3_PAD) && CONFIG_KEYPAD == YP3_PAD
    /* Chords (e.g. UP|RIGHT for a diagonal jump) arrive as one event:
       handle each key on its own; a key already held is not pressed again
       (that would add a jump or a double-tap dash). */
    if((base & (base-1)) && !(raw & SYS_EVENT)) {
        unsigned mods=(unsigned)raw&(BUTTON_REL|BUTTON_REPEAT);
        for(unsigned bit=1;bit && bit<=base;bit<<=1) {
            if(!(base&bit))continue;
            if(!rel && !repeat) {
                int ka=decode(bit);
                bool down=ka==PLA_SELECT ? center_armed :
                          ka==PLA_LEFT ? held[CE_LEFT]!=0 :
                          ka==PLA_RIGHT ? held[CE_RIGHT]!=0 :
                          ka==PLA_UP ? held[CE_UP]!=0 :
                          ka==PLA_DOWN ? held[CE_DOWN]!=0 : false;
                if(down)continue;
            }
            input_event((int)(bit|mods));
        }
        return;
    }
#endif
#ifdef HAVE_BLUETOOTH_HID
    /* Game controller: A jumps, B dashes towards the held directions,
     * Options and PS open the menu. */
    if(base==BUTTON_PAD_A || base==BUTTON_PAD_B || base==BUTTON_PAD_MENU || base==BUTTON_PAD_HOME) {
        if(rel || repeat)return;
        if(base==BUTTON_PAD_A){pulses|=1u<<CE_JUMP;pulse_age=0;tap_direction=0;}
        else if(base==BUTTON_PAD_B) {
            unsigned dirs=0;
            for(unsigned k=0;k<4;++k)if(held[k])dirs|=1u<<k;
            pulses&=~(1u<<CE_JUMP);pulses|=1u<<CE_DASH;dash_keys=dirs;pulse_age=0;tap_direction=0;
        }
        else menu_requested=true;
        return;
    }
#endif
    int a=decode(raw);
    if(a==PLA_SELECT) {
        /* Centre = dash (towards the held direction keys); hold it for the menu. */
        if(rel)center_armed=false;
        else if(repeat){if(center_armed&&!center_used){center_used=true;menu_requested=true;}}
        else {center_armed=true;center_used=false;dash_pending=true;dash_at=now;}
        return;
    }
    if(a==PLA_CANCEL){menu_requested=true;return;}
    int d=-1;
    if(a==PLA_LEFT)d=CE_LEFT;else if(a==PLA_RIGHT)d=CE_RIGHT;
    else if(a==PLA_UP)d=CE_UP;else if(a==PLA_DOWN)d=CE_DOWN;
#ifdef HAVE_SCROLLWHEEL
    if(a==PLA_SCROLL_BACK)d=CE_UP;else if(a==PLA_SCROLL_FWD)d=CE_DOWN;
#endif
    if(d<0)return;
    if(wheel_button(base)) {
        if(!rel){if(d==CE_UP)pulses|=1u<<CE_JUMP;else {pulses|=1u<<CE_DASH;dash_keys=1u<<CE_UP;}}
        pulse_age=0;tap_direction=0;return;
    }
    if(rel){held[d]=0;last_at[d]=now;return;}
    held[d]=base;
    if(d<2)last_h=d+1;else last_v=d+1;
    last_at[d]=now;
    if(repeat){tap_direction=0;return;}
    if(!ce_title() && tap_direction==(unsigned)d+1 && now-tap_at<=(unsigned)(HZ*3/10)) {
        tap_direction=0;pulses&=~(1u<<CE_JUMP);pulses|=1u<<CE_DASH;
        dash_keys=1u<<d;
        unsigned other=d<2?last_v:last_h;
        if(other && (held[other-1] || (diagonal_memory && now-last_at[other-1]<=(unsigned)(HZ/5))))
            dash_keys|=1u<<(other-1);
        pulse_age=0;
    } else {
        tap_direction=d+1;tap_at=now;
        if(d==CE_UP){pulses|=1u<<CE_JUMP;pulse_age=0;}
    }
}
static unsigned sample_keys(void)
{
    unsigned status=rb->button_status(),keys=pulses,dirs=0;
    for(unsigned d=0;d<4;++d) {
        if(held[d] && (status&held[d])!=held[d])held[d]=0;
        if(held[d]){dirs|=1u<<d;last_at[d]=*rb->current_tick;}
    }
    keys|=dirs;
    /* The centre key is read faster than the direction keys (debounce), so
     * a dash waits a moment for a direction pressed with it. */
    if(dash_pending && (dirs || *rb->current_tick-dash_at>=(unsigned long)(HZ/8))) {
        dash_pending=false;pulses|=1u<<CE_DASH;dash_keys=dirs;pulse_age=0;
        tap_direction=0;pulses&=~(1u<<CE_JUMP);keys|=pulses;
    }
    if(pulses&(1u<<CE_DASH))keys=(keys&~15u)|dash_keys;
    return keys;
}
static void consume(int accepted)
{
    /* Dash freeze lasts two frames: keep an action through freeze, but never
     * queue it indefinitely across a death/respawn or cutscene. */
    if(accepted || ++pulse_age>6){pulses=dash_keys=0;pulse_age=0;}
}
static unsigned char_col(const char *s,int x,int y)
{
    if(x<0||y<0||y>=5)return 0;
    unsigned i=x/4;if(i>=rb->strlen(s))return 0;
    unsigned c=(unsigned char)s[i];if(c<32||c>=128)c='?';
    return (ce_font[(c-32)*5+y]&(128>>(x%4)))?7:0;
}
static void present(void)
{
    int ox=(LCD_WIDTH-VIEW_SIZE)/2,oy=(LCD_HEIGHT-VIEW_SIZE)/2;
    int level,deaths,fruit,dashes,time;
    ce_status(&level,&deaths,&fruit,&dashes,&time);
    char top[40],bottom[48];
    if(ce_title())rb->snprintf(top,sizeof(top),"CELESTE CLASSIC");
    else if(level==30)rb->snprintf(top,sizeof(top),"SUMMIT");
    else if(ce_practice())rb->snprintf(top,sizeof(top),"P%02d %dF BEST:%dF",level+1,time,ce_best_time());
    else rb->snprintf(top,sizeof(top),"CELESTE  %dM",(level+1)*100);
    rb->snprintf(bottom,sizeof(bottom),"%d/30  D:%d  DASH:%d",fruit,deaths,dashes);
#ifdef HAVE_LCD_COLOR
    static fb_data row[LCD_WIDTH];fb_data palette[16];
#elif LCD_DEPTH==1
    static uint8_t row[LCD_WIDTH];
    static const uint8_t bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
#endif
#if !defined(HAVE_LCD_COLOR) || !CELESTE_COLOR
    uint8_t gray[16];
#endif
    for(unsigned i=0;i<16;++i) {
        int r=ce_rgb[i][0],g=ce_rgb[i][1],b=ce_rgb[i][2];
#if !defined(HAVE_LCD_COLOR) || !CELESTE_COLOR
        gray[i]=(r*77+g*150+b*29)>>8;
#endif
#ifdef HAVE_LCD_COLOR
#if !CELESTE_COLOR
        r=g=b=gray[i];
#endif
        palette[i]=FB_RGBPACK(r,g,b);
#endif
    }
    for(int y=0;y<LCD_HEIGHT;++y) {
        int gy=(y-oy)*128/VIEW_SIZE;
        for(int x=0;x<LCD_WIDTH;++x) {
            unsigned c=0;
            if(x>=ox&&x<ox+VIEW_SIZE&&y>=oy&&y<oy+VIEW_SIZE)
                c=ce_screen_palette[ce_pixel((x-ox)*128/VIEW_SIZE,gy)];
            else if(oy>=8) {
                if(y<oy)c=char_col(top,x-(LCD_WIDTH-(int)rb->strlen(top)*4)/2,y-(oy-5)/2);
                else c=char_col(bottom,x-(LCD_WIDTH-(int)rb->strlen(bottom)*4)/2,y-(LCD_HEIGHT+oy+VIEW_SIZE-5)/2);
            }
#ifdef HAVE_LCD_COLOR
            row[x]=palette[c];
#elif LCD_DEPTH==1
            row[x]=gray[c]>bayer[y&3][x&3]*16+8?0:1;
#else
            rb->lcd_set_foreground(LCD_BRIGHTNESS(gray[c]));rb->lcd_drawpixel(x,y);
#endif
        }
#ifdef HAVE_LCD_COLOR
        rb->lcd_bitmap(row,0,y,LCD_WIDTH,1);
#elif LCD_DEPTH==1
        rb->lcd_mono_bitmap(row,0,y,LCD_WIDTH,1);
#endif
        if((y&15)==0)rb->yield();
    }
    rb->lcd_update();
}
static uint32_t crc32(const uint8_t *p,unsigned n)
{
    uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return ~c;
}
static bool transfer(int fd,void *p,size_t n,bool writing)
{
    uint8_t *b=p;
    while(n){ssize_t k=writing?rb->write(fd,b,n):rb->read(fd,b,n);if(k<=0)return false;b+=k;n-=k;}
    return true;
}
static bool save_game(void)
{
    uint8_t data[172];unsigned n=ce_save_export(data+8);
    if(!n)return true;
    rb->memcpy(data,"CELESV1\0",8);
    uint32_t crc=crc32(data,8+n);for(unsigned i=0;i<4;++i)data[8+n+i]=crc>>(8*i);
    int fd=rb->open(TEMP_PATH,O_WRONLY|O_CREAT|O_TRUNC,0666);bool ok=false;
    if(fd>=0){ok=transfer(fd,data,12+n,true);if(rb->close(fd)<0)ok=false;}
    if(ok)ok=rb->rename(TEMP_PATH,SAVE_PATH)==0;
    if(!ok){rb->remove(TEMP_PATH);rb->splash(HZ,"CEleste: save failed");}
    return ok;
}
static void load_game(void)
{
    uint8_t data[152];int fd=rb->open(SAVE_PATH,O_RDONLY);if(fd<0)return;
    bool ok=rb->lseek(fd,0,SEEK_END)==sizeof(data)&&rb->lseek(fd,0,SEEK_SET)==0&&transfer(fd,data,sizeof(data),false);
    rb->close(fd);
    if(!ok)return;
    uint32_t crc=0;for(unsigned i=0;i<4;++i)crc|=(uint32_t)data[148+i]<<(8*i);
    if(!rb->memcmp(data,"CELESV1\0",8)&&crc32(data,148)==crc)ce_save_import(data+8,140);
}
static int menu_cb(int action,const struct menu_item_ex *this_item,struct gui_synclist *this_list)
{
    (void)this_list;
    int i=(intptr_t)this_item;
    if(action==ACTION_REQUEST_MENUITEM &&
       ((ce_title() && (i==0 || i==3 || i==10)) || (!ce_practice() && (i==4 || i==5))))
        return ACTION_EXIT_MENUITEM;
    return action;
}
/* Native Rockbox menu, shown at launch and on a centre click or Cancel.
 * Returns true to quit the plugin. */
static bool menu(void)
{
    MENUITEM_STRINGLIST(m,"Celeste",menu_cb,"Resume Game","New Game","Practice Mode",
      "Retry Room","Previous Room","Next Room","Diagonal Memory","Help",
      "About","Playback Control","Save and Quit","Quit without Saving");
    int selected=0;bool quit=false,go=false;
    menu_requested=false;clear_input();
    while(!go&&!quit&&!usb){
        switch(rb->do_menu(&m,&selected,NULL,false)){
          case 0:go=true;break;
          case 1:ce_new_game(0);go=true;break;
          case 2:ce_new_game(1);go=true;break;
          case 3:ce_retry();go=true;break;
          case 4:ce_change_room(-1);go=true;break;
          case 5:ce_change_room(1);go=true;break;
          case 6:rb->set_bool("Diagonal Memory",&diagonal_memory);break;
          case 7:rb->splash(0,"Left/Right: move. Up: jump (also off walls). Centre: dash in the direction "
              "held (Up, Left+Up, ...); you have one dash per jump and it refills on the ground, and "
              "from level 22 two. There is no second jump: dash in the air instead (double-tapping "
              "a direction also dashes). Hold Centre for the menu.");break;
          case 8:rb->splash(0,"Celeste Classic: Maddy Thorson and Noel Berry. "
              "CEleste (TI-84 CE port): John Cesarz. MIT licence, (c) 2018 Noel Berry; "
              "full text in .rockbox/licenses. Rockbox port: YP3.");break;
          case 9:playback_control(NULL);break;
          case 10:if(save_game())quit=true;break;
          case 11:quit=true;break;
          case MENU_ATTACHED_USB:usb=true;break;
          default:if(ce_title())quit=true;else go=true;break;
        }
    }
    rb->lcd_set_viewport(NULL);
#if LCD_DEPTH>1
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    clear_input();
    return quit;
}
enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;int old_mode=rb->lcd_get_drawmode();
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
    usb=false;menu_requested=true;diagonal_memory=true;clear_input();rb->button_clear_queue();
    ce_init();load_game();present();
    unsigned long last=*rb->current_tick;unsigned pending=0;
    int prev_level,d,f,j,t;ce_status(&prev_level,&d,&f,&j,&t);
    while(!usb&&!ce_fault){
        input_event(rb->button_get_w_tmo(1));
        for(unsigned i=0;i<32&&!usb;++i){int b=rb->button_get_w_tmo(0);if(b==BUTTON_NONE)break;input_event(b);}
        if(usb)break;
        if(menu_requested){if(menu())break;if(!usb)present();last=*rb->current_tick;pending=0;}
        if(usb)break;
        unsigned long now=*rb->current_tick;pending+=MIN(now-last,(unsigned long)(HZ/4))*30u;last=now;
        pending=MIN(pending,(unsigned)(HZ*3));if(pending<(unsigned)HZ)continue;pending-=HZ;
        consume(ce_step(sample_keys()));
        if(ce_fault)break;
        int level;ce_status(&level,&d,&f,&j,&t);
        if(level!=prev_level){if(level<31)save_game();prev_level=level;}
        present();
    }
    ce_shutdown();
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
    if(ce_fault)rb->splash(HZ*3,"CEleste: object pool full");
    return usb?PLUGIN_USB_CONNECTED:ce_fault?PLUGIN_ERROR:PLUGIN_OK;
}
