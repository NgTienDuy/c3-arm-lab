// aslr.c — cùng một vòng s[i] = g[i]*1.5 + 1 (s trên ngăn xếp, g toàn cục căn 4 KiB); thời gian có phụ thuộc vào (s − g) mod 4096 không?
// Trên x86, một lệnh nạp bị coi là phụ thuộc vào lệnh ghi trước nó nếu 12 bit thấp của địa chỉ trùng ("4K aliasing").
// In: offset = (s − g) mod 4096, ns mỗi phần tử (trung vị 7 lần), và địa chỉ ngăn xếp mod 4096.
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N 512
static double g[N] __attribute__((aligned(4096)));
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static int cmp(const void *a, const void *b) { double x = *(double *)a, y = *(double *)b; return (x > y) - (x < y); }
__attribute__((noinline)) static double run(double *s, long reps) {
  double t0 = now();
  for (long r = 0; r < reps; r++) { for (int i = 0; i < N; i++) s[i] = g[i] * 1.5 + 1.0; asm volatile("" ::: "memory"); }
  return (now() - t0) / ((double)reps * N) * 1e9;
}
int main(int argc, char **argv) {
  long reps = argc > 1 ? atol(argv[1]) : 100000; double s[N]; double v[7];
  for (int i = 0; i < N; i++) { g[i] = i; s[i] = 0; }
  for (int k = 0; k < 7; k++) v[k] = run(s, reps);
  qsort(v, 7, sizeof *v, cmp);
  printf("offset %4lu  ns/phần_tử %.4f  stack%%4096 %4lu  s[0]=%.1f\n", (unsigned long)(((uintptr_t)s - (uintptr_t)g) & 4095), v[3],
         (unsigned long)((uintptr_t)s & 4095), s[N - 1]);
  return 0;
}
