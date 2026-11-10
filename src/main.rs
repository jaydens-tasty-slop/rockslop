//! xX ROCKSLOP FLASHER Xx: installs Rockbox on the YP3 and TS17.
//!
//!   rockslop                      flash a real player
//!   rockslop --demo               a fake player, nothing touches USB
//!   rockslop --demo --auto        ... clicking through by itself
//!       [--rockbox]               ... the player turning up in Rockbox first
//!   rockslop --mute               start with the sound off (M toggles)
//!   rockslop --lang deutsch       language: a Rockbox language name, `auto`
//!                                 (the system's) or `pseudo`; the default is
//!                                 the last one picked, else `auto`
//!   rockslop --render-scale 0.5   render at half the screen's resolution
//!                                 (by default it starts at full, at most 2x,
//!                                 and steps down if frames are slow)
//!   rockslop --shot out.png --at 12 [--demo --auto --kind ts17]
//!                                 render one frame at t=12 s and exit
//!                                 (--at 4,8,12: out-4.png, out-8.png, ...)
//!       [--mouse X,Y]             ... with the pointer parked at X,Y
//!       [--tips-now]              ... with Clippy's tips (and his credits
//!                                 encore) up at once

#![cfg_attr(all(windows, not(debug_assertions)), windows_subsystem = "windows")]

mod anim;
mod app;
mod audio;
mod clippy;
mod credits;
mod gfx;
mod i18n;
mod nowplaying;
mod picker;
mod ui;

use app::{App, Opts};
use audio::Audio;
use macroquad::prelude::*;
use rockslop::device::{
    real::Real,
    sim::{self, Sim},
    Hub, Kind,
};

fn window_conf() -> macroquad::conf::Conf {
    macroquad::conf::Conf {
        miniquad_conf: Conf {
            window_title: "ROCKSLOP".to_string(),
            window_width: 1280,
            window_height: 800,
            high_dpi: true,
            sample_count: 1,
            ..Default::default()
        },
        draw_call_vertex_capacity: 60_000,
        draw_call_index_capacity: 90_000,
        ..Default::default()
    }
}

fn arg(args: &[String], name: &str) -> Option<String> {
    args.iter().position(|a| a == name).and_then(|i| args.get(i + 1).cloned())
}

#[macroquad::main(window_conf)]
async fn main() {
    let args: Vec<String> = std::env::args().collect();
    let has = |n: &str| args.iter().any(|a| a == n);
    let shot = arg(&args, "--shot");
    let mut shot_ts: Vec<f32> = arg(&args, "--at").map(|s| s.split(',').filter_map(|t| t.parse().ok()).collect()).unwrap_or_default();
    if shot_ts.is_empty() {
        shot_ts.push(3.0);
    }
    shot_ts.sort_by(f32::total_cmp);
    let many = shot_ts.len() > 1;
    let speed: f32 = arg(&args, "--speed").and_then(|s| s.parse().ok()).unwrap_or(1.0);
    let fps: f32 = arg(&args, "--fps").and_then(|s| s.parse().ok()).unwrap_or(60.0);
    let render_scale: Option<f32> = arg(&args, "--render-scale").and_then(|s| s.parse().ok()).map(|f: f32| f.clamp(0.25, 1.0));
    let demo = has("--demo");
    let kind = if arg(&args, "--kind").as_deref() == Some("ts17") { Kind::Ts17 } else { Kind::Yp3 };
    let clock: Option<sim::Clock> = shot.as_ref().map(|_| Default::default());
    let hub = if demo {
        let mut sim = Sim::new(speed, clock.clone());
        sim.fail = has("--fail");
        sim.rockbox = has("--rockbox");
        Hub::start(Box::new(sim))
    } else {
        Hub::start(Box::new(Real::new()))
    };
    let mouse = arg(&args, "--mouse").and_then(|s| {
        let (x, y) = s.split_once(',')?;
        Some(vec2(x.parse().ok()?, y.parse().ok()?))
    });
    // language: --lang, else the last choice, else the system's; the picker
    // opens at startup unless --lang said
    let lang = arg(&args, "--lang");
    let want = lang.clone().or_else(i18n::saved).unwrap_or_else(|| i18n::AUTO.into());
    let id = if want == i18n::AUTO { i18n::system().unwrap_or("english") } else { &want };
    if !i18n::set(id) {
        let ids: Vec<&str> = i18n::LANGS.iter().map(|l| l.id).collect();
        eprintln!("unknown language {id:?}; one of: {}, {}, {}", i18n::AUTO, ids.join(", "), i18n::PSEUDO);
        std::process::exit(2);
    }
    let opts = Opts {
        demo,
        auto: has("--auto"),
        kind,
        package: arg(&args, "--package").map(Into::into),
        mouse,
        tips_now: has("--tips-now"),
    };
    let audio = if shot.is_some() { Audio::silent() } else { Audio::new() };
    if has("--mute") {
        audio.toggle_mute();
    }
    // the log of what the flasher did to a real player (not the simulator's)
    if !demo && shot.is_none() {
        rockslop::log::open(&format!("rockslop {}", args[1..].join(" ")));
    }
    let mut app = App::new(hub, opts, audio);
    // screenshots render at the screen's resolution, however slowly
    if let Some(f) = render_scale.or(shot.as_ref().map(|_| 1.0)) {
        app.g.fix_scale(f);
    }
    // At most about 75 frames a second. A 120 Hz screen (ProMotion; macOS
    // paces frames by the display, not vsync) would otherwise draw twice the
    // frames of a 60 Hz one for nothing; there vsync's 16.7 ms is the longer
    // wait, so this changes nothing.
    const MIN_FRAME: std::time::Duration = std::time::Duration::from_micros(13_333);
    let mut frame_start = std::time::Instant::now();
    loop {
        let dt = if shot.is_some() { 1.0 / fps } else { get_frame_time().min(0.05) };
        if shot.is_none() {
            app.g.pace(get_frame_time());
        }
        app.update(dt);
        if let Some(c) = &clock {
            c.store((app.time as f64 * 1e6) as u64, std::sync::atomic::Ordering::Relaxed);
        }
        app.draw();
        if let Some(path) = &shot {
            if app.time >= shot_ts[0] {
                let t = shot_ts.remove(0);
                let out = if many { path.replace(".png", &format!("-{t}.png")) } else { path.clone() };
                get_screen_data().export_png(&out);
                if shot_ts.is_empty() {
                    return;
                }
            }
        }
        if shot.is_none() {
            if let Some(rest) = MIN_FRAME.checked_sub(frame_start.elapsed()) {
                std::thread::sleep(rest);
            }
        }
        next_frame().await;
        frame_start = std::time::Instant::now();
    }
}
