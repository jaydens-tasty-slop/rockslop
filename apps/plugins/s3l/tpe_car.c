/* tinyphysicsengine drivable car demo. Based on programs/car.c by Miloslav
 * Ciz, CC0 1.0. */
#define S3LRB_NAME "Car"
#define S3LRB_HINT "Up/Down=drive  L/R=steer"
#define S3LRB_HELP "Drive the physics", "car round the arena.", "", "UP: accelerate.", \
  "DOWN: reverse.", "LEFT/RIGHT: steer.", "Hold SEL+UP: menu.", "", \
  "Wheels only grip while", "touching the ground."
#define S3LRB_ABOUT "Car demo from", "tinyphysicsengine by", "drummyfish (Miloslav", "Ciz), CC0 1.0.", \
  "gitlab.com/drummyfish/", "tinyphysicsengine"
#define S3L_PERSPECTIVE_CORRECTION 0
#define S3L_NEAR_CROSS_STRATEGY 2
#include "tpe_helper.h"
#include "tpe_arenaModel.h"
#include "tpe_carModel.h"

#define ACCELERATION (TPE_F / 14)
#define TURN_RATE (3 * TPE_F / 4)
#define TURN_FRICTION (3 * TPE_F / 4) /* wheel side friction */
#define FORW_FRICTION (TPE_F / 14)    /* wheel forward friction */

static TPE_Unit rampPoints[6] = { 0, 0, -2400, 1400, -2400, 0 };

static uint8_t steering; /* 0: none, 1: right, 2: left */
static uint8_t jointCollisions, jointCollisionsPrev; /* touching wheels */
static TPE_Body *carBody;
static TPE_Vec3 carForw, carSide, carUp, carPos, carRot;

static TPE_Vec3 environmentDistance(TPE_Vec3 p, TPE_Unit maxD)
{
  (void)maxD;
  TPE_ENV_START( TPE_envGround(p, 0), p )
  TPE_ENV_NEXT( TPE_envSphereInside(p, TPE_vec3(0, 10000, 0), 20000), p )
  TPE_ENV_NEXT( TPE_envAABox(p, TPE_vec3(-8700, 100, -800), TPE_vec3(2200, 1000, 800)), p )
  TPE_ENV_NEXT( TPE_envAATriPrism(p, TPE_vec3(8700, 0, 0), rampPoints, 5000, 2), p )
  TPE_ENV_NEXT( TPE_envSphere(p, TPE_vec3(0, -200, 0), 1700), p )
  TPE_ENV_END
}

static uint8_t collisionCallback(uint16_t b1, uint16_t j1, uint16_t b2, uint16_t j2, TPE_Vec3 p)
{
  (void)j2; (void)p;
  /* record which wheels (joints) currently touch the ground */
  if (b1 == 1 && b1 == b2 && j1 < 4)
    jointCollisions |= 0x01 << j1;

  return 1;
}

static void accelerate_back_wheels(int sign)
{
  for (int j = 0; j < 2; ++j)
  {
    carBody->joints[j].velocity[0] += sign * (carForw.x * ACCELERATION) / TPE_F;
    carBody->joints[j].velocity[1] += sign * (carForw.y * ACCELERATION) / TPE_F;
    carBody->joints[j].velocity[2] += sign * (carForw.z * ACCELERATION) / TPE_F;
  }
}

static void demo_init(void)
{
  arenaModelInit();
  carModelInit();
  tpe_init();

  carPos = TPE_vec3(0, 0, 0);

  tpe_world.environmentFunction = environmentDistance;
  tpe_world.collisionCallback = collisionCallback;

  /* an interactive box */
  tpe_addRect(6 * TPE_F / 5, 6 * TPE_F / 5, 6 * TPE_F / 5, TPE_F / 2);
  TPE_bodyMoveBy(&tpe_lastBody, TPE_vec3(2000, 1000, 3000));
  tpe_lastBody.friction = TPE_F / 5;

  /* the car: a "center rect" */
  tpe_addCenterRectFull(1000, 1800, 400, 2000);
  carBody = &tpe_lastBody;

  carBody->joints[4].position.y += 600;
  carBody->joints[4].sizeDivided *= 3;
  carBody->joints[4].sizeDivided /= 2;

  /* reinit so the body recomputes its connection lengths */
  TPE_bodyInit(carBody, carBody->joints, carBody->jointCount, carBody->connections,
               carBody->connectionCount, TPE_F / 2);

  TPE_bodyMoveBy(carBody, TPE_vec3(6 * TPE_F, 2 * TPE_F, 0));
  carBody->elasticity = TPE_F / 100;
  carBody->friction = FORW_FRICTION;
  carBody->flags |= TPE_BODY_FLAG_ALWAYS_ACTIVE;

  tpe_scene.camera.transform.rotation.x = -35;
}

