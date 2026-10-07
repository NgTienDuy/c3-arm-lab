// mp.rs — phép thử MP trong Rust: AtomicU32 với Relaxed / Release-Acquire / SeqCst.
// Mỗi lượt hai luồng (ghim CPU 0 và 1) gặp nhau ở một hàng rào quay;
// người ghi: x = 1; y = 1; người đọc: r0 = y; r1 = x. Yếu: r0 == 1 && r1 == 0.
// build: rustc -O mp.rs ; chạy: ./mp <relaxed|relacq|seqcst> <số lượt>
use std::sync::atomic::{AtomicU32, AtomicU64, Ordering::*};
use std::sync::Arc;
use std::thread;

const B: usize = 4096;

#[repr(align(64))]
struct Cell(AtomicU32);

extern "C" {
    fn sched_setaffinity(pid: i32, size: usize, mask: *const u64) -> i32;
}
fn pin(cpu: usize) {
    let mut m = [0u64; 16];
    m[cpu / 64] |= 1 << (cpu % 64);
    unsafe { assert_eq!(sched_setaffinity(0, 128, m.as_ptr()), 0) };
}

struct Shared { x: Vec<Cell>, y: Vec<Cell>, bar: AtomicU64 }

fn barrier(s: &Shared, k: u64) {
    s.bar.fetch_add(1, SeqCst);
    while s.bar.load(SeqCst) < 2 * k { std::hint::spin_loop(); }
}

fn main() {
    let a: Vec<String> = std::env::args().collect();
    let var = a[1].clone();
    let n: usize = a[2].parse().unwrap();
    let (wo, ro) = match var.as_str() { "relaxed" => (Relaxed, Relaxed), "relacq" => (Release, Acquire), _ => (SeqCst, SeqCst) };
    let s = Arc::new(Shared {
        x: (0..B).map(|_| Cell(AtomicU32::new(0))).collect(),
        y: (0..B).map(|_| Cell(AtomicU32::new(0))).collect(),
        bar: AtomicU64::new(0),
    });
    let batches = (n + B - 1) / B;
    let mut hist = [0u64; 4];
    let mut k = 0u64;
    for _ in 0..batches {
        for i in 0..B { s.x[i].0.store(0, Relaxed); s.y[i].0.store(0, Relaxed); }
        let s2 = s.clone();
        let k0 = k;
        let w = thread::spawn(move || {
            pin(0);
            let mut kk = k0;
            for i in 0..B {
                kk += 1; barrier(&s2, kk);
                s2.x[i].0.store(1, Relaxed);       // dữ liệu
                s2.y[i].0.store(1, wo);            // cờ
            }
        });
        pin(1);
        let mut res = vec![0u8; B];
        let mut kk = k0;
        for i in 0..B {
            kk += 1; barrier(&s, kk);
            let r0 = s.y[i].0.load(ro);
            let r1 = s.x[i].0.load(Relaxed);
            res[i] = (r0 * 2 + r1) as u8;
        }
        w.join().unwrap();
        k = kk;
        for r in res { hist[r as usize] += 1; }
    }
    let total = (batches * B) as u64;
    println!("rust {} {:<7} n={}  yếu: {} ({:.3} /triệu)  r0r1: 00={} 01={} 10={}* 11={}", std::env::consts::ARCH, var,
             total, hist[2], 1e6 * hist[2] as f64 / total as f64, hist[0], hist[1], hist[2], hist[3]);
}
