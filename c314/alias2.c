// alias2.c [LP] — 4K aliasing có kiểm soát: dst = base + 8192 + off, src = base (căn 4 KiB); vòng dst[i] = src[i]*1.5 + 1 trên 512 double.
// Quét off = 0, 16, …, 4080 (byte); mỗi off: trung vị 7 lần đo, mỗi lần 20.000 lượt; in ns mỗi phần tử. Trên Windows ghim vào LP (mặc định 0).
// Lệnh nạp src[j] bị coi là có thể phụ thuộc lệnh ghi dst[i] trước nó nếu 12 bit thấp của hai địa chỉ trùng: off ≡ 8(j − i) (mod 4096).
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
#define N 512
static int cmp(const void *a, const void *b) { double x = *(double *)a, y = *(double *)b; return (x > y) - (x < y); }
__attribute__((noinline)) static double run(double *dst, const double *src, long reps) {
  double t0 = now();
  for (long r = 0; r < reps; r++) { for (int i = 0; i < N; i++) dst[i] = src[i] * 1.5 + 1.0; __asm__ volatile("" ::: "memory"); }
  return (now() - t0) / ((double)reps * N) * 1e9;
}
int main(int argc, char **argv) {
#ifdef _WIN32
  int lp = argc > 1 ? atoi(argv[1]) : 0; SetThreadAffinityMask(GetCurrentThread(), 1ull << lp);
  char *base = _aligned_malloc(4 * 4096 * 4, 4096);
#else
  char *base = aligned_alloc(4096, 4 * 4096 * 4);
#endif
  double *src = (double *)base; for (int i = 0; i < N; i++) src[i] = i;
  for (int k = 0; k < 3; k++) run((double *)(base + 8192), src, 20000);          // làm nóng
  for (int off = 0; off < 4096; off += 16) {
    double *dst = (double *)(base + 8192 + off), v[7];
    for (int k = 0; k < 7; k++) v[k] = run(dst, src, 20000);
    qsort(v, 7, sizeof *v, cmp);
    printf("off %4d  ns/phần_tử %.4f  (min %.4f max %.4f)\n", off, v[3], v[0], v[6]); fflush(stdout);
  }
  return 0;
}
