// gather.c MiB N — N lần đọc 8 byte ở chỉ số ngẫu nhiên trong bảng MiB MiB (16 luồng chỉ số độc lập → nhiều lần trượt chồng nhau),
// mô phỏng bước dò của hash join / tra bảng lớn. In ns mỗi lần đọc.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int c, char **v) {
  size_t w = (size_t)atol(v[1]) << 17; long n = atol(v[2]); uint64_t *a = aligned_alloc(64, w * 8);
  for (size_t i = 0; i < w; i++) a[i] = i * 0x9E3779B97F4A7C15ull;
  uint64_t st[16], s = 0; for (int k = 0; k < 16; k++) st[k] = 0x1234567 + k * 977;
  struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
  for (long i = 0; i < n / 16; i++)
    for (int k = 0; k < 16; k++) { uint64_t x = st[k]; x ^= x << 13; x ^= x >> 7; x ^= x << 17; st[k] = x; s += a[x & (w - 1)]; }
  clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
  printf("gather %s MiB: %.2f ns/lần đọc (tổng %llu)\n", v[1], ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec)) / n, (unsigned long long)(s & 255));
  return 0;
}
