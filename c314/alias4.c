// alias4.c [LP] — như alias3.c (tỉ số t(off)/t(2048), kẹp tham chiếu, 25 off × 15 lần, thứ tự xáo trộn), nhưng vòng được VECTOR HÓA:
// alias3.c dịch -O2 cho vòng vô hướng (vaddss/vmulsd) vì gcc không chứng minh được a và b không chồng nhau (cùng một vùng `base`).
// Ở đây thân vòng là hàm riêng nhận con trỏ `restrict` (hai vùng thật sự không chồng: b = a + 8192 + off, a dài 4 KiB) → gcc -O3
// sinh vaddps/vmulpd ymm (x86, -mavx2) hoặc fadd v.4s (AArch64) — đúng kiểu vòng alias4k của C3.6 §3.4 (AVX2, 1.024 float).
// Kiểm bằng objdump trước khi tin (run.sh in thân kD/kF). Trên Windows: ghim LP và tự khai HighQoS.
// Danh sách off có thêm các off LỆCH 32 B nhưng XA vùng 4K (1040, 1048, 1064, 2064, 2072, 3088): lệch 32 B làm một nửa lệnh ghi 32 B
// vắt qua hai dòng cache 64 B (split store) — nếu chúng chậm như off 16, nguyên nhân là ghi vắt dòng chứ không phải 4K aliasing.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
static double now(void) { LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c); return (double)c.QuadPart / f.QuadPart; }
#else
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
#endif
#define N 1024
#define K 25
#define R 15
static int cmp(const void *a, const void *b) { double x = *(double *)a, y = *(double *)b; return (x > y) - (x < y); }
static char *base;
__attribute__((noinline)) void kD(double *restrict dst, const double *restrict src) { for (int i = 0; i < N / 2; i++) dst[i] = src[i] * 1.5 + 1.0; }
__attribute__((noinline)) void kF(float *restrict b, const float *restrict a) { for (int i = 0; i < N; i++) b[i] = a[i] + 1.0f; }
static double runD(int off, long reps) {
  double *src = (double *)base, *dst = (double *)(base + 8192 + off); double t0 = now();
  for (long r = 0; r < reps; r++) { kD(dst, src); __asm__ volatile("" ::: "memory"); }
  return now() - t0;
}
static double runF(int off, long reps) {
  float *a = (float *)base, *b = (float *)(base + 8192 + off); double t0 = now();
  for (long r = 0; r < reps; r++) { kF(b, a); __asm__ volatile("" ::: "memory"); }
  return now() - t0;
}
int main(int argc, char **argv) {
#ifdef _WIN32
  int lp = argc > 1 ? atoi(argv[1]) : 0; SetThreadAffinityMask(GetCurrentThread(), 1ull << lp);
  PROCESS_POWER_THROTTLING_STATE st = {PROCESS_POWER_THROTTLING_CURRENT_VERSION, PROCESS_POWER_THROTTLING_EXECUTION_SPEED, 0};
  SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &st, sizeof st);
  base = _aligned_malloc(64 * 1024, 4096);
#else
  (void)argc; (void)argv;
  base = aligned_alloc(4096, 64 * 1024);
#endif
  for (int i = 0; i < 16384; i++) ((float *)base)[i] = (float)i;
  const int offs[K] = {0, 8, 16, 24, 32, 48, 64, 96, 128, 256, 512, 1024, 1040, 1048, 1064, 2064, 2072, 3072, 3088, 4000, 4032, 4048, 4064, 4080, 4088};
  long reps = 20000;
  for (int v = 0; v < 2; v++) {
    double (*fn)(int, long) = v ? runF : runD; double ratio[K][R], tref[K * R]; int order[K * R], nref = 0;
    for (int i = 0; i < K * R; i++) order[i] = i % K;
    srand(12345 + v); for (int i = K * R - 1; i > 0; i--) { int j = rand() % (i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    int cnt[K] = {0}; for (int k = 0; k < 3; k++) fn(2048, reps);
    for (int i = 0; i < K * R; i++) { int k = order[i];
      double a = fn(2048, reps), t = fn(offs[k], reps), b = fn(2048, reps); ratio[k][cnt[k]++] = t / ((a + b) / 2); tref[nref++] = (a + b) / 2; }
    qsort(tref, nref, sizeof(double), cmp);
    double per = tref[nref / 2] / reps / (v ? N : N / 2) * 1e9;
    printf("== vòng %s (%s), tỉ số t(off)/t(2048), %d lần mỗi off, thứ tự xáo trộn; tham chiếu off 2048: %.4f ns/phần tử (trung vị)\n",
           v ? "F" : "D", v ? "b[i] = a[i] + 1, float" : "dst[i] = src[i]*1.5 + 1, double", R, per);
    for (int k = 0; k < K; k++) { qsort(ratio[k], R, sizeof(double), cmp);
      printf("off %4d  trung vị %.3f  (Q1 %.3f  Q3 %.3f)\n", offs[k], ratio[k][R / 2], ratio[k][R / 4], ratio[k][3 * R / 4]); }
    fflush(stdout);
  }
  return 0;
}
