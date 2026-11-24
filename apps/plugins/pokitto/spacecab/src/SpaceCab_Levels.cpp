#include "SpaceCab.h"

// Counts the level files on the card and collects their names into the
// level table. (Upstream also rewrote a summary file and ran the level
// designer from here; the designer is not part of the Rockbox port.)
uint32_t Game::designer_RebuildLVLFile() {

    uint32_t _number = 1;

    while (true) {

        File file;

        char level[31] = "music/SPCAB_FILES/SPCAB000.raw";

        level[25] = (_number % 10) + 48;
        level[24] = (_number / 10) + 48;

        if (!file.openRO(level)) {
            break;
        }
        else {

            file.read(this->level.getLevelName(), ENTRY_LENGTH);
            memcpy(&this->level.getLevelData()[1 + ((_number - 1) * ENTRY_LENGTH)], this->level.getLevelName(), ENTRY_LENGTH);

        }

        _number++;

    }

    this->level.getLevelData()[0] = _number - 1;
    this->gamePlayVariables.maxNumberOfLevels = _number;
    return _number;

}
