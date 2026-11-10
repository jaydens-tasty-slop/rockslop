//! Embeds every translation in assets/lang/ (one file per Rockbox language),
//! and stamps the build with its version (see `version`).

use std::fmt::Write;
use std::path::Path;
use std::process::Command;

/// `20261028_141519-1a2b3c4`: the commit's time (UTC) and its short hash.
/// `ROCKSLOP_VERSION` overrides it; without git it is `dev`.
fn version(root: &Path) -> String {
    println!("cargo:rerun-if-env-changed=ROCKSLOP_VERSION");
    if let Ok(v) = std::env::var("ROCKSLOP_VERSION") {
        return v;
    }
    let git = |args: &[&str]| {
        Command::new("git")
            .current_dir(root)
            .env("TZ", "UTC")
            .args(args)
            .output()
            .ok()
            .filter(|o| o.status.success())
            .and_then(|o| String::from_utf8(o.stdout).ok())
            .map(|s| s.trim().to_string())
            .filter(|s| !s.is_empty())
    };
    if let Some(dir) = git(&["rev-parse", "--absolute-git-dir"]) {
        // every commit, checkout or reset appends to HEAD's reflog (before
        // the first commit it doesn't exist, and this reruns every build)
        println!("cargo:rerun-if-changed={dir}/HEAD");
        println!("cargo:rerun-if-changed={dir}/logs/HEAD");
    }
    git(&["log", "-1", "--format=%cd-%h", "--date=format-local:%Y%m%d_%H%M%S"]).unwrap_or_else(|| "dev".into())
}

fn main() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR"));
    println!("cargo:rustc-env=ROCKSLOP_VERSION={}", version(root));
    let dir = root.join("assets/lang");
    println!("cargo:rerun-if-changed={}", dir.display());
    let mut names: Vec<String> = std::fs::read_dir(&dir)
        .map(|rd| {
            rd.filter_map(|e| e.ok())
                .filter_map(|e| e.file_name().into_string().ok())
                .filter(|n| n.ends_with(".txt") && n != "strings.txt")
                .collect()
        })
        .unwrap_or_default();
    names.sort();
    let mut out = String::from("/// (Rockbox language name, file contents)\npub const DATA: &[(&str, &str)] = &[\n");
    for n in &names {
        let path = dir.join(n);
        println!("cargo:rerun-if-changed={}", path.display());
        writeln!(out, "    ({:?}, include_str!({:?})),", n.trim_end_matches(".txt"), path.display().to_string()).unwrap();
    }
    out.push_str("];\n");
    let dest = Path::new(&std::env::var("OUT_DIR").unwrap()).join("langs.rs");
    std::fs::write(dest, out).unwrap();
}
