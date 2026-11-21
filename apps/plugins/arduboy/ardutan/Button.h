#ifndef _INC_BUTTON    //まだ読み込まれていなければ以下の処理をする
#define _INC_BUTTON    //すでに読み込まれているという目印をつける

#include <Arduboy.h>

class Button
{
  private:
    Arduboy *arduboy;
    // ボタンタイプ
    uint8_t type;
    bool press;
    bool pre_press;
  public:
    Button();
    Button(Arduboy *_arduboy, uint8_t _type);
    void begin();
    void commit();
    bool isPress();
    bool isPush();
    bool isRelease();
};
#endif    //INC_BUTTON
