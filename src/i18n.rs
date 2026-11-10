//! Translations. The English strings are the keys (`assets/lang/strings.txt`
//! lists them); each `assets/lang/<language>.txt` gives one Rockbox
//! language's translation. A missing key falls back to English.
//!
//! - `tr("FLASH")` translates a fixed string;
//! - `tf("{0} files  ·  {1}", &[&n, &size])` fills numbered placeholders;
//! - `msg(line)` translates a device log line or error by matching it
//!   against the keys that are templates (the device code stays English,
//!   for the CLI).

use std::collections::HashMap;
use std::fmt::Display;
use std::sync::{Mutex, OnceLock, RwLock};

mod data {
    include!(concat!(env!("OUT_DIR"), "/langs.rs"));
}

const STRINGS: &str = include_str!("../assets/lang/strings.txt");

/// Which CJK glyph forms a language wants (Han characters differ by region).
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Cjk {
    Sc,
    Tc,
    Jp,
    Kr,
}

pub struct Lang {
    /// Rockbox's name for it (its .lang file and our .txt file)
    pub id: &'static str,
    /// what it calls itself
    pub name: &'static str,
    /// BCP 47 tags the operating system may report for it
    tags: &'static [&'static str],
    pub rtl: bool,
    pub cjk: Cjk,
}

const fn l(id: &'static str, name: &'static str, tags: &'static [&'static str]) -> Lang {
    Lang { id, name, tags, rtl: false, cjk: Cjk::Sc }
}

/// Every language Rockbox ships, in its own name.
pub const LANGS: &[Lang] = &[
    Lang { rtl: true, ..l("arabic", "العربية", &["ar"]) },
    l("basque", "Euskara", &["eu"]),
    l("bulgarian", "Български", &["bg"]),
    l("catala", "Català", &["ca"]),
    l("chinese-simp", "简体中文", &["zh-Hans", "zh-CN", "zh-SG", "zh"]),
    Lang { cjk: Cjk::Tc, ..l("chinese-trad", "繁體中文", &["zh-Hant", "zh-TW", "zh-HK", "zh-MO"]) },
    l("czech", "Čeština", &["cs"]),
    l("dansk", "Dansk", &["da"]),
    l("deutsch", "Deutsch", &["de"]),
    l("eesti", "Eesti", &["et"]),
    l("english", "English (UK)", &["en-GB", "en"]),
    l("english-us", "English (US)", &["en-US"]),
    l("espanol", "Español", &["es"]),
    l("esperanto", "Esperanto", &["eo"]),
    l("finnish", "Suomi", &["fi"]),
    l("francais", "Français", &["fr"]),
    l("galego", "Galego", &["gl"]),
    l("greek", "Ελληνικά", &["el"]),
    Lang { rtl: true, ..l("hebrew", "עברית", &["he", "iw"]) },
    l("hindi", "हिन्दी", &["hi"]),
    l("hrvatski", "Hrvatski", &["hr"]),
    l("islenska", "Íslenska", &["is"]),
    l("italiano", "Italiano", &["it"]),
    Lang { cjk: Cjk::Jp, ..l("japanese", "日本語", &["ja"]) },
    Lang { cjk: Cjk::Kr, ..l("korean", "한국어", &["ko"]) },
    l("latviesu", "Latviešu", &["lv"]),
    l("lietuviu", "Lietuvių", &["lt"]),
    l("magyar", "Magyar", &["hu"]),
    l("moldoveneste", "Moldovenească", &["ro-MD", "mo"]),
    l("nederlands", "Nederlands", &["nl"]),
    l("norsk", "Norsk bokmål", &["nb", "no"]),
    l("norsk-nynorsk", "Norsk nynorsk", &["nn"]),
    l("polski", "Polski", &["pl"]),
    l("portugues", "Português", &["pt-PT", "pt"]),
    l("portugues-brasileiro", "Português (Brasil)", &["pt-BR"]),
    l("romaneste", "Română", &["ro"]),
    l("russian", "Русский", &["ru"]),
    l("slovak", "Slovenčina", &["sk"]),
    l("slovenscina", "Slovenščina", &["sl"]),
    l("srpski", "Српски", &["sr"]),
    l("svenska", "Svenska", &["sv"]),
    l("tagalog", "Tagalog", &["tl", "fil"]),
    l("thai", "ไทย", &["th"]),
    l("turkce", "Türkçe", &["tr"]),
    l("ukrainian", "Українська", &["uk"]),
    l("vietnamese", "Tiếng Việt", &["vi"]),
    l("vlaams", "Vlaams", &["nl-BE"]),
    l("walon", "Walon", &["wa"]),
];

