// kern_c.c — hai hàm C thuần, dịch nhiều lần với cờ khác nhau (-DSFX=…) để so với bản intrinsics.
#include <stdint.h>
#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
// sat16 của C3.4 §9: bão hòa 32 → 16 bit có dấu
static inline int16_t sat16(int32_t y) { if (y > 32767) return 32767; if (y < -32768) return -32768; return (int16_t)y; }
void CAT(sat_c, SFX)(int16_t *restrict out, const int32_t *restrict in, int n) {
  for (int i = 0; i < n; i++) out[i] = sat16(in[i]);
}
// sum_branchy của C3.5 §8.2: tổng các phần tử ≥ ngưỡng
long CAT(sum_ge_c, SFX)(const int *v, int n, int t) {
  long s = 0;
  for (int i = 0; i < n; i++)
    if (v[i] >= t) s += v[i];
  return s;
}
