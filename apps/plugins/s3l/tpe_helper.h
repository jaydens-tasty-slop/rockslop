/* Rockbox glue for the tinyphysicsengine demos: bodies, camera and the small3dlib
 * drawing helpers. Condensed from programs/helper.h of tinyphysicsengine by
 * Miloslav Ciz (CC0 1.0). Include after defining the S3LRB_* macros;
 * it pulls in s3l_main.h. */
#include "plugin.h"
#include "s3l_main.h"

#define S3L_RESOLUTION_X 128
#define S3L_RESOLUTION_Y 128
#define S3L_PIXEL_FUNCTION tpe_pixel
#define S3L_Z_BUFFER 2
#ifndef S3L_PERSPECTIVE_CORRECTION
#define S3L_PERSPECTIVE_CORRECTION 0
#endif
#ifndef S3L_NEAR_CROSS_STRATEGY
#define S3L_NEAR_CROSS_STRATEGY 1
#endif

#include "tinyphysicsengine.h"
#include "small3dlib.h"

#define TPE_MAX_BODIES 8
#define TPE_MAX_JOINTS 64
#define TPE_MAX_CONNECTIONS 128
#define TPE_STEP_MS 33 /* the demos are tuned for 30 physics steps/s */
#define SPHERE_VERTEX_COUNT 42
static const S3L_Unit sphereVertices[SPHERE_VERTEX_COUNT * 3] = {
      0,  -512,     0,        // 0
    370,  -228,  -269,        // 3
   -141,  -228,  -435,        // 6
   -457,  -228,     0,        // 9
   -141,  -228,   435,        // 12
    370,  -228,   269,        // 15
    141,   228,  -435,        // 18
   -370,   228,  -269,        // 21
   -370,   228,   269,        // 24
    141,   228,   435,        // 27
    457,   228,     0,        // 30
      0,   512,     0,        // 33
    -83,  -435,  -255,        // 36
    217,  -435,  -158,        // 39
    134,  -269,  -414,        // 42
    435,  -269,     0,        // 45
    217,  -435,   158,        // 48
   -269,  -435,     0,        // 51
   -352,  -269,  -255,        // 54
    -83,  -435,   255,        // 57
   -352,  -269,   255,        // 60
    134,  -269,   414,        // 63
    486,     0,  -158,        // 66
    486,     0,   158,        // 69
      0,     0,  -512,        // 72
    300,     0,  -414,        // 75
   -486,     0,  -158,        // 78
   -300,     0,  -414,        // 81
   -300,     0,   414,        // 84
   -486,     0,   158,        // 87
    300,     0,   414,        // 90
      0,     0,   512,        // 93
    352,   269,  -255,        // 96
   -134,   269,  -414,        // 99
   -435,   269,     0,        // 102
   -134,   269,   414,        // 105
    352,   269,   255,        // 108
     83,   435,  -255,        // 111
    269,   435,     0,        // 114
   -217,   435,  -158,        // 117
   -217,   435,   158,        // 120
     83,   435,   255         // 123
}; // sphereVertices

