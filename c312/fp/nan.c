// nan.c — NaN đi qua so sánh, kẹp giá trị, max, sắp xếp, và -ffinite-math-only.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
__attribute__((noinline)) float clamp_if(float x, float lo, float hi) {     // kiểu kẹp "an toàn" thường gặp
  if (x > hi) x = hi;
  if (x < lo) x = lo;
  return x;
}
__attribute__((noinline)) float clamp_fminmax(float x, float lo, float hi) { return fminf(fmaxf(x, lo), hi); }
__attribute__((noinline)) float max_tern(float a, float b) { return a > b ? a : b; }
__attribute__((noinline)) int is_nan(float x) { return isnan(x); }
__attribute__((noinline)) int self_ne(float x) { return x != x; }
static int cmpf(const void *p, const void *q) { float a = *(const float *)p, b = *(const float *)q; return (a > b) - (a < b); }
int main(void) {
  volatile float z = 0.0f; float n = z / z;                                  // NaN lúc chạy
  printf("NaN == NaN: %d | NaN != NaN: %d | NaN < 1: %d | NaN > 1: %d | NaN >= NaN: %d\n", n == n, n != n, n < 1, n > 1, n >= n);
  float c1 = clamp_if(n, 0.0f, 100.0f), c2 = clamp_fminmax(n, 0.0f, 100.0f);
  printf("kẹp NaN về [0, 100]: bằng if → %g | bằng fminf(fmaxf()) → %g\n", c1, c2);
  printf("  rồi đổi sang int32 (lệnh PWM, §1.5): %d\n", (int32_t)c1);
  printf("max(a, b) = a > b ? a : b : max(NaN, 1) = %g | max(1, NaN) = %g | fmaxf(NaN, 1) = %g | fmaxf(1, NaN) = %g\n",
         max_tern(n, 1), max_tern(1, n), fmaxf(n, 1), fmaxf(1, n));
  float v[] = {5, 3, n, 1, 4, 2, n, 0};
  qsort(v, 8, sizeof v[0], cmpf);
  printf("qsort {5,3,NaN,1,4,2,NaN,0} →");
  for (int i = 0; i < 8; i++) printf(" %g", v[i]);
  printf("\nisnan(NaN) = %d | (x != x) = %d\n", is_nan(n), self_ne(n));
  return 0;
}
