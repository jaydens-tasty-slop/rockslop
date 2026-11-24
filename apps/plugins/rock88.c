/* Rock88 -- low-RAM Rockbox adaptation of CubeCoders' ESP 88 / Jet film.
 * Copyright (c) 2026 CubeCoders Limited. MIT license reproduced below.
 * C / Rockbox adaptation by OpenAI Codex at the user's request (2026).
 * Source: https://github.com/CubeCoders/JetExamples/tree/main/esp32-neon-film
 *
 * INSTALL: copy rock88.c to apps/plugins; add rock88.c to SOURCES under
 * #if LCD_WIDTH >= 96 && LCD_HEIGHT >= 64
 * Optionally add rock88,demos to CATEGORIES. Put rock88.r88 in
 * /.rockbox/rocks/demos/rock88.r88 (PLUGIN_DEMOS_DIR), or pass its path as the
 * plugin parameter. The asset file is read-only; no save files are written.
 *
 * This is a 3D cinematic, not a playable racing game or video decoder.
 * All twelve authored shots, original vehicle meshes, scenery and materials
 * are exported from the pinned upstream source into the streamed asset file.
 * Camera and vehicle poses are sampled at 4 Hz and interpolated; authored
 * hard cuts have separate boundary keys. Wheels, rain and lamps animate
 * continuously. Meshes are deduplicated and split into <=256-vertex pieces.
 *
 * Adaptations: reduced indexed textures (10 KiB), 256-color working image,
 * baked face lighting instead of Phong, affine texture mapping, 16-bit inverse
 * depth, simplified rain/glows/water reflection, compact reflowed credits.
 * No previous-field image, PSRAM, FPU, second CPU, audio, heap or reboot.
 * The 116-second film loops after its one-second black hold.
 *
 * RAM: one byte of image + two bytes of depth per logical pixel (61,440 B at
 * 128x160), 16 KiB SD page cache, 5 KiB transform scratch, 10 KiB textures,
 * 4 KiB color lookup, about 21 KiB mesh/scene metadata, small state, code and
 * stack. The Rockbox core
 * framebuffer is additional. Non-XIP code/const tables also occupy SRAM.
 * SD throughput and real YP3 frame rate remain hardware validation items.
 * Playback uses elapsed time, so slow rendering drops frames, not film speed.
 *
 * DISPLAY: generic color/gray/mono; YP3 renders at native 128x160. Larger LCDs
 * use integer scaling, maximum logical dimension 192. ROCK88_SCALE overrides.
 * Define ROCK88_COLOR=0 for luminance on a color LCD; mono selects this
 * automatically. Both modes use the same compact indexed working image.
 * ROCK88_CACHE_PAGES defaults to 4 (4 KiB each), configurable 2..16.
 *
 * CONTROLS: Left/Right tap or hold scrubs; Up/Down previous/next shot.
 * Short Select pauses/resumes. Hold Up or Double Left opens menu; Double Right restarts,
 * Double Up toggles FPS, Double Down toggles rain. First-tap changes are
 * restored on a recognized double tap. Wheel detents scrub, not double tap.
 * Center hold/repeat AND its release have no demo action, including menus.
 * Menu: Up/Down choose, Select/Right confirm, Left/Cancel return.
 * Explicit YP3_PAD mapping plus standard Rockbox pluginlib mappings, remote
 * controls and touchscreen zones. Touch mode/backlight/CPU boost are restored.
 */
#include "plugin.h"
#include "lib/pluginlib_actions.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#if LCD_WIDTH < 96 || LCD_HEIGHT < 64
#error "Rock88 requires a bitmap LCD of at least 96x64"
#endif
#ifndef ROCK88_SCALE
#define ROCK88_SCALE ((MAX(LCD_WIDTH,LCD_HEIGHT)+191)/192)
#endif
#define IW (LCD_WIDTH/ROCK88_SCALE)
#define IH (LCD_HEIGHT/ROCK88_SCALE)
#if ROCK88_SCALE < 1 || IW < 64 || IH < 64 || IW > 192 || IH > 192
#error "Choose ROCK88_SCALE so each logical dimension is 64..192"
#endif
#ifndef ROCK88_COLOR
#ifdef HAVE_LCD_COLOR
#define ROCK88_COLOR 1
#else
#define ROCK88_COLOR 0
#endif
#endif
#ifndef ROCK88_CACHE_PAGES
#define ROCK88_CACHE_PAGES 4
#endif
#if ROCK88_CACHE_PAGES < 2 || ROCK88_CACHE_PAGES > 16
#error "Choose 2..16 cache pages"
#endif
#ifndef PLUGIN_DEMOS_DIR
#define PLUGIN_DEMOS_DIR ROCKBOX_DIR "/rocks/demos"
#endif

#define PACK_SIZE 290344u
#define PACK_CRC 1186500644u
#define TEXTURE_BYTES 10112
#define MATERIAL_COUNT 79
#define MESH_COUNT 495
#define MAX_INSTANCES 487

static const int16_t sin_quarter[257]={
0,101,201,302,402,503,603,704,804,904,1005,1105,1205,1306,1406,1506,1606,1706,1806,1906,2006,2105,2205,2305,2404,2503,2603,2702,2801,2900,2999,3098,3196,3295,3393,3492,3590,3688,3786,3883,3981,4078,4176,4273,4370,4467,4563,4660,4756,4852,4948,5044,5139,5235,5330,5425,5520,5614,5708,5803,5897,5990,6084,6177,6270,6363,6455,6547,6639,6731,6823,6914,7005,7096,7186,7276,7366,7456,7545,7635,7723,7812,7900,7988,8076,8163,8250,8337,8423,8509,8595,8680,8765,8850,8935,9019,9102,9186,9269,9352,9434,9516,9598,9679,9760,9841,9921,10001,10080,10159,10238,10316,10394,10471,10549,10625,10702,10778,10853,10928,11003,11077,11151,11224,11297,11370,11442,11514,11585,11656,11727,11797,11866,11935,12004,12072,12140,12207,12274,12340,12406,12472,12537,12601,12665,12729,12792,12854,12916,12978,13039,13100,13160,13219,13279,13337,13395,13453,13510,13567,13623,13678,13733,13788,13842,13896,13949,14001,14053,14104,14155,14206,14256,14305,14354,14402,14449,14497,14543,14589,14635,14680,14724,14768,14811,14854,14896,14937,14978,15019,15059,15098,15137,15175,15213,15250,15286,15322,15357,15392,15426,15460,15493,15525,15557,15588,15619,15649,15679,15707,15736,15763,15791,15817,15843,15868,15893,15917,15941,15964,15986,16008,16029,16049,16069,16088,16107,16125,16143,16160,16176,16192,16207,16221,16235,16248,16261,16273,16284,16295,16305,16315,16324,16332,16340,16347,16353,16359,16364,16369,16373,16376,16379,16381,16383,16384,16384
};

