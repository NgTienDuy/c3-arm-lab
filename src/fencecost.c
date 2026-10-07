// fencecost.c — giá của các lệnh rào / nạp-ghi có thứ tự, trong hai hoàn cảnh:
//  (1) "rảnh": ghi một dòng L1 → [rào] → nạp một dòng L1 khác;
//  (2) "có ghi đang chờ": ghi một dòng NGẪU NHIÊN trong vùng 256 MiB (trượt cache, phải RFO) → [rào] → nạp L1.
// Số chu kỳ = thời gian / thời gian một lệnh add phụ thuộc (hiệu chuẩn kề bên, như C3.5 §0.3).
// build: gcc -O2 fencecost.c -o fencecost ; chạy: ./fencecost [lp]
#include "plat.h"
#include <string.h>

#define N 2000000
static volatile int a_line[16] __attribute__((aligned(64)));
static volatile int b_line[16] __attribute__((aligned(64)));
static int *big;                                // vùng lớn cho lần ghi trượt
#define BIGN (64u << 20)                        // 64 Mi int = 256 MiB
static uint32_t *idx;

#if defined(__aarch64__)
#define ADD100 asm volatile(".rept 100\n\tadd %0, %0, %0\n\t.endr" : "+r"(z))
#define F_none   ""
#define F_dmb    "dmb ish"
#define F_dmbld  "dmb ishld"
#define F_dmbst  "dmb ishst"
#define F_dsb    "dsb ish"
#define F_isb    "isb"
#else
#define ADD100 asm volatile(".rept 100\n\tadd %0, %0\n\t.endr" : "+r"(z))
#define F_none   ""
#define F_mfence "mfence"
#define F_lock   "lock addl $0, (%%rsp)"
#define F_sfence "sfence"
#define F_lfence "lfence"
#endif

static double cyc_ns;                            // ns mỗi chu kỳ
static void calib(void) {
  double v[9];
  for (int r = 0; r < 9; r++) {
    uint64_t z = 1; double t0 = now_ns();
    for (int i = 0; i < 100000; i++) ADD100;
    double t1 = now_ns();
    v[r] = (t1 - t0) / 1e7; if (z == 42) puts("");
  }
  cyc_ns = median(v, 9);
}

// thân vòng lặp: ST kiểu st, rào F, LD kiểu ld
#if defined(__aarch64__)
#define ST_PLAIN(p, v) asm volatile("str %w1, [%0]" :: "r"(p), "r"(v) : "memory")
#define ST_REL(p, v)   asm volatile("stlr %w1, [%0]" :: "r"(p), "r"(v) : "memory")
#define ST_SWP(p, v)   asm volatile("swpal %w1, wzr, [%0]" :: "r"(p), "r"(v) : "memory")
#define LD_PLAIN(p)    ({ int _r; asm volatile("ldr %w0, [%1]" : "=r"(_r) : "r"(p) : "memory"); _r; })
#define LD_ACQ(p)      ({ int _r; asm volatile("ldar %w0, [%1]" : "=r"(_r) : "r"(p) : "memory"); _r; })
#define LD_ACQPC(p)    ({ int _r; asm volatile(".arch armv8.3-a\n\tldapr %w0, [%1]" : "=r"(_r) : "r"(p) : "memory"); _r; })
#else
#define ST_PLAIN(p, v) asm volatile("movl %1, (%0)" :: "r"(p), "r"(v) : "memory")
#define ST_REL(p, v)   ST_PLAIN(p, v)
#define ST_SWP(p, v)   ({ int _v = (v); asm volatile("xchgl %0, (%1)" : "+r"(_v) : "r"(p) : "memory"); })
#define LD_PLAIN(p)    ({ int _r; asm volatile("movl (%1), %0" : "=r"(_r) : "r"(p) : "memory"); _r; })
#define LD_ACQ(p)      LD_PLAIN(p)
#define LD_ACQPC(p)    LD_PLAIN(p)
#endif

#define CASE(NAME, ST, F, LD)                                                        \
  static double NAME(int miss) {                                                     \
    double v[5];                                                                     \
    for (int r = 0; r < 5; r++) {                                                    \
      long s = 0; double t0 = now_ns();                                              \
      for (int i = 0; i < N; i++) {                                                  \
        int *p = miss ? &big[idx[i]] : (int *)&a_line[0];                            \
        ST(p, i); asm volatile(F ::: "memory"); s += LD(&b_line[0]);                 \
      }                                                                              \
      double t1 = now_ns(); v[r] = (t1 - t0) / N; if (s == 42) puts("");             \
    }                                                                                \
    return median(v, 5);                                                             \
  }

