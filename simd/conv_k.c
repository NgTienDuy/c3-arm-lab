// conv_k.c — convolution 1-D (FIR) viết bằng C thuần, hai dạng; dịch nhiều lần với cờ khác nhau (-DSFX=…).
//   y[n] = Σ_{k<K} h[k] · x[n + k],  n = 0 … N − K
#include <stdint.h>
#include <string.h>
#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
// dạng "trực tiếp": mỗi đầu ra là một tích vô hướng (vòng trong là phép giảm theo k)
void CAT(conv_direct, SFX)(float *restrict y, const float *restrict x, const float *restrict h, int n_out, int K) {
  for (int n = 0; n < n_out; n++) {
    float s = 0;
    for (int k = 0; k < K; k++) s += h[k] * x[n + k];
    y[n] = s;
  }
}
// dạng "hệ số ngoài": quét toàn bộ y cho mỗi hệ số (vòng trong không có phép giảm)
void CAT(conv_tapouter, SFX)(float *restrict y, const float *restrict x, const float *restrict h, int n_out, int K) {
  memset(y, 0, sizeof(float) * n_out);
  for (int k = 0; k < K; k++) {
    float hk = h[k];
    for (int n = 0; n < n_out; n++) y[n] += hk * x[n + k];
  }
}
// điểm cố định Q15 (DSP): x, h là int16 dạng Q15, cộng dồn int32, làm tròn, bão hòa về int16
static inline int16_t sat16(int32_t v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v; }
void CAT(conv_q15, SFX)(int16_t *restrict y, const int16_t *restrict x, const int16_t *restrict h, int n_out, int K) {
  for (int n = 0; n < n_out; n++) {
    int32_t s = 1 << 14;                                   // làm tròn
    for (int k = 0; k < K; k++) s += (int32_t)h[k] * x[n + k];
    y[n] = sat16(s >> 15);
  }
}
