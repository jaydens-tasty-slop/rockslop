/* Rockbox display, five-button input and scheduling. CC0-1.0. */
#include "port.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/pluginlib_actions.h"

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
static bool usb, firing, center_armed, center_used;
static unsigned long center_since, tap_at;
static unsigned center_mask, held[4], suppressed;
static unsigned pulses, tap_direction;
static uint8_t pulse_frames[16];
#ifdef HAVE_BLUETOOTH_HID
static unsigned pad_prev; /* game controller buttons already down */
#endif
#ifdef HAVE_LCD_COLOR
static fb_data native_palette[256];
#else
static uint8_t gray_palette[256];
#endif
static void video_init(void)
{
    for(unsigned i=0;i<256;++i) {
        unsigned c=anarch_color(i);
        int r=(c>>11)*255/31,g=((c>>5)&63)*255/63,b=(c&31)*255/31;
#if !ANARCH_COLOR || !defined(HAVE_LCD_COLOR)
        r=(r*77+g*150+b*29)>>8;
#ifdef HAVE_LCD_COLOR
        g=b=r;
#endif
#endif
#ifdef HAVE_LCD_COLOR
        native_palette[i]=FB_RGBPACK(r,g,b);
#else
        gray_palette[i]=r;
#endif
    }
}

/* Color/grayscale output uses a single native row, never a full RGB copy. */
static void present(void)
{
#ifdef HAVE_LCD_COLOR
    static fb_data row[ANARCH_W*ANARCH_SCALE];
#elif LCD_DEPTH == 1
    static uint8_t row[ANARCH_W*ANARCH_SCALE];
    static const uint8_t bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
#endif
    int ox=(LCD_WIDTH-ANARCH_W*ANARCH_SCALE)/2;
    int oy=(LCD_HEIGHT-ANARCH_H*ANARCH_SCALE)/2;
    for(int y=0;y<ANARCH_H;++y) {
        for(int x=0;x<ANARCH_W;++x) {
            unsigned c=anarch_pixels[y][x];
#ifdef HAVE_LCD_COLOR
            fb_data color=native_palette[c];
            for(int i=0;i<ANARCH_SCALE;++i)row[x*ANARCH_SCALE+i]=color;
#elif LCD_DEPTH == 1
            uint8_t color=gray_palette[c]>bayer[y&3][x&3]*16+8 ? 0 : 1;
            for(int i=0;i<ANARCH_SCALE;++i)row[x*ANARCH_SCALE+i]=color;
#else
            rb->lcd_set_foreground(LCD_BRIGHTNESS(gray_palette[c]));
            rb->lcd_fillrect(ox+x*ANARCH_SCALE,oy+y*ANARCH_SCALE,ANARCH_SCALE,ANARCH_SCALE);
#endif
        }
        for(int j=0;j<ANARCH_SCALE;++j) {
#ifdef HAVE_LCD_COLOR
            rb->lcd_bitmap(row,ox,oy+y*ANARCH_SCALE+j,ANARCH_W*ANARCH_SCALE,1);
#elif LCD_DEPTH == 1
            rb->lcd_mono_bitmap(row,ox,oy+y*ANARCH_SCALE+j,ANARCH_W*ANARCH_SCALE,1);
#endif
        }
        if((y&15)==0)rb->yield();
    }
    rb->lcd_update();
}
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
static void pulse(unsigned key)
{ pulses|=1u<<key;pulse_frames[key]=1; }
static void clear_input(void)
{
    rb->memset(held,0,sizeof(held));rb->memset(pulse_frames,0,sizeof(pulse_frames));
    pulses=suppressed=tap_direction=0;center_armed=center_used=false;anarch_keys=0;
#ifdef HAVE_BLUETOOTH_HID
    pad_prev=0;
#endif
}
static void input_event(int raw)
{
    if(raw==BUTTON_NONE)return;
    if(rb->default_event_handler(raw)==SYS_USB_CONNECTED){usb=true;return;}
#ifdef HAVE_BLUETOOTH_HID
    /* Game controller: A fires (held), B jumps (back on the map), L1/R1
       change weapon, L2/R2 strafe and Y shows the map (held), Options or PS
       opens the menu. An event carries every button held: split the
       controller's buttons from the keys and act on new presses only. */
    if(!(raw&SYS_EVENT) && (raw&BUTTON_PAD_ALL)) {
        unsigned pad=raw&BUTTON_PAD_ALL,mods=raw&(BUTTON_REL|BUTTON_REPEAT);
        unsigned rest=raw&~(BUTTON_PAD_ALL|BUTTON_REL|BUTTON_REPEAT);
        if(!mods) {
            unsigned fresh=pad&~pad_prev,h=center_armed?center_mask:0;
            pad_prev=pad;
            for(unsigned d=0;d<4;++d)h|=held[d];
            rest&=~h;
            if(fresh&BUTTON_PAD_A)pulse(anarch_map()?K_BACK:K_FIRE);
            if(fresh&BUTTON_PAD_B) {
                if(anarch_map())pulse(K_BACK);
                else if(anarch_playing())pulse(K_JUMP);
            }
            if(anarch_playing()) {
                if(fresh&BUTTON_PAD_L1)pulse(K_PREV);
                if(fresh&BUTTON_PAD_R1)pulse(K_NEXT);
            }
            if(fresh&(BUTTON_PAD_MENU|BUTTON_PAD_HOME)) {
                if(anarch_map())pulse(K_BACK);
                else if(anarch_playing()){pulse(K_MENU);firing=false;}
            }
        } else if(mods==BUTTON_REL)pad_prev&=~pad;
        if(!rest)return;
        raw=(int)(rest|mods);
    }
#endif
    unsigned base=(unsigned)raw&~(BUTTON_REL|BUTTON_REPEAT);
    bool release=raw&BUTTON_REL,repeat=raw&BUTTON_REPEAT;
    unsigned long now=*rb->current_tick;
#if defined(YP3_PAD) && CONFIG_KEYPAD == YP3_PAD
    /* Chords (e.g. UP|LEFT: walk while turning) arrive as one event:
       handle each key on its own; a key already held is not pressed again
       (that would add a double-tap action). */
    if((base & (base-1)) && !(raw & SYS_EVENT)) {
        unsigned mods=(unsigned)raw&(BUTTON_REL|BUTTON_REPEAT);
        for(unsigned bit=1;bit && bit<=base;bit<<=1) {
            if(!(base&bit))continue;
            if(!release && !repeat) {
                int ka=decode(bit);
                bool down=ka==PLA_SELECT ? center_armed :
                          ka==PLA_LEFT ? held[K_LEFT]!=0 :
                          ka==PLA_RIGHT ? held[K_RIGHT]!=0 :
                          ka==PLA_UP ? held[K_UP]!=0 :
                          ka==PLA_DOWN ? held[K_DOWN]!=0 : false;
                if(down)continue;
            }
            input_event((int)(bit|mods));
        }
        return;
    }
#endif
    int a=decode(raw);
    if(a==PLA_SELECT) {
        if(release) {
            bool click=center_armed&&!center_used&&now-center_since<(unsigned)(HZ*3/5);
            center_armed=false;
            if(click) {
                if(anarch_playing())firing=!firing;
                else pulse(anarch_map()?K_BACK:K_FIRE);
            }
        } else if(repeat) {center_armed=false;center_used=true;}
        else {center_armed=true;center_used=false;center_since=now;center_mask=base;}
        return;
    }
    if(center_armed){center_used=true;center_armed=false;}
    if(a==PLA_CANCEL) {
        firing=false;
        if(anarch_map())pulse(K_BACK);
        else if(anarch_playing())pulse(K_MENU);
        return;
    }
    int d=-1;
    if(a==PLA_UP)d=K_UP;else if(a==PLA_RIGHT)d=K_RIGHT;
    else if(a==PLA_DOWN)d=K_DOWN;else if(a==PLA_LEFT)d=K_LEFT;
#ifdef HAVE_SCROLLWHEEL
    if(a==PLA_SCROLL_BACK)d=K_UP;else if(a==PLA_SCROLL_FWD)d=K_DOWN;
#endif
    if(d<0)return;
    if(wheel_button(base)) { if(!release)pulse(d);tap_direction=0;return; }
    if(release){held[d]=0;suppressed&=~(1u<<d);return;}
    held[d]=base;
    if(repeat){tap_direction=0;return;}
    if(anarch_playing() && tap_direction==(unsigned)d+1 &&
       now-tap_at<=(unsigned)(HZ*3/10)) {
        tap_direction=0;
        if(d==K_UP)pulse(K_JUMP); /* forward+jump on the second press */
        else {
            suppressed|=1u<<d;
            pulses&=~(1u<<d);pulse_frames[d]=0;
            if(d==K_LEFT)pulse(K_PREV);
            else if(d==K_RIGHT)pulse(K_NEXT);
            else {pulse(K_MENU);firing=false;}
        }
    } else {tap_direction=d+1;tap_at=now;pulse(d);}
}
static void sample_keys(void)
{
    unsigned status=rb->button_status();
    if(center_armed && ((status&center_mask)!=center_mask ||
       (unsigned long)*rb->current_tick-center_since>=(unsigned)(HZ*3/5))) {
        /* A missing release cannot become a delayed click. */
        center_armed=false;center_used=true;
    }
    /* The ports' common pause gesture: hold Select+Up for half a second
       (Select alone acts only on a click, so the chord is free). */
    static unsigned long chord_since;static bool chord_done;
    if((status&(PORT_CENTER_BUTTON|BUTTON_UP))==(PORT_CENTER_BUTTON|BUTTON_UP)) {
        if(!chord_since)chord_since=*rb->current_tick|1;
        else if(!chord_done && *rb->current_tick-chord_since>=(unsigned long)(HZ/2)) {
            chord_done=true;firing=false;suppressed|=1u<<K_UP;
            if(anarch_playing())pulse(K_MENU);
        }
    } else {chord_since=0;chord_done=false;}
    anarch_keys=pulses;
    for(unsigned d=0;d<4;++d) {
        if(held[d] && (status&held[d])!=held[d]) {held[d]=0;suppressed&=~(1u<<d);}
        if(held[d] && !(suppressed&(1u<<d)))anarch_keys|=1u<<d;
    }
    if(anarch_playing()&&firing)anarch_keys|=1u<<K_FIRE;
#ifdef HAVE_BLUETOOTH_HID
    if(anarch_playing()) {
        if(status&BUTTON_PAD_A)anarch_keys|=1u<<K_FIRE;
        if(status&BUTTON_PAD_Y)anarch_keys|=1u<<K_MAP;
        if(status&BUTTON_PAD_L2)anarch_keys|=1u<<K_STRAFE_LEFT;
        if(status&BUTTON_PAD_R2)anarch_keys|=1u<<K_STRAFE_RIGHT;
    }
#endif
}
static void consumed_frame(void)
{
    for(unsigned k=0;k<16;++k)if(pulse_frames[k] && !--pulse_frames[k])pulses&=~(1u<<k);
}
static int level;
static int menu_cb(int action,const struct menu_item_ex *this_item,struct gui_synclist *this_list)
{
    (void)this_list;
    int i=(intptr_t)this_item;
    if(action==ACTION_REQUEST_MENUITEM &&
       ((!anarch_has_level() && (i==0 || i==1)) || (i==4 && !anarch_has_save())))
        return ACTION_EXIT_MENUITEM;
    return action;
}
/* Native Rockbox menu replacing the engine's own. Returns -1 to quit, else
 * the engine menu item to confirm (picked into the engine here). */