static const char glyph_chars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:/>+-; ";
static const uint16_t glyph_bits[]={0x2bed,0x6bae,0x3923,0x6b6e,0x79a7,0x79a4,0x396b,0x5bed,0x7497,0x126a,0x5bad,0x4927,0x5fed,0x5ffd,0x2b6a,0x6ba4,0x2b59,0x6bad,0x388e,0x7492,0x5b6f,0x5b6a,0x5bfd,0x5aad,0x5a92,0x72a7,0x7b6f,0x2c97,0x62a7,0x628e,0x5bc9,0x798e,0x39aa,0x7292,0x2aaa,0x2ace,0x2,0x410,0x12a4,0x4454,0x5d0,0x1c0,0x414,0x0,0};



#define Q 16384
#define PAGE_SIZE 4096u
#define KEY_SIZE 164u
#define NEAR_Z 40
struct vec { int32_t x,y,z,u,v; };
struct matrix { int m[9]; };
struct actor { int active,x,y,z,yaw,pitch,roll,hover; };
struct scene_info { uint32_t instances,keys,start,duration;unsigned count,key_count; };
static uint8_t frame[IH][IW];
#if PLUGIN_BUFFER_SIZE <= 0x20000
/* Small plugin arena (e.g. YP3): the inverse-depth buffer lives in the audio
 * buffer, which stops playback while the demo runs. */
