#pragma once

#include "Ball.h"
#include "Constants.h"
#include <ArduboyFX.h>
#include "FX/fxdata.h"
#include "Font4x6/Font4x6.h"
#include "Map.h"
#include "MapManager.h"
#include <Arduboy2.h>

class Camera
{
private:
    Arduboy2Base _arduboy;
    int16_t _cameraX;
    int16_t _cameraY;
    uint8_t _mapWidth;
    uint8_t _mapHeight;
    uint8_t _treadmillFrame = 0;
    uint8_t _startScreenFlagWaveFrame = 0;
    int8_t _startScreenFlagWaveFrameIncreasing = 1; // used to increment/decrement sprite frame (ex: 0,1,2,3,2,1,0...)
    uint8_t _holeFrame = 0;
    bool _borderFlickerToggle = false; // alternate true/false to make the DottedBorder dynamic
    Font4x6 _font4x6;
    bool _textFlashToggle = false;

    static constexpr uint8_t FontWidth = 4;
    static constexpr uint8_t FontHeight = 7;
    static constexpr uint8_t HalfScreenWidth = WIDTH / 2;
    static constexpr uint8_t HalfScreenHeight = HEIGHT / 2;
    static constexpr uint8_t MaxPowerLineLength = 40;
    static constexpr uint8_t MinPowerLineLength = 10;
    static constexpr uint8_t MaxBoundaryPadding = 5;
    static constexpr int8_t HoleNoFlagXOffset = -3;
    static constexpr int8_t HoleNoFlagYOffset = -3;
    static constexpr int8_t HoleWithFlagXOffset = -4;
    static constexpr int8_t HoleWithFlagYOffset = -11;

    const char *StartMenuTextOptions[StartScreenNumOptions] = {
        "Play All Holes",
        "Select Hole",
        "Instructions"};

    const char *PauseMenuTextOptions[StartScreenNumOptions] = {
        "Resume",
        "Exit to main menu"};

public:
    Camera() = default;
    Camera(Arduboy2Base arduboy, uint8_t x, uint8_t y, uint8_t mapWidth, uint8_t mapHeight)
        : _arduboy(arduboy), _mapWidth(mapWidth), _mapHeight(mapHeight)
    {
        _font4x6 = Font4x6();
        FocusOn(x, y);
    }

    void FocusOn(int16_t x, int16_t y)
    {
        _cameraX = x - HalfScreenWidth;
        _cameraY = y - HalfScreenHeight;

        KeepInBounds();
    }

