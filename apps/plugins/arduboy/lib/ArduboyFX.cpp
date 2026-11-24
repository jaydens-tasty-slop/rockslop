/* Arduboy FX library (Mr.Blinky, CC0) on a data file instead of the SPI flash
 * chip: the SPI data register is emulated by ab_glue.c (ab_fx_read), so the
 * library's portable C++ code runs unchanged. See ../README.md. */
#include "ArduboyFX.h"

uint16_t ab_fx_state_dummy;

/* --- emulated flash chip: a command parser over a single read stream --- */

static bool fx_selected;
static uint8_t fx_cmd_state;       /* 0: idle, 1: command byte seen, 2..4: address bytes */
static uint8_t fx_command;
static uint32_t fx_addr;
static uint32_t fx_pos;
static uint8_t fx_latch;

void fx_select(bool on)
{
  fx_selected = on;
  fx_cmd_state = 0;
}

FxSpiReg SPDR;

FxSpiReg::operator uint8_t() const
{
  return fx_latch;
}

FxSpiReg &FxSpiReg::operator=(uint8_t v)
{
  if (!fx_selected) {
    fx_latch = 0;
  } else if (fx_cmd_state == 0) {
    fx_command = v;
    fx_cmd_state = 1;
    fx_addr = 0;
    fx_latch = 0;
  } else if (fx_command == SFC_READ && fx_cmd_state < 4) {
    fx_addr = (fx_addr << 8) | v;
    fx_cmd_state++;
    if (fx_cmd_state == 4)
      fx_pos = fx_addr;
    fx_latch = 0;
  } else if (fx_command == SFC_READ) {
    uint8_t b = 0;
    ab_fx_read(fx_pos++, &b, 1);
    fx_latch = b;
  } else {
    fx_latch = 0;
  }
  return *this;
}

uint8_t FX::writeByte(uint8_t data)
{
  uint8_t result = fx_latch;
  SPDR = data;
  return result;
}


uint8_t FX::readByte()
{
  return writeByte(0);
}

uint16_t FX::programDataPage; // program read only data location in flash memory
uint16_t FX::programSavePage; // program read and write data location in flash memory
Font     FX::font;
Cursor   FX::cursor = {0,0,0,WIDTH};

FrameControl FX::frameControl;


void FX::begin()
{
  wakeUp();
}


/* The data file is addressed from offset 0: the flash page numbers the game
 * passes (programDataPage/programSavePage) are not used. */
void FX::begin(uint16_t developmentDataPage)
{
  (void)developmentDataPage;
  programDataPage = 0;
  wakeUp();
}


void FX::begin(uint16_t developmentDataPage, uint16_t developmentSavePage)
{
  (void)developmentDataPage;
  (void)developmentSavePage;
  programDataPage = 0;
  programSavePage = 0;
  wakeUp();
}

void FX::readJedecID(JedecID & id)
{
  enable();
  writeByte(SFC_JEDEC_ID);
  id.manufacturer = readByte();
  id.device = readByte();
  id.size = readByte();
  disable();
}

void FX::readJedecID(JedecID* id)
{
  enable();
  writeByte(SFC_JEDEC_ID);
  id -> manufacturer = readByte();
  id -> device = readByte();
  id -> size = readByte();
  disable();
}

bool FX::detect()
{
  seekCommand(SFC_READ, 0);
  SPDR = 0;
  return readPendingLastUInt16() == 0x4152;
}


void FX::writeCommand(uint8_t command)
{
  enable();
  writeByte(command);
  disable();
}

void FX::wakeUp()
{
  writeCommand(SFC_RELEASE_POWERDOWN);
}


void FX::sleep()
{
  writeCommand(SFC_POWERDOWN);
}

void FX::writeEnable()
{
  writeCommand(SFC_WRITE_ENABLE);
}


void FX::seekCommand(uint8_t command, uint24_t address)
{
  enable();
  writeByte(command);
  writeByte(address >> 16);
  writeByte(address >> 8);
  writeByte(address);
}


void FX::seekData(uint24_t address)
{
  seekCommand(SFC_READ, address);
  SPDR = 0;
}


void FX::seekDataArray(uint24_t address, uint8_t index, uint8_t offset, uint8_t elementSize)
{
  address += elementSize ? index * elementSize + offset : index * 256 + offset;
  seekData(address);
}


