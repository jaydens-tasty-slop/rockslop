//! Rockbox releases on GitHub, one per push to the port's master:
//!
//! ```text
//! tag    d0ca50f732-261121             the version Rockbox's boot screen shows
//! asset  rockbox-yp3-d0ca50f732-261121.tar.xz   a package (pkg.rs), per player
//! ```
//!
//! The list comes from the REST API with no token: 60 calls an hour per
//! address (an unchanged list, ETag and 304, saves the transfer, not the
//! call). rockslop asks once a run, and again after ten minutes at most.
//! Packages download into the cache and stay there, a few per player, so a
//! second flash (or one offline) doesn't fetch again.
//!
//! Editing a release on GitHub steers rockslop: a pre-release is listed
//! but never picked by itself, and one whose title or notes say
//! `rockslop: hide` isn't listed at all (a bad build, say).
//!
//! ```text
//! <cache>/rockslop/releases.json        the last list, its ETag beside it
//! <cache>/rockslop/firmware/<release id>/rockbox-yp3-....tar.xz
//! ```

use crate::err;
use crate::pkg::{hex, unhex32};
use sha2::{Digest, Sha256};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::Duration;

/// Where the firmware is released.
pub const REPO: &str = "jaydens-tasty-slop/rockbox";

/// Downloaded packages kept per player.
const KEEP: usize = 3;

/// The releases list's URL. `ROCKSLOP_RELEASES` overrides it (a fork, a
/// test server; plain http is allowed then).
fn list_url() -> (String, bool) {
    match std::env::var("ROCKSLOP_RELEASES") {
        Ok(u) if !u.is_empty() => (u, false),
        _ => (format!("https://api.github.com/repos/{REPO}/releases?per_page=100"), true),
    }
}

/// The page a person can look at.
pub fn page() -> String {
    format!("https://github.com/{REPO}/releases")
}

#[derive(Clone, Debug, PartialEq)]
pub struct Asset {
    pub name: String,
    pub url: String,
    pub size: u64,
    /// GitHub's digest of the file, where it gives one
    pub sha256: Option<[u8; 32]>,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Release {
    pub id: u64,
    /// the version
    pub tag: String,
    /// YYYY-MM-DD
    pub date: String,
    pub prerelease: bool,
    pub assets: Vec<Asset>,
}

impl Release {
    /// Its package for `target` (`yp3`, `ts17`).
    pub fn asset(&self, target: &str) -> Option<&Asset> {
        self.assets.iter().find(|a| is_for(&a.name, target))
    }
}

/// `rockbox-<target>-<version>.tar.xz`: the target exactly (a variant
/// would be `rockbox-yp3dev-...`), and not the `.sha256` beside it.
pub fn is_for(name: &str, target: &str) -> bool {
    name.strip_prefix("rockbox-").and_then(|r| r.strip_prefix(target)).is_some_and(|r| r.starts_with('-')) && name.ends_with(".tar.xz")
}

#[derive(Clone, Debug, Default)]
pub struct List {
    /// newest first, as GitHub lists them
    pub releases: Vec<Release>,
    /// GitHub couldn't be asked: this is the list saved last time, and why
    pub stale: Option<String>,
}

impl List {
    /// The releases with a package for `target`, newest first.
    pub fn for_target(&self, target: &str) -> impl Iterator<Item = (&Release, &Asset)> {
        let target = target.to_string();
        self.releases.iter().filter_map(move |r| Some((r, r.asset(&target)?)))
    }

    /// The one to flash unless told otherwise: the newest that isn't a
    /// prerelease.
    pub fn latest(&self, target: &str) -> Option<(&Release, &Asset)> {
        self.for_target(target).find(|(r, _)| !r.prerelease)
    }

