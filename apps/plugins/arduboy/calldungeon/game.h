#include "Arduboy.h"

#include "player.h"

//=============================================================================
typedef enum {
    GAME_ST_NONE,
    GAME_ST_TITLE,
    GAME_ST_PLAY,
    GAME_ST_WAIT_MSG,
    GAME_ST_CHANGE_SCENE,
    GAME_ST_MAX
} eGameState;

//=============================================================================
typedef enum {
    OBJ_NONE,
    OBJ_BG,
    OBJ_GATE,
    OBJ_ITEM,
    OBJ_MONSTER,
    OBJ_FG,
    OBJ_TEMP_BG,
    OBJ_TEMP_FG,
    OBJ_MAX
} eObjectType;

//=============================================================================
typedef struct {
    eObjectType nType:8;
    uint8_t ubNo;
    int16_t wX;
    int16_t wY;
    uint16_t uwParam;
} tObject;

//=============================================================================
typedef struct {
    uint8_t ubCnt;
    uint8_t ubState:4;
    uint8_t ubHP:4;
} tMonsterParam;

//=============================================================================
typedef struct {
    int16_t wLeft_room;
    int16_t wRight_room;
    int16_t awOther_room[2];

    tObject *prObjs;

    int16_t wMin_x;
    int16_t wMax_x;
} tRoom;

//=============================================================================
class Game {
 public:
    eGameState mState;
    int mCounter;
    uint16_t mAnim;
    uint16_t mInv;

    tRoom *prCur_room;

    tRoom *prNext_room;
    int16_t wNext_x;

    char *pcMsg;

    Player *mPlayer;

    Game();
    ~Game();

    void check_input();
    void draw();

    void draw_back();
    tObject *draw_bg_object(tObject *prObj);
    tObject *draw_gate(tObject *prObj);
    tObject *draw_item(tObject *prObj);
    tObject *draw_monster(tObject *prObj);
    tObject *draw_fg_object(tObject *prObj);
    void draw_temp_bg(void);
    void draw_temp_fg(void);
    void draw_param_win(void);
    void draw_message(void);

    void do_state();

    void set_message(char *msg);
    void set_next_scene(tRoom *prNext_room, int16_t wNext_x);
    void next_message(void);
};
