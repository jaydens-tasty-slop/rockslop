# miniquad 0.4.11, patched

A copy of [miniquad](https://github.com/not-fl3/miniquad) 0.4.11 from
crates.io (MIT OR Apache-2.0, see the licence files here), with three fixes,
marked `rockslop patch` in the source:

- **X11 drag and drop** (`src/native/linux_x11/drag_n_drop.rs`,
  `linux_x11.rs`, `libx11.rs`). Upstream accepts a drop only in the
  `UTF8_STRING` format and reads each line as a plain path. File managers
  offer `text/uri-list` (through XWayland, only that), and both formats
  carry percent-encoded `file://` URIs, so no file drop ever arrived. The
  patch prefers `text/uri-list`, decodes the URIs, and sends `XdndFinished`
  once the data is in, as the XDND spec asks.
- **High-DPI window size** (`src/native/linux_x11/libx11_ex.rs`,
  `windows.rs`). With `high_dpi`, macOS takes the configured window size in
  points, but X11 and Windows (once the process is DPI aware) took it as
  pixels, so the window opened at half size on a 2x screen. The patch
  scales it by the display's scale (`Xft.dpi` / 96 on X11, the primary
  monitor's effective DPI / 96 on Windows), keeping it within 90% of the
  screen.
- **Multisample resolves** (`src/graphics/gl.rs`, `gl/cache.rs`). Upstream
  resolves a multisampled pass into its texture at the end of every pass,
  and macroquad makes one pass per draw call: a frame drawing a few dozen
  things into a 4x MSAA target resolved the whole target a few dozen times
  (38 to 72 a frame here; on Apple's tile-based GPUs each pass also loads
  and stores the whole multisampled buffer). The patch leaves the resolve
  pending until a different pass begins, pixels are read, the pass is
  deleted, or the frame ends, so drawing on into the same target needs
  none: one resolve a frame, the same pixels.

Only `src/`, `build.rs`, `Cargo.toml`, the README and the licences are
copied; the Android and web glue upstream ships is not needed here.

For upstream, each fix is a branch on upstream's `master` in
<https://github.com/jaydens-tasty-slop/miniquad>: `x11-drop-uris`,
`high-dpi-window-size` and `gl-defer-msaa-resolve`. Upstream's b09486b
(after 0.4.11) already takes `text/uri-list` drops and strips `file://`,
so `x11-drop-uris` adds only the percent-decoding and `XdndFinished`.
The `#![allow]` in `src/lib.rs` (upstream's warnings) is not upstreamed.
