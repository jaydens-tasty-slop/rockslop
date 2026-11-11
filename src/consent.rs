//! The first flash's waiver: the player can be bricked, its warranty is
//! void, and it never goes back to the seller. Signed once per computer
//! (the GUI and the CLI share it), before rockslop first writes Rockbox to
//! a player. Returning to stock never asks.
//!
//!   <data>/rockslop/waiver    "rockslop waiver 1", then when and how

use crate::device::backup;
use std::path::PathBuf;

/// Bumped when the terms change enough to ask again.
pub const VERSION: u32 = 1;

/// Each clause: its heading, then what it means. English; the GUI
/// translates them (each is a key in strings.txt).
pub const CLAUSES: [(&str, &str); 3] = [
    (
        "THIS CAN BRICK YOUR PLAYER",
        "Rockbox on this player is unofficial and experimental. A flash that goes wrong (a loose cable, a crash, a bug of ours) can leave the player dead, and there may be no way to bring it back. rockslop keeps a backup of the stock firmware, but a backup can't save every player. The risk is yours alone.",
    ),
    (
        "YOUR WARRANTY IS VOID",
        "Flashing firmware the maker didn't write voids the warranty. Whatever happens to this player from now on is yours to deal with: no repair, no replacement, no refund. Going back to stock doesn't change that.",
    ),
    (
        "DON'T TAKE IT BACK",
        "Never return a flashed player, claim on it, or complain to the seller or the maker about it. Rockbox runs on these players because nobody has locked them down. If flashed players start coming back, the next batch will be locked, and that's the end of Rockbox on them for everyone. If yours breaks, bring it to rockslop, never to the shop.",
    ),
];

/// Said before the clauses: how this software was made.
pub const VIBES: &str = "rockslop and this player's Rockbox port are 100% vibe-coded: no human has ever looked at the code. It has been tested on real players, but that's all.";

/// What signing says, in the first person.
pub const STATEMENT: &str = "I've read all three. If it bricks, that's on me. The warranty is gone. It never goes back to the seller. No backsies.";

fn file() -> PathBuf {
    backup::data_dir().join("waiver")
}

/// This computer has signed this version (or a later one).
pub fn signed() -> bool {
    std::fs::read_to_string(file())
        .ok()
        .and_then(|s| s.lines().next()?.strip_prefix("rockslop waiver ")?.trim().parse::<u32>().ok())
        .is_some_and(|v| v >= VERSION)
}

/// Record the signature; `how` says where (the GUI, or the CLI and how).
pub fn sign(how: &str) -> std::io::Result<()> {
    std::fs::create_dir_all(backup::data_dir())?;
    std::fs::write(file(), format!("rockslop waiver {VERSION}\nsigned {} UTC, {how}, rockslop {}\n", backup::stamp_now(), crate::VERSION))
}