#define DEPTH_IN_AUDIO_BUFFER 1
static uint16_t (*depth)[IW];
#else
static uint16_t depth[IH][IW];
#endif
static uint16_t palette[256];
static uint8_t color_lut[4096],texture_pixels[TEXTURE_BYTES],materials[MATERIAL_COUNT][4];
static uint8_t model_table[MESH_COUNT][24],instance_table[MAX_INSTANCES][20];
static uint8_t current_keys[KEY_SIZE*2];
static bool keys_valid;
static struct { uint32_t offset;unsigned w,h; } textures[17];
static struct { uint32_t tag,used;uint8_t data[PAGE_SIZE]; } cache[ROCK88_CACHE_PAGES];
static struct vec transformed[256];
static struct scene_info scene_info;
static struct actor actors[9];
static struct matrix view;
static uint32_t model_offset,scene_offset,clock_ms,cache_clock,io_bytes,frames,fps_tenths;
static int asset_fd=-1,cut=-1,cam_x,cam_y,cam_z,cam_pitch,cam_yaw,focal,travel,rear_extent;
static int local_ms,wheel_angle,triangles;
static bool io_error,usb,paused,hud=false,rain=true,reflections=true;
static unsigned speed_percent=100;
static int clampi(int n,int a,int b){return n<a?a:n>b?b:n;}
static unsigned le16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static int si16(const uint8_t *p){unsigned n=le16(p);return n<32768?(int)n:(int)n-65536;}
static uint32_t le32(const uint8_t *p){return le16(p)|((uint32_t)le16(p+2)<<16);}
static int isin(int a){unsigned q=(unsigned)a&1023,i=q&255;int n=sin_quarter[q&256?256-i:i];return q&512?-n:n;}
static int icos(int a){return isin(a+256);}
static int red(uint16_t c){return ((c>>11)&31)*255/31;}
static int green(uint16_t c){return ((c>>5)&63)*255/63;}
static int blue(uint16_t c){return (c&31)*255/31;}
static uint8_t ink_rgb(int r,int g,int b){return color_lut[(clampi(r,0,255)>>4)*256+(clampi(g,0,255)>>4)*16+(clampi(b,0,255)>>4)];}
static uint8_t ink(uint16_t c){return ink_rgb(red(c),green(c),blue(c));}
static uint8_t mixed(uint8_t a,uint8_t b,int alpha,bool add){
 uint16_t ca=palette[a],cb=palette[b];int keep=add?255:255-alpha;
 return ink_rgb((red(ca)*alpha+red(cb)*keep)/255,(green(ca)*alpha+green(cb)*keep)/255,(blue(ca)*alpha+blue(cb)*keep)/255);
}
static void dot(int x,int y,uint8_t c){if((unsigned)x<IW&&(unsigned)y<IH)frame[y][x]=c;}
static bool read_bytes(uint32_t offset,void *target,unsigned count){
 uint8_t *out=target;
 if(io_error || offset>PACK_SIZE || count>PACK_SIZE-offset){io_error=true;rb->memset(out,0,count);return false;}
 while(count){
  uint32_t page=offset/PAGE_SIZE;unsigned slot=0;
  for(;slot<ROCK88_CACHE_PAGES;++slot)if(cache[slot].tag==page)break;
  if(slot==ROCK88_CACHE_PAGES){
   slot=0;for(unsigned i=1;i<ROCK88_CACHE_PAGES;++i)if(cache[i].used<cache[slot].used)slot=i;
   unsigned bytes=MIN(PAGE_SIZE,PACK_SIZE-page*PAGE_SIZE);
   if(rb->lseek(asset_fd,page*PAGE_SIZE,SEEK_SET)<0){io_error=true;rb->memset(out,0,count);return false;}
   unsigned done=0;while(done<bytes){int n=rb->read(asset_fd,cache[slot].data+done,bytes-done);if(n<=0){io_error=true;rb->memset(out,0,count);return false;}done+=n;}
   io_bytes+=bytes;cache[slot].tag=page;
  }
  cache[slot].used=++cache_clock;
  unsigned start=offset%PAGE_SIZE,n=MIN(count,PAGE_SIZE-start);
  rb->memcpy(out,cache[slot].data+start,n);offset+=n;out+=n;count-=n;
 }
 return true;
}
static bool open_assets(const char *path){
 io_error=false;cache_clock=io_bytes=0;
 for(unsigned i=0;i<ROCK88_CACHE_PAGES;++i){cache[i].tag=UINT32_MAX;cache[i].used=0;}
 asset_fd=rb->open(path,O_RDONLY);if(asset_fd<0)return false;
 uint8_t h[64];
 if(rb->lseek(asset_fd,0,SEEK_END)!=PACK_SIZE || !read_bytes(0,h,64) || rb->memcmp(h,"ROCK88\0\1",8) || le32(h+8)!=PACK_SIZE || le32(h+12)!=PACK_CRC || le32(h+40)!=MATERIAL_COUNT || le32(h+48)!=MESH_COUNT || le32(h+56)!=12){io_error=true;return false;}
 /* Header offsets are constrained to the generated pack by range checks. */
 uint8_t p[512];uint32_t crc=UINT32_MAX;
 for(uint32_t offset=64;offset<PACK_SIZE;offset+=sizeof(p)){
  unsigned n=MIN(sizeof(p),PACK_SIZE-offset);if(!read_bytes(offset,p,n))return false;
  for(unsigned i=0;i<n;++i){crc^=p[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&-(int32_t)(crc&1));}
  rb->yield();
 }
 if((crc^UINT32_MAX)!=PACK_CRC){io_error=true;return false;}
 if(!read_bytes(le32(h+16),p,512))return false;
 for(int i=0;i<256;++i)palette[i]=le16(p+i*2);
 read_bytes(le32(h+20),color_lut,sizeof(color_lut));
 if(le32(h+28)!=TEXTURE_BYTES){io_error=true;return false;}
 read_bytes(le32(h+24),texture_pixels,sizeof(texture_pixels));
 for(int i=0;i<17;++i){read_bytes(le32(h+32)+i*8,p,8);textures[i].offset=le32(p);textures[i].w=le16(p+4);textures[i].h=le16(p+6);
  if(!textures[i].w || !textures[i].h || textures[i].w>256 || textures[i].h>128 || textures[i].offset>TEXTURE_BYTES || textures[i].w*textures[i].h>TEXTURE_BYTES-textures[i].offset)io_error=true;
 }
 read_bytes(le32(h+36),materials,sizeof(materials));
 for(int i=0;i<MATERIAL_COUNT;++i)if((materials[i][1]>17&&materials[i][1]!=255)||materials[i][3]>6)io_error=true;
 model_offset=le32(h+44);scene_offset=le32(h+52);read_bytes(model_offset,model_table,sizeof(model_table));cut=-1;keys_valid=false;return !io_error;
}
static void identity(struct matrix *m){rb->memset(m,0,sizeof(*m));m->m[0]=m->m[4]=m->m[8]=Q;}
static void rotate(struct matrix *m,int axis,int angle){
 int c=icos(angle),s=isin(angle),a=axis==0?1:0,b=axis==2?1:2;
 if(axis==1)s=-s;
 for(int i=0;i<3;++i){int x=m->m[a*3+i],y=m->m[b*3+i];m->m[a*3+i]=(x*c-y*s)/Q;m->m[b*3+i]=(x*s+y*c)/Q;}
}
static struct vec transform(const struct matrix *m,struct vec v){
 struct vec r=v;
 r.x=((int64_t)m->m[0]*v.x+(int64_t)m->m[1]*v.y+(int64_t)m->m[2]*v.z)/Q;
 r.y=((int64_t)m->m[3]*v.x+(int64_t)m->m[4]*v.y+(int64_t)m->m[5]*v.z)/Q;
 r.z=((int64_t)m->m[6]*v.x+(int64_t)m->m[7]*v.y+(int64_t)m->m[8]*v.z)/Q;return r;
}
static int interpolate(int a,int b,int n,int d){return a+(int)((int64_t)(b-a)*n/d);}
static int interp_angle(int a,int b,int n,int d){return a+(((b-a+512)&1023)-512)*n/d;}
static void seek_film(uint32_t time){
 clock_ms=time%117000u;int next=0;
 static const unsigned starts[]={0,12000,22000,29000,38000,47000,53000,64000,78000,88000,93000,101000,116000};
 while(next<11&&clock_ms>=starts[next+1])++next;
 if(cut!=next){
  uint8_t s[20];read_bytes(scene_offset+next*20,s,20);cut=next;
  scene_info.instances=le32(s);scene_info.keys=le32(s+4);scene_info.count=le16(s+8);scene_info.key_count=le16(s+10);scene_info.start=le32(s+12);scene_info.duration=le32(s+16);
  if(scene_info.count>MAX_INSTANCES || scene_info.key_count<2 || scene_info.key_count>128 || scene_info.start!=starts[next] || scene_info.duration!=starts[next+1]-starts[next]){io_error=true;return;}
  read_bytes(scene_info.instances,instance_table,scene_info.count*20);keys_valid=false;
 }
 local_ms=clampi((int)(clock_ms-scene_info.start),0,scene_info.duration-1);
 uint8_t *key=current_keys;
 if(!keys_valid || local_ms<(int)le16(key) || local_ms>=(int)le16(key+KEY_SIZE)){
  unsigned lo=0,hi=scene_info.key_count-1;uint8_t stamp[2];
  while(lo+1<hi){unsigned mid=(lo+hi)/2;read_bytes(scene_info.keys+mid*KEY_SIZE,stamp,2);if(le16(stamp)<=(unsigned)local_ms)lo=mid;else hi=mid;}
  if(!read_bytes(scene_info.keys+lo*KEY_SIZE,key,sizeof(current_keys)))return;
  keys_valid=true;
 }
 unsigned t0=le16(key),t1=le16(key+KEY_SIZE);
 if(t1<=t0 || t1>scene_info.duration || local_ms<(int)t0){io_error=true;return;}
 int n=MIN((unsigned)local_ms-t0,t1-t0),d=t1-t0;
 cam_x=interpolate(si16(key+2),si16(key+KEY_SIZE+2),n,d);
 cam_y=interpolate(si16(key+4),si16(key+KEY_SIZE+4),n,d);
 cam_z=interpolate(si16(key+6),si16(key+KEY_SIZE+6),n,d);
 cam_pitch=interp_angle(si16(key+8),si16(key+KEY_SIZE+8),n,d);
 cam_yaw=interp_angle(si16(key+10),si16(key+KEY_SIZE+10),n,d);
 focal=IW*interpolate(si16(key+12),si16(key+KEY_SIZE+12),n,d)/256;
 if(focal<16 || focal>IW*4){io_error=true;return;}
 rear_extent=le16(key+14);travel=interpolate((int32_t)le32(key+16),(int32_t)le32(key+KEY_SIZE+16),n,d);
 for(int i=0;i<9;++i){const uint8_t *a=key+20+i*16,*b=a+KEY_SIZE;struct actor *o=&actors[i];
  o->active=si16(a);o->x=interpolate(si16(a+2),si16(b+2),n,d);o->y=interpolate(si16(a+4),si16(b+4),n,d);o->z=interpolate(si16(a+6),si16(b+6),n,d);
  o->yaw=interp_angle(si16(a+8),si16(b+8),n,d);o->pitch=interp_angle(si16(a+10),si16(b+10),n,d);o->roll=interp_angle(si16(a+12),si16(b+12),n,d);o->hover=interpolate(si16(a+14),si16(b+14),n,d);
 }
 /* Rolling tyre speed is independent of the sampled pose tracks. */
 int distance=local_ms*(cut<6?3964:4983)/1000;
 if(cut==8){int b=clampi(local_ms-2000,0,6000);distance=4983*(MIN(local_ms,2000)+b-b*b/12000)/1000;}
 if(cut>=9)distance=24915;
 wheel_angle=(int)((int64_t)distance*1024/276)&1023;
 identity(&view);rotate(&view,1,cam_yaw);rotate(&view,0,cam_pitch);
}
static int road_offset(int center){int n=travel-center+11520-rear_extent,q=n/11520;if(n<0&&n%11520)--q;return -travel+q*11520;}
static int plane(struct vec v,int p){
 switch(p){case 0:return v.z-NEAR_Z;case 1:return v.x*focal+v.z*(IW/2);case 2:return -v.x*focal+v.z*(IW-IW/2);case 3:return v.y*focal+v.z*(IH/2);default:return -v.y*focal+v.z*(IH-IH/2);}
}
static struct vec crossing(struct vec a,struct vec b,int da,int db){
 struct vec r;int64_t den=(int64_t)da-db;
 r.x=a.x+(b.x-a.x)*(int64_t)da/den;r.y=a.y+(b.y-a.y)*(int64_t)da/den;r.z=a.z+(b.z-a.z)*(int64_t)da/den;
 r.u=a.u+(b.u-a.u)*(int64_t)da/den;r.v=a.v+(b.v-a.v)*(int64_t)da/den;return r;
}
static uint8_t sample_texture(int tex,int u,int v){
 u=clampi(u,0,1023);v=clampi(v,0,1023);
 if(tex==17){
  static const uint8_t masks[]={0x3f,6,0x5b,0x4f,0x66,0x6d,0x7d,7,0x7f,0x6f};
  static const uint8_t rects[7][4]={{5,46,32,5},{37,26,5,24},{37,3,5,22},{5,0,32,5},{0,3,5,22},{0,26,5,24},{5,23,32,5}};
  int x=u/8-8,y=v/16-6,d=x>=56;if(d)x-=56;
  int mph=cut==5?70+clampi((local_ms*18+2500)/5000,0,18):70;
  for(int i=0;i<7;++i)if(x>=rects[i][0]&&x<rects[i][0]+rects[i][2]&&y>=rects[i][1]&&y<rects[i][1]+rects[i][3])return masks[d?mph%10:mph/10]&(1<<i)?ink_rgb(255,120,70):ink_rgb(36,27,33);
  return ink_rgb(9,20,32);
 }
 return texture_pixels[textures[tex].offset+(v*textures[tex].h/1024)*textures[tex].w+u*textures[tex].w/1024];
}
/* Clipped, subpixel edge functions and inverse-depth interpolation. Fixed
 * 64-bit setup/addition avoids overflow for very thin clipped triangles. */
