// denorm.c — giá của số dưới chuẩn (subnormal) trên x86: mỗi phép đo là 10^7 lệnh SSE/AVX viết bằng asm (không để compiler gập),
// tính theo chu kỳ bằng bench.h (hiệu chuẩn chuỗi add kề bên, trung vị 9 lần). Bốn chế độ MXCSR: mặc định, FTZ, DAZ, FTZ+DAZ.
#ifdef _WIN32
#include "../common/bench_win.h"
#else
#include "../common/bench.h"
#endif
#include <float.h>
#include <immintrin.h>
#include <string.h>
typedef struct { float x, c; int kind; float sink; } arg_t;   // sink: kết quả ghi ra đây, KHÔNG ghi đè x (xem denorm5_buggy.c)
#define REP ".rept 100\n\t"
static void lat_mul(void *p) { arg_t *a = p; float x = a->x, c = a->c;          // chuỗi phụ thuộc: x = x * c
  for (int i = 0; i < 100000; i++) asm volatile(REP "mulss %1, %0\n\t.endr" : "+x"(x) : "x"(c)); a->sink = x; }
static void lat_add(void *p) { arg_t *a = p; float x = a->x, c = a->c;          // x = x + c
  for (int i = 0; i < 100000; i++) asm volatile(REP "addss %1, %0\n\t.endr" : "+x"(x) : "x"(c)); a->sink = x; }
static void lat_fma(void *p) { arg_t *a = p; float x = a->x, c = a->c, z = 0;   // x = x * c + 0
  for (int i = 0; i < 100000; i++) asm volatile(REP "vfmadd213ss %2, %1, %0\n\t.endr" : "+x"(x) : "x"(c), "x"(z)); a->sink = x; }
static void thr_mul(void *p) { arg_t *a = p; float x = a->x, c = a->c;          // 4 lệnh độc lập, đầu vào giữ nguyên
  float r0, r1, r2, r3;
  for (int i = 0; i < 25000; i++)
    asm volatile(REP "vmulss %4, %5, %0\n\tvmulss %4, %5, %1\n\tvmulss %4, %5, %2\n\tvmulss %4, %5, %3\n\t.endr"
                 : "=&x"(r0), "=&x"(r1), "=&x"(r2), "=&x"(r3) : "x"(c), "x"(x));
  a->sink = r0 + r1 + r2 + r3; }
static void thr_add(void *p) { arg_t *a = p; float x = a->x, c = a->c;
  float r0, r1, r2, r3;
  for (int i = 0; i < 25000; i++)
    asm volatile(REP "vaddss %4, %5, %0\n\tvaddss %4, %5, %1\n\tvaddss %4, %5, %2\n\tvaddss %4, %5, %3\n\t.endr"
                 : "=&x"(r0), "=&x"(r1), "=&x"(r2), "=&x"(r3) : "x"(c), "x"(x));
  a->sink = r0 + r1 + r2 + r3; }
int main(int argc, char **argv) {
  int cpu = argc > 1 ? atoi(argv[1]) : 2;
#ifdef _WIN32
  printf("CPU logic %d, EfficiencyClass %d (1 = lõi P, 0 = lõi E)\n", cpu, bench_pin(cpu));
#else
  bench_pin(cpu);
#endif
  const float sub = 1e-40f, nrm = 1.0f, tiny = 4 * FLT_MIN;    // 1e-40 là số dưới chuẩn; 4·FLT_MIN là số chuẩn nhỏ
  struct { const char *name; void (*fn)(void *); float x, c; } t[] = {
    {"mulss  chuỗi   chuẩn  × 1      → chuẩn     ", lat_mul, nrm, 1.0f},
    {"mulss  chuỗi   dưới chuẩn × 1  → dưới chuẩn", lat_mul, sub, 1.0f},
    {"addss  chuỗi   chuẩn  + 0      → chuẩn     ", lat_add, nrm, 0.0f},
    {"addss  chuỗi   dưới chuẩn + 0  → dưới chuẩn", lat_add, sub, 0.0f},
    {"vfmadd chuỗi   chuẩn  × 1 + 0  → chuẩn     ", lat_fma, nrm, 1.0f},
    {"vfmadd chuỗi   dưới chuẩn × 1 + 0 → dưới chuẩn", lat_fma, sub, 1.0f},
    {"vmulss độc lập chuẩn  × 1      → chuẩn     ", thr_mul, nrm, 1.0f},
    {"vmulss độc lập dưới chuẩn × 1  → dưới chuẩn", thr_mul, sub, 1.0f},
    {"vmulss độc lập chuẩn  × 1/64   → dưới chuẩn", thr_mul, tiny, 1.0f / 64},
    {"vaddss độc lập dưới chuẩn + dưới chuẩn → dưới chuẩn", thr_add, sub, sub},
  };
  unsigned modes[] = {0, _MM_FLUSH_ZERO_ON, _MM_DENORMALS_ZERO_ON, _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON};
  const char *mn[] = {"mặc định", "FTZ", "DAZ", "FTZ+DAZ"};
  unsigned base = _mm_getcsr() & ~(_MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON);
  printf("%-52s %9s %9s %9s %9s   (chu kỳ mỗi lệnh)\n", "phép", mn[0], mn[1], mn[2], mn[3]);
  for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++) {
    printf("%-52s", t[i].name);
    for (int m = 0; m < 4; m++) {
      _mm_setcsr(base | modes[m]);
      arg_t a = {t[i].x, t[i].c, 0, 0};
      printf(" %9.2f", bench_cycles(t[i].fn, &a, 1e7));
    }
    _mm_setcsr(base); putchar('\n');
  }
  printf("(xung đo được ở lần cuối: %.2f GHz)\n", bench_ghz);
  return 0;
}
