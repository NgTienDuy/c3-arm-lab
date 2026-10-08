// fma.c — "co" a*b + c thành một lệnh FMA (một lần làm tròn) đổi kết quả: cùng mã nguồn, khác cờ biên dịch / kiến trúc.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
__attribute__((noinline)) float leg(float h, float a) { return sqrtf(h * h - a * a); }   // cạnh góc vuông; h == a → phải ra 0
__attribute__((noinline)) float dot3(const float *u, const float *v) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; }
int main(void) {
  volatile float h = 0.1f;
  float r = leg(h, h);
  float u[3] = {0.1f, 0.2f, 0.3f}, v[3] = {1.1f, -2.3f, 7.0f};
  float d = dot3(u, v); uint32_t bits; memcpy(&bits, &d, 4);
  int nan_count = 0, nz = 0;
  for (int i = 1; i <= 10000; i++) { float x = i * 0.001f; float q = leg(x, x); nan_count += isnan(q); nz += q != 0 && !isnan(q); }
  printf("sqrtf(h*h - a*a) với h = a = 0.1: %g | 10^4 giá trị x: %d lần NaN, %d lần khác 0\n", r, nan_count, nz);
  printf("dot3 = %.9g (0x%08X)\n", d, bits);
  return 0;
}
