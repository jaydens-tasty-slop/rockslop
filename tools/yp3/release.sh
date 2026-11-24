#!/bin/sh
# Build the YP3 release from this source tree and package it for the
# rockslop flasher (https://github.com/jaydens-tasty-slop/rockslop).
#   tools/yp3/release.sh VERSION OUTDIR
# Writes OUTDIR/rockbox-yp3-VERSION.tar.xz and its .sha256: the firmware and
# its XIP codecs for flash, and the SD card tree. OUTDIR/build is the build
# (every plugin; a few big ones do not fit and are left out), OUTDIR/card
# the card tree: Rockbox's own (make fulltar: codecs, fonts, icons, langs,
# cabbiev2), the plugins with their card files and licences (rocks.sh), the
# themes and RockGPT's models. The core, codecs and plugins come from one
# build, so they share a module ID.
#
# Fetched, and kept in YP3_CACHE: the Arm GNU toolchain (pinned by sha256)
# and the Pokitto games' upstream repositories at pinned commits
# (apps/plugins/pokitto/tools/fetch_upstream.sh), for their card data.
#
# Environment:
#   MKPKG       rockslop's packaging/mkpkg.py (required)
#   YP3_ASSETS  what is not built from source (required): a directory, or
#               the http(s)/file URL of a .tar.xz of one, holding
#                 themes/.rockbox/    the staged 128x160 themes (skins,
#                                     fonts, icons, licences) for the card
#                 rockgpt-C2-3k.llm   RockGPT's model
#                 draft-D0kd.llm      and its draft model
#               Such a tarball, reproducibly:
#                 tar --format=gnu --sort=name --mtime=@0 --owner=0 \
#                     --group=0 --numeric-owner --mode=u=rwX,go=rX -C DIR \
#                     -cf - themes rockgpt-C2-3k.llm draft-D0kd.llm | xz -9e
#   YP3_ASSETS_SHA256  the tarball's sha256 (required with a URL)
#   YP3_CACHE   download cache (default OUTDIR/cache)
#   JOBS        parallel make jobs (default: the CPUs, at most 10)
#   SOURCE_DATE_EPOCH  the build date (default: the commit's)
# The build reaches this tree through a short symlink, YP3_SRC_LINK
# (default /tmp/yp3-src): from a long path the dependency step overflows
# the kernel's limit on one shell argument. One build at a time per link.
set -e
die() { echo "release.sh: $*" >&2; exit 1; }
[ $# -eq 2 ] || die "usage: $0 VERSION OUTDIR"
VERSION=$1
case "$VERSION" in
    *[!A-Za-z0-9._+-]*|"") die "bad version '$VERSION'" ;;
esac
TREE=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(realpath -m "$2")
CACHE=$(realpath -m "${YP3_CACHE:-$OUT/cache}")
[ -f "${MKPKG:-}" ] || die "set MKPKG to rockslop's packaging/mkpkg.py"
MKPKG=$(realpath "$MKPKG")
[ -n "${YP3_ASSETS:-}" ] || die "set YP3_ASSETS (see $0)"
JOBS=${JOBS:-$(nproc)}; [ "$JOBS" -gt 10 ] && JOBS=10
if [ -z "${SOURCE_DATE_EPOCH:-}" ]; then
    SOURCE_DATE_EPOCH=$(git -C "$TREE" log -1 --format=%ct 2>/dev/null || date +%s)
fi
export SOURCE_DATE_EPOCH

TC_NAME=gcc-arm-none-eabi-10.3-2021.10
TC_FILE=$TC_NAME-x86_64-linux.tar.bz2
TC_URL=https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/$TC_FILE
TC_SHA256=97dbb4f019ad1650b732faffcc881689cedc14e2b7ee863d390e0a41ef16c9a3

# fetch URL FILE SHA256: download unless FILE already has that hash
fetch() {
    if [ -f "$2" ] && echo "$3  $2" | sha256sum -c --status; then
        return
    fi
    mkdir -p "$(dirname "$2")"
    curl -fsSL --retry 3 -o "$2.part" "$1"
    echo "$3  $2.part" | sha256sum -c --status || die "$1: sha256 mismatch"
    mv "$2.part" "$2"
}

TCDIR=$CACHE/toolchain/$TC_NAME
fetch "$TC_URL" "$CACHE/toolchain/$TC_FILE" "$TC_SHA256"
if [ ! -x "$TCDIR/bin/arm-none-eabi-gcc" ]; then
    rm -rf "$TCDIR" "$CACHE/toolchain/x"
    mkdir -p "$CACHE/toolchain/x"
    tar -xjf "$CACHE/toolchain/$TC_FILE" -C "$CACHE/toolchain/x"
    mv "$CACHE/toolchain/x/$TC_NAME" "$TCDIR"
    rmdir "$CACHE/toolchain/x"