/// A test language: every string accented and padded by about 40%, to see
/// what long translations do to the layout (`--lang pseudo`).
pub const PSEUDO: &str = "pseudo";

type Table = HashMap<&'static str, &'static str>;

struct State {
    lang: usize,
    pseudo: bool,
    table: &'static Table,
}

fn state() -> &'static RwLock<State> {
    static S: OnceLock<RwLock<State>> = OnceLock::new();
    S.get_or_init(|| RwLock::new(State { lang: english(), pseudo: false, table: table(english()) }))
}

fn english() -> usize {
    LANGS.iter().position(|l| l.id == "english").unwrap_or(0)
}

/// The parsed translation of LANGS[i] (empty for English and for a
/// language with no file yet), parsed once and kept.
fn table(i: usize) -> &'static Table {
    static T: OnceLock<Mutex<HashMap<usize, &'static Table>>> = OnceLock::new();
    let mut all = T.get_or_init(Default::default).lock().unwrap();
    all.entry(i).or_insert_with(|| {
        let text = data::DATA.iter().find(|(id, _)| *id == LANGS[i].id).map(|d| d.1).unwrap_or("");
        Box::leak(Box::new(parse(text)))
    })
}

fn parse(text: &'static str) -> Table {
    let mut t = Table::new();
    let mut key: Option<&'static str> = None;
    for line in text.lines() {
        let line = line.strip_suffix('\r').unwrap_or(line);
        if line.trim().is_empty() || line.starts_with('#') {
            continue;
        }
        match (line.strip_prefix("= "), key.take()) {
            (Some(v), Some(k)) => {
                t.insert(k, v);
            }
            (Some(_), None) => {}
            (None, _) => key = Some(line),
        }
    }
    t
}

/// The current language.
pub fn current() -> &'static Lang {
    &LANGS[state().read().unwrap().lang]
}

pub fn pseudo() -> bool {
    state().read().unwrap().pseudo
}

/// Switch language by Rockbox name (or `PSEUDO`); false if unknown.
pub fn set(id: &str) -> bool {
    let mut s = state().write().unwrap();
    if id == PSEUDO {
        s.pseudo = true;
        s.lang = english();
        s.table = table(s.lang);
        return true;
    }
    match LANGS.iter().position(|l| l.id == id) {
        Some(i) => {
            *s = State { lang: i, pseudo: false, table: table(i) };
            true
        }
        None => false,
    }
}

/// The language the operating system asks for, if Rockbox has it.
pub fn system() -> Option<&'static str> {
    sys_locale::get_locales().find_map(|tag| from_tag(&tag))
}

/// Map a locale ("pt_BR.UTF-8", "zh-Hant-TW", "de") to a Rockbox language.
fn from_tag(tag: &str) -> Option<&'static str> {
    let tag = tag.split(['.', '@']).next().unwrap_or("").replace('_', "-");
    let parts: Vec<&str> = tag.split('-').collect();
    let lower = |s: &str| s.to_ascii_lowercase();
    // the longest of the tags we know that this one starts with
    let mut best: Option<(&'static str, usize)> = None;
    for lang in LANGS {
        for t in lang.tags {
            let tp: Vec<&str> = t.split('-').collect();
            let hit = tp.len() <= parts.len()
                && tp[0].eq_ignore_ascii_case(parts[0])
                && tp[1..].iter().all(|x| parts[1..].iter().any(|p| lower(p) == lower(x)));
            if hit && best.map(|b| tp.len() > b.1).unwrap_or(true) {
                best = Some((lang.id, tp.len()));
            }
        }
    }
    best.map(|b| b.0)
}

/// The picker's "follow the system" choice, as saved.
pub const AUTO: &str = "auto";

fn choice_file() -> std::path::PathBuf {
    let d = rockslop::device::backup::dir();
    d.parent().map(|p| p.to_path_buf()).unwrap_or(d).join("language")
}

/// The language last picked (a Rockbox name, or AUTO).
pub fn saved() -> Option<String> {
    let s = std::fs::read_to_string(choice_file()).ok()?;
    let s = s.trim();
    (s == AUTO || LANGS.iter().any(|l| l.id == s)).then(|| s.to_string())
}

