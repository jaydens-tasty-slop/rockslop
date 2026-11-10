# xX ROCKSLOP FLASHER Xx
#
#   make                debug build for this machine (GUI + CLI)
#   make release        optimised build for this machine
#   make demo           run the GUI against a simulated player
#   make test           unit tests
#   make linux          portable Linux binaries in dist/ (container)
#   make linux-arm64    the same for arm64 (container, cross gcc)
#   make windows        Windows .exe files in dist/ (container, MinGW)
#   make windows-arm64  the same for arm64 (container, llvm-mingw)
#   make macos          universal macOS .app + CLI in dist/ (container, osxcross;
#                       needs an SDK tarball in packaging/containers/sdk/)
#   make flatpak        dist/rockslop.flatpak (needs flatpak-builder)
#   make dist           all of the above
#   make payloads       rebuild the on-device payloads (committed prebuilt)
#   make package        a release package, see packaging/mkpkg.py --help

PODMAN       ?= podman
CARGO_VOL     = -v rockslop-cargo:/usr/local/cargo/registry
# git inside the container reads the version (build.rs) from a checkout
# owned by another uid
GIT_SAFE      = -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory -e GIT_CONFIG_VALUE_0=/src
RUN           = $(PODMAN) run --rm $(CARGO_VOL) $(GIT_SAFE) -e ROCKSLOP_VERSION -v $(CURDIR):/src:z -w /src
WINTARGET     = x86_64-pc-windows-gnu
ARMTARGET     = aarch64-unknown-linux-gnu
WINARMTARGET  = aarch64-pc-windows-gnullvm

.PHONY: all build release demo test linux linux-arm64 windows windows-arm64 macos flatpak dist payloads package clean

all: build

build:
	cargo build

release:
	cargo build --release

demo: build
	./target/debug/rockslop --demo

test:
	cargo test

image-%:
	$(PODMAN) build -t rockslop-$* -f packaging/containers/Containerfile.$* packaging/containers

linux: image-linux
	$(RUN) rockslop-linux cargo build --release --target-dir target/linux
	mkdir -p dist
	cp target/linux/release/rockslop dist/rockslop-linux-x86_64
	cp target/linux/release/rockslop-cli dist/rockslop-cli-linux-x86_64

linux-arm64: image-linux-arm64
	$(RUN) rockslop-linux-arm64 cargo build --release --target $(ARMTARGET) --target-dir target/linux-arm64
	mkdir -p dist
	cp target/linux-arm64/$(ARMTARGET)/release/rockslop dist/rockslop-linux-arm64
	cp target/linux-arm64/$(ARMTARGET)/release/rockslop-cli dist/rockslop-cli-linux-arm64

windows: image-windows
	$(RUN) rockslop-windows cargo build --release --target $(WINTARGET) --target-dir target/win
	mkdir -p dist
	cp target/win/$(WINTARGET)/release/rockslop.exe target/win/$(WINTARGET)/release/rockslop-cli.exe dist/

windows-arm64: image-windows-arm64
	$(RUN) rockslop-windows-arm64 cargo build --release --target $(WINARMTARGET) --target-dir target/win-arm64
	mkdir -p dist
	cp target/win-arm64/$(WINARMTARGET)/release/rockslop.exe dist/rockslop-arm64.exe
	cp target/win-arm64/$(WINARMTARGET)/release/rockslop-cli.exe dist/rockslop-cli-arm64.exe

macos: image-macos
	$(RUN) rockslop-macos packaging/macos-build.sh

flatpak: linux
	flatpak-builder --user --install-deps-from=flathub --force-clean --repo=target/flatpak-repo \
		target/flatpak-build packaging/flatpak/io.github.rockslop.Flasher.yml
	flatpak build-bundle target/flatpak-repo dist/rockslop.flatpak io.github.rockslop.Flasher

dist: linux linux-arm64 windows windows-arm64 macos flatpak

payloads:
	payloads/build.sh

package:
	@python3 packaging/mkpkg.py --help

clean:
	cargo clean
	rm -rf dist
