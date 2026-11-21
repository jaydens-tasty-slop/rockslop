//! Device and package logic shared by the flasher's GUI and its CLI: no
//! graphics here.

/// This build: the commit's time (UTC) and short hash, `20261028_141519-1a2b3c4`
/// (build.rs).
pub const VERSION: &str = env!("ROCKSLOP_VERSION");

/// The project's website.
pub const WEBSITE: &str = "https://rockslop.jayd.ml";

pub mod consent;
pub mod device;
pub mod log;
pub mod pkg;
pub mod releases;
