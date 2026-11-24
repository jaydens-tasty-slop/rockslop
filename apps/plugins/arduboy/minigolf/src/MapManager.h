#pragma once

#include <ArduboyFX.h>
#include "FX/fxdata.h"
#include "Map.h"

class MapManager
{
public:
    static const uint8_t NumMaps = 9;

    static Map LoadMap(uint8_t index)
    {
        Map map;

        // read Map from FX data
        FX::seekArrayElement<Map>(Maps, index);
        FX::readObject(map);
        FX::readEnd();

        map.name = MapNames[index];

        return map;
    }

    static uint8_t GetTotalPar()
    {
        uint8_t total = 0;
        Map map;

        for (uint8_t i = 0; i < NumMaps; i++)
        {
            // read Map from FX data
            FX::seekArrayElement<Map>(Maps, i);
            FX::readObject(map);
            FX::readEnd();

            total += map.par;
        }

        return total;
    }

    static const char *MapNames[NumMaps];
};

const char *MapManager::MapNames[MapManager::NumMaps] = {
    "Squiggly Lane",
    "Solar System",
    "The Diamond",
    "Treadmill Twist",
    "Haunted Hallway",
    "Options",
    "Plinko",
    "Ricochet",
    "Quadrants"};