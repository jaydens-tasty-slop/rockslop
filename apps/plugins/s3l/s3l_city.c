/* small3dlib "City" demo (GTA-like driving) for Rockbox. Based on
 * programs/city.c by Miloslav Ciz, CC0 1.0. */
#include "plugin.h"

#define S3L_FLAT 0
#define S3L_NEAR_CROSS_STRATEGY 3
#define S3L_PERSPECTIVE_CORRECTION 2
#define S3L_SORT 0
#define S3L_STENCIL_BUFFER 0
#define S3L_Z_BUFFER 2
#define S3L_PIXEL_FUNCTION city_pixel
#define S3L_RESOLUTION_X 128
#define S3L_RESOLUTION_Y 128

#include "small3dlib.h"
#include "cityModel.h"
#include "carModel.h"
#include "textures.h"

#define S3LRB_NAME "City"
#define S3LRB_HINT "Up/Down=drive  L/R=steer"
#define S3LRB_HELP "City: drive the", "car round the town.", "", "UP: accelerate.", \
  "DOWN: brake/reverse.", "LEFT/RIGHT: steer.", "Hold SEL+UP: menu.", "", \
  "Driving into the", "coloured tiles teleports", "the car."
#define S3LRB_ABOUT "City demo from", "small3dlib by", "drummyfish (Miloslav", "Ciz), CC0 1.0.", \
  "gitlab.com/drummyfish/", "small3dlib", "Textures reduced to", "64x64."

#include "s3l_main.h"

#define MAX_VELOCITY 1000
#define ACCELERATION 700
#define TURN_SPEED 300
#define FRICTION 600

static S3L_Model3D models[2];
static S3L_Scene scene;
static int16_t velocity;

static const uint8_t collisionMap[8 * 10] =
{
  1,1,1,1,1,1,1,1,
  1,1,1,1,0,0,0,1,
  1,1,1,1,0,1,0,1,
  2,2,1,0,0,0,0,3,
  1,2,1,0,1,1,3,1,
  2,0,0,0,1,1,3,3,
  1,0,1,0,0,1,1,1,
  1,0,0,0,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1
};

static void clear_sky(void)
{
  for (int y = 0; y < S3LRB_H; ++y)
  {
    S3L_Unit t = S3L_min(S3L_F, ((y * S3L_F) / S3LRB_H) * 4);
    fb_data c = FB_RGBPACK(S3L_interpolateByUnit(200, 242, t),
                           S3L_interpolateByUnit(102, 255, t),
                           S3L_interpolateByUnit(255, 230, t));
    fb_data *row = s3lrb_fb + y * S3LRB_W;
    for (int x = 0; x < S3LRB_W; ++x)
      row[x] = c;
  }
}

static uint32_t previousTriangle = -1;
static S3L_Vec4 uv0, uv1, uv2;

void city_pixel(S3L_PixelInfo *p)
{
  if (p->triangleID != previousTriangle)
  {
    if (p->modelIndex == 0)
      S3L_getIndexedTriangleValues(p->triangleIndex, cityUVIndices, cityUVs, 2, &uv0, &uv1, &uv2);
    else
      S3L_getIndexedTriangleValues(p->triangleIndex, carUVIndices, carUVs, 2, &uv0, &uv1, &uv2);

    previousTriangle = p->triangleID;
  }

  int32_t u = S3L_interpolateBarycentric(uv0.x, uv1.x, uv2.x, p->barycentric) >> 3;
  int32_t v = S3L_interpolateBarycentric(uv0.y, uv1.y, uv2.y, p->barycentric) >> 3;
  uint16_t c = cityTexture[(v & 63) * 64 + (u & 63)];

  s3lrb_fb[p->y * S3LRB_W + p->x] = S3LRB_FROM565(c);
}

static uint8_t collision(S3L_Vec4 worldPosition)
{
  worldPosition.x /= S3L_F;
  worldPosition.z /= -S3L_F;

  uint16_t index = worldPosition.z * 8 + worldPosition.x;

  return index < sizeof(collisionMap) ? collisionMap[index] : 1;
}

