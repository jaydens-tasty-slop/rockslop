#ifndef GRAPHICS_H
#define GRAPHICS_H

extern const char TailleSprite;

extern void Position_camera(uint8_t Xp, uint8_t  Yp);
extern void DessineNiveau();
extern void DessinePerso();

extern Image Title;

struct Type_Camera {
  int X; // Position horizontale de la caméra
  int Y; // Position verticale de la caméra
};

extern Type_Camera Camera;

#endif
