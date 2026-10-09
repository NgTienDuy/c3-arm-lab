// alias3.c [LP] — 4K aliasing đo bằng TỈ SỐ: mỗi phép đo ở khoảng cách off được kẹp giữa hai phép đo ở off tham chiếu 2048 (xa mọi
// vùng aliasing), cùng tiến trình, cùng lõi, kề nhau trong thời gian → trôi xung, nhiễu nền triệt tiêu. 19 giá trị off × 15 lần,
// thứ tự xáo trộn. Hai vòng: D — dst[i] = src[i]*1.5 + 1 (double, như alias2.c); F — b[i] = a[i] + 1 (float, như alias4k của C3.6 §3.4).
// dst/b = vùng nguồn + 8192 + off. In trung vị tỉ số t(off)/t(2048) và tứ phân vị. Trên Windows: ghim LP và tự khai HighQoS.
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
static int cmp(const void *a, const void *b) { double x = *(double *)a, y = *(double *)b; return (x > y) - (x < y); }
static char *base;
__attribute__((noinline)) static double runD(int off, long reps) {
  double *src = (double *)base, *dst = (double *)(base + 8192 + off); double t0 = now();
  for (long r = 0; r < reps; r++) { for (int i = 0; i < N / 2; i++) dst[i] = src[i] * 1.5 + 1.0; __asm__ volatile("" ::: "memory"); }
  return now() - t0;
}
__attribute__((noinline)) static double runF(int off, long reps) {
  float *a = (float *)base, *b = (float *)(base + 8192 + off); double t0 = now();
  for (long r = 0; r < reps; r++) { for (int i = 0; i < N; i++) b[i] = a[i] + 1.0f; __asm__ volatile("" ::: "memory"); }
  return now() - t0;
}
int main(int argc, char **argv) {
#ifdef _WIN32
  int lp = argc > 1 ? atoi(argv[1]) : 0; SetThreadAffinityMask(GetCurrentThread(), 1ull << lp);
  PROCESS_POWER_THROTTLING_STATE st = {PROCESS_POWER_THROTTLING_CURRENT_VERSION, PROCESS_POWER_THROTTLING_EXECUTION_SPEED, 0};
  SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &st, sizeof st);
  base = _aligned_malloc(64 * 1024, 4096);
#else
  base = aligned_alloc(4096, 64 * 1024);
#endif
  for (int i = 0; i < 16384; i++) ((float *)base)[i] = (float)i;
  const int offs[] = {0, 8, 16, 24, 32, 48, 64, 96, 128, 256, 512, 1024, 3072, 4000, 4032, 4048, 4064, 4080, 4088};
  const int K = sizeof offs / sizeof *offs, R = 15; long reps = 20000;
  for (int v = 0; v < 2; v++) {
    double (*fn)(int, long) = v ? runF : runD; double ratio[19][15]; int order[19 * 15];
    for (int i = 0; i < K * R; i++) order[i] = i % K;
    srand(12345 + v); for (int i = K * R - 1; i > 0; i--) { int j = rand() % (i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    int cnt[19] = {0}; for (int k = 0; k < 3; k++) fn(2048, reps);
    for (int i = 0; i < K * R; i++) { int k = order[i];
      double a = fn(2048, reps), t = fn(offs[k], reps), b = fn(2048, reps); ratio[k][cnt[k]++] = t / ((a + b) / 2); }
    printf("== vòng %s (%s), tỉ số t(off)/t(2048), %d lần mỗi off, thứ tự xáo trộn\n", v ? "F" : "D", v ? "b[i] = a[i] + 1, float" : "dst[i] = src[i]*1.5 + 1, double", R);
    for (int k = 0; k < K; k++) { qsort(ratio[k], R, sizeof(double), cmp);
      printf("off %4d  trung vị %.3f  (Q1 %.3f  Q3 %.3f)\n", offs[k], ratio[k][R / 2], ratio[k][R / 4], ratio[k][3 * R / 4]); }
    fflush(stdout);
  }
  return 0;
}