static void handleCollision(S3L_Vec4 *pos, S3L_Vec4 previousPos)
{
  S3L_Vec4 newPos = *pos;
  newPos.x = previousPos.x;

  if (collision(newPos))
  {
    newPos = *pos;
    newPos.z = previousPos.z;

    if (collision(newPos))
      newPos = previousPos;
  }

  *pos = newPos;
}

static void demo_init(void)
{
  cityModelInit();
  carModelInit();

  models[0] = cityModel;
  models[1] = carModel;

  S3L_sceneInit(models, 2, &scene);

  S3L_transform3DSet(1909, 16, -3317, 0, -510, 0, 512, 512, 512, &(models[1].transform));

  scene.camera.transform.translation.y = S3L_F / 2;
  scene.camera.transform.rotation.x = -S3L_F / 16;
}

static void demo_frame(unsigned keys, int dt_ms)
{
  S3L_Transform3D *car = &models[1].transform;

  /* draw: the car is overturned to the side while steering */
  car->rotation.y += car->rotation.z;
  S3L_newFrame();
  clear_sky();
  S3L_drawScene(scene);
  car->rotation.y -= car->rotation.z;

  int16_t step = (velocity * dt_ms) / 1000;
  int16_t stepFriction = (FRICTION * dt_ms) / 1000;
  int16_t stepRotation = TURN_SPEED * dt_ms * S3L_max(0, velocity - 200) / (MAX_VELOCITY * 1000);
  int16_t stepVelocity = S3L_nonZero((ACCELERATION * dt_ms) / 1000);

  if (stepRotation == 0 && S3L_abs(velocity) >= 200)
    stepRotation = 1;
  if (velocity < 0)
    stepRotation *= -1;

  if (keys & BUTTON_LEFT)
  {
    car->rotation.y += stepRotation;
    car->rotation.z = S3L_min(S3L_abs(velocity) / 64, car->rotation.z + 1);
  }
  else if (keys & BUTTON_RIGHT)
  {
    car->rotation.y -= stepRotation;
    car->rotation.z = S3L_max(-S3L_abs(velocity) / 64, car->rotation.z - 1);
  }
  else
    car->rotation.z = (car->rotation.z * 3) / 4;

  S3L_Vec4 carDirection;
  S3L_vec4Init(&carDirection);
  S3L_rotationToDirections(car->rotation, S3L_F, &carDirection, 0, 0);

  S3L_Vec4 previousCarPos = car->translation;
  int16_t friction = 0;

  if (keys & BUTTON_UP)
    velocity = S3L_min(MAX_VELOCITY, velocity + (velocity < 0 ? (2 * stepVelocity) : stepVelocity));
  else if (keys & BUTTON_DOWN)
    velocity = S3L_max(-MAX_VELOCITY, velocity - (velocity > 0 ? (2 * stepVelocity) : stepVelocity));
  else
    friction = 1;

  car->translation.x += (carDirection.x * step) / S3L_F;
  car->translation.z += (carDirection.z * step) / S3L_F;

  uint8_t coll = collision(car->translation);

  if (coll == 1)
  {
    handleCollision(&car->translation, previousCarPos);
    friction = 2;
  }
  else if (coll == 2)
  {
    car->translation.x += 5 * S3L_F;
    car->translation.z += 2 * S3L_F;
  }
  else if (coll != 0)
  {
    car->translation.x -= 5 * S3L_F;
    car->translation.z -= 2 * S3L_F;
  }

  if (velocity > 0)
    velocity = S3L_max(0, velocity - stepFriction * friction);
  else
    velocity = S3L_min(0, velocity + stepFriction * friction);

  S3L_Unit cameraDistance = S3L_interpolate(S3L_F / 2, (3 * S3L_F) / 4, S3L_abs(velocity), MAX_VELOCITY);

  scene.camera.transform.translation.x = car->translation.x - (carDirection.x * cameraDistance) / S3L_F;
  scene.camera.transform.translation.z = car->translation.z - (carDirection.z * cameraDistance) / S3L_F;
  scene.camera.transform.rotation.y = car->rotation.y;
}
