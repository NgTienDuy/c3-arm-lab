// align.c — giá của lần nạp vector không căn lề: trong một dòng, vắt qua hai dòng 64 B, vắt qua hai trang 4 KiB.
// x86: lần nạp 32 B (AVX2, vmovups); ARM: lần nạp 16 B (NEON, ldr q). Dữ liệu nằm trong L1.
// Phần cuối: lệnh nạp *căn lề* (vmovaps) trên địa chỉ lệch → lỗi phần cứng (SIGSEGV).
#include "bench.h"
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#if defined(__aarch64__)
#include <arm_neon.h>
#define W 16
typedef float32x4_t vec;
#define LOADU(p) vld1q_f32(p)
#define ADD(a, b) vaddq_f32(a, b)
#define ZERO() vdupq_n_f32(0)
static float hsum(vec v) { return vaddvq_f32(v); }
#else
#include <immintrin.h>
#define W 32
typedef __m256 vec;
#define LOADU(p) _mm256_loadu_ps(p)
#define ADD(a, b) _mm256_add_ps(a, b)
#define ZERO() _mm256_setzero_ps()
static float hsum(vec v) { float t[8]; _mm256_storeu_ps(t, v); return t[0] + t[1] + t[2] + t[3] + t[4] + t[5] + t[6] + t[7]; }
#endif

static char *buf;                 // 66 trang, căn 4 KiB
static const char *base;          // địa chỉ đầu của lượt nạp
static volatile float sink;
#define REP 200000
#define L(o) LOADU((const float *)(q + (o)))      // độ dời hằng: không tốn lệnh tính địa chỉ

// mẫu A: 64 dòng liên tiếp (4 KiB, mỗi dòng một set L1), mỗi lần nạp lệch như nhau so với đầu dòng
static void run_lines(void *p) {
  for (int r = 0; r < REP; r++) {
    vec s0 = ZERO(), s1 = ZERO(), s2 = ZERO(), s3 = ZERO(), s4 = ZERO(), s5 = ZERO(), s6 = ZERO(), s7 = ZERO();
    for (const char *q = base; q < base + 4096; q += 512) {
      s0 = ADD(s0, L(0));   s1 = ADD(s1, L(64));  s2 = ADD(s2, L(128)); s3 = ADD(s3, L(192));
      s4 = ADD(s4, L(256)); s5 = ADD(s5, L(320)); s6 = ADD(s6, L(384)); s7 = ADD(s7, L(448));
    }
    sink = hsum(ADD(ADD(ADD(s0, s1), ADD(s2, s3)), ADD(ADD(s4, s5), ADD(s6, s7))));
  }
}
// mẫu B: 8 trang (8 dòng cùng set — vẫn vừa L1 12 đường), 64 lần nạp mỗi lượt
static void run_pages(void *p) {
  for (int r = 0; r < REP; r++) {
    vec s0 = ZERO(), s1 = ZERO(), s2 = ZERO(), s3 = ZERO(), s4 = ZERO(), s5 = ZERO(), s6 = ZERO(), s7 = ZERO();
    for (int k = 0; k < 8; k++) {
      const char *q = base;
      s0 = ADD(s0, L(0));     s1 = ADD(s1, L(4096));  s2 = ADD(s2, L(8192));  s3 = ADD(s3, L(12288));
      s4 = ADD(s4, L(16384)); s5 = ADD(s5, L(20480)); s6 = ADD(s6, L(24576)); s7 = ADD(s7, L(28672));
      asm volatile("" ::: "memory");
    }
    sink = hsum(ADD(ADD(ADD(s0, s1), ADD(s2, s3)), ADD(ADD(s4, s5), ADD(s6, s7))));
  }
}
static void setup_lines(int off) { base = buf + off; }
static void setup_pages(int off) { base = buf + 4096 + off; }   // lệch `off` so với đầu trang kế (off < 0: cuối trang trước)

static sigjmp_buf jb;
static void on_segv(int s) { (void)s; siglongjmp(jb, 1); }

int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  buf = aligned_alloc(4096, 4096 * 66); memset(buf, 0, 4096 * 66);
  printf("%s, lần nạp %d B, chu kỳ mỗi lần nạp (trung vị 9; 64 lần nạp / lượt, 8 tổng độc lập)\n",
#if defined(__aarch64__)
         "AArch64 NEON",
#else
         "x86-64 AVX2",
#endif
         W);
  int offs[] = {0, 4, 16, 32, 48, 60};
  for (unsigned i = 0; i < sizeof offs / sizeof offs[0]; i++) {
    int o = offs[i];
    setup_lines(o);
    printf("  lệch %2d B trong dòng%-28s %6.3f\n", o, o + W > 64 ? " (vắt qua 2 dòng)" : "", bench_cycles(run_lines, 0, 64.0 * REP));
  }
  setup_pages(-W / 2);
  printf("  8 trang: vắt qua ranh giới trang 4 KiB    %6.3f\n", bench_cycles(run_pages, 0, 64.0 * REP));
  setup_pages(-W / 2 - 64);
  printf("  8 trang: vắt qua 2 dòng, cùng trang       %6.3f\n", bench_cycles(run_pages, 0, 64.0 * REP));
  setup_pages(-64);
  printf("  8 trang: căn lề (đối chứng)               %6.3f\n", bench_cycles(run_pages, 0, 64.0 * REP));
  printf("  (xung %.2f GHz)\n", bench_ghz);
  signal(SIGSEGV, on_segv);
  const char *q = buf + 4;                            // lệch 4 B so với ranh giới 32 B
  volatile int ok;
#if defined(__aarch64__)
  if (sigsetjmp(jb, 1) == 0) { float32x4_t v; asm volatile("ldr %q0, [%1]" : "=w"(v) : "r"(q)); sink = hsum(v); ok = 1; } else ok = 0;
  printf("  ldr q (16 B) trên địa chỉ lệch 4 B: %s\n", ok ? "chạy được — AArch64 không đòi căn lề với bộ nhớ thường" : "lỗi");
#else
  if (sigsetjmp(jb, 1) == 0) { __m256 v; asm volatile("vmovaps (%1), %0" : "=x"(v) : "r"(q)); sink = hsum(v); ok = 1; } else ok = 0;
  printf("  vmovaps  (AVX, lệnh nạp căn lề)       trên địa chỉ lệch 4 B: %s\n", ok ? "chạy được" : "SIGSEGV");
  if (sigsetjmp(jb, 1) == 0) { __m128 v = _mm_setzero_ps(); asm volatile("addps (%1), %0" : "+x"(v) : "r"(q)); sink = v[0]; ok = 1; } else ok = 0;
  printf("  addps    (SSE cũ, toán hạng bộ nhớ)   trên địa chỉ lệch 4 B: %s\n", ok ? "chạy được" : "SIGSEGV");
  if (sigsetjmp(jb, 1) == 0) { __m256 v = _mm256_setzero_ps(); asm volatile("vaddps (%1), %0, %0" : "+x"(v) : "r"(q)); sink = hsum(v); ok = 1; } else ok = 0;
  printf("  vaddps   (AVX, toán hạng bộ nhớ)      trên địa chỉ lệch 4 B: %s\n", ok ? "chạy được" : "SIGSEGV");
#endif
  return 0;
}
