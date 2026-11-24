/**
  The classic tangram puzzle game for SAF. It is implemented in a simple way
  and contains slight inaccuracies that rule out certain valid tangram shapes.
  Only patterns that are legally safe (public domain) are included.

  by drummyfish, released under CC0 1.0, public domain
*/

#define SAF_SETTING_1BIT_DITHER 1
#define SAF_SETTING_FASTER_1BIT 0

#define SAF_PROGRAM_NAME "Tangram"
#define SAF_SETTING_ENABLE_SOUND 0 

#include "saf.h"

#define PATTERN_COLOR_LIGHT 0xdb
#define PATTERN_COLOR_MENU 0xba
#define CURSOR_COLOR SAF_COLOR_BLACK
#define SOLVED_COLOR 0x59
#define PIECE_BASE_COLOR 0x0e
#define PATTERNS 15                 // number of patterns

#if SAF_PLATFORM_COLOR_COUNT == 2
  #undef CURSOR_COLOR
  #define CURSOR_COLOR SAF_COLOR_WHITE
  #undef SOLVED_COLOR
  #define SOLVED_COLOR SAF_COLOR_BLACK
#endif

/*
  Pattern database follows. Each pattern is marked with one of the following
  sources (for legal safety):

  [0]: trivial shape, uncopyrightable
  [1]: 1917, Amusements in Mathematics
  [2]: 1899, The fashionable Chinese puzzle
  [3]: 1823, Ch'i ch'iao hsin p'u: ch'i chiao t'u chieh
*/

const uint8_t patterns[PATTERNS * 22] =
{
// x0  y0  r0   x1  y1  r1   x2  y2  r2   x3  y3  r3   x4  y4  r4   x5  y5  r5   x6  y6  r6   f6
   31, 27, 216, 23, 19, 152, 35, 38, 192, 23, 37, 0,   40, 26, 128, 27, 23, 232, 40, 44, 128, 0, // fan [3]
   17, 38, 88,  15, 27, 24,  43, 36, 64,  31, 35, 32,  21, 30, 160, 46, 28, 228, 38, 30, 0,   0, // cat [3]
   26, 29, 0,   33, 37, 64,  40, 40, 192, 39, 28, 128, 25, 42, 128, 28, 31, 230, 34, 41, 162, 0, // bird [3]
   30, 52, 128, 33, 12, 24,  29, 45, 192, 30, 39, 64,  30, 28, 128, 33, 16, 0,   24, 26, 0,   1, // monk [1]
   25, 34, 232, 33, 12, 24,  32, 46, 128, 33, 29, 224, 28, 39, 96,  33, 16, 0,   24, 26, 0,   1, // monk2 [1]
   30, 50, 192, 26, 45, 226, 41, 17, 192, 29, 34, 0,   27, 26, 32,  37, 25, 226, 35, 18, 192, 0, // flamingo [1]
   25, 42, 192, 13, 46, 64,  43, 32, 64,  39, 39, 96,  46, 47, 32,  19, 44, 244, 39, 26, 128, 0, // snake [3]
   41, 27, 226, 33, 36, 26,  38, 38, 128, 32, 24, 160, 25, 32, 96,  37, 31, 224, 30, 39, 0,   1, // square [0]
   35, 28, 224, 31, 24, 168, 29, 50, 192, 33, 43, 224, 28, 33, 96,  30, 20, 0,   33, 12, 206, 0, // candle [3]
   34, 11, 228, 30, 16, 88,  27, 53, 32,  28, 46, 64,  28, 35, 128, 31, 24, 224, 40, 24, 0,   0, // bunny [1]
   18, 25, 52,  45, 34, 192, 32, 35, 32,  39, 31, 224, 27, 32, 0,   45, 26, 0,   18, 33, 96,  1, // pig [1]
   21, 19, 24,  27, 14, 234, 33, 50, 192, 35, 41, 224, 30, 31, 96,  30, 19, 26,  21, 24, 218, 0, // indian [1]
   32, 29, 0,   25, 36, 0,   40, 32, 224, 48, 34, 192, 16, 34, 128, 25, 29, 0,   36, 29, 226, 0, // bridge [3]
   17, 34, 64,  23, 28, 128, 44, 31, 96,  32, 30, 0,   36, 33, 128, 24, 35, 0,   20, 28, 224, 0, // hexagon [0]
   25, 34, 0,   39, 30, 38,  40, 37, 128, 34, 36, 0,   23, 40, 128, 37, 23, 228, 36, 43, 0,   1  // sitting man [3]
};

