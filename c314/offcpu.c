// offcpu.c SECONDS — vòng điều khiển định kỳ: tính ~1 ms rồi ngủ 9 ms. Dùng CPU ~10%, không đọc đĩa — "không bận CPU" nhưng KHÔNG phải nút thắt I/O.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
int main(int c, char **v) { double T = atof(v[1]), t0 = now(); uint64_t x = 1, cycles = 0;
  while (now() - t0 < T) { double a = now(); while (now() - a < 1e-3) for (int i = 0; i < 1000; i++) x = x * 6364136223846793005ull + 1442695040888963407ull;
    struct timespec z = {0, 9000000}; nanosleep(&z, 0); cycles++; }
  printf("offcpu: %llu chu kỳ điều khiển (x=%llu)\n", (unsigned long long)cycles, (unsigned long long)(x & 7)); return 0; }
