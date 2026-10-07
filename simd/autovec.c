// autovec.c — mười hai vòng lặp nhỏ: compiler có tự vector hóa được không, và nhanh lên bao nhiêu.
// Dịch hai lần: vô hướng (-O3 -fno-tree-vectorize) và vector (-O3 -march=x86-64-v3, hoặc -O3 trên AArch64),
// cộng -fopt-info-vec-all để đọc lý do. Mọi mảng nằm trong L1 (N = 2048).
#include "bench.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#define N 2048
#define REP 20000
static float A[N] __attribute__((aligned(64))), B[N] __attribute__((aligned(64))), C[N] __attribute__((aligned(64)));
static int32_t I[N] __attribute__((aligned(64))), J[N] __attribute__((aligned(64))), IDX[N] __attribute__((aligned(64)));
static uint8_t U8[N] __attribute__((aligned(64)));
static int8_t S8[N] __attribute__((aligned(64)));
volatile float sinkf; volatile long sinkl;

#define NI __attribute__((noinline))
NI float  k01_sum_f32(const float *a, int n)          { float s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
NI long   k02_sum_i32(const int32_t *a, int n)        { long s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
NI void   k03_saxpy_restrict(float *restrict y, const float *restrict x, float a, int n) { for (int i = 0; i < n; i++) y[i] += a * x[i]; }
NI void   k04_saxpy_alias(float *y, const float *x, float a, int n)                   { for (int i = 0; i < n; i++) y[i] += a * x[i]; }
NI void   k05_prefix(float *a, int n)                 { for (int i = 1; i < n; i++) a[i] += a[i - 1]; }
NI void   k06_stride2(float *restrict y, const float *restrict x, int n) { for (int i = 0; i < n / 2; i++) y[i] = x[2 * i] + x[2 * i + 1]; }
NI float  k07_gather(const float *a, const int32_t *idx, int n) { float s = 0; for (int i = 0; i < n; i++) s += a[idx[i]]; return s; }
NI void   k08_cond_store(float *restrict y, const float *restrict x, int n) { for (int i = 0; i < n; i++) if (x[i] > 0.5f) y[i] = x[i]; }
NI void   k09_cond_select(float *restrict y, const float *restrict x, int n) { for (int i = 0; i < n; i++) y[i] = x[i] > 0.5f ? x[i] : y[i]; }
NI int    k10_find(const int32_t *a, int n, int key)  { for (int i = 0; i < n; i++) if (a[i] == key) return i; return -1; }
NI void   k11_sinf(float *restrict y, const float *restrict x, int n) { for (int i = 0; i < n; i++) y[i] = sinf(x[i]); }
NI int32_t k12_dot_u8s8(const uint8_t *a, const int8_t *b, int n) { int32_t s = 0; for (int i = 0; i < n; i++) s += a[i] * b[i]; return s; }

static void t01(void *p) { for (int r = 0; r < REP; r++) { sinkf = k01_sum_f32(A, N); } }
static void t02(void *p) { for (int r = 0; r < REP; r++) { sinkl = k02_sum_i32(I, N); } }
static void t03(void *p) { for (int r = 0; r < REP; r++) { k03_saxpy_restrict(C, A, 1e-6f, N); asm volatile("" ::: "memory"); } }
static void t04(void *p) { for (int r = 0; r < REP; r++) { k04_saxpy_alias(C, A, 1e-6f, N); asm volatile("" ::: "memory"); } }
static void t05(void *p) { for (int r = 0; r < REP; r++) { memcpy(C, A, sizeof A); k05_prefix(C, N); asm volatile("" ::: "memory"); } }
static void t06(void *p) { for (int r = 0; r < REP; r++) { k06_stride2(C, A, N); asm volatile("" ::: "memory"); } }
static void t07(void *p) { for (int r = 0; r < REP; r++) { sinkf = k07_gather(A, IDX, N); } }
static void t08(void *p) { for (int r = 0; r < REP; r++) { k08_cond_store(C, A, N); asm volatile("" ::: "memory"); } }
static void t09(void *p) { for (int r = 0; r < REP; r++) { k09_cond_select(C, A, N); asm volatile("" ::: "memory"); } }
static void t10(void *p) { for (int r = 0; r < REP; r++) { sinkl = k10_find(J, N, -1); } }
static void t11(void *p) { for (int r = 0; r < REP / 10; r++) { k11_sinf(C, A, N); asm volatile("" ::: "memory"); } }
static void t12(void *p) { for (int r = 0; r < REP; r++) { sinkl = k12_dot_u8s8(U8, S8, N); } }

int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  uint32_t s = 1;
  for (int i = 0; i < N; i++) {
    s = s * 1664525u + 1013904223u;
    A[i] = (s >> 8) * (1.0f / 16777216.0f); B[i] = 1; C[i] = 0;
    I[i] = (int32_t)(s >> 12); J[i] = i; IDX[i] = (int32_t)(s % N); U8[i] = (uint8_t)(s >> 3); S8[i] = (int8_t)(s >> 11);
  }
  struct { const char *n; void (*f)(void *); double u; } T[] = {
    {"k01 tổng float", t01, (double)N * REP}, {"k02 tổng int32 → long", t02, (double)N * REP},
    {"k03 saxpy, restrict", t03, (double)N * REP}, {"k04 saxpy, có thể chồng lấn", t04, (double)N * REP},
    {"k05 tổng tiền tố", t05, (double)N * REP}, {"k06 bước 2", t06, (double)N / 2 * REP},
    {"k07 gather a[idx[i]]", t07, (double)N * REP}, {"k08 ghi có điều kiện", t08, (double)N * REP},
    {"k09 chọn có điều kiện", t09, (double)N * REP}, {"k10 tìm (thoát sớm)", t10, (double)N * REP},
    {"k11 sinf", t11, (double)N * REP / 10}, {"k12 tích vô hướng u8×s8", t12, (double)N * REP}};
  for (size_t k = 0; k < sizeof T / sizeof T[0]; k++)
    printf("  %-30s %7.3f chu kỳ/phần tử\n", T[k].n, bench_cycles(T[k].f, 0, T[k].u));
  printf("  (xung %.2f GHz; kiểm: k01=%.4f k12=%ld)\n", bench_ghz, k01_sum_f32(A, N), (long)k12_dot_u8s8(U8, S8, N));
  return 0;
}