static void raster(struct vec a,struct vec b,struct vec c,unsigned mat,int alpha_override){
 struct vec v[3]={a,b,c};int x[3],y[3],iz[3];
 for(int i=0;i<3;++i){x[i]=IW*8+v[i].x*focal*16/MAX(NEAR_Z,v[i].z);y[i]=IH*8-v[i].y*focal*16/MAX(NEAR_Z,v[i].z);iz[i]=2097152/MAX(NEAR_Z,v[i].z);}
 int area=(x[1]-x[0])*(y[2]-y[0])-(y[1]-y[0])*(x[2]-x[0]);
 if(!area)return;
 if(area<0){struct vec t=v[1];v[1]=v[2];v[2]=t;int q=x[1];x[1]=x[2];x[2]=q;q=y[1];y[1]=y[2];y[2]=q;q=iz[1];iz[1]=iz[2];iz[2]=q;area=-area;}
 int xmin=clampi((MIN(x[0],MIN(x[1],x[2]))+7)/16,0,IW-1),xmax=clampi(MAX(x[0],MAX(x[1],x[2]))/16,0,IW-1);
 int ymin=clampi((MIN(y[0],MIN(y[1],y[2]))+7)/16,0,IH-1),ymax=clampi(MAX(y[0],MAX(y[1],y[2]))/16,0,IH-1);
 int dx[3],dy[3],w[3];
 for(int i=0;i<3;++i){int j=(i+1)%3,k=(i+2)%3;dx[i]=-(y[k]-y[j])*16;dy[i]=(x[k]-x[j])*16;w[i]=(x[k]-x[j])*(ymin*16+8-y[j])-(y[k]-y[j])*(xmin*16+8-x[j]);}
 int64_t zz=0,uu=0,vv=0,zx=0,ux=0,vx=0,zy=0,uy=0,vy=0;
 for(int i=0;i<3;++i){zz+=(int64_t)w[i]*iz[i]*256;uu+=(int64_t)w[i]*v[i].u*256;vv+=(int64_t)w[i]*v[i].v*256;zx+=(int64_t)dx[i]*iz[i]*256;ux+=(int64_t)dx[i]*v[i].u*256;vx+=(int64_t)dx[i]*v[i].v*256;zy+=(int64_t)dy[i]*iz[i]*256;uy+=(int64_t)dy[i]*v[i].u*256;vy+=(int64_t)dy[i]*v[i].v*256;}
 zz/=area;uu/=area;vv/=area;zx/=area;ux/=area;vx/=area;zy/=area;uy/=area;vy/=area;
 const uint8_t *m=materials[mat];int alpha=alpha_override>=0?alpha_override:m[2];bool add=m[3]==6;
 ++triangles;
 for(int py=ymin;py<=ymax;++py){
  int e0=w[0],e1=w[1],e2=w[2];int64_t z=zz,u=uu,vv_=vv;
  for(int px=xmin;px<=xmax;++px){
   int z16=z/256;
   if(e0>=0&&e1>=0&&e2>=0&&z16>0&&z16>=depth[py][px]){
    uint8_t color=m[1]==255?m[0]:sample_texture(m[1],u/256,vv_/256);
    if(add || alpha<255)frame[py][px]=mixed(color,frame[py][px],alpha,add);
    else {frame[py][px]=color;depth[py][px]=MIN(65535,z16);}
   }
   e0+=dx[0];e1+=dx[1];e2+=dx[2];z+=zx;u+=ux;vv_+=vx;
  }
  for(int i=0;i<3;++i)w[i]+=dy[i];
  zz+=zy;uu+=uy;vv+=vy;
 }
}
static void clipped_triangle(struct vec a,struct vec b,struct vec c,unsigned m,int alpha){
 struct vec work[2][10];work[0][0]=a;work[0][1]=b;work[0][2]=c;int count=3,src=0;
 for(int p=0;p<5;++p){int out=0;struct vec last=work[src][count-1];int dl=plane(last,p);
  for(int i=0;i<count;++i){struct vec now=work[src][i];int dn=plane(now,p);
   if((dn>=0)!=(dl>=0)){if(out>=10){io_error=true;return;}work[1-src][out++]=crossing(last,now,dl,dn);}
   if(dn>=0){if(out>=10){io_error=true;return;}work[1-src][out++]=now;}last=now;dl=dn;
  }
  if(out<3)return;
  count=out;src=1-src;
 }
 for(int i=1;i<count-1;++i)raster(work[src][0],work[src][i],work[src][i+1],m,alpha);
}
static void draw_instance(unsigned index,int pass){
 const uint8_t *record=instance_table[index];
 unsigned mesh=le16(record),bind=record[2],kind=record[3],flags=record[18];
 if(mesh>=MESH_COUNT || (bind>5 && (bind<10||bind>18)) || flags>3){io_error=true;return;}
 struct vec pos={si16(record+4),si16(record+6),si16(record+8),0,0};
 int rx=si16(record+10),ry=si16(record+12),rz=si16(record+14),center=si16(record+16),alpha=-1;
 struct matrix model;identity(&model);
 if(bind>=10){
  const struct actor *a=&actors[bind-10];if(!a->active)return;
  if((kind==5||kind==6) && ((local_ms*8/1000)%2)!=(kind==5?0:1))return;
  bool wheel=kind==2||kind==3,suspension=a->hover==0&&(a->pitch||a->roll);
  if(wheel){pos.x+=(kind==2?-28:28)*a->hover/256;pos.y+=10*a->hover/256;}
  if(kind==11){if(a->y>=500)return;pos=(struct vec){a->x,0,a->z,0,0};rotate(&model,1,a->yaw);}
  else {
   struct matrix body;identity(&body);
   if(!wheel||!suspension){rotate(&body,2,a->roll);rotate(&body,0,-a->pitch);}
   rotate(&body,1,a->yaw);
   if(suspension&&!wheel)pos.y-=70;
   pos=transform(&body,pos);pos.x+=a->x;pos.y+=a->y+(suspension&&!wheel?70:0);pos.z+=a->z;
   if(wheel){rotate(&model,0,wheel_angle);if(!suspension){rotate(&model,2,(kind==2?256:-256)*a->hover/256+a->roll);rotate(&model,0,-a->pitch);}rotate(&model,1,a->yaw);}
   else model=body;
  }
  if(wheel)alpha=a->hover*190/256+(cut>=9?65:0);
 } else {
  if(bind==1){int shift=road_offset(center);if(cut==6)pos.x+=shift;else pos.z+=shift;}
  if(bind==2)pos.x-=travel;
  if(bind==3){rz=36*isin(local_ms*134/1000+kind*163)/360;ry=48*isin(local_ms*93/1000+kind*163)/360;rz=rz*1024/Q;ry=ry*1024/Q;}
  if(bind==4)pos.y=1000+kind*110+24*isin(local_ms*114/1000+kind*163)/Q;
  if(flags&2)ry=-cam_yaw;
  rotate(&model,0,rx);rotate(&model,1,ry);rotate(&model,2,rz);
 }
 record=model_table[mesh];
 uint32_t vo=le32(record),to=le32(record+4);unsigned nv=le16(record+8),nt=le16(record+10),passes=le16(record+20);
 if(nv>256 || !nv || nt>2048){io_error=true;return;}
 if(!(passes&(1u<<pass)))return;
 if(pass<2 && !!(flags&1)!=(pass==0))return;
 struct vec mid={si16(record+12),si16(record+14),si16(record+16),0,0};int radius=le16(record+18);
 mid=transform(&model,mid);mid.x+=pos.x-cam_x;mid.y+=pos.y-cam_y;mid.z+=pos.z-cam_z;mid=transform(&view,mid);
 int far_z=(cut==0||cut>=10)?15000:9000;
 if(mid.z+radius<NEAR_Z || mid.z-radius>far_z)return;
 if(mid.x*focal-(mid.z*IW/2)>radius*(focal+IW/2) || -mid.x*focal-mid.z*IW/2>radius*(focal+IW/2) || mid.y*focal-mid.z*IH/2>radius*(focal+IH/2) || -mid.y*focal-mid.z*IH/2>radius*(focal+IH/2))return;
 for(unsigned i=0;i<nv;++i){uint8_t v[10];if(!read_bytes(vo+i*10,v,10))return;struct vec p={si16(v),si16(v+2),si16(v+4),si16(v+6),si16(v+8)};
  p=transform(&model,p);p.x+=pos.x-cam_x;p.y+=pos.y-cam_y;p.z+=pos.z-cam_z;transformed[i]=transform(&view,p);
 }
 for(unsigned i=0;i<nt;++i){uint8_t tr[8];if(!read_bytes(to+i*8,tr,8))return;
  unsigned a=le16(tr),b=le16(tr+2),c=le16(tr+4),m=le16(tr+6);
  if(a>=nv||b>=nv||c>=nv||m>=MATERIAL_COUNT){io_error=true;return;}
  int shader=materials[m][3],opacity=materials[m][2];if(shader==5)continue;
  bool transparent=shader==6||opacity<255;if(transparent!=(pass==2))continue;
  if(cut==1 && (flags&1) && !transparent && transformed[a].z>0 && nv==4){
   /* The opaque base road must not hide the authored mirrored facades. */
   if(pos.y==0)continue;
  }
  int al=(shader==6&&bind>=10)?alpha:-1;
  if(al==0)continue;
  uint8_t original=materials[m][0];
  if(bind==5){int cursor=local_ms*9/1000%26;if(cursor>13)cursor=26-cursor;materials[m][0]=kind+1>=(unsigned)cursor&&kind<=(unsigned)cursor+1?ink_rgb(255,64,60):ink_rgb(74,23,39);}
  clipped_triangle(transformed[a],transformed[b],transformed[c],m,al);
  materials[m][0]=original;
 }
}
static uint32_t weather_hash(uint32_t v){v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;return v^(v>>16);}
static void draw_weather(void){
 if(!rain || cut==4 || cut==5)return;
 uint8_t color=ink_rgb(84,115,143);
 for(int i=0;i<IW*IH/360;++i){uint32_t phase=local_ms*45u/10+i*618u,seed=i*0x9e3779b9u+(phase/1000)*0x85ebca6bu;
  int x=weather_hash(seed)%IW,y=(weather_hash(seed^0x27d4eb2fu)+phase%1000*IH/1000)%IH;
  for(int j=0;j<3;++j)dot(x+j/2,y+j,color);
 }
}
static void draw_background(void){
 rb->memset(depth,0,sizeof(uint16_t)*IH*IW);
 for(int y=0;y<IH;++y){uint8_t color=ink_rgb(7+y*10/IH,10+y*20/IH,25+y*35/IH);rb->memset(frame[y],color,IW);}
}
static void water_reflection(void){
 if(!reflections || (cut!=0&&cut!=11))return;
 struct vec shore={0,-cam_y,1900-cam_z,0,0};shore.x=-cam_x;shore=transform(&view,shore);
 if(shore.z<=NEAR_Z)return;
 int waterline=clampi(IH/2-shore.y*focal/shore.z,0,IH);
 uint8_t tint=ink_rgb(9,33,56);
 for(int y=waterline;y<IH;++y){int source=waterline-1-(y-waterline),wave=isin(local_ms/8+y*19)*2/Q;
  for(int x=0;x<IW;++x)frame[y][x]=source>=0?mixed(frame[source][clampi(x+wave,0,IW-1)],tint,MAX(25,145-(y-waterline)*150/IH),false):tint;
 }
}
static void hero_glows(void){
 const struct actor *a=&actors[0];if(!a->active)return;
 struct matrix body;identity(&body);rotate(&body,2,a->roll);rotate(&body,0,-a->pitch);rotate(&body,1,a->yaw);
 struct vec forward=transform(&body,(struct vec){0,0,1024,0,0});
 bool front=(int64_t)forward.x*(cam_x-a->x)+(int64_t)forward.y*(cam_y-a->y)+(int64_t)forward.z*(cam_z-a->z)>0;
 for(int i=0;i<6;++i){if(i<2&&!front)continue;if(i>=2&&!a->hover)continue;
  struct vec p=i<2?(struct vec){i?78:-78,57,290,0,0}:(struct vec){i%2?150:-150,35,i<4?-175:175,0,0};
  p=transform(&body,p);p.x+=a->x-cam_x;p.y+=a->y-cam_y;p.z+=a->z-cam_z;p=transform(&view,p);if(p.z<NEAR_Z)continue;
  int x=IW/2+p.x*focal/p.z,y=IH/2-p.y*focal/p.z;
  if((unsigned)x>=IW||(unsigned)y>=IH || 2097152/p.z+100<depth[y][x])continue;
  int radius=clampi(30*focal/p.z,1,4),strength=i<2?140:a->hover*160/256;
  uint8_t color=i<2?ink_rgb(174,236,255):ink_rgb(40,235,255);
  for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){int d=dx*dx+dy*dy;if(d>radius*radius||(unsigned)(x+dx)>=IW||(unsigned)(y+dy)>=IH)continue;frame[y+dy][x+dx]=mixed(color,frame[y+dy][x+dx],strength*(radius*radius+1-d)/(radius*radius+1),true);}
 }
}
static void render(void){
 triangles=0;draw_background();if(io_error)return;
 for(int pass=0;pass<3;++pass)for(unsigned i=0;i<scene_info.count && !io_error;++i){draw_instance(i,pass);if((i&15)==0)rb->yield();}
 water_reflection();hero_glows();draw_weather();
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


static void text_at(int x,int y,const char *s,int scale,uint16_t color){
 uint8_t c=ink(color);
 for(;*s;++s,x+=4*scale){char ch=*s;if(ch>='a'&&ch<='z')ch-=32;int n=0;while(glyph_chars[n]&&glyph_chars[n]!=ch)++n;
  uint16_t bits=glyph_bits[n];for(int row=0;row<5;++row)for(int col=0;col<3;++col)if(bits&(1u<<(14-row*3-col)))for(int j=0;j<scale;++j)for(int i=0;i<scale;++i)dot(x+col*scale+i,y+row*scale+j,c);
 }
}
static void centered(int y,const char *s,int scale){text_at((IW-(int)rb->strlen(s)*4*scale)/2,y,s,scale,0xffff);}
static void present(void){
#ifdef HAVE_LCD_COLOR
 static fb_data row[IW*ROCK88_SCALE];
#elif LCD_DEPTH == 1
 static uint8_t row[IW*ROCK88_SCALE];
 static const uint8_t bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
#endif
 const int ox=(LCD_WIDTH-IW*ROCK88_SCALE)/2,oy=(LCD_HEIGHT-IH*ROCK88_SCALE)/2;
 for(int y=0;y<IH;++y){for(int x=0;x<IW;++x){uint16_t c=palette[frame[y][x]];int r=red(c),g=green(c),b=blue(c);
#if !ROCK88_COLOR || !defined(HAVE_LCD_COLOR)
 r=(r*77+g*150+b*29)>>8;
#ifdef HAVE_LCD_COLOR
 g=b=r;
#endif
#endif
#ifdef HAVE_LCD_COLOR
 fb_data out=FB_RGBPACK(r,g,b);for(int i=0;i<ROCK88_SCALE;++i)row[x*ROCK88_SCALE+i]=out;
#elif LCD_DEPTH == 1
 uint8_t out=r>bayer[y&3][x&3]*16+8?0:1;for(int i=0;i<ROCK88_SCALE;++i)row[x*ROCK88_SCALE+i]=out;
#else
 rb->lcd_set_foreground(LCD_BRIGHTNESS(r));rb->lcd_fillrect(ox+x*ROCK88_SCALE,oy+y*ROCK88_SCALE,ROCK88_SCALE,ROCK88_SCALE);
#endif
 }
 for(int j=0;j<ROCK88_SCALE;++j){
#ifdef HAVE_LCD_COLOR
 rb->lcd_bitmap(row,ox,oy+y*ROCK88_SCALE+j,IW*ROCK88_SCALE,1);
#elif LCD_DEPTH == 1
 rb->lcd_mono_bitmap(row,ox,oy+y*ROCK88_SCALE+j,IW*ROCK88_SCALE,1);
#endif
 }
 if((y&15)==0)rb->yield();}rb->lcd_update();
}
static void titles_and_fade(void){
 if(cut==11 && local_ms>=2000){
  for(int y=MAX(0,IH/2-30);y<MIN(IH,IH/2+34);++y)for(int x=4;x<IW-4;++x)frame[y][x]=mixed(ink_rgb(0,0,0),frame[y][x],155,false);
  centered(IH/2-26,"ROCK88",2);centered(IH/2-10,"ESP 88 BY CUBECODERS",1);
  centered(IH/2+1,"ORIGINAL JET FILM",1);centered(IH/2+12,"ROCKBOX / CPU0 / C",1);centered(IH/2+23,"116 SECONDS. 12 SHOTS.",1);
 }
 int fade=0;
 if(cut==0)fade=255-clampi(local_ms*255/3000,0,255);
 if(cut==10)fade=clampi((local_ms-5000)*255/3000,0,255);
 if(cut==11)fade=MAX(255-clampi(local_ms*255/2000,0,255),clampi((local_ms-12000)*255/3000,0,255));
 if(clock_ms>=116000)fade=255;
 if(fade){uint8_t map[256];for(int i=0;i<256;++i)map[i]=mixed(ink_rgb(0,0,0),i,fade,false);for(int y=0;y<IH;++y)for(int x=0;x<IW;++x)frame[y][x]=map[frame[y][x]];}
 if(hud){char s[36];rb->snprintf(s,sizeof(s),"%u.%u FPS %d/12",(unsigned)(fps_tenths/10),(unsigned)(fps_tenths%10),cut+1);text_at(2,2,s,1,0xffff);}
 if(paused)text_at(2,IH-7,"PAUSED",1,0xffff);
}
/* Generic action translation and explicit YP3 key context are above. */
#ifdef HAVE_BLUETOOTH_HID
/* Game controller: A pauses, B/Options/PS leave (I_MENU) */
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
#ifdef HAVE_BLUETOOTH_HID
static const struct button_mapping *contexts[]={pad_context, PORT_MAIN_CONTEXT,
#else
static const struct button_mapping *contexts[]={PORT_MAIN_CONTEXT,
#endif
#ifdef HAVE_REMOTE_LCD
 pla_remote_ctx,
#endif
};
enum {I_NONE,I_UP,I_DOWN,I_LEFT,I_RIGHT,I_SELECT,I_MENU,I_DOUBLE_UP,I_DOUBLE_DOWN,I_DOUBLE_LEFT,I_DOUBLE_RIGHT,I_REPEAT=256,I_WHEEL=512};
static bool select_armed,select_long;
static unsigned long select_since;
static int suppress;
static int read_action(int timeout){
#ifdef PORT_CENTER_BUTTON
 /* Common pause gesture: hold Select+Up for 0.5 s; the release is no click. */
 static unsigned long chord_since;static bool chord_done;
 if((rb->button_status()&(PORT_CENTER_BUTTON|BUTTON_UP))==(PORT_CENTER_BUTTON|BUTTON_UP)){
  if(!chord_since)chord_since=*rb->current_tick|1;
  else if(!chord_done&&*rb->current_tick-chord_since>=(unsigned long)(HZ/2)){chord_done=true;select_armed=false;select_long=true;suppress=I_UP;return I_MENU;}
 }else{chord_since=0;chord_done=false;}
#endif
 int a=pluginlib_getaction(timeout,contexts,ARRAYLEN(contexts));
 if(rb->default_event_handler(a)==SYS_USB_CONNECTED){usb=true;return I_NONE;}
 if(a==PLA_SELECT){select_armed=true;select_long=false;select_since=*rb->current_tick;suppress=0;return I_NONE;}
 if(a==PLA_SELECT_REPEAT){select_armed=false;select_long=true;return I_NONE;}
 if(a==PLA_SELECT_REL){bool click=select_armed&&!select_long&&(unsigned long)*rb->current_tick-select_since<(unsigned)(HZ*3/5);select_armed=false;return click?I_SELECT:I_NONE;}
 if(a==PLA_CANCEL||a==PLA_EXIT){select_armed=false;return I_MENU;}
 int d=0,repeat=0;
 switch(a){case PLA_UP:d=I_UP;break;case PLA_DOWN:d=I_DOWN;break;case PLA_LEFT:d=I_LEFT;break;case PLA_RIGHT:d=I_RIGHT;break;
 case PLA_UP_REPEAT:d=I_UP;repeat=I_REPEAT;break;case PLA_DOWN_REPEAT:d=I_DOWN;repeat=I_REPEAT;break;case PLA_LEFT_REPEAT:d=I_LEFT;repeat=I_REPEAT;break;case PLA_RIGHT_REPEAT:d=I_RIGHT;repeat=I_REPEAT;break;
#ifdef HAVE_SCROLLWHEEL
 case PLA_SCROLL_BACK:case PLA_SCROLL_BACK_REPEAT:d=I_UP;repeat=I_REPEAT|I_WHEEL;break;
 case PLA_SCROLL_FWD:case PLA_SCROLL_FWD_REPEAT:d=I_DOWN;repeat=I_REPEAT|I_WHEEL;break;
#endif
 }
 if(!d)return I_NONE;
 select_armed=false;if(repeat&&!(repeat&I_WHEEL)&&suppress==d)return I_NONE;if(!repeat)suppress=0;return d|repeat;
}
static struct {int direction,phase;uint32_t time;unsigned long when;} gesture;
static int gesture_action(int a){
 int d=a&~(I_REPEAT|I_WHEEL);unsigned long now=*rb->current_tick;
 if(a&I_WHEEL){gesture.phase=0;return d==I_UP?I_LEFT:I_RIGHT;}
 if(d<I_UP||d>I_RIGHT){if(a)gesture.phase=0;return a;}
 if(a&I_REPEAT){if(d==I_UP&&gesture.phase==1&&gesture.direction==d)seek_film(gesture.time);gesture.phase=0;return d==I_UP?I_MENU:d;}
 if(gesture.phase==1&&gesture.direction==d&&now-gesture.when<=(unsigned)(HZ*3/10)){
  seek_film(gesture.time);gesture.phase=0;suppress=d;return I_DOUBLE_UP+d-I_UP;
 }
 gesture.direction=d;gesture.phase=1;gesture.when=now;gesture.time=clock_ms;return d;
}
static bool apply_action(int a){
 switch(a){
 case I_LEFT:seek_film((clock_ms+117000-400)%117000);break;
 case I_RIGHT:seek_film(clock_ms+400);break;
 case I_UP:{uint8_t s[20];read_bytes(scene_offset+((cut+11)%12)*20,s,20);seek_film(le32(s+12));break;}
 case I_DOWN:seek_film(cut==11?0:scene_info.start+scene_info.duration);break;
 case I_SELECT:paused=!paused;break;
 case I_DOUBLE_UP:hud=!hud;break;case I_DOUBLE_DOWN:rain=!rain;break;
 case I_DOUBLE_RIGHT:seek_film(0);break;case I_DOUBLE_LEFT:case I_MENU:return true;
 }return false;
}
static bool menu(void){
 static const struct opt_items speeds[]={{"0.5x",-1},{"1.0x",-1},{"2.0x",-1}};
 MENUITEM_STRINGLIST(m,"Rock88",NULL,"Play Film","Restart Film","Speed",
  "Rain","Reflection","Show FPS","Playback Control","Quit");
 int sel=0,speed=speed_percent==50?0:speed_percent==100?1:2;
 bool quit=false,go=false;
 while(!go&&!quit){
  switch(rb->do_menu(&m,&sel,NULL,false)){
   case 0:go=true;break;
   case 1:seek_film(0);go=true;break;
   case 2:rb->set_option("Speed",&speed,RB_INT,speeds,3,NULL);speed_percent=speed==0?50:speed==1?100:200;break;
   case 3:rb->set_bool("Rain",&rain);break;
   case 4:rb->set_bool("Reflection",&reflections);break;
   case 5:rb->set_bool("Show FPS",&hud);break;
   case 6:playback_control(NULL);break;
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
enum plugin_status plugin_start(const void *parameter){
#ifdef DEPTH_IN_AUDIO_BUFFER
 size_t abuf_size;
 depth=rb->plugin_get_audio_buffer(&abuf_size);
 if(!depth || abuf_size<sizeof(uint16_t)*IH*IW){rb->splash(HZ*3,"Rock88: not enough memory");return PLUGIN_ERROR;}
#endif
 const char *path=parameter?parameter:PLUGIN_DEMOS_DIR "/rock88.r88";
 if(!open_assets(path)){if(asset_fd>=0)rb->close(asset_fd);asset_fd=-1;rb->splash(HZ*3,"Rock88: missing or invalid rock88.r88");return PLUGIN_ERROR;}
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
 usb=paused=false;rain=reflections=true;hud=false;speed_percent=100;frames=fps_tenths=0;select_armed=select_long=false;suppress=0;gesture.phase=0;seek_film(0);
 if(menu())goto done;
 unsigned long last=*rb->current_tick,last_fps=last,next=last;
 unsigned time_remainder=0;
 while(!usb&&!io_error){
  unsigned long now=*rb->current_tick;int timeout=TIME_BEFORE(now,next)?next-now:0;
  int a=gesture_action(read_action(timeout));if(usb)break;
  if(apply_action(a)){if(menu())break;last=last_fps=*rb->current_tick;frames=0;next=last;}
  now=*rb->current_tick;if(TIME_BEFORE(now,next))continue;
  unsigned ticks=now-last;last=now;
  if(!paused){uint64_t units=(uint64_t)ticks*10*speed_percent+time_remainder;seek_film(clock_ms+(units/HZ)%117000u);time_remainder=units%HZ;}
  render();titles_and_fade();present();++frames;now=*rb->current_tick;
  if(now-last_fps>=(unsigned)HZ){fps_tenths=frames*10u*HZ/MAX(1u,now-last_fps);last_fps=now;frames=0;}
  next=last+MAX(1,HZ/25);rb->yield();
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
 rb->close(asset_fd);asset_fd=-1;if(io_error)rb->splash(HZ*3,"Rock88: asset read error");
 return usb?PLUGIN_USB_CONNECTED:io_error?PLUGIN_ERROR:PLUGIN_OK;
}

/* Pinned source: 57b05a240f5bcaa02a18d62a1206ce7053dc09dc */

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