/// Remember the picker's choice (best effort).
pub fn save(choice: &str) {
    let f = choice_file();
    if let Some(d) = f.parent() {
        let _ = std::fs::create_dir_all(d);
    }
    let _ = std::fs::write(f, format!("{choice}\n"));
}

/// Translate a fixed string (a key in strings.txt).
pub fn tr(key: &'static str) -> &'static str {
    let s = state().read().unwrap();
    if s.pseudo {
        return pseudo_of(key);
    }
    s.table.get(key).copied().unwrap_or(key)
}

/// Translate a template and fill in {0}, {1}, ...
pub fn tf(key: &'static str, args: &[&dyn Display]) -> String {
    fill(tr(key), args)
}

fn fill(tpl: &str, args: &[&dyn Display]) -> String {
    let mut out = String::with_capacity(tpl.len() + 16);
    let mut rest = tpl;
    while let Some(i) = rest.find('{') {
        out.push_str(&rest[..i]);
        let after = &rest[i + 1..];
        match after.find('}').and_then(|j| after[..j].parse::<usize>().ok().map(|n| (n, j))) {
            Some((n, j)) => {
                match args.get(n) {
                    Some(a) => out.push_str(&a.to_string()),
                    None => out.push_str(&rest[i..i + j + 2]),
                }
                rest = &after[j + 1..];
            }
            None => {
                out.push('{');
                rest = after;
            }
        }
    }
    out.push_str(rest);
    out
}

/// The error number an error starts with ("E214: ..."), if any.
pub fn code(line: &str) -> Option<&str> {
    split_code(line).0
}

fn split_code(line: &str) -> (Option<&str>, &str) {
    let b = line.as_bytes();
    let n = b.iter().skip(1).take_while(|c| c.is_ascii_digit()).count();
    if b.first() == Some(&b'E') && n == 3 && line[1 + n..].starts_with(": ") {
        (Some(&line[..1 + n]), &line[3 + n..])
    } else {
        (None, line)
    }
}

/// Translate a message from the device layer: an exact key, or a template
/// key whose fixed parts match (its values are carried over). An error's
/// number stays in front, untranslated.
pub fn msg(line: &str) -> String {
    match split_code(line) {
        (Some(c), rest) => format!("{c}: {}", text(rest)),
        (None, rest) => text(rest),
    }
}

fn text(line: &str) -> String {
    let s = state().read().unwrap();
    if s.table.is_empty() && !s.pseudo {
        return line.to_string();
    }
    drop(s);
    for key in templates() {
        if let Some(vals) = match_template(key, line) {
            // a value that is itself a key (a hint, say) translates too
            let vals: Vec<String> = vals.iter().map(|v| exact(v).map(|k| tr(k).to_string()).unwrap_or_else(|| v.to_string())).collect();
            let args: Vec<&dyn Display> = vals.iter().map(|v| v as &dyn Display).collect();
            return tf(key, &args);
        }
    }
    line.to_string()
}

/// `s` if it is a key with no placeholders.
fn exact(s: &str) -> Option<&'static str> {
    templates().iter().copied().find(|k| *k == s && !k.contains('{'))
}

/// The keys, the most specific (longest fixed text) first.
fn templates() -> &'static [&'static str] {
    static K: OnceLock<Vec<&'static str>> = OnceLock::new();
    K.get_or_init(|| {
        let mut k = keys();
        k.sort_by_key(|k| std::cmp::Reverse(literal_len(k)));
        k
    })
}

/// Every key in strings.txt.
pub fn keys() -> Vec<&'static str> {
    STRINGS.lines().filter(|l| !l.trim().is_empty() && !l.starts_with('#')).collect()
}

fn literal_len(tpl: &str) -> usize {
    split_template(tpl).iter().map(|p| if let Part::Lit(l) = p { l.len() } else { 0 }).sum()
}

#[derive(Debug)]
enum Part<'a> {
    Lit(&'a str),
    Arg,
}