void FX::seekSave(uint24_t address)
{
  seekCommand(SFC_READ, address);
  SPDR = 0;
}


uint8_t FX::readPendingUInt8()
{
  wait();
  uint8_t result = SPDR;
  SPDR = 0;
  return result;
}


uint8_t FX::readPendingLastUInt8()
{
  wait();                 // wait for a pending read to complete
  return readUnsafeEnd(); // read last byte and disable flash
}


uint16_t FX::readPendingUInt16()
{
  /* FX data is big endian; the reads must happen in this order */
  uint16_t hi = readPendingUInt8();
  uint16_t lo = readPendingUInt8();
  return (hi << 8) | lo;
}


uint16_t FX::readPendingLastUInt16()
{
  uint16_t hi = readPendingUInt8();
  uint16_t lo = readEnd();
  return (hi << 8) | lo;
}


uint24_t FX::readPendingUInt24()
{
  uint24_t hi = readPendingUInt16();
  uint24_t lo = readPendingUInt8();
  return (hi << 8) | lo;
}


uint24_t FX::readPendingLastUInt24()
{
  uint24_t hi = readPendingUInt16();
  uint24_t lo = readEnd();
  return (hi << 8) | lo;
}


uint32_t FX::readPendingUInt32()
{
  uint32_t hi = readPendingUInt16();
  uint32_t lo = readPendingUInt16();
  return (hi << 16) | lo;
}


uint32_t FX::readPendingLastUInt32()
{
  uint32_t hi = readPendingUInt16();
  uint32_t lo = readPendingLastUInt16();
  return (hi << 16) | lo;
}


void FX::readBytes(uint8_t* buffer, size_t length)
{
  for (size_t i = 0; i < length; i++)
  {
    buffer[i] = readPendingUInt8();
  }
}


void FX::readBytesEnd(uint8_t* buffer, size_t length)
{
  for (size_t i = 0; i + 1 < length; i++)
  {
    buffer[i] = readPendingUInt8();
  }
  if (length)
    buffer[length - 1] = readEnd();
  else
    readEnd();
}


uint8_t FX::readEnd()
{
  wait();                 // wait for a pending read to complete
  return readUnsafeEnd(); // read last byte and disable flash
}


void FX::readDataBytes(uint24_t address, uint8_t* buffer, size_t length)
{
  seekData(address);
  readBytesEnd(buffer, length);
}


void FX::readSaveBytes(uint24_t address, uint8_t* buffer, size_t length)
{
  seekSave(address);
  readBytesEnd(buffer, length);
}

uint8_t FX::loadGameState(uint8_t* gameState, size_t size)
{
  return ab_fx_load_state(gameState, size);
}

void FX::saveGameState(const uint8_t* gameState, size_t size)
{
  ab_fx_save_state(gameState, size);
}

void FX::eraseSaveBlock(uint16_t page)
{
  (void)page;
}

void FX::writeSavePage(uint16_t page, uint8_t* buffer)
{
  (void)page;
  (void)buffer;
}

void FX::waitWhileBusy()
{
}


