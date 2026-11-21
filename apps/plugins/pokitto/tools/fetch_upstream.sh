#!/bin/sh
# Fetch the pinned upstream game repositories that pokitto.make extracts
# card data (music, speech, images, levels) from: one for each game with a
# tools/<game>_data.py. A checkout already at its commit is left alone.
#   tools/fetch_upstream.sh [DIR]     default: POKITTO_UPSTREAM's default,
#                                     build/np/src next to the rockbox tree
set -e
DIR=${1:-$(cd "$(dirname "$0")/../../../../.." && pwd)/build/np/src}
mkdir -p "$DIR"
while read -r name url commit; do
    if [ "$(git -C "$DIR/$name" rev-parse -q --verify HEAD 2>/dev/null)" != "$commit" ]; then
        if [ ! -d "$DIR/$name/.git" ]; then
            git init -q "$DIR/$name"
            git -C "$DIR/$name" remote add origin "$url"
        fi
        git -C "$DIR/$name" fetch -q --depth 1 origin "$commit"
        git -C "$DIR/$name" checkout -q --detach "$commit"
    fi
    echo "$name $commit"
done <<LIST
OBS_Pokitto https://github.com/Press-Play-On-Tape/OBS_Pokitto 7807ee08f7e214ecaa8c5e1d269ac7a1e6df20ac
FireWorks_Pokitto https://github.com/Press-Play-On-Tape/FireWorks_Pokitto a935761e53abc90a8e4148bd7f4805ed4a3ce7d3
DarkRitual_Pokitto https://github.com/Press-Play-On-Tape/DarkRitual_Pokitto 3eb86e5eb51043e55a4a9b4e6357991be10a496d
SpaceCab_Pokitto https://github.com/Press-Play-On-Tape/SpaceCab_Pokitto 53380f9bcde87a269c40e037f93545bb9d15672f
Abbaye_Pokitto https://gitlab.com/HomineLudens/abbaye-des-morts-pokitto 06f6e1c5a4f8f5ca53b306723b43193d2f4ed71f
1nvader_Pokitto https://github.com/Press-Play-On-Tape/1nvader_Pokitto 15be430a208852d7e76aff90467f7d2679f1ba05
MiniRogue_Pokitto https://github.com/Press-Play-On-Tape/MiniRogue_Pokitto 63cb1da22b759d1bc9f3809c53d3c51a9f70f5d5
TrialsOfAstarok_Pokitto https://github.com/Press-Play-On-Tape/TrialsOfAstarok_Pokitto 1a7ecb0a17d163731b97b540c981a318ed94682a
MineSweeper_Pokitto https://github.com/Press-Play-On-Tape/MineSweeper_Pokitto c7fbcea983d3970be35de0cb6c3f6a8ef8497069
LightsOut_Pokitto https://github.com/Press-Play-On-Tape/LightsOut_Pokitto 948042e453c9b3368f449efc1ee1dbcf866d7f2f
ButtonsTrail_Pokitto https://github.com/Press-Play-On-Tape/ButtonsTrail_Pokitto 84b62100cb4b784d85cf9924f85fa15cec311f2b
GalaxyFighter_Pokitto https://github.com/Press-Play-On-Tape/GalaxyFighter_Pokitto ce135d529f3ed53a4f7ee27e0efe2dbd06a5b29b
LeWord_Pokitto https://github.com/Press-Play-On-Tape/LeWord_Pokitto 99e3f8e16e04d15a9833ebf26fe3cb4180320ab5
Scrambler_Pokitto https://github.com/Press-Play-On-Tape/Scrambler_Pokitto bf21d077d7d1953e977ff428e96e3816e31c88d6
RoadTrip_Pokitto https://github.com/Press-Play-On-Tape/RoadTrip_Pokitto a84e9837d1e9b180facc6ef11347ec220f9672c9
Scopa_V1_Pokitto https://github.com/Press-Play-On-Tape/Scopa_V1_Pokitto 9ffaf7f092500a31fdab3dd5f33bcde7e16a61e3
StackShift_Pokitto https://github.com/Press-Play-On-Tape/StackShift_Pokitto fe472f4f57eaf09b3f34b3ba826fee13816f0f76
LIST
