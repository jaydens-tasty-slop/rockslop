/* PICO-8 drawing subset into an 8 KiB packed framebuffer. MIT. */
#define CE_ROCKBOX 1
#include "../src/emu.h"
uint8_t ce_frame[8192],ce_screen_palette[16];
static uint8_t draw_palette[16];
static int camera_x,camera_y,cursor_x,cursor_y,text_color;
unsigned ce_pixel(unsigned x,unsigned y)
{uint8_t v=ce_frame[y*64+x/2];return x&1?v>>4:v&15;}
static void dot(int x,int y,unsigned c)
{
    x+=camera_x;y+=camera_y;if((unsigned)x>=128||(unsigned)y>=128)return;
    uint8_t *p=ce_frame+y*64+x/2;c=draw_palette[c&15];
    *p=x&1?(*p&15)|(c<<4):(*p&240)|c;
}
void rectfill(int x0,int y0,int x1,int y1,uint8_t c)
{
    if(x0>x1||y0>y1)return;
    x0=max(x0,-camera_x);x1=min(x1,127-camera_x);
    y0=max(y0,-camera_y);y1=min(y1,127-camera_y);
    for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)dot(x,y,c);
}
void circfill(int x,int y,int r,int c)
{for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i)if(i*i+j*j<=r*r)dot(x+i,y+j,c);}
void draw_plus(int x,int y,int c){dot(x,y,c);dot(x+1,y,c);dot(x-1,y,c);dot(x,y+1,c);dot(x,y-1,c);}
void vert_line(int x,int y0,int y1,int c){if(y0>y1){int t=y0;y0=y1;y1=t;}rectfill(x,y0,x,y1,c);}
void camera(){camera(0,0);}
void camera(int x,int y){camera_x=x;camera_y=y;}
void pal(){for(unsigned i=0;i<16;++i)draw_palette[i]=ce_screen_palette[i]=i;}
void pal(int a,int b){draw_palette[a&15]=b&15;}
void pal(int a,int b,int p){if(p)ce_screen_palette[a&15]=b&15;else pal(a,b);}
bool fget(uint8_t tile,uint8_t flag){return tile<128 && flag<8 && (ce_flags[tile]&(1u<<flag));}
void spr(uint8_t n,int x,int y){spr(n,x,y,1,1,false,false);}
void spr(uint8_t n,int x,int y,uint8_t w,uint8_t h,bool fx,bool fy)
{
    for(unsigned j=0;j<(unsigned)h*8;++j)for(unsigned i=0;i<(unsigned)w*8;++i) {
        unsigned xx=fx?w*8-1-i:i,yy=fy?h*8-1-j:j;
        unsigned tile=n+xx/8+16*(yy/8);if(tile>=128)continue;
        unsigned v=ce_tiles[tile*32+(yy%8)*4+(xx%8)/2];v=xx&1?v>>4:v&15;
        if(v)dot(x+i,y+j,v);
    }
}
void map(int cx,int cy,int sx,int sy,uint8_t layer)
{
    for(int y=0;y<16;++y)for(int x=0;x<16;++x) {
        if((unsigned)(cx+x)>=128||(unsigned)(cy+y)>=64)continue;
        unsigned t=ce_map[(cy+y)*128+cx+x];if(t&&fget(t,layer))spr(t,sx+8*x,sy+8*y);
    }
}
void print(char ch,int x,int y,uint8_t c)
{
    unsigned g=(unsigned char)ch;if(g<32||g>=128)g=63;
    for(unsigned j=0;j<5;++j)for(unsigned i=0;i<4;++i)if(ce_font[(g-32)*5+j]&(128>>i))dot(x+i,y+j,c);
}
void print(const char *s){while(*s){if(*s=='\n'){cursor_x=0;cursor_y+=6;}else {print(*s,cursor_x,cursor_y,text_color);cursor_x+=4;}++s;}}
void print(const char *s,int x,int y,uint8_t c){cursor_x=x;cursor_y=y;text_color=c;print(s);}
void print_int(int n,int len)
{
    char b[16];unsigned k=0;unsigned v=n<0?0u-(unsigned)n:(unsigned)n;
    do{b[k++]='0'+v%10;v/=10;}while(v && k<12);
    while(k<(unsigned)len && k<12)b[k++]='0';
    if(n<0)b[k++]='-';
    while(k){print(b[--k],cursor_x,cursor_y,text_color);cursor_x+=4;}
}
void print_int(int n){print_int(n,1);}
void print_int(int n,int x,int y,uint8_t c,int len){cursor_x=x;cursor_y=y;text_color=c;print_int(n,len);}
void print_int(int n,int x,int y,uint8_t c){print_int(n,x,y,c,1);}
void ce_text(int x,int y,const char *s,unsigned c)
{int a=camera_x,b=camera_y;camera();print(s,x,y,c);camera(a,b);}
void ce_clear(unsigned c){camera();pal();rectfill(0,0,127,127,c);}
