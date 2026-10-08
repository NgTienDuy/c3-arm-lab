// fp754.c — tách các trường của float32/float64, khoảng cách giữa hai số liền kề (ulp), và bit của NaN do phần cứng sinh ra.
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t fbits(float x) { uint32_t u; memcpy(&u, &x, 4); return u; }
static void show(const char *nm, float x) {
  uint32_t u = fbits(x), s = u >> 31, e = (u >> 23) & 0xFF, m = u & 0x7FFFFF;
  const char *k = e == 0xFF ? (m ? "NaN" : "Inf") : e == 0 ? (m ? "dưới chuẩn" : "không") : "chuẩn";
  printf("  %-22s 0x%08X  dấu %u  mũ %3u (%4d)  định trị 0x%06X  %-10s = %.9g\n", nm, u, s, e,
         e == 0 ? -126 : (int)e - 127, m, k, x);
}
int main(void) {
  volatile float zero = 0.0f, one = 1.0f, big = 3e38f;
  puts("float32 = 1 bit dấu | 8 bit mũ (độ lệch 127) | 23 bit định trị (+1 bit ẩn):");
  show("1.0", 1.0f); show("0.1", 0.1f); show("-0.0", -0.0f);
  show("16777216 = 2^24", 16777216.0f); show("16777217 (làm tròn)", 16777217.0f);
  show("FLT_MAX", FLT_MAX); show("FLT_MIN (chuẩn nhỏ nhất)", FLT_MIN);
  show("dưới chuẩn nhỏ nhất", nextafterf(0.0f, 1.0f)); show("FLT_MIN / 4", FLT_MIN / 4);
  show("1/0", one / zero); show("FLT_MAX * 10", big * 10.0f);
  show("0/0", zero / zero); show("sqrtf(-1)", sqrtf(-one)); show("Inf - Inf", (one / zero) - (one / zero));
  show("nanf(\"\")", nanf(""));
  printf("0.1 lưu thật là %.30f\n", (double)0.1f);
  printf("0.1 (double)    %.30f\n", 0.1);
  puts("khoảng cách tới số kế tiếp (ulp):");
  double xs[] = {1.0, 1000.0, 1e6, 16777216.0, 86400.0, 31557600.0, 1.79e9, 106.7};
  const char *ns[] = {"1", "1000", "10^6", "2^24", "86400 (1 ngày, s)", "3,16e7 (1 năm, s)", "1,79e9 (giờ Unix 2026, s)", "106,7 (kinh độ, °)"};
  for (int i = 0; i < 8; i++)
    printf("  %-26s float32 %-12.6g float64 %.6g\n", ns[i], (double)(nextafterf((float)xs[i], INFINITY) - (float)xs[i]),
           nextafter(xs[i], INFINITY) - xs[i]);
  return 0;
}
