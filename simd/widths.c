// widths.c — cùng bốn vòng lặp, dịch với các độ rộng vector khác nhau (cờ biên dịch quyết định),
// đo chu kỳ mỗi phần tử khi dữ liệu nằm trong L1 (N = 2048 phần tử mỗi mảng).
// x86: -O3 -fno-tree-vectorize (vô hướng) · -O3 (SSE2, 128 bit) · -O3 -march=x86-64-v3 (AVX2, 256 bit)
// ARM: -O3 -fno-tree-vectorize · -O3 (NEON, 128 bit) · -O3 -march=armv8.2-a+sve (SVE)
#include "bench.h"
#include <stdint.h>
#include <string.h>
#define N 2048
#define REP 20000
static float xf[N] __attribute__((aligned(64))), yf[N] __attribute__((aligned(64)));
static double ad[N] __attribute__((aligned(64))), bd[N] __attribute__((aligned(64))), cd[N] __attribute__((aligned(64)));
static int8_t a8[N] __attribute__((aligned(64))), b8[N] __attribute__((aligned(64))), c8[N] __attribute__((aligned(64)));
static int32_t a32[N] __attribute__((aligned(64))), b32[N] __attribute__((aligned(64))), c32[N] __attribute__((aligned(64)));

__attribute__((noinline)) void saxpy(float *restrict y, const float *restrict x, float a, int n) {
  for (int i = 0; i < n; i++) y[i] = a * x[i] + y[i];
}
__attribute__((noinline)) void add_f64(double *restrict c, const double *restrict a, const double *restrict b, int n) {
  for (int i = 0; i < n; i++) c[i] = a[i] + b[i];
}
__attribute__((noinline)) void add_i8(int8_t *restrict c, const int8_t *restrict a, const int8_t *restrict b, int n) {
  for (int i = 0; i < n; i++) c[i] = a[i] + b[i];
}
__attribute__((noinline)) void add_i32(int32_t *restrict c, const int32_t *restrict a, const int32_t *restrict b, int n) {
  for (int i = 0; i < n; i++) c[i] = a[i] + b[i];
}
static void r_saxpy(void *p) { for (int r = 0; r < REP; r++) { saxpy(yf, xf, 1.0001f, N); asm volatile("" ::: "memory"); } }
static void r_f64(void *p)   { for (int r = 0; r < REP; r++) { add_f64(cd, ad, bd, N); asm volatile("" ::: "memory"); } }
static void r_i8(void *p)    { for (int r = 0; r < REP; r++) { add_i8(c8, a8, b8, N); asm volatile("" ::: "memory"); } }
static void r_i32(void *p)   { for (int r = 0; r < REP; r++) { add_i32(c32, a32, b32, N); asm volatile("" ::: "memory"); } }

int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  const char *tag = argc > 2 ? argv[2] : "?";
  for (int i = 0; i < N; i++) { xf[i] = i * 1e-3f; yf[i] = 1; ad[i] = i; bd[i] = 2; a8[i] = i; b8[i] = 3; a32[i] = i; b32[i] = 7; }
  double u = (double)N * REP;
  double s1 = bench_cycles(r_saxpy, 0, u), s2 = bench_cycles(r_f64, 0, u), s3 = bench_cycles(r_i8, 0, u), s4 = bench_cycles(r_i32, 0, u);
  printf("%-26s saxpy f32 %6.3f | add f64 %6.3f | add i8 %6.3f | add i32 %6.3f   chu kỳ/phần tử  (%.2f GHz)\n",
         tag, s1, s2, s3, s4, bench_ghz);
  return (int)(yf[5] + cd[5] + c8[5] + c32[5]) == 12345;
}