static int native_menu(void)
{
    static const struct opt_items sounds[]={
        {"Off",-1},{"Effects",-1},{"Music",-1},{"Effects and Music",-1}};
    static const struct opt_items shears[]={{"0",-1},{"1",-1},{"2",-1}};
    MENUITEM_STRINGLIST(m,"Anarch",menu_cb,"Continue","Map","Start New Game","Level",
        "Load Game","Sound","Shear","Help","Playback Control","Quit");
    static int selected;
    int result=-2;
    /* Paused, it opens on Continue (Select resumes, Up wraps to Quit), not
       on the last pick: that was Start New Game, a restart on one Select. */
    if(anarch_has_level())selected=0;
    else if(selected<2)selected=2;
    while(result==-2 && !usb) {
        switch(rb->do_menu(&m,&selected,NULL,false)) {
        case 0:result=0;break;           /* SFG_MENU_ITEM_CONTINUE */
        case 1:result=1;break;           /* SFG_MENU_ITEM_MAP */
        case 2:result=2;break;           /* SFG_MENU_ITEM_PLAY */
        case 3: {
            int n=level+1;
            rb->set_int("Level","",UNIT_INT,&n,NULL,1,1,anarch_max_level()+1,NULL);
            level=n-1;
            break;
        }
        case 4:result=3;break;           /* SFG_MENU_ITEM_LOAD */
        case 5: {
            int v=anarch_get_setting(0);
            rb->set_option("Sound",&v,RB_INT,sounds,4,NULL);
            anarch_set_setting(0,v);
            break;
        }
        case 6: {
            int v=anarch_get_setting(1);if(v==3)v=2;
            rb->set_option("Shear",&v,RB_INT,shears,3,NULL);
            anarch_set_setting(1,v==2?3:v);
            break;
        }
        case 7:
            rb->splash(0,"Hold Up/Down: move. Hold Left/Right: turn. Double Up: jump. "
                "Double Left/Right: change weapon. Double Down or hold Select+Up: menu. Centre click: fire on/off.");
            break;
        case 8:playback_control(NULL);break;
        case MENU_ATTACHED_USB:usb=true;break;
        case 9:result=-1;break;
        default:result=anarch_has_level()?0:-1;break;
        }
    }
    rb->lcd_set_viewport(NULL);
#if LCD_DEPTH>1
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    return usb?-1:result;
}
enum plugin_status plugin_start(const void *parameter)
{
    const char *path=parameter?parameter:PLUGIN_GAMES_DATA_DIR "/anarch.dat";
    if(!anarch_assets_open(path)) {
        rb->splash(3*HZ,"Anarch: missing/invalid anarch.dat");return PLUGIN_ERROR;
    }
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
    usb=firing=false;clear_input();rb->button_clear_queue();
    anarch_audio_init();anarch_init();video_init();
    unsigned long last=*rb->current_tick;
    unsigned remainder=0,pending=0;
    bool picked=false;
    while(!usb&&!anarch_io_error) {
        input_event(rb->button_get_w_tmo(1));
        for(unsigned i=0;i<32&&!usb;++i) {
            int b=rb->button_get_w_tmo(0);if(b==BUTTON_NONE)break;input_event(b);
        }
        if(usb)break;
        if(anarch_menu_state() && !picked) {
            int item=native_menu();
            if(item<0)break;
            anarch_menu_pick(item,level);
            clear_input();pulse(K_FIRE);picked=true;
            last=*rb->current_tick;pending=0;
        }
        unsigned long now=*rb->current_tick;
        unsigned ticks=MIN((unsigned long)(HZ/4),now-last);
        last=now;remainder+=ticks*1000u;
        unsigned elapsed=remainder/HZ;remainder%=HZ;
        /* One logical step per input sample: a click never repeats across a
         * large catch-up burst. Excess stall time intentionally slows play. */
        pending=MIN(pending+elapsed,200u);
        if(pending<40)continue;
        pending-=40;anarch_time+=40;
        sample_keys();
        bool was_playing=anarch_playing();
        if(!anarch_frame())break;
        picked=false;
        consumed_frame();
        if(was_playing&&!anarch_playing()){firing=false;clear_input();}
        if(anarch_io_error)break;
        if(firing&&anarch_playing())anarch_label("FIRE");
        present();
    }
    anarch_audio_close();anarch_assets_close();
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
    if(anarch_io_error)rb->splash(3*HZ,"Anarch: level read/checksum failed");
    return usb?PLUGIN_USB_CONNECTED:anarch_io_error?PLUGIN_ERROR:PLUGIN_OK;
}
