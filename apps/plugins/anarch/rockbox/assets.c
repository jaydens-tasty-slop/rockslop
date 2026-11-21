/* One level in RAM, portable little-endian records on SD. CC0-1.0. */
#include "port.h"
#define SFG_EXTERNAL_LEVELS 1
#include "../levels.h"
#include "format.h"
#define SAVE_PATH PLUGIN_GAMES_DATA_DIR "/anarch.sav"
#define TEMP_PATH PLUGIN_GAMES_DATA_DIR "/anarch.tmp"
static int asset_fd = -1;
static SFG_Level level;
bool anarch_io_error;
static uint32_t le32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static uint32_t crc32(const uint8_t *p, unsigned n)
{
    uint32_t c = ~0u;
    while (n--) {
        c ^= *p++;
        for (unsigned i=0; i<8; ++i) c=(c>>1)^((0u-(c&1u))&0xedb88320u);
    }
    return ~c;
}
static bool read_full(int fd, void *buffer, size_t n)
{
    uint8_t *p=buffer;
    while (n) { ssize_t k=rb->read(fd,p,n); if(k<=0)return false; p+=k;n-=k; }
    return true;
}
static bool write_full(int fd, const void *buffer, size_t n)
{
    const uint8_t *p=buffer;
    while(n) { ssize_t k=rb->write(fd,p,n); if(k<=0)return false;p+=k;n-=k; }
    return true;
}
bool anarch_assets_open(const char *path)
{
    uint8_t h[16];
    anarch_io_error=false;
    asset_fd=rb->open(path,O_RDONLY);
    if(asset_fd<0)return false;
    if(rb->lseek(asset_fd,0,SEEK_END)!=16+10*ANARCH_LEVEL_BYTES ||
       rb->lseek(asset_fd,0,SEEK_SET)!=0 || !read_full(asset_fd,h,sizeof(h)) ||
       rb->memcmp(h,"ANARCH1\0",8) || le32(h+8)!=ANARCH_LEVEL_BYTES || le32(h+12)!=10) {
        anarch_assets_close(); return false;
    }
    return true;
}
void anarch_assets_close(void)
{ if(asset_fd>=0)rb->close(asset_fd);asset_fd=-1; }
const SFG_Level *SFG_readLevel(uint8_t number)
{
    /* Struct layout is checked; swap the dictionary after CRC on big endian.
     * No pointers, host padding or native integers are stored in the file. */
    typedef char check_size[sizeof(SFG_Level)==ANARCH_LEVEL_BYTES ? 1 : -1];
    (void)sizeof(check_size);
    if(number>=10 || rb->lseek(asset_fd,16+number*ANARCH_LEVEL_BYTES,SEEK_SET)<0 ||
       !read_full(asset_fd,&level,sizeof(level)) ||
       crc32((const uint8_t*)&level,sizeof(level))!=anarch_level_crc[number]) {
        anarch_io_error=true;
        rb->memset(&level,0,sizeof(level));
        return &level; /* safe empty map until the frontend exits */
    }
    for(unsigned i=0;i<64;++i) {
        const uint8_t *p=(const uint8_t*)&level.tileDictionary[i];
        uint16_t v=p[0] | (uint16_t)p[1]<<8;
        level.tileDictionary[i]=v;
    }
    return &level;
}
bool anarch_load_save(uint8_t *data)
{
    uint8_t saved[24];
    int fd=rb->open(SAVE_PATH,O_RDONLY);
    if(fd<0)return true; /* first run: retain engine defaults */
    bool ok=rb->lseek(fd,0,SEEK_END)==24 && rb->lseek(fd,0,SEEK_SET)==0 &&
        read_full(fd,saved,sizeof(saved));
    rb->close(fd);
    if(ok && !rb->memcmp(saved,"ANRSAVE1",8) &&
       le32(saved+20)==crc32(saved,20) && (saved[8]&15)<10 && (saved[8]>>4)<10)
        rb->memcpy(data,saved+8,12);
    return true;
}
void anarch_store_save(const uint8_t *data)
{
    uint8_t saved[24];
    rb->memcpy(saved,"ANRSAVE1",8);rb->memcpy(saved+8,data,12);
    uint32_t crc=crc32(saved,20);
    for(unsigned i=0;i<4;++i)saved[20+i]=crc>>(8*i);
    int fd=rb->open(TEMP_PATH,O_WRONLY|O_CREAT|O_TRUNC,0666);
    bool ok=false;
    if(fd>=0){ok=write_full(fd,saved,sizeof(saved));if(rb->close(fd)<0)ok=false;}
    if(ok)ok=rb->rename(TEMP_PATH,SAVE_PATH)==0;
    if(!ok){rb->remove(TEMP_PATH);rb->splash(HZ,"Anarch: save failed");}
}