#define SPHERE_TRIANGLE_COUNT 80
static const S3L_Index sphereTriangleIndices[SPHERE_TRIANGLE_COUNT * 3] = {
      0,    13,    12,        // 0
      1,    13,    15,        // 3
      0,    12,    17,        // 6
      0,    17,    19,        // 9
      0,    19,    16,        // 12
      1,    15,    22,        // 15
      2,    14,    24,        // 18
      3,    18,    26,        // 21
      4,    20,    28,        // 24
      5,    21,    30,        // 27
      1,    22,    25,        // 30
      2,    24,    27,        // 33
      3,    26,    29,        // 36
      4,    28,    31,        // 39
      5,    30,    23,        // 42
      6,    32,    37,        // 45
      7,    33,    39,        // 48
      8,    34,    40,        // 51
      9,    35,    41,        // 54
     10,    36,    38,        // 57
     38,    41,    11,        // 60
     38,    36,    41,        // 63
     36,     9,    41,        // 66
     41,    40,    11,        // 69
     41,    35,    40,        // 72
     35,     8,    40,        // 75
     40,    39,    11,        // 78
     40,    34,    39,        // 81
     34,     7,    39,        // 84
     39,    37,    11,        // 87
     39,    33,    37,        // 90
     33,     6,    37,        // 93
     37,    38,    11,        // 96
     37,    32,    38,        // 99
     32,    10,    38,        // 102
     23,    36,    10,        // 105
     23,    30,    36,        // 108
     30,     9,    36,        // 111
     31,    35,     9,        // 114
     31,    28,    35,        // 117
     28,     8,    35,        // 120
     29,    34,     8,        // 123
     29,    26,    34,        // 126
     26,     7,    34,        // 129
     27,    33,     7,        // 132
     27,    24,    33,        // 135
     24,     6,    33,        // 138
     25,    32,     6,        // 141
     25,    22,    32,        // 144
     22,    10,    32,        // 147
     30,    31,     9,        // 150
     30,    21,    31,        // 153
     21,     4,    31,        // 156
     28,    29,     8,        // 159
     28,    20,    29,        // 162
     20,     3,    29,        // 165
     26,    27,     7,        // 168
     26,    18,    27,        // 171
     18,     2,    27,        // 174
     24,    25,     6,        // 177
     24,    14,    25,        // 180
     14,     1,    25,        // 183
     22,    23,    10,        // 186
     22,    15,    23,        // 189
     15,     5,    23,        // 192
     16,    21,     5,        // 195
     16,    19,    21,        // 198
     19,     4,    21,        // 201
     19,    20,     4,        // 204
     19,    17,    20,        // 207
     17,     3,    20,        // 210
     17,    18,     3,        // 213
     17,    12,    18,        // 216
     12,     2,    18,        // 219
     15,    16,     5,        // 222
     15,    13,    16,        // 225
     13,     0,    16,        // 228
     12,    14,     2,        // 231
     12,    13,    14,        // 234
     13,     1,    14         // 237
}; // sphereTriangleIndices
static S3L_Unit cubeVertices[] = { S3L_CUBE_VERTICES(TPE_F) };
static S3L_Index cubeTriangles[] = { S3L_CUBE_TRIANGLES };
static S3L_Model3D cubeModel, sphereModel;

static TPE_Body tpe_bodies[TPE_MAX_BODIES];
static TPE_Joint tpe_joints[TPE_MAX_JOINTS];
static TPE_Connection tpe_connections[TPE_MAX_CONNECTIONS];
static unsigned tpe_jointsUsed, tpe_connectionsUsed;
static TPE_World tpe_world;
static S3L_Scene tpe_scene;
static S3L_Vec4 tpe_camForw, tpe_camRight;
static S3L_Model3D *tpe_drawn;
static TPE_Vec3 tpe_lightDir;
static uint8_t tpe_r = 0, tpe_g = 255, tpe_b = 0, tpe_rr, tpe_gg, tpe_bb;
static int32_t tpe_prevTriangle = -1;
static int tpe_stepAcc;

#define tpe_lastBody tpe_world.bodies[tpe_world.bodyCount - 1]

void tpe_pixel(S3L_PixelInfo *p)
{
  if (p->triangleIndex != tpe_prevTriangle)
  {
    const S3L_Index *v = tpe_drawn->triangles + 3 * p->triangleIndex;
    const S3L_Unit *vs = tpe_drawn->vertices;
    TPE_Vec3 a = TPE_vec3(vs[v[0] * 3], vs[v[0] * 3 + 1], vs[v[0] * 3 + 2]);
    TPE_Vec3 b = TPE_vec3(vs[v[1] * 3], vs[v[1] * 3 + 1], vs[v[1] * 3 + 2]);
    TPE_Vec3 c = TPE_vec3(vs[v[2] * 3], vs[v[2] * 3 + 1], vs[v[2] * 3 + 2]);
    TPE_Vec3 n = TPE_vec3Normalized(TPE_vec3Cross(TPE_vec3Minus(c, a), TPE_vec3Minus(c, b)));
    TPE_Unit intensity = 190 + TPE_vec3Dot(n, tpe_lightDir) / 8;

    tpe_rr = (tpe_r * intensity) / 256;
    tpe_gg = (tpe_g * intensity) / 256;
    tpe_bb = (tpe_b * intensity) / 256;
    tpe_prevTriangle = p->triangleIndex;
  }

  s3lrb_set_pixel(p->x, p->y, tpe_rr, tpe_gg, tpe_bb);
}

