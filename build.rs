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

/// Windows: each .exe gets version info, the icon and a manifest (run as
/// the user, Windows 10/11). An .exe with none of them is one more thing
/// antivirus heuristics count against it.
fn windows_resources(root: &Path, version: &str) {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() != Ok("windows") {
        return;
    }
    let win = root.join("packaging/windows");
    let (icon, manifest) = (win.join("rockslop.ico"), win.join("rockslop.manifest"));
    for f in [&icon, &manifest] {
        println!("cargo:rerun-if-changed={}", f.display());
    }
    // FILEVERSION is four 16-bit numbers: 20261114_143546-1a2b3c4 is
    // 2026,1114,1435,46 (dev is 0,0,0,0)
    let digits: String = version.split('-').next().unwrap_or("").chars().filter(char::is_ascii_digit).collect();
    let num = |a: usize, b: usize| digits.get(a..b).and_then(|s| s.parse::<u16>().ok()).unwrap_or(0);
    let v = format!("{},{},{},{}", num(0, 4), num(4, 8), num(8, 12), num(12, 14));
    let text: String = version.chars().filter(|c| c.is_ascii_alphanumeric() || "._+-".contains(*c)).collect();
    let fwd = |p: &Path| p.display().to_string().replace('\\', "/");
    let out = Path::new(&std::env::var("OUT_DIR").unwrap()).to_path_buf();
    for (bin, what) in [
        ("rockslop", "rockslop, a Rockbox flasher for the YP3 and TS17"),
        ("rockslop-cli", "rockslop-cli, a Rockbox flasher for the YP3 and TS17 (command line)"),
    ] {
        // numbers, not winver.h's names, so no header is needed:
        // 24 RT_MANIFEST, 0x40004 VOS_NT_WINDOWS32, 1 VFT_APP
        let rc = format!(
            "1 ICON \"{icon}\"\n1 24 \"{manifest}\"\n\
             1 VERSIONINFO\nFILEVERSION {v}\nPRODUCTVERSION {v}\nFILEFLAGSMASK 0x3f\nFILEFLAGS 0\n\
             FILEOS 0x40004\nFILETYPE 1\nFILESUBTYPE 0\nBEGIN\n\
             BLOCK \"StringFileInfo\"\nBEGIN\nBLOCK \"040904b0\"\nBEGIN\n\
             VALUE \"CompanyName\", \"rockslop\"\n\
             VALUE \"FileDescription\", \"{what}\"\n\
             VALUE \"FileVersion\", \"{text}\"\n\
             VALUE \"InternalName\", \"{bin}\"\n\
             VALUE \"LegalCopyright\", \"GPL-2.0-or-later\"\n\
             VALUE \"OriginalFilename\", \"{bin}.exe\"\n\
             VALUE \"ProductName\", \"rockslop\"\n\
             VALUE \"ProductVersion\", \"{text}\"\n\
             END\nEND\n\
             BLOCK \"VarFileInfo\"\nBEGIN\nVALUE \"Translation\", 0x409, 1200\nEND\nEND\n",
            icon = fwd(&icon),
            manifest = fwd(&manifest),
        );
        let path = out.join(format!("{bin}.rc"));
        std::fs::write(&path, rc).unwrap();
        embed_resource::compile_for(&path, [bin], embed_resource::NONE).manifest_required().unwrap();
    }
}

fn main() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR"));
    let version = version(root);
    println!("cargo:rustc-env=ROCKSLOP_VERSION={version}");
    windows_resources(root, &version);
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
