// denorm_a64.c — bản AArch64 của denorm.c: giá của số dưới chuẩn trên lõi ARM (Neoverse N2 của runner GitHub), đo bằng bench.h.
// Hai chế độ FPCR: mặc định và FZ (bit 24, xả về 0 — ARM không tách FTZ/DAZ như x86: FZ xử lý cả đầu vào lẫn đầu ra).
#include "../common/bench.h"
#include <float.h>
#include <math.h>
typedef struct { float x, c, sink; } arg_t;
#define REP ".rept 100\n\t"
static void lat_mul(void *p) { arg_t *a = p; float x = a->x, c = a->c;
  for (int i = 0; i < 100000; i++) asm volatile(REP "fmul %s0, %s0, %s1\n\t.endr" : "+w"(x) : "w"(c)); a->sink = x; }
static void lat_add(void *p) { arg_t *a = p; float x = a->x, c = a->c;
  for (int i = 0; i < 100000; i++) asm volatile(REP "fadd %s0, %s0, %s1\n\t.endr" : "+w"(x) : "w"(c)); a->sink = x; }
static void lat_fma(void *p) { arg_t *a = p; float x = a->x, c = a->c, z = 0;
  for (int i = 0; i < 100000; i++) asm volatile(REP "fmadd %s0, %s0, %s1, %s2\n\t.endr" : "+w"(x) : "w"(c), "w"(z)); a->sink = x; }
static void thr_mul(void *p) { arg_t *a = p; float x = a->x, c = a->c, r0, r1, r2, r3;
  for (int i = 0; i < 25000; i++)
    asm volatile(REP "fmul %s0, %s5, %s4\n\tfmul %s1, %s5, %s4\n\tfmul %s2, %s5, %s4\n\tfmul %s3, %s5, %s4\n\t.endr"
                 : "=&w"(r0), "=&w"(r1), "=&w"(r2), "=&w"(r3) : "w"(c), "w"(x));
  a->sink = r0 + r1 + r2 + r3; }
static void thr_add(void *p) { arg_t *a = p; float x = a->x, c = a->c, r0, r1, r2, r3;
  for (int i = 0; i < 25000; i++)
    asm volatile(REP "fadd %s0, %s5, %s4\n\tfadd %s1, %s5, %s4\n\tfadd %s2, %s5, %s4\n\tfadd %s3, %s5, %s4\n\t.endr"
                 : "=&w"(r0), "=&w"(r1), "=&w"(r2), "=&w"(r3) : "w"(c), "w"(x));
  a->sink = r0 + r1 + r2 + r3; }
static uint64_t get_fpcr(void) { uint64_t v; asm volatile("mrs %0, fpcr" : "=r"(v)); return v; }
static void set_fpcr(uint64_t v) { asm volatile("msr fpcr, %0" :: "r"(v)); }
int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  const float sub = 1e-40f, nrm = 1.0f, tiny = 4 * FLT_MIN;
  struct { const char *name; void (*fn)(void *); float x, c; } t[] = {
    {"fmul   chuỗi   chuẩn  × 1      → chuẩn     ", lat_mul, nrm, 1.0f},
    {"fmul   chuỗi   dưới chuẩn × 1  → dưới chuẩn", lat_mul, sub, 1.0f},
    {"fadd   chuỗi   chuẩn  + 0      → chuẩn     ", lat_add, nrm, 0.0f},
    {"fadd   chuỗi   dưới chuẩn + 0  → dưới chuẩn", lat_add, sub, 0.0f},
    {"fmadd  chuỗi   chuẩn  × 1 + 0  → chuẩn     ", lat_fma, nrm, 1.0f},
    {"fmadd  chuỗi   dưới chuẩn × 1 + 0 → dưới chuẩn", lat_fma, sub, 1.0f},
    {"fmul   độc lập chuẩn  × 1      → chuẩn     ", thr_mul, nrm, 1.0f},
    {"fmul   độc lập dưới chuẩn × 1  → dưới chuẩn", thr_mul, sub, 1.0f},
    {"fmul   độc lập chuẩn  × 1/64   → dưới chuẩn", thr_mul, tiny, 1.0f / 64},
    {"fadd   độc lập dưới chuẩn + dưới chuẩn → dưới chuẩn", thr_add, sub, sub},
  };
  uint64_t base = get_fpcr() & ~(1ull << 24);
  printf("%-52s %9s %9s   (chu kỳ mỗi lệnh)\n", "phép", "mặc định", "FZ");
  for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++) {
    printf("%-52s", t[i].name);
    for (int m = 0; m < 2; m++) { set_fpcr(base | (m ? 1ull << 24 : 0)); arg_t a = {t[i].x, t[i].c, 0}; printf(" %9.2f", bench_cycles(t[i].fn, &a, 1e7)); }
    set_fpcr(base); putchar('\n');
  }
  printf("(xung đo được ở lần cuối: %.2f GHz)\n", bench_ghz);
  return 0;
}