void FX::drawBitmap(int16_t x, int16_t y, uint24_t address, uint8_t frame, uint8_t mode)
{
  // read bitmap dimensions from flash
  seekData(address);
  int16_t width  = readPendingUInt16();
  int16_t height = readPendingLastUInt16();
  // return if the bitmap is completely off screen
  if (x + width <= 0 || x >= WIDTH || y + height <= 0 || y >= HEIGHT) return;

  // determine render width
  int16_t skipleft = 0;
  uint8_t renderwidth;
  if (x<0)
  {
    skipleft = -x;
    if (width - skipleft < WIDTH) renderwidth = width - skipleft;
    else renderwidth = WIDTH;
  }
  else
  {
    if (x + width > WIDTH) renderwidth = WIDTH - x;
    else renderwidth = width;
  }

  //determine render height
  int16_t skiptop;     // pixel to be skipped at the top
  int8_t renderheight; // in pixels
  if (y < 0)
  {
    skiptop = -y & -8; // optimized -y / 8 * 8
    if (height - skiptop <= HEIGHT) renderheight = height - skiptop;
    else renderheight = HEIGHT + (-y & 7);
    skiptop = fastDiv8(skiptop); // pixels to displayrows
  }
  else
  {
    skiptop = 0;
    if (y + height > HEIGHT) renderheight = HEIGHT - y;
    else renderheight = height;
  }
  uint24_t offset = (uint24_t)(frame * (fastDiv8((uint16_t)(height + 7))) + skiptop) * width + skipleft;
  if (mode & dbmMasked)
  {
    offset += offset; // double for masked bitmaps
    width += width;
  }
  address += offset + 4; // skip non rendered pixels, width, height
  int8_t displayrow = (y >> 3) + skiptop;
  uint16_t displayoffset = displayrow * WIDTH + x + skipleft;
  if (mode & dbmFlip) displayoffset += renderwidth - 1;
  uint8_t yshift = bitShiftLeftUInt8(y); //shift by multiply
  uint8_t lastmask = bitShiftRightMaskUInt8(8 - height); // mask for bottom most pixels
  do
  {
    seekData(address);
    address += width;
    mode &= ~((1 << dbfExtraRow));
    if (yshift != 1 && displayrow < (HEIGHT / 8 - 1)) mode |= (1 << dbfExtraRow);
    uint8_t rowmask = 0xFF;
    if (renderheight < 8) rowmask = lastmask;
    wait();
    for (uint8_t c = 0; c < renderwidth; c++)
    {
      uint8_t bitmapbyte = readUnsafe();
      if (mode & (1 << dbfReverseBlack)) bitmapbyte ^= rowmask;
      uint8_t maskbyte = rowmask;
      if (mode & (1 << dbfWhiteBlack)) maskbyte = bitmapbyte;
      if (mode & (1 << dbfBlack)) bitmapbyte = 0;
      uint16_t bitmap = multiplyUInt8(bitmapbyte, yshift);
      if (mode & (1 << dbfMasked))
      {
        wait();
        uint8_t tmp = readUnsafe();
        if ((mode & dbfWhiteBlack) == 0) maskbyte = tmp;
      }
      uint16_t mask = multiplyUInt8(maskbyte, yshift);
      if (displayrow >= 0)
      {
        uint8_t pixels = bitmap;
        uint8_t display = Arduboy2Base::sBuffer[displayoffset];
        if ((mode & (1 << dbfInvert)) == 0) pixels ^= display;
        pixels &= mask;
        pixels ^= display;
        Arduboy2Base::sBuffer[displayoffset] = pixels;
      }
      if (mode & (1 << dbfExtraRow))
      {
        uint8_t display = Arduboy2Base::sBuffer[(uint16_t)(displayoffset + WIDTH)];
        uint8_t pixels = bitmap >> 8;
        if ((mode & dbfInvert) == 0) pixels ^= display;
        pixels &= mask >> 8;
        pixels ^= display;
        Arduboy2Base::sBuffer[(uint16_t)(displayoffset + WIDTH)] = pixels;
      }
      (mode & (1 << dbfFlip)) ? displayoffset-- : displayoffset++;
    }
    displayoffset += WIDTH;
    (mode & (1 << dbfFlip)) ?  displayoffset += renderwidth : displayoffset -= renderwidth;
    displayrow ++;
    renderheight -= 8;
    readEnd();
  } while (renderheight > 0);
}

void FX::setFrame(uint24_t frame, uint8_t repeat) //~22 bytes
{
  frameControl.start   = frame;
  frameControl.current = frame;
  frameControl.repeat  = repeat;
  frameControl.count   = repeat;
}

uint8_t FX::drawFrame() // ~66 bytes
{
  uint24_t frame = drawFrame(frameControl.current);
  uint8_t moreFrames;
  moreFrames = (frame != 0) | frameControl.count;
  if (frameControl.count > 0)
  {
    frameControl.count--;
  }
  else
  {
    frameControl.count = frameControl.repeat;
    if (!moreFrames) frame = frameControl.start;
    frameControl.current = frame;
  }
  return moreFrames;
}

uint24_t FX::drawFrame(uint24_t address) //~94 bytes
{
  FrameData f;
  for(;;)
  {
    seekData(address);
    address += 9; /* sizeof(FrameData) on AVR: uint24_t is 3 bytes there */
    f.x = readPendingUInt16();
    f.y = readPendingUInt16();
    f.bmp = readPendingUInt24();
    f.frame = readPendingUInt8();
    f.mode = readEnd();
    drawBitmap(f.x, f.y, f.bmp, f.frame, f.mode);
    if (f.mode & dbmEndFrame) return address;
    if (f.mode & dbmLastFrame) return 0;
  }
}

