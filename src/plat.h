// plat.h — lớp mỏng cho Linux (x86-64, AArch64) và Windows (mingw-w64, x86-64):
// ghim luồng vào CPU, đồng hồ ns, tạo luồng, xóa một dòng khỏi mọi cache.
#pragma once
#if !defined(_WIN32) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
static inline void pin(int lp) {
  if (!SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << lp)) { fprintf(stderr, "pin %d lỗi\n", lp); exit(1); }
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
}
#include <x86intrin.h>
// QueryPerformanceCounter chỉ phân giải 100 ns → dùng TSC, hiệu chuẩn một lần theo QPC (200 ms)
static inline double now_ns(void) {
  static double ns_per_tick;
  if (!ns_per_tick) {
    LARGE_INTEGER f, a, b; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&a);
    uint64_t t0 = __rdtsc();
    do QueryPerformanceCounter(&b); while ((b.QuadPart - a.QuadPart) * 5 < f.QuadPart);
    uint64_t t1 = __rdtsc();
    ns_per_tick = (double)(b.QuadPart - a.QuadPart) * 1e9 / (double)f.QuadPart / (double)(t1 - t0);
  }
  return (double)__rdtsc() * ns_per_tick;
}
typedef HANDLE thr_t;
typedef struct { void *(*fn)(void *); void *arg; } thr_box;
static DWORD WINAPI thr_tramp(LPVOID p) { thr_box *b = p; b->fn(b->arg); free(b); return 0; }
static inline thr_t thr_start(void *(*fn)(void *), void *arg) {
  thr_box *b = malloc(sizeof *b); b->fn = fn; b->arg = arg;
  return CreateThread(NULL, 0, thr_tramp, b, 0, NULL);
}
static inline void thr_join(thr_t t) { WaitForSingleObject(t, INFINITE); CloseHandle(t); }
static inline void *amalloc(size_t al, size_t n) { return _aligned_malloc(n, al); }
#else
#include <pthread.h>
#include <sched.h>
#include <time.h>
static inline void pin(int lp) {
  cpu_set_t s; CPU_ZERO(&s); CPU_SET(lp, &s);
  if (pthread_setaffinity_np(pthread_self(), sizeof s, &s)) { fprintf(stderr, "pin %d lỗi\n", lp); exit(1); }
}
static inline double now_ns(void) {
  struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1e9 + t.tv_nsec;
}
typedef pthread_t thr_t;
static inline thr_t thr_start(void *(*fn)(void *), void *arg) { pthread_t t; pthread_create(&t, NULL, fn, arg); return t; }
static inline void thr_join(thr_t t) { pthread_join(t, NULL); }
static inline void *amalloc(size_t al, size_t n) { return aligned_alloc(al, (n + al - 1) / al * al); }
#endif

#if defined(__aarch64__)
#define ARCH "aarch64"
static inline void flush_line(const void *p) { asm volatile("dc civac, %0" :: "r"(p) : "memory"); }
static inline void flush_fence(void) { asm volatile("dsb ish" ::: "memory"); }
static inline void relax(void) { asm volatile("yield" ::: "memory"); }
#else
#define ARCH "x86-64"
static inline void flush_line(const void *p) { asm volatile("clflush (%0)" :: "r"(p) : "memory"); }
static inline void flush_fence(void) { asm volatile("mfence" ::: "memory"); }
static inline void relax(void) { asm volatile("pause" ::: "memory"); }
#endif

static int cmp_d(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
static inline double median(double *v, int n) { qsort(v, n, sizeof *v, cmp_d); return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]); }
