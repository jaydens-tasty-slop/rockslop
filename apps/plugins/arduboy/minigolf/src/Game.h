#pragma once

#include "Ball.h"
#include "Camera.h"
#include "CollisionHandler.h"
#include "Constants.h"
#include "Map.h"
#include "MapManager.h"
#include <Arduboy2.h>

enum class GameState
{
    StartScreen,
    HoleSelection,
    Instructions,
    MapSummary,
    Aiming,
    ChoosingPower,
    MapExplorer,
    PauseMenu,
    BallInMotion,
    MapComplete,
    GameSummary,
};

class Game
{
private:
    Arduboy2Base _arduboy;
    uint8_t _mapIndex;
    Map _map;
    Camera _camera;
    Ball _ball;
    GameState _gameState = GameState::StartScreen;
    GameState _gameStateBeforePause = GameState::StartScreen;
    uint8_t _totalPar;
    uint8_t _strokes[MapManager::NumMaps] = {0};
    int8_t _totalOverUnder = 0;
    float _secondsDelta;
    bool _doubleSpeedEnabled;
    uint8_t _startScreenOptionIdx;
    uint8_t _holeSelectionIdx;
    bool _singleHoleMode;
    uint8_t _instructionsPageIdx;
    float _pauseButtonHeldSeconds;
    bool _BButtonPressStartedDuringAim;
    uint8_t _pauseOptionIdx;

    const static float _pauseButtonHoldPauseTime = 0.5;

public:
    Game(Arduboy2Base arduboy) : _arduboy(arduboy)
    {
    }

    void Init(uint8_t mapIndex = 0)
    {
        _mapIndex = mapIndex;
        _map = MapManager::LoadMap(_mapIndex);
        _camera = Camera(_arduboy, 0, 0, _map.width, _map.height);
        _ball = Ball(static_cast<float>(_map.start.x), static_cast<float>(_map.start.y));
        _secondsDelta = 0;
        _doubleSpeedEnabled = false;
        _totalPar = MapManager::GetTotalPar();
        _pauseOptionIdx = 0;
        _totalOverUnder = 0;

        for (uint8_t i = 0; i < MapManager::NumMaps; i++)
            _strokes[i] = 0;
    }

    void Tick(float secondsDelta)
    {
        _secondsDelta = secondsDelta;

        HandleInput();

        if (_gameState == GameState::ChoosingPower)
        {
            _ball.TickPower(secondsDelta);
        }

        if (_gameState == GameState::BallInMotion)
        {
            TickBallInMotion();

            // move ball twice per tick if in 2x speed
            if (_doubleSpeedEnabled)
                TickBallInMotion();
        }

        if (_gameState != GameState::MapExplorer)
            _camera.FocusOn(_ball.X, _ball.Y);
    }

    void Display()
    {
        switch (_gameState)
        {
            case GameState::StartScreen:
                _camera.DrawStartScreen(_startScreenOptionIdx);
                break;
            case GameState::HoleSelection:
                _camera.DrawHoleSelection(_holeSelectionIdx);
                break;
            case GameState::Instructions:
                _camera.DrawInstructions(_instructionsPageIdx);
                break;
            case GameState::MapSummary:
                _camera.DrawMap(_map);
                _camera.DrawHole(_map.end.x, _map.end.y, !IsBallNearHole());
                _camera.DrawBall(_ball);
                _camera.DrawMapSummary(_mapIndex + 1, _map);
                break;
            case GameState::Aiming:
                _camera.DrawMap(_map);
                _camera.DrawHole(_map.end.x, _map.end.y, !IsBallNearHole());
                _camera.DrawBall(_ball);
                _camera.DrawAimHud(_ball);
                break;
            case GameState::ChoosingPower:
                _camera.DrawMap(_map);
                _camera.DrawHole(_map.end.x, _map.end.y, !IsBallNearHole());
                _camera.DrawBall(_ball);
                _camera.DrawAimHud(_ball);
                break;
            case GameState::MapExplorer:
                _camera.DrawMap(_map);
                _camera.DrawHole(_map.end.x, _map.end.y, !IsBallNearHole());
                _camera.DrawBall(_ball);
                _camera.DrawAimHud(_ball);
                _camera.DrawMapExplorerIndicator();
                break;
            case GameState::PauseMenu:
                _camera.DrawPauseMenu(_mapIndex + 1, _map, _strokes[_mapIndex], _pauseOptionIdx);
                break;
            case GameState::BallInMotion:
                _camera.DrawMap(_map);
                _camera.DrawHole(_map.end.x, _map.end.y, !IsBallNearHole());
                _camera.DrawBall(_ball);
                if (_doubleSpeedEnabled)
                    _camera.DrawDoubleSpeedIndicator();
                break;
            case GameState::MapComplete:
                _camera.DrawMap(_map);
                _camera.DrawHole(_map.end.x, _map.end.y, !IsBallNearHole());
                _camera.DrawBall(_ball);
                if (_singleHoleMode)
                    _camera.DrawMapCompleteNoTotal(_mapIndex + 1, _map, _strokes[_mapIndex]);
                else
                    _camera.DrawMapComplete(_mapIndex + 1, _map, _strokes[_mapIndex], _totalOverUnder);
                break;
            case GameState::GameSummary:
                uint16_t totalStrokes = 0;
                for (uint8_t i = 0; i < MapManager::NumMaps; i++)
                    totalStrokes += _strokes[i];
                _camera.DrawGameSummary(totalStrokes, _totalPar);
                break;
        }
    }

private:
    void HandleInput()
    {
        // check for pauses
        if (InPausableMode())
        {
            if (_arduboy.pressed(B_BUTTON))
            {
                _pauseButtonHeldSeconds += _secondsDelta;
                if (_pauseButtonHeldSeconds > _pauseButtonHoldPauseTime)
                {
                    _pauseButtonHeldSeconds = 0;
                    _gameStateBeforePause = _gameState;
                    _gameState = GameState::PauseMenu;
                }
            }
            else
                _pauseButtonHeldSeconds = 0;
        }

        switch (_gameState)
        {
            case GameState::StartScreen:
                HandleInputStartScreen();
                break;
            case GameState::HoleSelection:
                HandleInputHoleSelection();
                break;
            case GameState::Instructions:
                HandleInputInstructions();
                break;
            case GameState::MapSummary:
                HandleInputMapSummary();
                break;
            case GameState::Aiming:
                HandleInputAiming();
                break;
            case GameState::ChoosingPower:
                HandleInputChoosingPower();
                break;
            case GameState::MapExplorer:
                HandleInputMapExplorer();
                break;
            case GameState::PauseMenu:
                HandleInputPauseMenu();
                break;
            case GameState::BallInMotion:
                HandleInputBallInMotion();
                break;
            case GameState::MapComplete:
                HandleInputMapComplete();
                break;
            case GameState::GameSummary:
                HandleInputGameSummary();
                break;
        }
    }

