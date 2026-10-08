// bench.h — đo theo chu kỳ khi không có PMU (cùng phương pháp C3.5 §0.3):
// mỗi phép đo được kẹp giữa hai lần hiệu chuẩn bằng 10^7 lệnh add thanh ghi phụ thuộc (1 lệnh / chu kỳ),
// lấy trung vị của 9 tỉ số. Dùng được trên x86-64 và AArch64 (Linux).
#pragma once
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static inline double bench_now(void) {
  struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1e9 + t.tv_nsec;
}
static inline void bench_pin(int cpu) {
  cpu_set_t s; CPU_ZERO(&s); CPU_SET(cpu, &s);
  if (sched_setaffinity(0, sizeof s, &s)) { perror("sched_setaffinity"); exit(1); }
}
static double bench_calib(void) {             // ns mỗi chu kỳ
  uint64_t z = 1;
  double t0 = bench_now();
  for (int i = 0; i < 100000; i++)
#if defined(__aarch64__)
    asm volatile(".rept 100\n\tadd %0, %0, %0\n\t.endr" : "+r"(z));
#else
    asm volatile(".rept 100\n\tadd %0, %0\n\t.endr" : "+r"(z));
#endif
  double t1 = bench_now();
  if (z == 42) puts("");
  return (t1 - t0) / 1e7;
}
static int bench_cmpd(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y);
}
static double bench_ghz;                      // xung đo được ở lần gọi gần nhất
// chạy fn(arg) — một lần gọi nên dài 5–50 ms — trả về số chu kỳ chia cho `units`
static double bench_cycles(void (*fn)(void *), void *arg, double units) {
  double r[9], g[9];
  fn(arg);                                     // làm nóng
  for (int k = 0; k < 9; k++) {
    double c1 = bench_calib();
    double t0 = bench_now(); fn(arg); double t1 = bench_now();
    double c2 = bench_calib();
    double c = 0.5 * (c1 + c2);
    r[k] = (t1 - t0) / c / units; g[k] = 1.0 / c;
  }
  qsort(r, 9, sizeof r[0], bench_cmpd); qsort(g, 9, sizeof g[0], bench_cmpd);
  bench_ghz = g[4];
  return r[4];
}
