#!/bin/sh
# Runs inside the macOS container: build both architectures, glue them
# into one universal binary each, wrap the GUI in an .app and sign both.
set -e
cd /src
# the oldest macOS it runs on, whatever SDK it is built with: matches
# LSMinimumSystemVersion in packaging/macos/Info.plist
export MACOSX_DEPLOYMENT_TARGET=11.0
for arch in x86_64 aarch64; do
    t=$arch-apple-darwin
    cc=$(ls /osxcross/target/bin/ | grep -E "^$( [ $arch = aarch64 ] && echo arm64 || echo x86_64 )-apple-darwin[0-9.]*-clang$" | head -1)
    [ -n "$cc" ] || { echo "no osxcross clang for $arch"; exit 1; }
    up=$(echo $t | tr a-z- A-Z_)
    export CARGO_TARGET_${up}_LINKER=$cc CC_$(echo $t | tr - _)=$cc
    export AR_$(echo $t | tr - _)=$(echo $cc | sed 's/clang$/ar/')
    cargo build --release --target $t --target-dir target/mac
done
lipo=$(ls /osxcross/target/bin/ | grep -E 'lipo$' | head -1)
universal() {
    $lipo -create target/mac/x86_64-apple-darwin/release/$1 target/mac/aarch64-apple-darwin/release/$1 -output "$2"
}
mkdir -p dist
rm -f dist/rockslop-macos   # older builds left the bare GUI binary here
app=dist/Rockslop.app
rm -rf $app
mkdir -p $app/Contents/MacOS $app/Contents/Resources
universal rockslop $app/Contents/MacOS/rockslop
cp packaging/macos/Info.plist $app/Contents/
universal rockslop-cli dist/rockslop-cli-macos
# Ad hoc signatures (no Apple identity) over the whole bundle, Info.plist
# included: a downloaded copy then gets macOS's "Open Anyway" in Privacy &
# Security, not "damaged". Notarization would need an Apple developer account.
rcodesign sign $app
rcodesign sign --binary-identifier io.github.rockslop.cli dist/rockslop-cli-macos
# (rcodesign verify reports ad hoc signatures, which have no CMS blob, as
# broken: check what was written instead)
signed() {   # FILE PATTERN...: both slices ad hoc signed, binding each PATTERN
    info=$(rcodesign print-signature-info "$1")
    f=$1; shift
    for want in "flags: CodeSignatureFlags(ADHOC)" "$@"; do
        n=$(printf '%s\n' "$info" | grep -cF "$want")
        [ "$n" = 2 ] || { echo "$f: \"$want\" in $n of 2 slices"; exit 1; }
    done
}
signed $app/Contents/MacOS/rockslop "identifier: io.github.rockslop.Flasher" "'Info (1): " "'Resources (3): "
signed dist/rockslop-cli-macos "identifier: io.github.rockslop.cli"
(cd dist && rm -f Rockslop-macos.zip && zip -qry Rockslop-macos.zip Rockslop.app rockslop-cli-macos)
ls -l dist
