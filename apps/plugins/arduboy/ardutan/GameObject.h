#ifndef _INC_GAMEOBJECT    //まだ読み込まれていなければ以下の処理をする
#define _INC_GAMEOBJECT    //すでに読み込まれているという目印をつける

#include <Arduboy.h>

class GameObject
{
  protected:
    Arduboy *arduboy;
    // 位置情報
    float x;
    float y;

    bool active_flag;
public:
    GameObject();
    virtual void draw();
    float getX();
    float getY();
    void setX(float _x);
    void setY(float _y);
    virtual float getTop();
    virtual float getDown();
    virtual float getRight();
    virtual float getLeft();
    bool isActive();
    void setActive(bool flag);
};
#endif    //INC_GAMEOBJECT