static inline void tpe_color(uint8_t r, uint8_t g, uint8_t b)
{
  tpe_r = r;
  tpe_g = g;
  tpe_b = b;
}

static inline void tpe_drawModel(S3L_Model3D *m, TPE_Vec3 pos, TPE_Vec3 scale, TPE_Vec3 rot)
{
  tpe_drawn = m;
  tpe_prevTriangle = -1;

  m->transform.translation.x = pos.x;
  m->transform.translation.y = pos.y;
  m->transform.translation.z = pos.z;
  m->transform.scale.x = scale.x;
  m->transform.scale.y = scale.y;
  m->transform.scale.z = scale.z;
  m->transform.rotation.x = rot.x;
  m->transform.rotation.y = rot.y;
  m->transform.rotation.z = rot.z;

  tpe_scene.models = m;
  S3L_drawScene(tpe_scene);
}

static inline void tpe_drawBox(TPE_Vec3 pos, TPE_Vec3 scale, TPE_Vec3 rot)
{
  cubeModel.config.backfaceCulling = 2;
  tpe_drawModel(&cubeModel, pos, scale, rot);
}

static inline void tpe_drawBoxInside(TPE_Vec3 pos, TPE_Vec3 scale, TPE_Vec3 rot)
{
  cubeModel.config.backfaceCulling = 1;
  tpe_drawModel(&cubeModel, pos, scale, rot);
}

static inline void tpe_drawSphere(TPE_Vec3 pos, TPE_Vec3 scale, TPE_Vec3 rot)
{
  sphereModel.config.backfaceCulling = 2;
  tpe_drawModel(&sphereModel, pos, scale, rot);
}

static inline void tpe_drawLine3D(TPE_Vec3 p1, TPE_Vec3 p2, uint8_t r, uint8_t g, uint8_t b)
{
  S3L_Vec4 a, b4, c, d;

  a.x = p1.x; a.y = p1.y; a.z = p1.z; a.w = 0;
  b4.x = p2.x; b4.y = p2.y; b4.z = p2.z; b4.w = 0;
  S3L_project3DPointToScreen(a, tpe_scene.camera, &c);
  S3L_project3DPointToScreen(b4, tpe_scene.camera, &d);

  if (c.x < 0 || c.x >= S3LRB_W || c.y < 0 || c.y >= S3LRB_H || c.z <= 0 ||
      d.x < 0 || d.x >= S3LRB_W || d.y < 0 || d.y >= S3LRB_H || d.z <= 0)
    return;

  int dx = d.x - c.x, dy = d.y - c.y;
  int n = S3L_max(S3L_abs(dx), S3L_abs(dy));

  for (int i = 0; i <= n; ++i)
    s3lrb_set_pixel(n ? c.x + dx * i / n : c.x, n ? c.y + dy * i / n : c.y, r, g, b);
}

static inline void tpe_bodyAdded(int joints, int conns, TPE_Unit mass)
{
  TPE_bodyInit(&tpe_bodies[tpe_world.bodyCount], &tpe_joints[tpe_jointsUsed], joints,
               &tpe_connections[tpe_connectionsUsed], conns, mass);
  tpe_jointsUsed += joints;
  tpe_connectionsUsed += conns;
  tpe_world.bodyCount++;
}

