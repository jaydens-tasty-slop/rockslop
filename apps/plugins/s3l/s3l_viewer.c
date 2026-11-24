/* small3dlib model viewer for Rockbox. Based on programs/modelViewer.c by
 * Miloslav Ciz, CC0 1.0. */
#include "plugin.h"

#define S3L_FLAT 0
#define S3L_NEAR_CROSS_STRATEGY 0
#define S3L_PERSPECTIVE_CORRECTION 2
#define S3L_SORT 0
#define S3L_STENCIL_BUFFER 0
#define S3L_Z_BUFFER 2 /* 8 bit, saves 16 KiB */
#define S3L_PIXEL_FUNCTION viewer_pixel
#define S3L_RESOLUTION_X 128
#define S3L_RESOLUTION_Y 128

#include "small3dlib.h"
#include "houseModel.h"
#include "chestModel.h"
#include "plantModel.h"
#include "cat1Model.h"
#include "cat2Model.h"
#include "textures.h"

#define S3LRB_NAME "Model viewer"
#define S3LRB_MENU_MS 1000
#define S3LRB_HINT "Arrows=rotate"
#define S3LRB_SETTINGS viewer_settings
#define S3LRB_HELP "Model viewer.", "", "Arrows: rotate.", "SELECT+UP/DOWN: move", \
  "closer/further.", "SELECT+LEFT/RIGHT:", "zoom (field of view).", "Hold SEL+UP 1 s: menu", \
  "(SEL+UP also moves closer).", "Settings changes the", "model, shading, fog,", "light and wireframe."
#define S3LRB_ABOUT "Model viewer from", "small3dlib by", "drummyfish (Miloslav", "Ciz), CC0 1.0.", \
  "gitlab.com/drummyfish/", "small3dlib", "Textures: 64x64."

#include "s3l_main.h"

enum { MODE_TEXTURED, MODE_FLAT, MODE_NORMALS, MODES };

static S3L_Unit houseNormals[HOUSE_VERTEX_COUNT * 3];
static S3L_Unit chestNormals[CHEST_VERTEX_COUNT * 3];
static S3L_Unit catNormals[CAT1_VERTEX_COUNT * 3];
static S3L_Unit plantNormals[PLANT_VERTEX_COUNT * 3];
static S3L_Unit catVertices[CAT1_VERTEX_COUNT * 3];

static S3L_Model3D model;
static S3L_Scene scene;
static const uint16_t *texture;
static const S3L_Unit *uvs, *normals;
static const S3L_Index *uvIndices;

static uint32_t previousTriangle = -1;
static S3L_Vec4 uv0, uv1, uv2, n0, n1, n2, toLight;
static uint16_t l0, l1, l2;
static int modelIndex, mode = MODE_TEXTURED;
static bool light = true, fog, wire, transparency;

static void viewer_cat_animate(void)
{
  /* oscillate between the two cat keyframes, about 1.3 s per cycle */
  S3L_Unit t = (S3L_F + S3L_sin(*rb->current_tick * 5)) / 2;

  for (S3L_Index i = 0; i < CAT1_VERTEX_COUNT * 3; ++i)
    catVertices[i] = S3L_interpolateByUnit(cat1Vertices[i], cat2Vertices[i], t);
}

void viewer_pixel(S3L_PixelInfo *p)
{
  if (p->triangleID != previousTriangle)
  {
    if (mode == MODE_TEXTURED)
      S3L_getIndexedTriangleValues(p->triangleIndex, uvIndices, uvs, 2, &uv0, &uv1, &uv2);

    if (light || mode == MODE_NORMALS)
    {
      S3L_getIndexedTriangleValues(p->triangleIndex, model.triangles, normals, 3, &n0, &n1, &n2);

      l0 = 256 + S3L_clamp(S3L_vec3Dot(n0, toLight), -511, 511) / 2;
      l1 = 256 + S3L_clamp(S3L_vec3Dot(n1, toLight), -511, 511) / 2;
      l2 = 256 + S3L_clamp(S3L_vec3Dot(n2, toLight), -511, 511) / 2;
    }

    previousTriangle = p->triangleID;
  }

  if (wire && p->barycentric[0] != 0 && p->barycentric[1] != 0 && p->barycentric[2] != 0)
    return;

  int16_t r = 128, g = 128, b = 128;

  if (mode == MODE_TEXTURED)
  {
    int32_t u = S3L_interpolateBarycentric(uv0.x, uv1.x, uv2.x, p->barycentric) / 8;
    int32_t v = S3L_interpolateBarycentric(uv0.y, uv1.y, uv2.y, p->barycentric) / 8;
    uint16_t c = texture[(v & 63) * 64 + (u & 63)];

    if (transparency && c == 0xf800) /* pure red marks transparent texels */
    {
      S3L_zBufferWrite(p->x, p->y, p->previousZ);
      return;
    }

    r = (c >> 11) << 3;
    g = ((c >> 5) & 63) << 2;
    b = (c & 31) << 3;
  }
  else if (mode == MODE_NORMALS)
  {
    S3L_Vec4 n;
    n.x = S3L_interpolateBarycentric(n0.x, n1.x, n2.x, p->barycentric);
    n.y = S3L_interpolateBarycentric(n0.y, n1.y, n2.y, p->barycentric);
    n.z = S3L_interpolateBarycentric(n0.z, n1.z, n2.z, p->barycentric);
    S3L_vec3Normalize(&n);

    r = S3L_clamp(128 + n.x / 4, 0, 255);
    g = S3L_clamp(128 + n.y / 4, 0, 255);
    b = S3L_clamp(128 + n.z / 4, 0, 255);
  }

  if (light)
  {
    int16_t l = S3L_interpolateBarycentric(l0, l1, l2, p->barycentric);

    r = S3L_clamp((r * l) / S3L_F, 0, 255);
    g = S3L_clamp((g * l) / S3L_F, 0, 255);
    b = S3L_clamp((b * l) / S3L_F, 0, 255);
  }

  if (fog)
  {
    int16_t f = (((p->depth - S3L_NEAR) * 255) / (S3L_F * 64)) * 2;

    r = S3L_clamp(r + f, 0, 255);
    g = S3L_clamp(g + f, 0, 255);
    b = S3L_clamp(b + f, 0, 255);
  }

  s3lrb_set_pixel(p->x, p->y, r, g, b);
}

