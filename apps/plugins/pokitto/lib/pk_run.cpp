/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Entry for games written against PokittoLib's init()/update() hooks
 * instead of their own main(). */
#include "Pokitto.h"

void init();
void update();

extern "C" int pk_main(void)
{
    Pokitto::Core::begin();
    init();
    while (Pokitto::Core::isRunning())
        if (Pokitto::Core::update())
            update();
    return 0;
}