    void HandleInputStartScreen()
    {
        if (_arduboy.justPressed(UP_BUTTON))
            _startScreenOptionIdx = max(0, _startScreenOptionIdx - 1);
        if (_arduboy.justPressed(DOWN_BUTTON))
            _startScreenOptionIdx = min(_startScreenOptionIdx + 1, StartScreenNumOptions - 1);
        if (_arduboy.justPressed(A_BUTTON))
        {
            switch (_startScreenOptionIdx)
            {
                // Play all holes
                case (0):
                    Init();
                    _gameState = GameState::MapSummary;
                    break;

                // Select a single hole
                case (1):
                    _gameState = GameState::HoleSelection;
                    break;

                // Instructions
                case (2):
                    _gameState = GameState::Instructions;
                    break;
            }
        }
    }

    void HandleInputHoleSelection()
    {
        if (_arduboy.justPressed(UP_BUTTON))
            _holeSelectionIdx = max(0, _holeSelectionIdx - 1);
        if (_arduboy.justPressed(DOWN_BUTTON))
            _holeSelectionIdx = min(_holeSelectionIdx + 1, MapManager::NumMaps - 1);
        if (_arduboy.justPressed(A_BUTTON))
        {
            _singleHoleMode = true;
            Init(_holeSelectionIdx);
            _gameState = GameState::MapSummary;
        }
        if (_arduboy.justPressed(B_BUTTON))
        {
            _holeSelectionIdx = 0;
            _gameState = GameState::StartScreen;
        }
    }

    void HandleInputInstructions()
    {
        if (_arduboy.justPressed(B_BUTTON))
        {
            _instructionsPageIdx = 0;
            _gameState = GameState::StartScreen;
        }
        if (_arduboy.justPressed(LEFT_BUTTON) && _instructionsPageIdx > 0)
        {
            --_instructionsPageIdx;
        }
        if (_arduboy.justPressed(RIGHT_BUTTON) && _instructionsPageIdx < InstructionsSpriteFrames - 1)
        {
            ++_instructionsPageIdx;
        }
    }

    void HandleInputMapSummary()
    {
        if (AnyButtonPressed(_arduboy)) {
            _gameState = GameState::Aiming;
            _BButtonPressStartedDuringAim = false;
        }
    }

    void HandleInputAiming()
    {
        if (_arduboy.justPressed(A_BUTTON))
            _gameState = GameState::ChoosingPower;
        if (_arduboy.pressed(LEFT_BUTTON))
            _ball.RotateDirectionCounterClockwise(_secondsDelta);
        if (_arduboy.pressed(RIGHT_BUTTON))
            _ball.RotateDirectionClockwise(_secondsDelta);

        
        if (_arduboy.justPressed(B_BUTTON))
            _BButtonPressStartedDuringAim = true;
        // maybe have a boolean that ensures the release was started in this mode
        if (_arduboy.justReleased(B_BUTTON) && _BButtonPressStartedDuringAim) // fix this activating when exiting menu
            _gameState = GameState::MapExplorer;
    }