static void set_model(int index)
{
  S3L_Transform3D keep;

  if (scene.models[0].vertices)
    keep = scene.models[0].transform;
  else
    S3L_transform3DInit(&keep);

  switch (index)
  {
#define MODEL_CASE(n, m, tex, nrm) \
    case n: \
      texture = tex##Texture; uvs = m##UVs; uvIndices = m##UVIndices; normals = nrm; \
      scene.models[0] = m##Model; break;
    MODEL_CASE(0, house, house, houseNormals)
    MODEL_CASE(1, chest, chest, chestNormals)
    MODEL_CASE(3, plant, plant, plantNormals)
    case 2:
      texture = catTexture; uvs = cat1UVs; uvIndices = cat1UVIndices; normals = catNormals;
      scene.models[0] = cat1Model;
      scene.models[0].vertices = catVertices;
      viewer_cat_animate();
      break;
#undef MODEL_CASE
  }

  S3L_computeModelNormals(scene.models[0], (S3L_Unit *)normals, 0);

  scene.models[0].transform = keep;
  S3L_drawConfigInit(&scene.models[0].config);
  transparency = index == 3;
  scene.models[0].config.backfaceCulling = transparency ? 0 : 2;
  modelIndex = index;
  previousTriangle = -1;
}

static void viewer_settings(void)
{
  static const struct opt_items models[] = { {"House", -1}, {"Chest", -1}, {"Cat", -1}, {"Plant", -1} };
  static const struct opt_items modes[] = { {"Textured", -1}, {"Flat", -1}, {"Normals", -1} };
  int m = modelIndex, d = mode;

  rb->set_option("Model", &m, RB_INT, models, 4, NULL);
  rb->set_option("Shading", &d, RB_INT, modes, MODES, NULL);
  rb->set_bool("Light", &light);
  rb->set_bool("Fog", &fog);
  rb->set_bool("Wireframe", &wire);

  mode = d;
  if (m != modelIndex)
    set_model(m);
  previousTriangle = -1;
}

static void demo_init(void)
{
  toLight.x = 10;
  toLight.y = 10;
  toLight.z = 10;
  S3L_vec3Normalize(&toLight);

  houseModelInit();
  chestModelInit();
  plantModelInit();
  cat1ModelInit();
  cat2ModelInit();

  S3L_sceneInit(&model, 1, &scene);
  scene.camera.transform.translation.z = -S3L_F * 8;
  viewer_cat_animate();
  set_model(0);
}

static void demo_frame(unsigned keys, int dt_ms)
{
  S3L_newFrame();

  for (int i = 0; i < S3LRB_W * S3LRB_H; ++i)
    s3lrb_fb[i] = FB_RGBPACK(200, 200, 200);

  S3L_drawScene(scene);

  S3L_Transform3D *mt = &scene.models[0].transform;
  S3L_Camera *cam = &scene.camera;
  int rot = S3L_max(1, 300 * dt_ms / 1000);
  int move = S3L_max(1, 3000 * dt_ms / 1000);
  int fov = S3L_max(1, 1000 * dt_ms / 1000);

  if (!(keys & BUTTON_SELECT))
  {
    if (keys & BUTTON_LEFT)
      mt->rotation.y += rot;
    else if (keys & BUTTON_RIGHT)
      mt->rotation.y -= rot;
    if (keys & BUTTON_DOWN)
      mt->rotation.x += rot;
    else if (keys & BUTTON_UP)
      mt->rotation.x -= rot;
  }
  else
  {
    if (keys & BUTTON_LEFT)
      cam->focalLength = S3L_min(S3L_F * 5, cam->focalLength + fov);
    else if (keys & BUTTON_RIGHT)
      cam->focalLength = S3L_max(S3L_F / 2, cam->focalLength - fov);
    if (keys & BUTTON_UP)
      cam->transform.translation.z = S3L_min(S3L_F, cam->transform.translation.z + move);
    else if (keys & BUTTON_DOWN)
      cam->transform.translation.z = S3L_max(-S3L_F * 16, cam->transform.translation.z - move);
  }

  if (modelIndex == 2)
    viewer_cat_animate();
}