fn split_template(tpl: &str) -> Vec<Part<'_>> {
    let mut parts = Vec::new();
    let mut rest = tpl;
    while let Some(i) = rest.find('{') {
        let after = &rest[i + 1..];
        match after.find('}') {
            Some(j) if after[..j].parse::<usize>().is_ok() => {
                if i > 0 {
                    parts.push(Part::Lit(&rest[..i]));
                }
                parts.push(Part::Arg);
                rest = &after[j + 1..];
            }
            _ => {
                parts.push(Part::Lit(&rest[..i + 1]));
                rest = after;
            }
        }
    }
    if !rest.is_empty() {
        parts.push(Part::Lit(rest));
    }
    parts
}

/// The values in `line` if it is `tpl` with its placeholders filled in
/// (each value non-empty; a value runs to the next fixed part).
fn match_template(tpl: &str, line: &str) -> Option<Vec<String>> {
    let parts = split_template(tpl);
    let mut vals = Vec::new();
    let mut pos = 0;
    let mut open = false; // a value is being read up to the next literal
    for (k, p) in parts.iter().enumerate() {
        match p {
            Part::Arg => open = true,
            Part::Lit(lit) => {
                let at = if !open {
                    if !line[pos..].starts_with(lit) {
                        return None;
                    }
                    pos
                } else if k == parts.len() - 1 {
                    if !line.ends_with(lit) || line.len() < pos + lit.len() {
                        return None;
                    }
                    line.len() - lit.len()
                } else {
                    pos + line[pos..].find(lit)?
                };
                if open {
                    if at == pos {
                        return None;
                    }
                    vals.push(line[pos..at].to_string());
                    open = false;
                }
                pos = at + lit.len();
            }
        }
    }
    if open {
        if pos >= line.len() {
            return None;
        }
        vals.push(line[pos..].to_string());
    } else if pos != line.len() {
        return None;
    }
    // map the values (in text order) back to placeholder numbers
    let order: Vec<usize> = {
        let mut o = Vec::new();
        let mut rest = tpl;
        while let Some(i) = rest.find('{') {
            let after = &rest[i + 1..];
            if let Some(j) = after.find('}') {
                if let Ok(n) = after[..j].parse::<usize>() {
                    o.push(n);
                }
                rest = &after[j + 1..];
            } else {
                break;
            }
        }
        o
    };
    let mut out = vec![String::new(); order.iter().max().map(|m| m + 1).unwrap_or(0)];
    for (v, n) in vals.into_iter().zip(order) {
        out[n] = v;
    }
    Some(out)
}