#define PIECES 7    // number of basic pieces to make shapes from

typedef struct
{
  int8_t x;
  int8_t y;
  uint8_t rot;
  uint8_t flip;
} PieceTransform;

#define STATE_MENU 0
#define STATE_STARTING 1
#define STATE_PLAYING 2
#define STATE_SOLVED 3

PieceTransform pieceTransforms[PIECES];
PieceTransform patternTransforms[PIECES];
uint8_t shapeDiameters[PIECES]; // precomputed diameters for optimizations
uint8_t state;
uint8_t selectedPiece;
uint8_t selectedPattern;
uint8_t animationFrame;

#define ANIMATION_FRAMES 40

uint8_t pointIsInTri(int px, int py, int tp[6])
{
  // winding of the whole triangle:
  int w = (tp[3] - tp[1]) * (tp[4] - tp[2]) - (tp[2] - tp[0]) * (tp[5] - tp[3]);
  int sign = w > 0 ? 1 : (w < 0 ? -1 : 0);

  for (int i = 0; i < 3; ++i) // test winding of point with each side
  {
    int i1 = 2 * i;
    int i2 = i1 != 4 ? i1 + 2 : 0;

    int w2 = (tp[i1 + 1] - py) * (tp[i2] - tp[i1]) - 
      (tp[i1] - px) * (tp[i2 + 1] - tp[i1 + 1]);
    int sign2 = w2 > 0 ? 1 : (w2 < 0 ? -1 : 0);
      
    if (sign * sign2 == -1) // includes edges
      return 0;
  }
    
  return 1;
}

uint8_t pointIsInTransformedTri(int px, int py, int tp[6],
  const PieceTransform *t)
{
  if (t->flip)
  {
    tp[0] *= -1;
    tp[2] *= -1;
    tp[4] *= -1;
  }

  int s = SAF_sin(t->rot);
  int c = SAF_cos(t->rot);

  for (uint8_t i = 0; i < 6; i += 2)
  {
    int x = tp[i];
    int y = tp[i + 1];

    tp[i] = (x * c - y * s) / 127 + t->x;
    tp[i + 1] = (x * s + y * c) / 127 + t->y;
  }

  return pointIsInTri(px,py,tp);
}

uint8_t pointIsInShape(uint8_t shape, int8_t x, int8_t y,
  const PieceTransform *t)
{
  int coords[6];

  switch (shape)
  {
    case 0: // small triangle
    case 1:
      coords[0] = -3; coords[1] = -3;
      coords[2] = 3;  coords[3] = -3;
      coords[4] = -3; coords[5] = 3;

      return pointIsInTransformedTri(x,y,coords,t);
      break;

    case 2: // mid triangle
      coords[0] = -3; coords[1] = -3;
      coords[2] = 6;  coords[3] = -3;
      coords[4] = -3; coords[5] = 6;

      return pointIsInTransformedTri(x,y,coords,t);
      break;

    case 3: // big triangle
    case 4:
      coords[0] = -5; coords[1] = -5;
      coords[2] = 8; coords[3] = -5;
      coords[4] = -5; coords[5] = 8;

      return pointIsInTransformedTri(x,y,coords,t);
      break;

    case 5: // square
      coords[0] = -3; coords[1] = -3;
      coords[2] = 3;  coords[3] = -3;
      coords[4] = -3; coords[5] = 3;

      if (pointIsInTransformedTri(x,y,coords,t))
        return 1;

      coords[0] = 3;  coords[1] = 3;
      coords[2] = 3;  coords[3] = -3;
      coords[4] = -3; coords[5] = 3;

      return pointIsInTransformedTri(x,y,coords,t);
      break;

    case 6: // parallelogram
      coords[0] = -2; coords[1] = 2;
      coords[2] = 3;  coords[3] = -2;
      coords[4] = -6; coords[5] = -2;

      if (pointIsInTransformedTri(x,y,coords,t))
        return 1;

      coords[0] = -2;  coords[1] = 2;
      coords[2] = 3;  coords[3] = -2;
      coords[4] = 7; coords[5] = 2;

      return pointIsInTransformedTri(x,y,coords,t);
      break;

    default: break;
  }

  return 0;
}

