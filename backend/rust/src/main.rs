use std::io::{self, BufRead};
use std::sync::{Arc, atomic::{AtomicBool, AtomicU64, Ordering}};
use std::thread;
use std::time::{Duration, Instant};
use sha2::Digest;

fn hash(s: &str, algorithm: &str) -> String {
    let b = s.as_bytes();
    match algorithm {
        "md5" => format!("{:x}", md5::Md5::digest(b)),
        "sha1" => format!("{:x}", sha1::Sha1::digest(b)),
        "sha224" => format!("{:x}", sha2::Sha224::digest(b)),
        "sha256" => format!("{:x}", sha2::Sha256::digest(b)),
        "sha384" => format!("{:x}", sha2::Sha384::digest(b)),
        "sha512" => format!("{:x}", sha2::Sha512::digest(b)),
        "sha3_224" => format!("{:x}", sha3::Sha3_224::digest(b)),
        "sha3_256" => format!("{:x}", sha3::Sha3_256::digest(b)),
        "sha3_384" => format!("{:x}", sha3::Sha3_384::digest(b)),
        "sha3_512" => format!("{:x}", sha3::Sha3_512::digest(b)),
        "blake2s" => format!("{:x}", blake2::Blake2s256::digest(b)),
        "blake2b" => format!("{:x}", blake2::Blake2b512::digest(b)),
        _ => String::new(),
    }
}
fn argument(args: &[String], flag: &str, default: &str) -> String {
    args.windows(2).find(|a| a[0] == flag).map(|a| a[1].clone()).unwrap_or_else(|| default.to_string())
}
fn main() {
    let args: Vec<String> = std::env::args().collect();
    let chars = argument(&args, "--chars", "");
    let algorithm = argument(&args, "--algorithm", "sha256");
    let target = argument(&args, "--target", "").to_lowercase();
    let length = argument(&args, "--length", "6").parse::<usize>().unwrap_or(0);
    let workers = argument(&args, "--workers", "1").parse::<usize>().unwrap_or(0);
    if chars.is_empty() || length == 0 || length > 12 || workers == 0 || hash("", &algorithm).is_empty() {
        println!("ERROR\tInvalid configuration"); return;
    }
    if args.contains(&"--benchmark".to_string()) {
        let start = Instant::now();
        for _ in 0..25000 { let _ = hash(&chars[..1], &algorithm); }
        println!("RATE\t{:.2}", 25000.0 / start.elapsed().as_secs_f64().max(0.000001));
        return;
    }
    if hash("", &algorithm).len() != target.len() || !target.bytes().all(|b| b.is_ascii_hexdigit()) {
        println!("ERROR\tInvalid digest length or characters"); return;
    }
    let alphabet: Vec<char> = chars.chars().collect();
    let n = alphabet.len();
    let threads = workers.min(n);
    let stop = Arc::new(AtomicBool::new(false));
    let checked = Arc::new(AtomicU64::new(0));
    let found = Arc::new(std::sync::Mutex::new(None::<String>));
    let stdin_stop = stop.clone();
    thread::spawn(move || {
        for line in io::stdin().lock().lines() {
            if let Ok(line) = line { if line.trim() == "STOP" { stdin_stop.store(true, Ordering::Relaxed); break; } }
        }
    });
    let start = Instant::now();
    let mut handles = Vec::new();
    for id in 0..threads {
        let a = alphabet.clone(); let alg = algorithm.clone(); let t = target.clone();
        let flag = stop.clone(); let counter = checked.clone(); let answer = found.clone();
        handles.push(thread::spawn(move || {
            let mut local = 0_u64;
            for first in (id..n).step_by(threads) {
                let mut indices = vec![0_usize; length.saturating_sub(1)];
                loop {
                    if flag.load(Ordering::Relaxed) { break; }
                    let mut candidate = String::with_capacity(length);
                    candidate.push(a[first]);
                    for &j in &indices { candidate.push(a[j]); }
                    local += 1;
                    if hash(&candidate, &alg) == t {
                        *answer.lock().unwrap() = Some(candidate);
                        flag.store(true, Ordering::Relaxed);
                        break;
                    }
                    if local >= 4096 { counter.fetch_add(local, Ordering::Relaxed); local = 0; }
                    let mut pos = indices.len();
                    while pos > 0 {
                        pos -= 1;
                        indices[pos] += 1;
                        if indices[pos] < n { break; }
                        indices[pos] = 0;
                    }
                    if indices.is_empty() || (pos == 0 && indices[0] == 0) { break; }
                }
            }
            counter.fetch_add(local, Ordering::Relaxed);
        }));
    }
    while handles.iter().any(|h| !h.is_finished()) {
        println!("PROGRESS\t{}\t{:.3}", checked.load(Ordering::Relaxed), start.elapsed().as_secs_f64());
        thread::sleep(Duration::from_millis(150));
    }
    for h in handles { let _ = h.join(); }
    let result = found.lock().unwrap().clone().unwrap_or_default();
    let cancelled = if stop.load(Ordering::Relaxed) && result.is_empty() { 1 } else { 0 };
    println!("DONE\t{}\t{}\t{:.3}\t{}", result, checked.load(Ordering::Relaxed), start.elapsed().as_secs_f64(), cancelled);
}