    /// The release tagged `tag`, with its package for `target`.
    pub fn find(&self, target: &str, tag: &str) -> Option<(&Release, &Asset)> {
        self.for_target(target).find(|(r, _)| r.tag == tag)
    }
}

/// rockslop's folder in the user's cache directory.
pub fn cache_dir() -> PathBuf {
    let base = if cfg!(windows) {
        std::env::var_os("LOCALAPPDATA").map(PathBuf::from)
    } else if cfg!(target_os = "macos") {
        std::env::var_os("HOME").map(|h| PathBuf::from(h).join("Library/Caches"))
    } else {
        std::env::var_os("XDG_CACHE_HOME")
            .map(PathBuf::from)
            .or_else(|| std::env::var_os("HOME").map(|h| PathBuf::from(h).join(".cache")))
    };
    base.unwrap_or_else(std::env::temp_dir).join("rockslop")
}

fn agent(https_only: bool) -> ureq::Agent {
    ureq::Agent::config_builder()
        .user_agent(format!("rockslop/{}", crate::VERSION))
        .https_only(https_only)
        .http_status_as_error(false)
        .timeout_connect(Some(Duration::from_secs(15)))
        .timeout_recv_response(Some(Duration::from_secs(30)))
        // a stalled download gives up; a slow one doesn't
        .timeout_per_read(Some(Duration::from_secs(60)))
        .build()
        .into()
}

fn unreachable(e: ureq::Error) -> String {
    err!(E601, "can't reach GitHub: {e}")
}

fn nonsense(what: &str) -> String {
    err!(E603, "GitHub's list of releases makes no sense: {}", what)
}

/// What GitHub said, when it isn't the answer: its status and message.
fn refused(status: u16, body: &str) -> String {
    let m = serde_json::from_str::<serde_json::Value>(body).ok().and_then(|v| v["message"].as_str().map(str::to_string));
    err!(E602, "GitHub answered {}", m.map_or(status.to_string(), |m| format!("{status} ({m})")))
}

/// The list of releases: from GitHub, or the one saved last time when
/// GitHub can't be reached (with why, in `stale`).
pub fn list() -> Result<List, String> {
    let dir = cache_dir();
    let (saved, tag) = (dir.join("releases.json"), dir.join("releases.etag"));
    let old = std::fs::read_to_string(&saved).ok();
    let fresh = (|| {
        let (url, https) = list_url();
        let mut req = agent(https).get(&url).header("Accept", "application/vnd.github+json").header("X-GitHub-Api-Version", "2022-11-28");
        if let (Some(_), Ok(etag)) = (&old, std::fs::read_to_string(&tag)) {
            req = req.header("If-None-Match", etag.trim());
        }
        let mut res = req.call().map_err(unreachable)?;
        let status = res.status().as_u16();
        if status == 304 {
            return Ok(None);
        }
        let etag = res.headers().get("etag").and_then(|v| v.to_str().ok()).map(str::to_string);
        let body = res.body_mut().with_config().limit(16 << 20).read_to_string().map_err(unreachable)?;
        if status != 200 {
            return Err(refused(status, &body));
        }
        let list = parse(&body)?;
        // saved only once it reads: a broken answer doesn't replace a good one
        if std::fs::create_dir_all(&dir).is_ok() && std::fs::write(&saved, &body).is_ok() {
            match etag {
                Some(e) => drop(std::fs::write(&tag, e)),
                None => drop(std::fs::remove_file(&tag)),
            }
        }
        Ok(Some(list))
    })();
    match (fresh, old) {
        (Ok(Some(releases)), _) => Ok(List { releases, stale: None }),
        (Ok(None), Some(old)) => Ok(List { releases: parse(&old)?, stale: None }),
        (Ok(None), None) => Err(nonsense("304 with nothing saved")),
        (Err(e), Some(old)) => match parse(&old) {
            Ok(releases) => Ok(List { releases, stale: Some(e) }),
            Err(_) => Err(e),
        },
        (Err(e), None) => Err(e),
    }
}

/// GitHub's JSON list of releases. Drafts (never there without a token)
/// and assets still uploading are left out.
pub fn parse(body: &str) -> Result<Vec<Release>, String> {
    let bad = nonsense;
    let v: serde_json::Value = serde_json::from_str(body).map_err(|e| bad(&e.to_string()))?;
    let mut out = Vec::new();
    for r in v.as_array().ok_or_else(|| bad("not a list"))? {
        if r["draft"].as_bool() == Some(true) || hidden(r) {
            continue;
        }
        let (Some(id), Some(tag)) = (r["id"].as_u64(), r["tag_name"].as_str()) else { continue };
        let date = r["published_at"].as_str().or(r["created_at"].as_str()).and_then(|d| d.get(..10)).unwrap_or("").to_string();
        let assets = r["assets"]
            .as_array()
            .map(|l| {
                l.iter()
                    .filter(|a| a["state"].as_str().is_none_or(|s| s == "uploaded"))
                    .filter_map(|a| {
                        Some(Asset {
                            name: a["name"].as_str()?.to_string(),
                            url: a["browser_download_url"].as_str()?.to_string(),
                            size: a["size"].as_u64()?,
                            sha256: a["digest"].as_str().and_then(|d| d.strip_prefix("sha256:")).and_then(unhex32),
                        })
                    })
                    .collect()
            })
            .unwrap_or_default();
        out.push(Release { id, tag: tag.to_string(), date, prerelease: r["prerelease"].as_bool() == Some(true), assets });
    }
    Ok(out)
}

/// Hidden from rockslop on GitHub: `rockslop: hide` in the release's title
/// or notes (any case, any spacing).
fn hidden(r: &serde_json::Value) -> bool {
    [&r["name"], &r["body"]].iter().filter_map(|v| v.as_str()).any(|s| {
        let s: String = s.chars().filter(|c| !c.is_whitespace()).collect::<String>().to_lowercase();
        s.contains("rockslop:hide")
    })
}

/// A file name GitHub gave that is safe to use as one here.
fn safe_name(n: &str) -> bool {
    !n.is_empty() && !n.starts_with('.') && n.chars().all(|c| c.is_ascii_alphanumeric() || "-_.+".contains(c))
}

/// Where release `rel`'s `a` is kept once downloaded.
fn path_of(rel: &Release, a: &Asset) -> Option<PathBuf> {
    safe_name(&a.name).then(|| cache_dir().join("firmware").join(rel.id.to_string()).join(&a.name))
}

/// The downloaded copy, if there is one (by its size: `fetch` checks it
/// properly).
pub fn cached(rel: &Release, a: &Asset) -> Option<PathBuf> {
    path_of(rel, a).filter(|p| std::fs::metadata(p).is_ok_and(|m| m.len() == a.size))
}

fn file_sha256(p: &Path) -> std::io::Result<[u8; 32]> {
    let mut f = std::fs::File::open(p)?;
    let mut h = Sha256::new();
    let mut buf = vec![0u8; 1 << 16];
    loop {
        let n = f.read(&mut buf)?;
        if n == 0 {
            return Ok(h.finalize().into());
        }
        h.update(&buf[..n]);
    }
}

/// The message of a download stopped by `cancel`.
pub const CANCELLED: &str = "cancelled";

/// Release `rel`'s package `a`, downloaded into the cache unless it is
/// there already: its path. `progress(bytes, of)` as it comes; `cancel`
/// stops it (the error is then `CANCELLED`).
pub fn fetch(rel: &Release, a: &Asset, cancel: &AtomicBool, mut progress: impl FnMut(u64, u64)) -> Result<PathBuf, String> {
    let path = path_of(rel, a).ok_or_else(|| err!(E604, "GitHub gave the package an odd name: {}", a.name))?;
    let save = |e: std::io::Error| err!(E605, "can't save the download: {e}");
    if std::fs::metadata(&path).is_ok_and(|m| m.len() == a.size) {
        match a.sha256 {
            Some(want) if file_sha256(&path).map_err(save)? != want => drop(std::fs::remove_file(&path)),
            _ => {
                progress(a.size, a.size);
                return Ok(path);
            }
        }
    }
    let dir = path.parent().unwrap_or(Path::new("."));
    std::fs::create_dir_all(dir).map_err(save)?;
    // one per process, so two rockslops don't write the same file
    let part = dir.join(format!("{}.{}.part", a.name, std::process::id()));
    let r = (|| {
        let mut res = agent(list_url().1).get(&a.url).call().map_err(unreachable)?;
        let status = res.status().as_u16();
        if status != 200 {
            let body = res.body_mut().read_to_string().unwrap_or_default();
            return Err(refused(status, &body));
        }
        let mut body = res.into_body().into_reader();
        let mut f = std::fs::File::create(&part).map_err(save)?;
        let mut h = Sha256::new();
        let mut buf = vec![0u8; 1 << 16];
        let mut got = 0u64;
        loop {
            if cancel.load(Ordering::Relaxed) {
                return Err(CANCELLED.to_string());
            }
            let n = body.read(&mut buf).map_err(|e| err!(E606, "the download stopped: {e}"))?;
            if n == 0 {
                break;
            }
            got += n as u64;
            if got > a.size {
                break;
            }
            h.update(&buf[..n]);
            f.write_all(&buf[..n]).map_err(save)?;
            progress(got, a.size);
        }
        if got != a.size {
            return Err(err!(E607, "the download is the wrong size: {} bytes, not {}", got, a.size));
        }
        let sum: [u8; 32] = h.finalize().into();
        if a.sha256.is_some_and(|want| want != sum) {
            return Err(err!(E608, "the download doesn't match GitHub's checksum: sha256 {}", hex(&sum)));
        }
        f.sync_all().map_err(save)?;
        drop(f);
        std::fs::rename(&part, &path).map_err(save)
    })();
    if r.is_err() {
        let _ = std::fs::remove_file(&part);
    }
    r?;
    prune(&path);
    Ok(path)
}

/// Keep the newest few packages per player: the one just downloaded, and
/// the most recently fetched others for the same player.
fn prune(keep: &Path) {
    let Some(name) = keep.file_name().and_then(|n| n.to_str()) else { return };
    let Some(target) = name.strip_prefix("rockbox-").and_then(|r| r.split('-').next()) else { return };
    let root = cache_dir().join("firmware");
    let mut found: Vec<(std::time::SystemTime, PathBuf)> = std::fs::read_dir(&root)
        .into_iter()
        .flatten()
        .flatten()
        .flat_map(|d| std::fs::read_dir(d.path()).into_iter().flatten().flatten())
        .filter(|f| f.file_name().to_str().is_some_and(|n| is_for(n, target)))
        .filter_map(|f| Some((f.metadata().ok()?.modified().ok()?, f.path())))
        .collect();
    found.sort_by(|a, b| b.0.cmp(&a.0));
    for (_, p) in found.into_iter().filter(|(_, p)| p != keep).skip(KEEP - 1) {
        crate::log::line("FETCH", &format!("dropping an old download: {}", p.display()));
        let _ = std::fs::remove_file(&p);
        if let Some(d) = p.parent() {
            let _ = std::fs::remove_dir(d); // only if that left it empty
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn names() {
        assert!(is_for("rockbox-yp3-d0ca50f732-261121.tar.xz", "yp3"));
        assert!(!is_for("rockbox-yp3-d0ca50f732-261121.tar.xz.sha256", "yp3"));
        assert!(!is_for("rockbox-yp3dev-d0ca50f732-261121.tar.xz", "yp3"));
        assert!(!is_for("rockbox-yp3-d0ca50f732-261121.tar.xz", "ts17"));
        assert!(!is_for("yp3-assets-1.tar.xz", "yp3"));
        assert!(safe_name("rockbox-yp3-d0ca50f732-261121.tar.xz"));
        assert!(!safe_name("../x.tar.xz") && !safe_name("a/b.tar.xz") && !safe_name(".hidden") && !safe_name("a\\b"));
    }

    #[test]
    fn parses_github() {
        let body = r#"[
          {"id": 3, "tag_name": "d0ca50f732-261121", "name": "Rockbox for the YP3 d0ca50f732-261121",
           "draft": false, "prerelease": false, "published_at": "2026-11-21T20:00:00Z",
           "assets": [
             {"name": "rockbox-yp3-d0ca50f732-261121.tar.xz", "state": "uploaded", "size": 34000000,
              "browser_download_url": "https://github.com/x/y/releases/download/t/rockbox-yp3-d0ca50f732-261121.tar.xz",
              "digest": "sha256:47fe9ad6074d83b8d2a0619bdfb66a021bd94ba4e86abdfaeda574952ed038af"},
             {"name": "rockbox-yp3-d0ca50f732-261121.tar.xz.sha256", "state": "uploaded", "size": 100,
              "browser_download_url": "https://github.com/x/y/releases/download/t/s", "digest": null}]},
          {"id": 2, "tag_name": "yp3-assets-1", "name": "", "draft": false, "prerelease": true,
           "published_at": "2026-11-20T10:00:00Z",
           "assets": [{"name": "yp3-assets-1.tar.xz", "state": "uploaded", "size": 5, "browser_download_url": "https://x"}]},
          {"id": 1, "tag_name": "1111111111-261119", "draft": false, "prerelease": false, "published_at": null,
           "created_at": "2026-11-19T10:00:00Z",
           "assets": [{"name": "rockbox-yp3-1111111111-261119.tar.xz", "state": "starter", "size": 5, "browser_download_url": "https://x"}]},
          {"id": 5, "tag_name": "5555555555-261122", "name": "Rockbox (broken)", "body": "Boots to a black screen.\r\n\r\nRockslop:  Hide",
           "draft": false, "prerelease": false, "published_at": "2026-11-22T10:00:00Z",
           "assets": [{"name": "rockbox-yp3-5555555555-261122.tar.xz", "state": "uploaded", "size": 5, "browser_download_url": "https://x"}]},
          {"id": 4, "tag_name": "4444444444-261122", "name": "rockslop:hide", "body": null,
           "draft": false, "prerelease": false, "published_at": "2026-11-22T09:00:00Z",
           "assets": [{"name": "rockbox-yp3-4444444444-261122.tar.xz", "state": "uploaded", "size": 5, "browser_download_url": "https://x"}]}
        ]"#;
        let l = List { releases: parse(body).unwrap(), stale: None };
        assert_eq!(l.releases.len(), 3);
        let (r, a) = l.latest("yp3").unwrap();
        assert_eq!((r.tag.as_str(), r.date.as_str(), a.size), ("d0ca50f732-261121", "2026-11-21", 34000000));
        assert_eq!(a.sha256.map(|s| hex(&s)).as_deref(), Some("47fe9ad6074d83b8d2a0619bdfb66a021bd94ba4e86abdfaeda574952ed038af"));
        // the build inputs aren't firmware; an asset still uploading isn't there
        assert_eq!(l.for_target("yp3").count(), 1);
        assert_eq!(l.releases[2].date, "2026-11-19");
        assert!(l.latest("ts17").is_none());
        // hidden on GitHub, in the notes or the title: not there
        assert!(l.find("yp3", "5555555555-261122").is_none() && l.find("yp3", "4444444444-261122").is_none());
        assert!(parse("{\"message\": \"Not Found\"}").is_err());
    }
}