    void DrawMap(const Map &map)
    {
        // draw floor texture (dots on ground)
        uint8_t dotSpacing = 16;
        for (int i = dotSpacing / 2; i < _mapWidth; i += dotSpacing)
        {
            for (int j = dotSpacing / 2; j < _mapHeight; j += dotSpacing)
            {
                _arduboy.drawPixel(i - _cameraX, j - _cameraY);
            }
        }

        // draw treadmills
        for (auto tread : map.treadmills)
        {
            if (tread.IsEmpty())
                continue;

            uint24_t sprite = 0;
            switch (tread.direction)
            {
                case Direction::Up:
                    sprite = TreadmillUpSprite;
                    break;
                case Direction::Down:
                    sprite = TreadmillDownSprite;
                    break;
                case Direction::Left:
                    sprite = TreadmillLeftSprite;
                    break;
                case Direction::Right:
                    sprite = TreadmillRightSprite;
                    break;
            }

            for (uint8_t x = 0; x < tread.width; x += TreadmillUpSpriteWidth)
            {
                int16_t drawX = x + tread.x - _cameraX;
                if (drawX < -(int8_t)TreadmillUpSpriteWidth || drawX > WIDTH)
                    continue;

                for (uint8_t y = 0; y < tread.height; y += TreadmillUpSpriteHeight)
                {
                    int16_t drawY = y + tread.y - _cameraY;
                    if (drawY < -(int8_t)TreadmillUpSpriteHeight || drawY > HEIGHT)
                        continue;

                    FX::drawBitmap(drawX, drawY, sprite, _treadmillFrame, dbmNormal);
                }
            }
        }

        // draw circles
        for (auto circle : map.circles)
        {
            if (circle.IsEmpty())
                continue;

            int16_t drawX = circle.location.x - _cameraX;
            int16_t drawY = circle.location.y - _cameraY;

            if (drawX + circle.radius < 0 || drawX - circle.radius > WIDTH ||
                drawY + circle.radius < 0 || drawY - circle.radius > HEIGHT)
                continue;

            _arduboy.fillCircle(drawX, drawY, circle.radius, BLACK);
            _arduboy.drawCircle(drawX, drawY, circle.radius, WHITE);
        }

        // draw sand traps
        uint8_t gridWidth = 5;
        for (auto sandtrap : map.sandTraps)
        {
            if (sandtrap.IsEmpty())
                continue;

            for (uint8_t x = 0; x < sandtrap.width; x += SandtrapSpriteWidth)
            {
                int16_t drawX = x + sandtrap.x - _cameraX;
                if (drawX < -(int8_t)SandtrapSpriteWidth || drawX > WIDTH)
                    continue;

                for (uint8_t y = 0; y < sandtrap.height; y += SandtrapSpriteHeight)
                {
                    int16_t drawY = y + sandtrap.y - _cameraY;
                    if (drawY < -(int8_t)SandtrapSpriteHeight || drawY > HEIGHT)
                        continue;

                    FX::drawBitmap(drawX, drawY, SandtrapSprite, 0, dbmNormal);
                }
            }
        }

        // draw walls
        for (auto wall : map.walls)
        {
            if (wall.IsEmpty())
                continue;

            _arduboy.drawLine(wall.p1.x - _cameraX,
                              wall.p1.y - _cameraY,
                              wall.p2.x - _cameraX,
                              wall.p2.y - _cameraY);
        }

        // cycle sprite frames
        if (_arduboy.everyXFrames(5))
            _treadmillFrame = ++_treadmillFrame % TreadmillUpSpriteFrames;
    }

    void DrawHole(uint8_t x, uint8_t y, bool withFlag = false)
    {
        if (withFlag)
        {
            FX::drawBitmap(x - _cameraX + HoleWithFlagXOffset,
                           y - _cameraY + HoleWithFlagYOffset,
                           HoleWithFlagSprite, _holeFrame, dbmMasked);
        }
        else
        {
            FX::drawBitmap(x - _cameraX + HoleNoFlagXOffset,
                           y - _cameraY + HoleNoFlagYOffset,
                           HoleNoFlagSprite, _holeFrame, dbmMasked);
        }

        if (_arduboy.everyXFrames(15))
            _holeFrame = ++_holeFrame % HoleNoFlagSpriteFrames;
    }

    void DrawBall(const Ball &ball)
    {
        _arduboy.fillCircle(static_cast<int16_t>(ball.X) - _cameraX,
                            static_cast<int16_t>(ball.Y) - _cameraY,
                            Ball::Radius);
    }

    void DrawAimHud(const Ball &ball)
    {
        float lineLength = map(ball.Power, Ball::MinPower, Ball::MaxPower, MinPowerLineLength, MaxPowerLineLength);

        float x = ball.X + cos(ball.Direction) * lineLength;
        float y = ball.Y + -(sin(ball.Direction) * lineLength);

        _arduboy.drawLine(ball.X - _cameraX,
                          ball.Y - _cameraY,
                          x - _cameraX,
                          y - _cameraY);
    }

    void DrawStartScreen(uint8_t optionIdx)
    {
        _font4x6.setCursor(0, 0);

        for (uint8_t i = 0; i < StartScreenNumOptions; i++)
        {
            if (i == optionIdx)
                _font4x6.println(String(F(">")) + StartMenuTextOptions[i]);
            else
                _font4x6.println(String(F(" ")) + StartMenuTextOptions[i]);
        }

        DrawMenuBackgroundAnimation();
    }