/// "[Ŝéļéçţ ţåŕĝéţ ~~~]": accented letters (so missing glyphs show up) and
/// about 40% extra length (so tight boxes show up).
fn pseudo_of(key: &'static str) -> &'static str {
    static P: OnceLock<Mutex<HashMap<&'static str, &'static str>>> = OnceLock::new();
    let mut m = P.get_or_init(Default::default).lock().unwrap();
    m.entry(key).or_insert_with(|| {
        let mut out = String::from("[");
        let mut in_ph = false;
        for c in key.chars() {
            in_ph = (in_ph || c == '{') && c != '}';
            out.push(if in_ph || c == '}' { c } else { accent(c) });
        }
        let pad = (key.chars().count() * 2).div_ceil(5);
        out.extend(std::iter::repeat_n('~', pad));
        out.push(']');
        Box::leak(out.into_boxed_str())
    })
}

fn accent(c: char) -> char {
    const FROM: &str = "aceinorstuyACEINORSTUY";
    const TO: &str = "åçéîñöŕşţûýÅÇÉÎÑÖŔŞŢÛÝ";
    FROM.chars().position(|x| x == c).and_then(|i| TO.chars().nth(i)).unwrap_or(c)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn templates_match_and_fill() {
        assert_eq!(match_template("{0} files written", "12 files written"), Some(vec!["12".into()]));
        assert_eq!(match_template("0x{0} erase / program / verify ok", "0x00e000 erase / program / verify ok"), Some(vec!["00e000".into()]));
        assert_eq!(
            match_template("the card needs {0} MiB free; it has {1} MiB", "the card needs 12 MiB free; it has 3 MiB"),
            Some(vec!["12".into(), "3".into()])
        );
        assert_eq!(match_template("{0} files written", "files written"), None);
        assert_eq!(match_template("card ejected", "card ejected!"), None);
        assert_eq!(fill("{1} / {0}", &[&1, &"b"]), "b / 1");
    }

    #[test]
    fn locale_tags() {
        assert_eq!(from_tag("de_DE.UTF-8"), Some("deutsch"));
        assert_eq!(from_tag("pt-BR"), Some("portugues-brasileiro"));
        assert_eq!(from_tag("pt"), Some("portugues"));
        assert_eq!(from_tag("zh-Hant-TW"), Some("chinese-trad"));
        assert_eq!(from_tag("zh_CN"), Some("chinese-simp"));
        assert_eq!(from_tag("en-US"), Some("english-us"));
        assert_eq!(from_tag("en-AU"), Some("english"));
        assert_eq!(from_tag("nl-BE"), Some("vlaams"));
        assert_eq!(from_tag("xx"), None);
    }

    /// Every language file: known keys only, each translated once, the
    /// same placeholders as the English. english-us only overrides a few.
    #[test]
    fn translations_are_well_formed() {
        let keys = keys();
        let set: std::collections::HashSet<&str> = keys.iter().copied().collect();
        assert_eq!(set.len(), keys.len(), "duplicate keys in strings.txt");
        let ph = |s: &str| {
            let mut v: Vec<String> = Vec::new();
            let mut rest = s;
            while let Some(i) = rest.find('{') {
                let after = &rest[i + 1..];
                match after.find('}') {
                    Some(j) if after[..j].parse::<usize>().is_ok() => {
                        v.push(after[..j].to_string());
                        rest = &after[j + 1..];
                    }
                    _ => rest = after,
                }
            }
            v.sort();
            v
        };
        let mut problems = Vec::new();
        for (id, text) in data::DATA {
            assert!(LANGS.iter().any(|l| l.id == *id), "{id}.txt is not a Rockbox language");
            let t = parse(text);
            for (k, v) in &t {
                if !set.contains(k) {
                    problems.push(format!("{id}: unknown key {k:?}"));
                } else if ph(k) != ph(v) {
                    problems.push(format!("{id}: placeholders differ in {k:?} -> {v:?}"));
                }
            }
            if *id != "english-us" {
                for k in &keys {
                    if !t.contains_key(k) {
                        problems.push(format!("{id}: missing {k:?}"));
                    }
                }
            }
        }
        assert!(problems.is_empty(), "{} problems:\n{}", problems.len(), problems.join("\n"));
    }

    /// Every tr("...") / tf("...") in the GUI's source is a key.
    #[test]
    fn source_strings_are_keys() {
        let keys: std::collections::HashSet<&str> = keys().into_iter().collect();
        let srcs = [include_str!("app.rs"), include_str!("clippy.rs"), include_str!("ui.rs"), include_str!("credits.rs"), include_str!("main.rs"), include_str!("picker.rs"), include_str!("nowplaying.rs")];
        let mut missing = Vec::new();
        for src in srcs {
            for pat in ["tr(\"", "tf(\""] {
                for (i, _) in src.match_indices(pat) {
                    // only the i18n calls, not e.g. "set(\""
                    let before = src[..i].chars().last().unwrap_or(' ');
                    if before.is_alphanumeric() || before == '_' {
                        continue;
                    }
                    let lit = &src[i + pat.len()..];
                    let end = lit.find('"').unwrap();
                    let s = unescape(&lit[..end]);
                    if !keys.contains(s.as_str()) {
                        missing.push(s);
                    }
                }
            }
        }
        assert!(missing.is_empty(), "not in strings.txt: {missing:?}");
    }

    fn unescape(s: &str) -> String {
        let mut out = String::new();
        let mut rest = s;
        while let Some(i) = rest.find("\\u{") {
            out.push_str(&rest[..i]);
            let j = rest[i..].find('}').unwrap() + i;
            out.push(char::from_u32(u32::from_str_radix(&rest[i + 3..j], 16).unwrap()).unwrap());
            rest = &rest[j + 1..];
        }
        out.push_str(rest);
        out
    }

    #[test]
    fn values_that_are_keys_translate_too() {
        set("deutsch");
        let hint = "Install the udev rule (see README) or run as root.";
        let line = format!("E212: usb open: permission denied. {hint}");
        let out = msg(&line);
        set("english");
        assert!(out.starts_with("E212: "), "{out}");
        assert!(!out.contains(hint) && out.contains("usb open"), "{out}");
    }

    #[test]
    fn pseudo_keeps_placeholders() {
        let p = pseudo_of("{0} files  ·  {1}");
        assert!(p.contains("{0}") && p.contains("{1}") && p.len() > 20, "{p}");
    }
}