static inline void tpe_addBox(TPE_Unit w, TPE_Unit h, TPE_Unit d, TPE_Unit jointSize, TPE_Unit mass)
{
  TPE_makeBox(tpe_joints + tpe_jointsUsed, tpe_connections + tpe_connectionsUsed, w, h, d, jointSize);
  tpe_bodyAdded(8, 16, mass);
}

static inline void tpe_addRect(TPE_Unit w, TPE_Unit d, TPE_Unit jointSize, TPE_Unit mass)
{
  TPE_makeRect(tpe_joints + tpe_jointsUsed, tpe_connections + tpe_connectionsUsed, w, d, jointSize);
  tpe_bodyAdded(4, 6, mass);
}

static inline void tpe_addCenterRectFull(TPE_Unit w, TPE_Unit d, TPE_Unit jointSize, TPE_Unit mass)
{
  TPE_makeCenterRectFull(tpe_joints + tpe_jointsUsed, tpe_connections + tpe_connectionsUsed, w, d, jointSize);
  tpe_bodyAdded(5, 10, mass);
}

/* Resets the world and the camera. */
static inline void tpe_reset(void)
{
  tpe_jointsUsed = tpe_connectionsUsed = 0;
  tpe_stepAcc = 0;
  S3L_sceneInit(0, 1, &tpe_scene);
  TPE_worldInit(&tpe_world, tpe_bodies, 0, 0);
}

static inline void tpe_init(void)
{
  tpe_lightDir = TPE_vec3Normalized(TPE_vec3(300, 200, 100));

  S3L_model3DInit(cubeVertices, S3L_CUBE_VERTEX_COUNT, cubeTriangles, S3L_CUBE_TRIANGLE_COUNT, &cubeModel);
  S3L_model3DInit(sphereVertices, SPHERE_VERTEX_COUNT, sphereTriangleIndices, SPHERE_TRIANGLE_COUNT, &sphereModel);
  tpe_reset();
}

/* Starts a frame: clears the screen and returns how many fixed physics steps
 * the elapsed time asks for (at most 2). */
static int tpe_frameStart(int dt_ms)
{
  for (int i = 0; i < S3LRB_W * S3LRB_H; ++i)
    s3lrb_fb[i] = FB_RGBPACK(0, 0, 0);

  S3L_newFrame();
  S3L_rotationToDirections(tpe_scene.camera.transform.rotation, 100, &tpe_camForw, &tpe_camRight, 0);

  tpe_stepAcc += dt_ms;
  int steps = 0;
  while (tpe_stepAcc >= TPE_STEP_MS && steps < 2)
  {
    tpe_stepAcc -= TPE_STEP_MS;
    steps++;
  }
  if (tpe_stepAcc > TPE_STEP_MS)
    tpe_stepAcc = TPE_STEP_MS;
  return steps;
}

/* Arrows turn the camera; with SELECT they move it. */
static inline void tpe_cameraFreeMovement(unsigned keys, int dt_ms)
{
  S3L_Transform3D *t = &tpe_scene.camera.transform;
  int rot = S3L_max(1, 5 * dt_ms / TPE_STEP_MS);

  if (keys & BUTTON_SELECT)
  {
    if (keys & BUTTON_UP)
      S3L_vec3Add(&t->translation, tpe_camForw);
    else if (keys & BUTTON_DOWN)
      S3L_vec3Sub(&t->translation, tpe_camForw);
    else if (keys & BUTTON_LEFT)
      S3L_vec3Sub(&t->translation, tpe_camRight);
    else if (keys & BUTTON_RIGHT)
      S3L_vec3Add(&t->translation, tpe_camRight);
  }
  else
  {
    if (keys & BUTTON_UP)
      t->rotation.x += rot;
    else if (keys & BUTTON_DOWN)
      t->rotation.x -= rot;
    else if (keys & BUTTON_LEFT)
      t->rotation.y += rot;
    else if (keys & BUTTON_RIGHT)
      t->rotation.y -= rot;
  }
}
