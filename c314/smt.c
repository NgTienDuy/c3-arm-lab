// smt.c VCPU NCPU MODE [NMODE] — đo thời gian một tải "nạn nhân" ghim ở VCPU, có/không có tải "hàng xóm" ghim ở NCPU (-1 = không có).
//   MODE/NMODE: alu (8 chuỗi nhân-cộng độc lập) | mem (đuổi con trỏ 64 MiB) — in ns mỗi đơn vị việc của nạn nhân, trung vị 5 lần.
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static volatile int stop;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static void pin(int c) { cpu_set_t s; CPU_ZERO(&s); CPU_SET(c, &s); pthread_setaffinity_np(pthread_self(), sizeof s, &s); }
static uint64_t *ring(size_t bytes, unsigned seed) { size_t n = bytes / 64; uint64_t *a = aligned_alloc(64, n * 64); size_t *p = malloc(n * sizeof *p);
  for (size_t i = 0; i < n; i++) p[i] = i; srand(seed); for (size_t i = n - 1; i > 0; i--) { size_t j = ((size_t)rand() * 2654435761u + rand()) % (i + 1); size_t t = p[i]; p[i] = p[j]; p[j] = t; }
  for (size_t i = 0; i < n; i++) a[p[i] * 8] = (uint64_t)&a[p[(i + 1) % n] * 8]; free(p); return a; }
static double work(const char *m, uint64_t *a, long units) {     // trả ns/đơn vị
  double t0 = now();
  if (!strcmp(m, "alu")) { uint64_t x[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    for (long i = 0; i < units; i++) { for (int k = 0; k < 8; k++) x[k] = x[k] * 3 + 1; asm volatile("" : : "r"(x[0]), "r"(x[7]) : "memory"); }
    if (x[0] == 42) puts("");
  } else { uint64_t p = (uint64_t)a; for (long i = 0; i < units; i++) p = *(uint64_t *)p; if (p == 1) puts(""); }
  return (now() - t0) / units * 1e9;
}
static const char *nmode; static int ncpu;
static void *neighbor(void *x) { pin(ncpu); uint64_t *a = !strcmp(nmode, "mem") ? ring(64 << 20, 2) : NULL; while (!stop) work(nmode, a, 1 << 16); return x; }
static int cmp(const void *a, const void *b) { double x = *(double *)a, y = *(double *)b; return (x > y) - (x < y); }
int main(int argc, char **argv) {
  int vcpu = atoi(argv[1]); ncpu = atoi(argv[2]); const char *m = argv[3]; nmode = argc > 4 ? argv[4] : m;
  pin(vcpu); uint64_t *a = !strcmp(m, "mem") ? ring(64 << 20, 1) : NULL; long units = !strcmp(m, "mem") ? 2000000 : 20000000;
  pthread_t t; if (ncpu >= 0) { pthread_create(&t, 0, neighbor, 0); struct timespec z = {0, 200000000}; nanosleep(&z, 0); }
  double v[5]; for (int k = 0; k < 5; k++) v[k] = work(m, a, units); qsort(v, 5, sizeof *v, cmp);
  stop = 1; if (ncpu >= 0) pthread_join(t, 0);
  printf("nạn nhân %s @cpu%d, hàng xóm %s @cpu%d: %.3f ns/đơn vị (min %.3f, max %.3f)\n", m, vcpu, ncpu >= 0 ? nmode : "-", ncpu, v[2], v[0], v[4]);
  return 0;
}