void loadPattern(uint8_t n)
{
  const uint8_t *b = patterns + n * 22;

  for (uint8_t i = 0; i < PIECES; ++i)
  {
    patternTransforms[i].x = *b;
    b++;
    patternTransforms[i].y = *b;
    b++;
    patternTransforms[i].rot = *b;
    b++;
  }

  patternTransforms[PIECES - 1].flip = *b;
}

void resetShapes(void)
{
  selectedPiece = 0;

  for (int8_t i = 0; i < PIECES; ++i)
  {
    pieceTransforms[i].x = SAF_SCREEN_WIDTH / 2;
    pieceTransforms[i].y = SAF_SCREEN_HEIGHT / 2;
    pieceTransforms[i].rot = 0;
    pieceTransforms[i].flip = 0;
  }
}

void SAF_init(void)
{
  state = STATE_MENU;
  selectedPattern = 0;

  resetShapes();

  for (uint8_t i = 0; i < PIECES; ++i) // precompute shape diameters
  {
    shapeDiameters[i] = 0;

    for (uint8_t y = 0; y < SAF_SCREEN_HEIGHT; ++y)
      for (uint8_t x = 0; x < SAF_SCREEN_WIDTH; ++x)
        if (pointIsInShape(i,x,y,&pieceTransforms[i]))
        {
          int dx = pieceTransforms[i].x - x;

          if (dx < 0)
            dx *= -1;

          int dy = pieceTransforms[i].y - y;

          if (dy < 0)
            dy *= -1;

          dx += dy;

          if (dx > shapeDiameters[i])
            shapeDiameters[i] = dx;
        }
  }

  loadPattern(0);
}

uint8_t SAF_loop(void)
{
  SAF_clearScreen(SAF_COLOR_WHITE);

  uint8_t uncovered = state != STATE_PLAYING; /* whether there still remains an
                                                 uncovered pattern pixel */
  const PieceTransform *transforms = patternTransforms;
  uint8_t patternColor = 
#if SAF_PLATFORM_COLOR_COUNT == 2
    SAF_COLOR_GRAY_DARK;
#else
    state != STATE_MENU ? PATTERN_COLOR_LIGHT : PATTERN_COLOR_MENU;
#endif

  uint8_t drawPieces = (state == STATE_PLAYING) || (state == STATE_STARTING) ||
    (state == STATE_SOLVED && (SAF_frame() & 0x08));

  // draw pattern (j == 0) and potentially also the pieces (j == 1):
  for (uint8_t j = 0; j < 1 + drawPieces; ++j)
  {
    for (uint8_t i = 0; i < PIECES; ++i)
    {
      for (int8_t y = transforms[i].y - shapeDiameters[i];
                  y <= transforms[i].y + shapeDiameters[i]; ++y)
      {
        for (int8_t x = transforms[i].x - shapeDiameters[i];
                  x <= transforms[i].x + shapeDiameters[i]; ++x)
        {
          if (pointIsInShape(i,x,y,transforms + i))
          {
            if (j == 0)
            {
              SAF_drawPixel(x,y,patternColor);

              if (!uncovered)
              {
                uncovered = 1;

                for (uint8_t k = 0; k < PIECES; ++k)
                  if (pointIsInShape(k,x,y,pieceTransforms + k))
                  {
                    uncovered = 0;
                    break;
                  }
              } 
            }
            else
              SAF_drawPixel(x,y,
#if SAF_PLATFORM_COLOR_COUNT == 2
                SAF_COLOR_BLACK);
#else
                PIECE_BASE_COLOR + 32 * i);
#endif
          }
        }
      }
    }

    transforms = pieceTransforms;
  }

  if (state == STATE_PLAYING)
  {
    if (SAF_buttonPressed(SAF_BUTTON_A))
      SAF_drawCircle(pieceTransforms[selectedPiece].x,
        pieceTransforms[selectedPiece].y,2,CURSOR_COLOR,0);
    else
      for (int8_t d = -2; d <= 2; ++d) // draw cursor
      {
        SAF_drawPixel(pieceTransforms[selectedPiece].x + d,
          pieceTransforms[selectedPiece].y,CURSOR_COLOR);
        SAF_drawPixel(pieceTransforms[selectedPiece].x,
          pieceTransforms[selectedPiece].y + d,CURSOR_COLOR);
      }
  }

  if (state == STATE_MENU && ((SAF_load(selectedPattern / 8) >>
    (selectedPattern % 8)) & 0x01)) // pattern was solved?
  {
    SAF_drawLine(48,10,53,15,SOLVED_COLOR);
    SAF_drawLine(49,10,54,15,SOLVED_COLOR);
    SAF_drawLine(53,14,58,4,SOLVED_COLOR);
    SAF_drawLine(54,14,59,4,SOLVED_COLOR);
  }

  if (state == STATE_PLAYING && !uncovered)
  {
    state = STATE_SOLVED;

    SAF_save(selectedPattern / 8,SAF_load(selectedPattern / 8) |
      (0x01 << (selectedPattern % 8)));
  }

  // when making new patterns, use the following to print it to console:

