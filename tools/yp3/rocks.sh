#!/bin/sh
# Lay out built plugins as on the SD card: OUT/.rockbox/rocks/<category>/
# (apps/plugins/CATEGORIES, as tools/buildzip.pl), OUT/.rockbox/viewers.config
# for the plugins that exist and their licence texts in OUT/.rockbox/licenses/,
# Rockbox's own in OUT/.rockbox/docs/ (System > Legal Notices opens them).
# Plugins that failed to build are simply absent.
#   tools/yp3/rocks.sh BUILD_DIR OUT_DIR
# BUILD_DIR is a build of this source tree.
set -e
[ $# -eq 2 ] || { echo "usage: $0 BUILD_DIR OUT_DIR" >&2; exit 2; }
RB=$(cd "$(dirname "$0")/../.." && pwd)
B=$(realpath "$1")
OUT=$(realpath -m "$2")
SRC=$RB/apps/plugins
rm -rf "$OUT/.rockbox/rocks" "$OUT/.rockbox/licenses"
# Overlays: only the image viewer's decoders (they load into the plugin's
# own buffer); rockbox apps/plugins/SOURCES drops the other loader stubs.
for f in $(find "$B/apps/plugins" -name '*.rock' -o -path '*/imageviewer/*.ovl'); do
    n=$(basename "$f"); base=${n%.*}
    case "$base" in
        sgt-*) dir=games/sgt_puzzles ;;
        *) dir=$(sed -n "s/^$base,//p" "$SRC/CATEGORIES" | head -1) ;;
    esac
    [ -n "$dir" ] || dir=apps
    mkdir -p "$OUT/.rockbox/rocks/$dir"
    cp "$f" "$OUT/.rockbox/rocks/$dir/"
done
# Level sets next to their games, as tools/buildzip.pl does.
for lv in sokoban snake2; do
    [ -f "$OUT/.rockbox/rocks/games/$lv.rock" ] && cp "$SRC/$lv.levels" "$OUT/.rockbox/rocks/games/"
done
[ -f "$OUT/.rockbox/rocks/demos/rock88.rock" ] && cp "$SRC/rock88.r88" "$OUT/.rockbox/rocks/demos/"
# Plugin subdirs list extra card files (licenses, game data) in CARDFILES:
# "<source> <destination under .rockbox/rocks>" per line; a source with a
# build/ prefix is a generated file in the build dir. Only for built dirs,
# or, with a third field naming a single-file plugin, when its .rock built.
for cf in "$SRC"/*/CARDFILES; do
    [ -f "$cf" ] || continue
    d=$(dirname "$cf"); name=$(basename "$d")
    built=$(find "$B/apps/plugins/$name" -name '*.rock' 2>/dev/null | head -1)
    grep -v '^#' "$cf" | while read -r from to rock; do
        [ -n "$from" ] || continue
        if [ -n "$rock" ]; then
            [ -f "$B/apps/plugins/$rock.rock" ] || continue
        else
            [ -n "$built" ] || continue
        fi
        case "$from" in
            build/*) from=$B/apps/plugins/$name/${from#build/} ;;
            *) from=$d/$from ;;
        esac
        to=$OUT/.rockbox/rocks/$to
        if [ -d "$from" ]; then          # a directory's contents merge into $to
            mkdir -p "$to"
            cp -r "$from/." "$to"
        elif [ -f "$from" ]; then
            mkdir -p "$(dirname "$to")"
            cp "$from" "$to"
        else
            echo "rocks.sh: $name: missing card file $from" >&2
        fi
    done
done
# Licence texts move to .rockbox/licenses/ (same paths): among the plugins a
# folder holding only them shows as an empty one. Drop folders left empty.
(cd "$OUT/.rockbox/rocks" &&
 find . -type f \( -iname 'licen[cs]e*' -o -iname 'notice*' -o -iname 'copying*' \
     -o -iname '*-by-*.txt' \) | while read -r f; do
    mkdir -p "$OUT/.rockbox/licenses/$(dirname "$f")"
    mv "$f" "$OUT/.rockbox/licenses/$f"
 done
 find . -mindepth 1 -type d -empty -delete)
# Rockbox's licence texts, as tools/buildzip.pl names them
mkdir -p "$OUT/.rockbox/docs"
for doc in COPYING LICENSES; do
    cp "$RB/docs/$doc" "$OUT/.rockbox/docs/$doc.txt"
done
# viewers.config lines name viewers/<plugin>; keep those that were built.
while IFS= read -r line; do
    p=$(echo "$line" | cut -d, -f2)
    [ -f "$OUT/.rockbox/rocks/$p.rock" ] && echo "$line"
done < "$SRC/viewers.config" > "$OUT/.rockbox/viewers.config"
echo "$(find "$OUT/.rockbox/rocks" -name '*.rock' | wc -l) plugins, $(du -sh "$OUT/.rockbox/rocks" | cut -f1) in $OUT/.rockbox/rocks;" \
     "$(find "$OUT/.rockbox/licenses" -type f 2>/dev/null | wc -l) licence texts in $OUT/.rockbox/licenses"