    void DrawHoleSelection(uint8_t holeIdx)
    {
        // scroll lines as holeIdx changes
        uint8_t baseHeight = 0;
        uint8_t height = baseHeight - ((FontHeight + 1) * holeIdx);

        // stop scrolling if we're towards the end of the list
        uint8_t numEntriesFitOnScreen = 5;
        if (holeIdx >= MapManager::NumMaps - numEntriesFitOnScreen)
            height = baseHeight - (FontHeight + 1) * (MapManager::NumMaps - numEntriesFitOnScreen);

        _font4x6.setCursor(0, height);

        uint8_t holeNum = 1;
        for (uint8_t i = 0; i < MapManager::NumMaps; i++, holeNum++)
        {
            auto holeNumAndName = holeNum + String(F(": ")) + MapManager::MapNames[i];

            if (i == holeIdx)
                _font4x6.println(String(F(">")) + holeNumAndName);
            else
                _font4x6.println(String(F(" ")) + holeNumAndName);
        }

        DrawMenuBackgroundAnimation();
    }

    void DrawInstructions(uint8_t pageIdx)
    {
        FX::drawBitmap(0, 0, InstructionsSprite, pageIdx, dbmNormal);

        // draw indicator 'arrows' for changing pages
        if (_textFlashToggle)
        {
            if (pageIdx > 0)
            {
                _font4x6.setCursor(0, HEIGHT - FontHeight);
                _font4x6.print("<");
            }
            if (pageIdx < InstructionsSpriteFrames - 1)
            {
                _font4x6.setCursor(WIDTH - FontWidth, HEIGHT - FontHeight);
                _font4x6.print(">");
            }
        }

        if (_arduboy.everyXFrames(45))
            _textFlashToggle = !_textFlashToggle;
    }

    void DrawMenuBackgroundAnimation()
    {
        // draw ground and flag pole
        FX::drawBitmap(0, 0, StartScreenGroundAndPoleSprite, 0, dbmMasked);
        FX::drawBitmap(100, 11, StartScreenFlagWaveSprite, _startScreenFlagWaveFrame, dbmNormal);

        if (_arduboy.everyXFrames(8))
        {
            // iterate forward/backward through dog tail wag frames
            _startScreenFlagWaveFrame += _startScreenFlagWaveFrameIncreasing;
            if (_startScreenFlagWaveFrame >= StartScreenFlagWaveSpriteFrames - 1)
                _startScreenFlagWaveFrameIncreasing = -1;
            else if (_startScreenFlagWaveFrame <= 0)
                _startScreenFlagWaveFrameIncreasing = 1;
        }
    }

    void DrawMapSummary(uint8_t mapNum, const Map &map)
    {
        _font4x6.setCursor(0, 20);
        PrintCenteredWithBackground(String(F("Hole ")) + mapNum + String(F("\n")) +
                                    F("\"") + map.name + F("\"") +
                                    String(F("\npar ")) + String(map.par));
    }

    void DrawMapExplorerIndicator()
    {
        if (_arduboy.everyXFrames(30))
            _textFlashToggle = !_textFlashToggle;

        if (_textFlashToggle)
            DrawTextBottomLeft("View Map");
    }

    void DrawPauseMenu(uint8_t mapNum, const Map &map, uint8_t strokes, uint8_t optionIdx)
    {
        // print "Paused" in top left corner with a border
        auto pausedText = "Paused";
        _font4x6.setCursor(2, 1);
        Rect bgRect = Rect(1, 1, GetTextPixelWidth(pausedText) + 2, FontHeight + 1);
        Rect borderRect = ExpandRect(bgRect, 1);
        DrawDottedBorder(borderRect);
        _arduboy.fillRect(bgRect.x, bgRect.y, bgRect.width, bgRect.height, BLACK);
        _font4x6.print(pausedText);

        // map info summary
        _font4x6.setCursor(64, 0);
        _font4x6.println(String(F("   Hole ")) + mapNum);
        _font4x6.println(String(F("par:      ")) + map.par);
        _font4x6.println(String(F("strokes:  ")) + strokes);

        // menu options
        _font4x6.setCursor(0, 48);
        for (uint8_t i = 0; i < PauseScreenNumOptions; i++)
        {
            if (i == optionIdx)
                _font4x6.println(String(F(">")) + PauseMenuTextOptions[i]);
            else
                _font4x6.println(String(F(" ")) + PauseMenuTextOptions[i]);
        }
    }