#if 0
  if (SAF_frame() % 32 == 0)
  {
    for (uint8_t i = 0; i < PIECES; ++i)
      printf("%d, %d, %d, ",pieceTransforms[i].x,pieceTransforms[i].y,
        pieceTransforms[i].rot);

    printf("%d,\n",pieceTransforms[PIECES - 1].flip);
  }
#endif

  switch (state) // state transitions
  {
    case STATE_PLAYING:
      if (SAF_buttonJustPressed(SAF_BUTTON_B))
        selectedPiece = (selectedPiece + 1) % PIECES;

      if (SAF_buttonPressed(SAF_BUTTON_A))
      {
        if (SAF_buttonPressed(SAF_BUTTON_LEFT))
          pieceTransforms[selectedPiece].rot -= 2;
        else if (SAF_buttonPressed(SAF_BUTTON_RIGHT))
          pieceTransforms[selectedPiece].rot += 2;
        else if (SAF_buttonJustPressed(SAF_BUTTON_DOWN) ||
          SAF_buttonJustPressed(SAF_BUTTON_UP))
          pieceTransforms[selectedPiece].flip ^= 0x01;
      }
      else
      {
        if (SAF_buttonPressed(SAF_BUTTON_LEFT) &&
          pieceTransforms[selectedPiece].x > 0)
          pieceTransforms[selectedPiece].x--;
        else if (SAF_buttonPressed(SAF_BUTTON_RIGHT) &&
          pieceTransforms[selectedPiece].x < SAF_SCREEN_WIDTH - 1)
          pieceTransforms[selectedPiece].x++;
        else if (SAF_buttonPressed(SAF_BUTTON_UP) &&
          pieceTransforms[selectedPiece].y > 0)
          pieceTransforms[selectedPiece].y--;
        else if (SAF_buttonPressed(SAF_BUTTON_DOWN) &&
          pieceTransforms[selectedPiece].y < SAF_SCREEN_HEIGHT - 1)
          pieceTransforms[selectedPiece].y++;
      }

      if (SAF_buttonPressed(SAF_BUTTON_B) >= 60)
        state = STATE_MENU;

      break;

    case STATE_MENU:
      if (SAF_buttonJustPressed(SAF_BUTTON_RIGHT))
      {
        selectedPattern = (selectedPattern + 1) % PATTERNS;
        loadPattern(selectedPattern);
      }
      else if (SAF_buttonJustPressed(SAF_BUTTON_LEFT))
      {
        selectedPattern = selectedPattern == 0 ? PATTERNS - 1 :
          (selectedPattern - 1);
        loadPattern(selectedPattern);
      }
      else if (SAF_buttonJustPressed(SAF_BUTTON_A))
      {
        resetShapes();
        animationFrame = 0;
        state = STATE_STARTING;
      }

      break;

    case STATE_STARTING:
      for (int8_t i = 0; i < PIECES; ++i)
      {
        int angle = (i * 256) / PIECES + SAF_frame() * 16;

        pieceTransforms[i].x = SAF_SCREEN_WIDTH / 2 + 
          SAF_sin(angle) / (ANIMATION_FRAMES + 4 - animationFrame);
        pieceTransforms[i].y = SAF_SCREEN_HEIGHT / 2 + 
          SAF_cos(angle) / (ANIMATION_FRAMES + 4 - animationFrame);
      }

      animationFrame++;

      if (animationFrame >= ANIMATION_FRAMES)
        state = STATE_PLAYING;

      break;

    case STATE_SOLVED:
      if (SAF_buttonJustPressed(SAF_BUTTON_A) ||
        SAF_buttonJustPressed(SAF_BUTTON_B))
      {
        state = STATE_MENU;
      }

      break;

    default: break;
  }

  return 1;
}