    void HandleInputChoosingPower()
    {
        if (_arduboy.justPressed(B_BUTTON))
        {
            _gameState = GameState::Aiming;
            _BButtonPressStartedDuringAim = false;
            return;
        }
        if (_arduboy.justPressed(A_BUTTON))
        {
            _gameState = GameState::BallInMotion;
            _ball.StartHit();
            _strokes[_mapIndex]++;
            return;
        }
    }

    void HandleInputMapExplorer()
    {
        if (_arduboy.pressed(UP_BUTTON))
            _camera.MoveUp();
        if (_arduboy.pressed(DOWN_BUTTON))
            _camera.MoveDown();
        if (_arduboy.pressed(LEFT_BUTTON))
            _camera.MoveLeft();
        if (_arduboy.pressed(RIGHT_BUTTON))
            _camera.MoveRight();
        if (_arduboy.justReleased(B_BUTTON))
        {
            _gameState = GameState::Aiming;
            _BButtonPressStartedDuringAim = false;
            _camera.FocusOn(_ball.X, _ball.Y);
        }
    }

    void HandleInputPauseMenu()
    {
        if (_arduboy.justPressed(B_BUTTON)) {
            _gameState = _gameStateBeforePause;
            _pauseOptionIdx = 0;
            _BButtonPressStartedDuringAim = false;
        }

        if (_arduboy.justPressed(UP_BUTTON))
            _pauseOptionIdx = max(0, _pauseOptionIdx - 1);
        if (_arduboy.justPressed(DOWN_BUTTON))
            _pauseOptionIdx = min(_pauseOptionIdx + 1, PauseScreenNumOptions - 1);
        if (_arduboy.justPressed(A_BUTTON))
        {
            switch (_pauseOptionIdx)
            {
                // back
                case (0):
                    _gameState = _gameStateBeforePause;
                    _pauseOptionIdx = 0;
                    _BButtonPressStartedDuringAim = false;
                    break;

                // main menu 
                case (1):
                    Init();
                    _gameState = GameState::StartScreen;
                    break;
            }
        }
    }

    void HandleInputBallInMotion()
    {
        // only start double speed upon a new button press
        if (!_doubleSpeedEnabled)
            _doubleSpeedEnabled = _arduboy.justPressed(A_BUTTON);
        else
            _doubleSpeedEnabled = _arduboy.pressed(A_BUTTON);
    }

    void HandleInputMapComplete()
    {
        if (_arduboy.justPressed(A_BUTTON))
        {
            if (_singleHoleMode)
            {
                _singleHoleMode = false;
                _totalOverUnder = 0;
                _gameState = GameState::HoleSelection;
            }
            else
            {
                if (_mapIndex >= MapManager::NumMaps - 1)
                    _gameState = GameState::GameSummary;
                else
                    LoadNextMap();
            }
        }
    }

    void HandleInputGameSummary()
    {
        // for now, just restart the game
        if (_arduboy.justPressed(A_BUTTON))
        {
            Init();
            _gameState = GameState::StartScreen;
        }
    }

    void TickBallInMotion()
    {
        // do a couple collision checks per tick
        //  (less likely to miss a collision at high speeds)
        uint8_t numCollisionChecks = 2;
        float splitDelta = _secondsDelta / numCollisionChecks;

        for (uint8_t i = 0; i < numCollisionChecks; i++)
        {
            _ball.Move(splitDelta);

            if (_ball.IsStopped())
            {
                _gameState = GameState::Aiming;
                _BButtonPressStartedDuringAim = false;
                _ball.ResetPower();
                _doubleSpeedEnabled = false;
                break;
            }

            CollisionHandler::HandleAllCollisions(_ball, _map, splitDelta);

            if (CollisionHandler::BallInHole(_ball, _map))
            {
                _ball.X = _map.end.x;
                _ball.Y = _map.end.y;
                _ball.Velocity = {0, 0};
                _gameState = GameState::MapComplete;
                _totalOverUnder += _strokes[_mapIndex] - _map.par;
                break;
            }
        }
    }

    void LoadNextMap()
    {
        _mapIndex += 1;
        _map = MapManager::LoadMap(_mapIndex);

        _camera = Camera(_arduboy, 0, 0, _map.width, _map.height);
        _ball = Ball(static_cast<float>(_map.start.x), static_cast<float>(_map.start.y));
        _gameState = GameState::MapSummary;
        _secondsDelta = 0;
    }

    bool IsBallNearHole()
    {
        return CollisionHandler::Distance(_ball.X, _ball.Y, _map.end.x, _map.end.y) <= 25;
    }

    bool InPausableMode()
    {
        return _gameState == GameState::Aiming ||
               _gameState == GameState::ChoosingPower ||
               _gameState == GameState::MapExplorer ||
               _gameState == GameState::BallInMotion;
    }

    static bool AnyButtonPressed(Arduboy2Base arduboy)
    {
        return (arduboy.justPressed(UP_BUTTON) ||
                arduboy.justPressed(DOWN_BUTTON) ||
                arduboy.justPressed(LEFT_BUTTON) ||
                arduboy.justPressed(RIGHT_BUTTON) ||
                arduboy.justPressed(A_BUTTON) ||
                arduboy.justPressed(B_BUTTON));
    }
};