#if defined(__aarch64__)
CASE(c_none,  ST_PLAIN, F_none,  LD_PLAIN)
CASE(c_dmb,   ST_PLAIN, F_dmb,   LD_PLAIN)
CASE(c_dmbld, ST_PLAIN, F_dmbld, LD_PLAIN)
CASE(c_dmbst, ST_PLAIN, F_dmbst, LD_PLAIN)
CASE(c_dsb,   ST_PLAIN, F_dsb,   LD_PLAIN)
CASE(c_isb,   ST_PLAIN, F_isb,   LD_PLAIN)
CASE(c_stlr,  ST_REL,   F_none,  LD_PLAIN)
CASE(c_ldar,  ST_PLAIN, F_none,  LD_ACQ)
CASE(c_ldapr, ST_PLAIN, F_none,  LD_ACQPC)
CASE(c_stlr_ldar,  ST_REL, F_none, LD_ACQ)
CASE(c_stlr_ldapr, ST_REL, F_none, LD_ACQPC)
CASE(c_swpal, ST_SWP,   F_none,  LD_PLAIN)
static struct { const char *n; double (*f)(int); } C[] = {
  {"str ; ldr (không rào)", c_none}, {"str ; dmb ish ; ldr", c_dmb}, {"str ; dmb ishld ; ldr", c_dmbld},
  {"str ; dmb ishst ; ldr", c_dmbst}, {"str ; dsb ish ; ldr", c_dsb}, {"str ; isb ; ldr", c_isb},
  {"stlr ; ldr", c_stlr}, {"str ; ldar", c_ldar}, {"str ; ldapr", c_ldapr},
  {"stlr ; ldar", c_stlr_ldar}, {"stlr ; ldapr", c_stlr_ldapr}, {"swpal ; ldr", c_swpal}};
#else
CASE(c_none,   ST_PLAIN, F_none,   LD_PLAIN)
CASE(c_mfence, ST_PLAIN, F_mfence, LD_PLAIN)
CASE(c_lock,   ST_PLAIN, F_lock,   LD_PLAIN)
CASE(c_sfence, ST_PLAIN, F_sfence, LD_PLAIN)
CASE(c_lfence, ST_PLAIN, F_lfence, LD_PLAIN)
CASE(c_xchg,   ST_SWP,   F_none,   LD_PLAIN)
static struct { const char *n; double (*f)(int); } C[] = {
  {"mov st ; mov ld (không rào)", c_none}, {"mov st ; mfence ; mov ld", c_mfence},
  {"mov st ; lock add [rsp] ; mov ld", c_lock}, {"mov st ; sfence ; mov ld", c_sfence},
  {"mov st ; lfence ; mov ld", c_lfence}, {"xchg st ; mov ld", c_xchg}};
#endif

int main(int argc, char **argv) {
  if (argc > 1) pin(atoi(argv[1]));
  big = amalloc(4096, (size_t)BIGN * sizeof(int)); memset(big, 1, (size_t)BIGN * sizeof(int));
  idx = malloc(N * sizeof *idx);
  uint64_t s = 88172645463325252ull;
  for (int i = 0; i < N; i++) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; idx[i] = (uint32_t)(s % BIGN) & ~15u; }
  calib(); double c0 = cyc_ns;
  printf("%s  hiệu chuẩn: %.3f ns/chu kỳ (%.2f GHz)\n", ARCH, c0, 1 / c0);
  printf("  %-34s %22s %24s\n", "", "rảnh (ns | chu kỳ)", "ghi trượt chờ (ns | chu kỳ)");
  for (size_t k = 0; k < sizeof C / sizeof C[0]; k++) {
    calib(); double a = C[k].f(0), b = C[k].f(1); double c1 = cyc_ns; calib();
    double cc = (c1 + cyc_ns) / 2;
    printf("  %-34s %10.2f | %6.1f %12.2f | %6.1f\n", C[k].n, a, a / cc, b, b / cc);
  }
  return 0;
}
