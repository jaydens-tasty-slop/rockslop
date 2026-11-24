/* tinyphysicsengine "cubes" demo: a wrecking ball hits a pile of boxes.
 * Based on programs/cubes.c by Miloslav Ciz, CC0 1.0. */
#define S3LRB_NAME "Cubes"
#define S3LRB_MENU_MS 1000
#define S3LRB_HINT "Arrows=look"
#define S3LRB_SETTINGS cubes_settings
#define S3LRB_HELP "Wrecking ball and", "boxes physics demo.", "", "Arrows: turn camera.", \
  "SELECT+arrows: move", "camera.", "Hold SEL+UP 1 s: menu", "(SEL+UP also moves", "the camera).", "Settings: explode the", \
  "cubes or reset."
#define S3LRB_ABOUT "Cubes demo from", "tinyphysicsengine by", "drummyfish (Miloslav", "Ciz), CC0 1.0.", \
  "gitlab.com/drummyfish/", "tinyphysicsengine"
#include "tpe_helper.h"

#define ROOM_SIZE (14 * TPE_F)
#define CUBE_SIZE (3 * TPE_F / 2)
#define GRAVITY (TPE_F / 100)

static TPE_Vec3 cubeOrientations[6];
static TPE_Vec3 cubePositions[6];
static TPE_Joint ballJoints[4];
static TPE_Connection ballConnections[3];
static bool explode, reset;

static TPE_Vec3 environmentDistance(TPE_Vec3 p, TPE_Unit maxD)
{
  (void)maxD;
  return TPE_envAABoxInside(p, TPE_vec3(0, ROOM_SIZE / 4, 0),
    TPE_vec3(ROOM_SIZE, ROOM_SIZE / 2, ROOM_SIZE));
}

static void update_orient_pos(int i)
{
  cubeOrientations[i] = TPE_bodyGetRotation(&tpe_world.bodies[i], 0, 2, 1);
  cubePositions[i] = TPE_bodyGetCenterOfMass(&tpe_world.bodies[i]);
}

static void build_scene(void)
{
  tpe_reset();
  tpe_world.environmentFunction = environmentDistance;

  tpe_scene.camera.transform.translation.z -= ROOM_SIZE / 2;
  tpe_scene.camera.transform.translation.y += ROOM_SIZE / 3;
  tpe_scene.camera.transform.translation.x -= ROOM_SIZE / 4;
  tpe_scene.camera.transform.rotation.y = -1 * TPE_F / 16;

  for (int i = 0; i < 6; ++i)
  {
    tpe_addBox(CUBE_SIZE / 2, CUBE_SIZE / 2, CUBE_SIZE / 2, CUBE_SIZE / 4, TPE_F / 5);
    tpe_lastBody.friction = TPE_F / 20;
  }

#define MOVE(i, x, y) \
  TPE_bodyMoveBy(&tpe_world.bodies[i], TPE_vec3((CUBE_SIZE / 2 + 10) * x, 10 + CUBE_SIZE / 2 + y * (CUBE_SIZE + 10), 0));
  MOVE(0, 0, 0)
  MOVE(1, -2, 0)
  MOVE(2, 2, 0)
  MOVE(3, -1, 1)
  MOVE(4, 1, 1)
  MOVE(5, 0, 2)
#undef MOVE

  for (int i = 0; i < 6; ++i)
  {
    update_orient_pos(i);
    /* the cubes are made of spheres and would not stay stacked, so keep them
       inactive until the ball hits them */
    TPE_bodyDeactivate(&tpe_bodies[i]);
  }

  ballJoints[0] = TPE_joint(TPE_vec3(0, ROOM_SIZE / 2, 0), 0);
  ballJoints[1] = TPE_joint(TPE_vec3(0, ROOM_SIZE / 2 - TPE_F / 5, -1 * TPE_F), 0);
  ballJoints[2] = TPE_joint(TPE_vec3(0, ROOM_SIZE / 2 - TPE_F / 2, -2 * TPE_F), 0);
  ballJoints[3] = TPE_joint(TPE_vec3(0, ROOM_SIZE / 2 - TPE_F, -4 * TPE_F), TPE_F);
  ballJoints[3].velocity[1] = -1 * TPE_F / 2;
  ballJoints[3].velocity[2] = -1 * TPE_F / 2;

  ballConnections[0].joint1 = 0; ballConnections[0].joint2 = 1;
  ballConnections[1].joint1 = 1; ballConnections[1].joint2 = 2;
  ballConnections[2].joint1 = 2; ballConnections[2].joint2 = 3;

  TPE_Body *ball = &tpe_world.bodies[6];

  TPE_bodyInit(ball, ballJoints, 4, ballConnections, 3, 2 * TPE_F);
  ball->flags |= TPE_BODY_FLAG_SIMPLE_CONN;
  ball->flags |= TPE_BODY_FLAG_ALWAYS_ACTIVE;
  ball->friction = 0;
  ball->elasticity = TPE_F;

  tpe_world.bodyCount++;
}

static void cubes_settings(void)
{
  explode = false;
  reset = false;
  rb->set_bool("Explode the cubes", &explode);
  rb->set_bool("Reset the scene", &reset);
}

static void demo_init(void)
{
  tpe_init();
  build_scene();
}

static void demo_frame(unsigned keys, int dt_ms)
{
  TPE_Body *ball = &tpe_world.bodies[6];

  if (reset)
  {
    reset = false;
    build_scene();
    ball = &tpe_world.bodies[6];
  }

  int steps = tpe_frameStart(dt_ms);
  tpe_cameraFreeMovement(keys, dt_ms);

  if (explode)
  {
    explode = false;
    for (int i = 0; i < 6; ++i)
    {
      TPE_bodyActivate(&tpe_world.bodies[i]);
      TPE_bodyAccelerate(&tpe_world.bodies[i],
        TPE_vec3Plus(TPE_vec3(0, TPE_F / 5, 0), TPE_vec3Times(cubePositions[i], TPE_F / 4)));
    }
  }

  while (steps-- > 0)
  {
    /* pin the top of the wrecking ball to the ceiling */
    TPE_jointPin(&ball->joints[0], TPE_vec3(0, ROOM_SIZE / 2 - TPE_F / 100, 0));
    TPE_worldStep(&tpe_world);

    for (int i = 0; i < 6; ++i)
      TPE_bodyApplyGravity(&tpe_world.bodies[i], GRAVITY);

    ball->joints[3].velocity[1] -= GRAVITY; /* gravity only on one joint */
  }

  tpe_color(170, 170, 170);
  tpe_drawBoxInside(TPE_vec3(0, ROOM_SIZE / 4, 0), TPE_vec3(ROOM_SIZE, ROOM_SIZE / 2, ROOM_SIZE), TPE_vec3(0, 0, 0));

  tpe_color(200, 10, 10);
  for (int i = 0; i < 6; ++i)
  {
    if (TPE_bodyIsActive(&tpe_world.bodies[i]))
      update_orient_pos(i);

    tpe_drawBox(cubePositions[i], TPE_vec3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE), cubeOrientations[i]);
  }

  tpe_drawSphere(ball->joints[3].position, TPE_vec3(400, 400, 400), TPE_vec3(0, 0, 0));

  for (int i = 0; i < 3; ++i)
    tpe_drawLine3D(ball->joints[ball->connections[i].joint1].position,
                   ball->joints[ball->connections[i].joint2].position, 255, 0, 0);
}