fi
export PATH="$TCDIR/bin:$PATH"   # for package.py

sh "$TREE/apps/plugins/pokitto/tools/fetch_upstream.sh" "$CACHE/pokitto" >/dev/null

case "$YP3_ASSETS" in
    http://*|https://*|file://*)
        [ -n "${YP3_ASSETS_SHA256:-}" ] || die "set YP3_ASSETS_SHA256 for $YP3_ASSETS"
        fetch "$YP3_ASSETS" "$OUT/assets.tar.xz" "$YP3_ASSETS_SHA256"
        rm -rf "$OUT/assets"
        mkdir -p "$OUT/assets"
        tar -xJf "$OUT/assets.tar.xz" -C "$OUT/assets"
        A=$OUT/assets ;;
    *)  A=$(realpath "$YP3_ASSETS") ;;
esac
for f in themes/.rockbox rockgpt-C2-3k.llm draft-D0kd.llm; do
    [ -e "$A/$f" ] || die "assets: no $A/$f"
done

SRC=${YP3_SRC_LINK:-/tmp/yp3-src}
[ ! -e "$SRC" ] || [ -L "$SRC" ] || die "$SRC is not a symlink"
ln -sfn "$TREE" "$SRC"
B=$OUT/build
C=$OUT/card
LOG=$OUT/build.log
mkdir -p "$B"
cd "$B"
[ -f Makefile ] || "$SRC/tools/configure" --no-ccache --compiler-prefix="$TCDIR/bin/arm-none-eabi-" \
    --target=yp3 --ram=1 --rbdir=/.rockbox --type=N >/dev/null
mk() {
    make EXTRA_DEFINES=-DYP3_RECOVERY_WATCHDOG POKITTO_UPSTREAM="$CACHE/pokitto" "$@"
}
# Generated headers first: in a parallel build some apps/ objects can
# compile before lang.h exists.
mk "$B/lang_enum.h" > "$LOG" 2>&1 || true
mk -j"$JOBS" "$B/rockbox.elf" "$B/lib/rbcodec/codecs/xip/codecs.flash" >> "$LOG" 2>&1 ||
    { tail -30 "$LOG"; die "build failed (log: $LOG)"; }
# -k: a few big plugins (duke3d, ...) do not fit the plugin buffer
mk -k -j"$JOBS" >> "$LOG" 2>&1 || true
# Expected: those plugins, and the scrambled image (the firmware is placed
# in flash by package.py instead)
echo "not built:" $(sed -n 's/^make: \*\*\* \[.*\/\([^/]*\)\] Error.*/\1/p' "$LOG")
mk NODEPS=1 fulltar >> "$LOG" 2>&1 || { tail -30 "$LOG"; die "fulltar failed"; }

rm -rf "$C"
mkdir -p "$C"
tar -xf rockbox-full.tar -C "$C"
rm -f "$C/.rockbox/rockbox.yp3"   # the firmware goes to flash, not the card
"$SRC/tools/yp3/rocks.sh" "$B" "$C"
# rocks.sh lays .rockbox/rocks out afresh: the plugin data Rockbox's own
# packaging adds (disktidy.config, Lua libraries, Picross puzzles...) again
tar -xf rockbox-full.tar -C "$C" --skip-old-files --exclude='*.rock' --exclude='*.ovl' \
    .rockbox/rocks
cp -r "$A/themes/.rockbox/." "$C/.rockbox/"
cp "$A/rockgpt-C2-3k.llm" "$C/.rockbox/rocks/apps/rockgpt.llm"
cp "$A/draft-D0kd.llm" "$C/.rockbox/rocks/apps/rockgpt-draft.llm"
echo "module ID $(sed 's/.*= //; s/;//' module_build_id.lds); card tree $(du -sh "$C/.rockbox" | cut -f1)," \
     "$(find "$C/.rockbox/rocks" -name '*.rock' | wc -l) plugins," \
     "$(ls "$C/.rockbox/themes" | wc -l) themes"

PKG=rockbox-yp3-$VERSION.tar.xz
python3 "$MKPKG" yp3 --placer "$SRC/tools/yp3/package.py" --elf "$B/rockbox.elf" \
    --codecs "$B/lib/rbcodec/codecs/xip/codecs.flash" --card "$C" \
    --version "$VERSION" --title "Rockbox for the YP3" -o "$OUT/$PKG"
cd "$OUT"
sha256sum "$PKG" > "$PKG.sha256"
cat "$PKG.sha256"