void FX::readDataArray(uint24_t address, uint8_t index, uint8_t offset, uint8_t elementSize, uint8_t* buffer, size_t length)
{
  seekDataArray(address, index, offset, elementSize);
  readBytesEnd(buffer, length);
}


uint8_t FX::readIndexedUInt8(uint24_t address, uint8_t index)
{
  seekDataArray(address, index, 0, sizeof(uint8_t));
  return readEnd();
}


uint16_t FX::readIndexedUInt16(uint24_t address, uint8_t index)
{
  seekDataArray(address, index, 0, sizeof(uint16_t));
  return readPendingLastUInt16();
}


uint24_t FX::readIndexedUInt24(uint24_t address, uint8_t index)
{
  seekDataArray(address, index, 0, 3);
  return readPendingLastUInt24();
}


uint32_t FX::readIndexedUInt32(uint24_t address, uint8_t index)
{
  seekDataArray(address, index, 0, 4);
  return readPendingLastUInt32();
}

void FX::display()
{
  enableOLED();
  Arduboy2Base::display();
  disableOLED();
}

void FX::display(bool clear)
{
  enableOLED();
  Arduboy2Base::display(clear);
  disableOLED();
}

void FX::setFont(uint24_t address, uint8_t mode)
{
  font.address = address;
  font.mode = mode;
  seekData(address);
  font.width = readPendingUInt16();
  font.height = readPendingLastUInt16();
}

void FX::setFontMode(uint8_t mode)
{
  font.mode = mode;
}

void FX::setCursor(int16_t x, int16_t y)
{
  cursor.x = x;
  cursor.y = y;
}

void FX::setCursorX(int16_t x)
{
  cursor.x = x;
}

void FX::setCursorY(int16_t y)
{
  cursor.y = y;
}

void FX::setCursorRange(int16_t left, int16_t wrap)
{
  cursor.left = left;
  cursor.wrap = wrap;
}

void FX::setCursorLeft(int16_t left)
{
  cursor.left = left;
}

void FX::setCursorWrap(int16_t wrap)
{
  cursor.wrap = wrap;
}

void FX::drawChar(uint8_t c)
{
  if (c == '\r') return;
  uint8_t mode = font.mode;
  int16_t x = cursor.x;
  int16_t y = cursor.y;
  if (c != '\n')
  {
    drawBitmap(x, y, font.address, c, mode);
    if (mode & dcmProportional)
    {
      seekData(font.address - 256 + c);
      x += readEnd();
    }
    else
    {
      x += font.width;
    }
  }
  if ((c == '\n') || (x >= cursor.wrap))
  {
    x = cursor.left;
    y += font.height;
  }
  setCursor(x,y);
}

void FX::drawString(const uint8_t* buffer)
{
  for(;;)
  {
    uint8_t c = *buffer++;
    if (c) drawChar(c);
    else break;
  }
}

void FX::drawString(const char* str)
{
  FX::drawString((const uint8_t*)str);
}

void FX::drawString(uint24_t address)
{
  for(;;)
  {
    seekData(address++);
    uint8_t c = readEnd();
    if (c) drawChar(c);
    else break;
  }
}

void FX::drawNumber(int16_t n, int8_t digits)
{
  drawNumber((int32_t)n, digits);
}

void FX::drawNumber(uint16_t n, int8_t digits)
{
  drawNumber((uint32_t)n, digits);
}

void FX::drawNumber(int32_t n, int8_t digits)
{
  if (n < 0)
  {
    n = -n;
    drawChar('-');
  }
  else if (digits != 0)
  {
    drawChar(' ');
  }
  drawNumber((uint32_t)n, digits);
}

void FX::drawNumber(uint32_t n, int8_t digits) //
{
  uint8_t buf[33]; //max 32 digits + terminator
  uint8_t *str = &buf[sizeof(buf) - 1];
  *str = '\0';
  do {
    char c = n % 10;
    n /= 10;
    *--str = c + '0';
    if ((digits > 0) && (--digits == 0)) break;
    if ((digits < 0) && (++digits == 0)) break;
  } while(n);
    while (digits > 0) {--digits; *--str = '0';}
    while (digits < 0) {++digits; *--str = ' ';}
  drawString(str);
}