    void DrawDoubleSpeedIndicator()
    {
        if (_arduboy.everyXFrames(30))
            _textFlashToggle = !_textFlashToggle;

        if (_textFlashToggle)
            DrawTextBottomLeft("2x");
    }

    void DrawMapComplete(uint8_t mapNum, const Map &map, uint8_t strokes, int8_t totalOverUnder)
    {
        String gameTotal = String(F("game total: "));
        if (totalOverUnder > 0)
            gameTotal += "+";
        gameTotal += totalOverUnder;

        _font4x6.setCursorY(8);
        PrintCenteredWithBackground(String(F("Hole ")) + mapNum + F((" Complete!\n")) +
                                    String(F("par:     ")) + map.par + String(F("\n")) +
                                    String(F("strokes: ")) + String(strokes) + String(F("\n\n")) +
                                    gameTotal +
                                    String(F("\nPress A to continue")));
    }

    void DrawMapCompleteNoTotal(uint8_t mapNum, const Map &map, uint8_t strokes)
    {
        _font4x6.setCursorY(10);
        PrintCenteredWithBackground(String(F("Hole ")) + mapNum + F((" Complete!\n")) +
                                    String(F("par:     ")) + map.par + String(F("\n")) +
                                    String(F("strokes: ")) + String(strokes) +
                                    String(F("\nPress A to continue")));
    }

    void DrawGameSummary(uint16_t totalStrokes, uint8_t totalPar)
    {
        int8_t totalOverUnder = totalStrokes - totalPar;

        String gameTotal = String(F("Total:   "));
        if (totalOverUnder > 0)
            gameTotal += "+";
        gameTotal += totalOverUnder;

        _font4x6.setCursorY(7);
        PrintCenteredWithBackground(String(F("All Holes Complete!\n\n")) +
                                    String(F("Final Score\n")) +
                                    String(F("Par:     ")) + totalPar + String(F("\n")) +
                                    String(F("Strokes: ")) + totalStrokes + String(F("\n")) +
                                    gameTotal);
    }

    void MoveUp()
    {
        _cameraY -= 2;
        KeepInBounds();
    }

    void MoveDown()
    {
        _cameraY += 2;
        KeepInBounds();
    }

    void MoveLeft()
    {
        _cameraX -= 2;
        KeepInBounds();
    }

    void MoveRight()
    {
        _cameraX += 2;
        KeepInBounds();
    }

private:
    void KeepInBounds()
    {
        int16_t maxX = _mapWidth - WIDTH + MaxBoundaryPadding;
        int16_t maxY = _mapHeight - HEIGHT + MaxBoundaryPadding;

        if (_cameraX > maxX)
            _cameraX = maxX;
        if (_cameraY > maxY)
            _cameraY = maxY;
        if (_cameraX < -MaxBoundaryPadding)
            _cameraX = -MaxBoundaryPadding;
        if (_cameraY < -MaxBoundaryPadding)
            _cameraY = -MaxBoundaryPadding;
    }

    void DrawTextBottomLeft(const String &text)
    {
        _font4x6.setCursor(0, HEIGHT - FontHeight - 1);

        Rect bgRect = Rect(_font4x6.getCursorX() - 1,
                           _font4x6.getCursorY(),
                           GetTextPixelWidth(text) + 2,
                           FontHeight + 1);

        Rect borderRect = ExpandRect(bgRect, 1);

        DrawDottedBorderNoFlicker(borderRect);
        _arduboy.fillRect(bgRect.x, bgRect.y, bgRect.width, bgRect.height, BLACK);
        _font4x6.print(text);
    }

    // Prints the provided text centered on the screen
    // with a checkered background
    void PrintCenteredWithBackground(const String &text)
    {
        Rect backgroundRect = GetBoundingRectOfCenteredText(text);
        backgroundRect = ExpandRect(backgroundRect, 2);
        DrawDottedBorder(backgroundRect);

        int8_t lineLength = 0;
        String temp = String(text);

        do
        {
            lineLength = temp.indexOf('\n');
            if (lineLength < 0)
                lineLength = temp.length();

            auto line = temp.substring(0, lineLength);
            temp = temp.substring(lineLength + 1);

            uint8_t textWidth = GetTextPixelWidth(line);
            uint8_t offset = HalfScreenWidth - (textWidth / 2);

            _font4x6.setCursorX(offset);
            PrintlnCenteredOverBlack(line);
        } while (temp.length() > 0);
    }

