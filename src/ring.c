// ring.c — checkpoint C3.7: bộ đệm vòng một-người-ghi-một-người-đọc chuyển "khung mẫu cảm biến"
// từ luồng thu (producer) sang luồng xử lý (consumer). Mỗi khung = (seq, giá trị, tổng kiểm).
// Biến thể chọn lúc biên dịch bằng -DV=<số>:
//   V=0  "kiểu x86": head/tail volatile + rào compiler asm("":::"memory") — đúng trên x86, SAI trên ARM
//   V=1  rào đầy đủ: dmb ish (ARM) / mfence (x86) ở cả bốn chỗ
//   V=2  rào tối thiểu ARM: P2 = dmb ishst, C1 = dmb ishld, C2 = dmb ishld, P1 = phụ thuộc điều khiển
//   V=3  C11: head/tail atomic, store release / load acquire
//   V=4  C11 nhưng tất cả relaxed (lỗi thường gặp khi "đã dùng atomic rồi")
//   V=5  rào tối thiểu bỏ P2 (đột biến) · V=6 bỏ C1 · V=7 bỏ C2
// build: gcc -O2 -pthread -DV=0 ring.c -o ring0 ; chạy: ./ring0 N lpProd lpCons
#include "plat.h"
#include <stdatomic.h>
#include <string.h>

#ifndef V
#define V 0
#endif
#define SIZE 64                                   // vòng nhỏ → quay vòng thường xuyên
typedef struct { uint64_t seq, val, chk; } frame; // 24 byte

static frame buf[SIZE] __attribute__((aligned(64)));
static struct { _Alignas(64) _Atomic uint32_t head; _Alignas(64) _Atomic uint32_t tail; char pad[64]; } q;

static inline uint64_t mix(uint64_t s) { s *= 0x9E3779B97F4A7C15ull; return s ^ (s >> 29); }

#define CB() asm volatile("" ::: "memory")
#if defined(__aarch64__)
#define FULL() asm volatile("dmb ish" ::: "memory")
#define FLD()  asm volatile("dmb ishld" ::: "memory")
#define FST()  asm volatile("dmb ishst" ::: "memory")
#else
#define FULL() asm volatile("mfence" ::: "memory")
#define FLD()  CB()
#define FST()  CB()
#endif

// cách đọc/ghi head, tail và bốn điểm rào theo biến thể
#if V == 3
#define LDH() atomic_load_explicit(&q.head, memory_order_acquire)
#define LDT() atomic_load_explicit(&q.tail, memory_order_acquire)
#define STH(v) atomic_store_explicit(&q.head, (v), memory_order_release)
#define STT(v) atomic_store_explicit(&q.tail, (v), memory_order_release)
#elif V == 4
#define LDH() atomic_load_explicit(&q.head, memory_order_relaxed)
#define LDT() atomic_load_explicit(&q.tail, memory_order_relaxed)
#define STH(v) atomic_store_explicit(&q.head, (v), memory_order_relaxed)
#define STT(v) atomic_store_explicit(&q.tail, (v), memory_order_relaxed)
#else
#define LDH() (*(volatile uint32_t *)&q.head)
#define LDT() (*(volatile uint32_t *)&q.tail)
#define STH(v) (*(volatile uint32_t *)&q.head = (v))
#define STT(v) (*(volatile uint32_t *)&q.tail = (v))
#endif

#if V == 0
#define P1() CB()
#define P2() CB()
#define C1() CB()
#define C2() CB()
#elif V == 1
#define P1() FULL()
#define P2() FULL()
#define C1() FULL()
#define C2() FULL()
#elif V == 2
#define P1() CB()
#define P2() FST()
#define C1() FLD()
#define C2() FLD()
#elif V == 5
#define P1() CB()
#define P2() CB()
#define C1() FLD()
#define C2() FLD()
#elif V == 6
#define P1() CB()
#define P2() FST()
#define C1() CB()
#define C2() FLD()
#elif V == 7
#define P1() CB()
#define P2() FST()
#define C1() FLD()
#define C2() CB()
#else                                           // V == 3, 4: thứ tự nằm trong chính lệnh atomic
#define P1() ((void)0)
#define P2() ((void)0)
#define C1() ((void)0)
#define C2() ((void)0)
#endif

static uint64_t N;
static int lpP, lpC;

static void *producer(void *arg) {
  (void)arg; pin(lpP);
  uint32_t h = 0;
  for (uint64_t s = 1; s <= N; s++) {
    while (h - LDT() == SIZE) ;                  // đầy: chờ người đọc
    P1();                                        // đọc tail  →  ghi khung
    frame *f = &buf[h % SIZE];
    uint64_t v = mix(s);
    f->seq = s; f->val = v; f->chk = s ^ v;
    P2();                                        // ghi khung →  ghi head
    STH(++h);
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 4) { fprintf(stderr, "dùng: %s N lpProd lpCons\n", argv[0]); return 1; }
  N = strtoull(argv[1], 0, 10); lpP = atoi(argv[2]); lpC = atoi(argv[3]);
  pin(lpC);
  uint64_t stale = 0, future = 0, torn = 0, shown = 0;
  double t0 = now_ns();
  thr_t tp = thr_start(producer, NULL);
  uint32_t t = 0;
  for (uint64_t s = 1; s <= N; s++) {
    while (LDH() == t) ;                         // rỗng: chờ người ghi
    C1();                                        // đọc head  →  đọc khung
    frame *f = &buf[t % SIZE];
    uint64_t fs = f->seq, fv = f->val, fc = f->chk;
    C2();                                        // đọc khung →  ghi tail
    STT(++t);
    if (fs != s || fv != mix(s) || fc != (s ^ mix(s))) {
      if (fs < s) stale++; else if (fs > s) future++; else torn++;
      if (shown++ < 3) printf("  khung %llu: đọc được seq=%llu (%s)\n", (unsigned long long)s, (unsigned long long)fs,
                              fs < s ? "cũ — vòng trước" : fs > s ? "mới — vòng sau" : "rách");
    }
  }
  thr_join(tp);
  double dt = (now_ns() - t0) * 1e-9;
  printf("%s V=%d N=%llu  cũ=%llu mới=%llu rách=%llu  (%.2f /triệu)  %.1f triệu khung/s\n", ARCH, V,
         (unsigned long long)N, (unsigned long long)stale, (unsigned long long)future, (unsigned long long)torn,
         1e6 * (stale + future + torn) / N, N / dt / 1e6);
  return 0;
}
