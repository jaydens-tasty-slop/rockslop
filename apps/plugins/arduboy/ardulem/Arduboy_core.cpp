#include "Arduboy_core.h"

bool ArduboyCore::inverted;

void ArduboyCore::blank()
{
    static const unsigned char zero[WIDTH * HEIGHT / 8] = {0};
    ab_present(zero, 0);
}