    // Prints the provided text centered on the screen with a black background
    void PrintlnCenteredOverBlack(const String &text)
    {
        Rect rect = GetBoundingRectOfCenteredText(text);
        _arduboy.fillRect(rect.x, rect.y, rect.width, rect.height, BLACK);
        _font4x6.println(text);
    }

    // Draws a black rectangle on the provided Rect with a dotted border
    void DrawDottedBorder(const Rect &rect)
    {
        _arduboy.fillRect(rect.x, rect.y, rect.width, rect.height, BLACK);

        uint16_t x = 0;
        uint16_t y = 0;

        // Top border
        for (x = 0; x < rect.width; x++)
        {
            bool draw = (x % 2 == 1) ^ true;
            if (draw == _borderFlickerToggle)
                _arduboy.drawPixel(x + rect.x, rect.y);
        }

        // Bottom border
        for (x = 0; x < rect.width; x++)
        {
            y = rect.height - 1;
            bool draw = (x % 2 == 1) ^ (y % 2 == 0);
            if (draw == _borderFlickerToggle)
                _arduboy.drawPixel(x + rect.x, y + rect.y);
        }

        // Left border
        for (y = 0; y < rect.height; y++)
        {
            bool draw = (y % 2 == 0) ^ false;
            if (draw == _borderFlickerToggle)
                _arduboy.drawPixel(rect.x, y + rect.y);
        }

        // Right border
        for (y = 0; y < rect.height; y++)
        {
            x = rect.width - 1;
            bool draw = x % 2 == 1 ^ y % 2 == 0;
            if (draw == _borderFlickerToggle)
                _arduboy.drawPixel(x + rect.x, y + rect.y);
        }

        if (_arduboy.everyXFrames(10))
            _borderFlickerToggle = !_borderFlickerToggle;
    }

    void DrawDottedBorderNoFlicker(const Rect &rect)
    {
        _borderFlickerToggle = false;
        DrawDottedBorder(rect);
    }

    // Returns a Rect that represents the boundary of a block of text.
    // Margin of 2 pixels on left/right, and margin of 1 pixel on top/bottom.
    // Provided text can contain multiple lines.
    Rect GetBoundingRectOfCenteredText(const String &text)
    {
        int8_t lineLength = 0;
        uint8_t largestStr = 0;
        uint8_t numLines = 0;
        String temp = String(text);

        do
        {
            // get next line of text
            lineLength = temp.indexOf('\n');
            if (lineLength < 0)
                lineLength = temp.length();

            auto line = temp.substring(0, lineLength);
            temp = temp.substring(lineLength + 1);

            largestStr = max(largestStr, line.length());
            numLines++;
        } while (temp.length() > 0);

        uint8_t width = (largestStr * FontWidth);
        width += largestStr - 1; // include pixel between chars
        width += 4;              // account for 2 pixel margin on left/right

        uint8_t x = (HalfScreenWidth - (width / 2));
        uint8_t y = _font4x6.getCursorY();
        uint8_t height = (numLines * FontHeight) + (numLines - 1);
        height += 2; // account for 1 pixel margin on top/bottom

        return Rect(x, y, width, height);
    }

    Rect ExpandRect(const Rect &rect, uint8_t i)
    {
        return Rect(rect.x - i, rect.y - i, rect.width + i * 2, rect.height + i * 2);
    }

    static uint8_t GetTextPixelWidth(const String &text)
    {
        uint8_t textWidth = (text.length() * FontWidth);
        textWidth += text.length() - 1; // include pixel between chars

        return textWidth;
    }

    void MoveTextCursorDown(uint8_t offset)
    {
        _font4x6.setCursorY(_font4x6.getCursorY() + offset);
    }
};