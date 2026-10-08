// conv.c — đổi số thực sang số nguyên khi giá trị nằm ngoài miền: UB trong C, mỗi kiến trúc trả một kiểu.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
__attribute__((noinline)) int32_t f2i(float x) { return (int32_t)x; }
__attribute__((noinline)) uint32_t f2u(float x) { return (uint32_t)x; }
__attribute__((noinline)) int16_t d2s(double x) { return (int16_t)x; }     // kiểu đổi của Ariane 501 (64 bit thực → 16 bit nguyên)
int main(void) {
  volatile float in[] = {NAN, INFINITY, 3e9f, -3e9f, -1.0f, -0.5f};
  const char *nm[] = {"NaN", "+Inf", "3e9", "-3e9", "-1.0", "-0.5"};
  for (int i = 0; i < 6; i++)
    printf("  x = %-5s (int32_t)x = %11d   (uint32_t)x = %10u\n", nm[i], f2i(in[i]), f2u(in[i]));
  volatile double d[] = {32767.0, 40000.0, 1e10};
  for (int i = 0; i < 3; i++) printf("  (int16_t)%.0f = %d\n", d[i], d2s(d[i]));
  return 0;
}