static void car_step(unsigned keys)
{
  jointCollisions = 0;

  steering = (keys & BUTTON_RIGHT) ? 1 : ((keys & BUTTON_LEFT) ? 2 : 0);

  TPE_worldStep(&tpe_world);

  carPos = TPE_vec3KeepWithinBox(carPos, carBody->joints[4].position,
    TPE_vec3(TPE_F / 50, TPE_F / 50, TPE_F / 50));

  /* direction vectors from the joint positions */
  carForw = TPE_vec3Normalized(TPE_vec3Plus(
    TPE_vec3Minus(carBody->joints[2].position, carBody->joints[0].position),
    TPE_vec3Minus(carBody->joints[3].position, carBody->joints[1].position)));

  carSide = TPE_vec3Normalized(TPE_vec3Plus(
    TPE_vec3Minus(carBody->joints[1].position, carBody->joints[0].position),
    TPE_vec3Minus(carBody->joints[3].position, carBody->joints[2].position)));

  carUp = TPE_vec3Cross(carForw, carSide);

  /* directional wheel friction, which TPE itself doesn't have */
  for (int i = 0; i < 4; ++i)
    if (jointCollisions & (0x01 << i))
    {
      TPE_Vec3 jv = TPE_vec3(carBody->joints[i].velocity[0],
                             carBody->joints[i].velocity[1],
                             carBody->joints[i].velocity[2]);
      TPE_Vec3 ja = carSide; /* wheel axis */

      if (i >= 2 && steering) /* front wheels: tilt the axis when turning */
      {
        if (steering == 2)
          ja = TPE_vec3Plus(TPE_vec3Times(carForw, TURN_RATE), carSide);
        else
          ja = TPE_vec3Minus(TPE_vec3Times(carForw, TURN_RATE), carSide);

        ja = TPE_vec3Normalized(ja);
      }

      TPE_Vec3 fric = TPE_vec3Times(ja, (TPE_vec3Dot(ja, jv) * TURN_FRICTION) / TPE_F);

      jv = TPE_vec3Minus(jv, fric);
      carBody->joints[i].velocity[0] = jv.x;
      carBody->joints[i].velocity[1] = jv.y;
      carBody->joints[i].velocity[2] = jv.z;
    }

  if (TPE_vec3Dot(carUp, TPE_vec3Minus(carBody->joints[4].position,
                                       carBody->joints[0].position)) < 0)
  {
    /* on its roof the center joint may flip to the other side: fix it */
    carBody->joints[4].position = TPE_vec3Plus(TPE_vec3Times(carUp, 300),
                                               carBody->joints[0].position);
  }

  for (int i = 0; i < tpe_world.bodyCount; ++i)
    TPE_bodyApplyGravity(&tpe_world.bodies[i], 5);

  if ((jointCollisions | jointCollisionsPrev) & 0x03) /* back wheels grip */
  {
    if (keys & BUTTON_UP)
      accelerate_back_wheels(1);
    else if (keys & BUTTON_DOWN)
      accelerate_back_wheels(-1);
  }

  jointCollisionsPrev = jointCollisions;
}

static void demo_frame(unsigned keys, int dt_ms)
{
  int steps = tpe_frameStart(dt_ms);

  while (steps-- > 0)
    car_step(keys);

  carRot = TPE_bodyGetRotation(carBody, 0, 2, 1);

  tpe_color(20, 150, 150);
  tpe_drawModel(&arenaModel, TPE_vec3(0, 0, 0), TPE_vec3(512 * 32, 512 * 32, 512 * 32), TPE_vec3(0, 0, 0));

  tpe_color(20, 50, 250);
  tpe_drawBox(TPE_bodyGetCenterOfMass(&tpe_world.bodies[0]), TPE_vec3(1000, 800, 1000),
              TPE_bodyGetRotation(&tpe_world.bodies[0], 0, 2, 1));

  S3L_zBufferClear();

  tpe_color(200, 200, 200);
  tpe_drawModel(&carModel, TPE_vec3Minus(carPos, TPE_vec3Times(carUp, 400)),
                TPE_vec3(600, 600, 600), carRot);

  /* camera follows the car */
  S3L_Transform3D *ct = &tpe_scene.camera.transform;

  ct->translation.y = carPos.y + 800;

  TPE_Vec3 cPos = TPE_vec3KeepWithinDistanceBand(
    TPE_vec3(ct->translation.x, ct->translation.y, ct->translation.z),
    carBody->joints[4].position, 4 * TPE_F, 6 * TPE_F);

  ct->translation.x = cPos.x;
  ct->translation.y = cPos.y;
  ct->translation.z = cPos.z;

  TPE_Unit angleDiff = ct->rotation.y -
    (TPE_vec2Angle(carPos.x - ct->translation.x, carPos.z - ct->translation.z) - 128);

  ct->rotation.y -= (angleDiff < 100 && angleDiff > -100) ? angleDiff / 2 : angleDiff;
}
