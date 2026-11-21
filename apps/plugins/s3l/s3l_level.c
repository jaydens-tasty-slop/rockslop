/* small3dlib "Level" demo (Quake-like level, fog) for Rockbox. Based on
 * programs/level.c by Miloslav Ciz, CC0 1.0. */
#include "plugin.h"

#define S3L_RESOLUTION_X 128
#define S3L_RESOLUTION_Y 128
#define S3L_NEAR_CROSS_STRATEGY 3
#define S3L_PERSPECTIVE_CORRECTION 2
#define S3L_NEAR (S3L_F / 5)
#define S3L_USE_WIDER_TYPES 0
#define S3L_FLAT 0
#define S3L_SORT 0
#define S3L_Z_BUFFER 2 /* 8 bit: the 16 bit buffer would cost 32 KiB more */
#define S3L_MAX_TRIANGES_DRAWN 512
#define S3L_PIXEL_FUNCTION level_pixel

#include "small3dlib.h"
#include "levelModel.h"
#include "textures.h"

#define S3LRB_NAME "Level"
#define S3LRB_MENU_MS 1000
#define S3LRB_HINT "Up/Down=walk  L/R=turn"
#define S3LRB_HELP "Level: walk round a", "textured level.", "", "UP/DOWN: walk.", \
  "LEFT/RIGHT: turn.", "SELECT+UP/DOWN: look", "up/down.", "SELECT+LEFT/RIGHT:", \
  "strafe.", "Hold SEL+UP 1 s:", "menu (SEL+UP is also", "look up).", "", "Find the red teleport", "marker."
#define S3LRB_ABOUT "Level demo from", "small3dlib by", "drummyfish (Miloslav", "Ciz), CC0 1.0.", \
  "gitlab.com/drummyfish/", "small3dlib", "Textures: 64x64."

#include "s3l_main.h"

static S3L_Scene scene;
static S3L_Vec4 teleportPoint;
static const uint16_t *texture;
static uint32_t previousTriangle = 1000;
static S3L_Vec4 uv0, uv1, uv2;
static uint32_t rng = 1;

void level_pixel(S3L_PixelInfo *p)
{
  if (p->triangleID != previousTriangle)
  {
    switch (levelMaterials[p->triangleIndex])
    {
      case 0: texture = level1Texture; break;
      case 1: texture = level2Texture; break;
      default: texture = level3Texture; break;
    }

    S3L_getIndexedTriangleValues(p->triangleIndex, levelUVIndices, levelUVs, 2, &uv0, &uv1, &uv2);
    previousTriangle = p->triangleID;
  }

  int32_t u = S3L_interpolateBarycentric(uv0.x, uv1.x, uv2.x, p->barycentric) / 16;
  int32_t v = S3L_interpolateBarycentric(uv0.y, uv1.y, uv2.y, p->barycentric) / 16;
  uint16_t c = texture[(v & 63) * 64 + (u & 63)];

  /* fog: darken with distance */
  S3L_Unit fog = (p->depth * 8) / S3L_F;
  S3L_Unit r = S3L_clamp(((c >> 11) << 3) - fog, 0, 255);
  S3L_Unit g = S3L_clamp((((c >> 5) & 63) << 2) - fog, 0, 255);
  S3L_Unit b = S3L_clamp(((c & 31) << 3) - fog, 0, 255);

  s3lrb_set_pixel(p->x, p->y, r, g, b);
}

static void draw_teleport(int x, int y, int size)
{
  int half = size / 2;
  int x0 = S3L_max(0, x - half), x1 = S3L_min(S3LRB_W, x + half);
  int y0 = S3L_max(0, y - half), y1 = S3L_min(S3LRB_H, y + half);
  int row = y0 - (y - half);

  for (int j = y0; j < y1; ++j, ++row)
  {
    int i0, i1;

    if (row <= half)
    {
      i0 = S3L_max(x0, x - row);
      i1 = S3L_min(x1, x + row);
    }
    else
    {
      i0 = S3L_max(x0, x - size + row);
      i1 = S3L_min(x1, x + size - row);
    }

    for (int i = i0; i < i1; ++i)
    {
      rng = rng * 1103515245u + 12345u;
      if (((rng >> 16) & 7) == 0)
        s3lrb_set_pixel(i, j, 255, 0, 0);
    }
  }
}

static void demo_init(void)
{
  teleportPoint.x = 6 * S3L_F;
  teleportPoint.y = -3 * S3L_F;
  teleportPoint.z = 3 * S3L_F / 2;
  teleportPoint.w = S3L_F;

  levelModelInit();
  S3L_sceneInit(&levelModel, 1, &scene);
}

static void demo_frame(unsigned keys, int dt_ms)
{
  S3L_newFrame();

  for (int i = 0; i < S3LRB_W * S3LRB_H; ++i)
    s3lrb_fb[i] = FB_RGBPACK(200, 200, 200);

  S3L_drawScene(scene);

  S3L_Vec4 sp;
  S3L_project3DPointToScreen(teleportPoint, scene.camera, &sp);

  if (sp.w > 0 && sp.x >= 0 && sp.x < S3LRB_W && sp.y >= 0 && sp.y < S3LRB_H &&
      sp.z < S3L_zBufferRead(sp.x, sp.y))
    draw_teleport(sp.x, sp.y, sp.w);

  /* movement per frame scaled to ~30 fps of the original's unit steps */
  int n = S3L_max(1, dt_ms / 33);
  S3L_Vec4 camF, camR;
  S3L_rotationToDirections(scene.camera.transform.rotation, 20 * n, &camF, &camR, 0);

  S3L_Transform3D *t = &scene.camera.transform;
  bool sel = keys & BUTTON_SELECT;

  if (sel)
  {
    if (keys & BUTTON_UP)
      t->rotation.x += n;
    else if (keys & BUTTON_DOWN)
      t->rotation.x -= n;
    if (keys & BUTTON_LEFT)
      S3L_vec3Sub(&t->translation, camR);
    else if (keys & BUTTON_RIGHT)
      S3L_vec3Add(&t->translation, camR);
  }
  else
  {
    if (keys & BUTTON_UP)
      S3L_vec3Add(&t->translation, camF);
    else if (keys & BUTTON_DOWN)
      S3L_vec3Sub(&t->translation, camF);
    if (keys & BUTTON_LEFT)
      t->rotation.y += 2 * n;
    else if (keys & BUTTON_RIGHT)
      t->rotation.y -= 2 * n;
  }
}
