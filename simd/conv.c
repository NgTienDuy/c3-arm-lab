// conv.c — checkpoint C3.8: vector hóa convolution 1-D trên tín hiệu rung thật (CWRU, 12 kHz), bộ lọc thông dải
// 2.625–3.000 Hz, K = 128 hệ số (sinc cửa sổ Hamming). So bản vô hướng với bản compiler tự vector hóa (128 bit và
// rộng nhất) và bản intrinsics "tile thanh ghi"; kiểm sai số so với tham chiếu double.  chạy: ./conv cpu x.f32
#include "bench.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#if defined(__aarch64__)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif
#define K 128
#define DECL(S) void conv_direct##S(float *restrict, const float *restrict, const float *restrict, int, int); \
                void conv_tapouter##S(float *restrict, const float *restrict, const float *restrict, int, int); \
                void conv_q15##S(int16_t *restrict, const int16_t *restrict, const int16_t *restrict, int, int);
DECL(_s) DECL(_v128) DECL(_vw) DECL(_fast)

static float *X, *Y, H[K];
static double *REF;
static int16_t *XQ, *YQ, HQ[K], *YQREF;
static int NOUT;

#if defined(__aarch64__)
// 32 đầu ra mỗi khối = 8 thanh ghi × 4 làn; mỗi hệ số: 1 lần phát hệ số (theo làn) + 8 lần nạp + 8 FMA
__attribute__((noinline)) void conv_intr(float *restrict y, const float *restrict x, const float *restrict h, int n_out, int k_) {
  int n = 0;
  for (; n + 32 <= n_out; n += 32) {
    float32x4_t a0 = vdupq_n_f32(0), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0;
    const float *xp = x + n;
    for (int k = 0; k < K; k++, xp++) {
      float32x4_t hk = vdupq_n_f32(h[k]);
      a0 = vfmaq_f32(a0, hk, vld1q_f32(xp));      a1 = vfmaq_f32(a1, hk, vld1q_f32(xp + 4));
      a2 = vfmaq_f32(a2, hk, vld1q_f32(xp + 8));  a3 = vfmaq_f32(a3, hk, vld1q_f32(xp + 12));
      a4 = vfmaq_f32(a4, hk, vld1q_f32(xp + 16)); a5 = vfmaq_f32(a5, hk, vld1q_f32(xp + 20));
      a6 = vfmaq_f32(a6, hk, vld1q_f32(xp + 24)); a7 = vfmaq_f32(a7, hk, vld1q_f32(xp + 28));
    }
    vst1q_f32(y + n, a0);      vst1q_f32(y + n + 4, a1);  vst1q_f32(y + n + 8, a2);  vst1q_f32(y + n + 12, a3);
    vst1q_f32(y + n + 16, a4); vst1q_f32(y + n + 20, a5); vst1q_f32(y + n + 24, a6); vst1q_f32(y + n + 28, a7);
  }
  for (; n < n_out; n++) { float s = 0; for (int k = 0; k < K; k++) s = fmaf(h[k], x[n + k], s); y[n] = s; }
}
#define NAME_INTR "NEON tile 8×4"
#else
// 64 đầu ra mỗi khối = 8 thanh ghi ymm × 8 làn; mỗi hệ số: 1 lần phát hệ số + 8 lần nạp không căn + 8 FMA
__attribute__((target("avx2,fma"), noinline)) void conv_intr(float *restrict y, const float *restrict x, const float *restrict h, int n_out, int k_) {
  int n = 0;
  for (; n + 64 <= n_out; n += 64) {
    __m256 a0 = _mm256_setzero_ps(), a1 = a0, a2 = a0, a3 = a0, a4 = a0, a5 = a0, a6 = a0, a7 = a0;
    const float *xp = x + n;
    for (int k = 0; k < K; k++, xp++) {
      __m256 hk = _mm256_broadcast_ss(h + k);
      a0 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp), a0);      a1 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp + 8), a1);
      a2 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp + 16), a2); a3 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp + 24), a3);
      a4 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp + 32), a4); a5 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp + 40), a5);
      a6 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp + 48), a6); a7 = _mm256_fmadd_ps(hk, _mm256_loadu_ps(xp + 56), a7);
    }
    _mm256_storeu_ps(y + n, a0);      _mm256_storeu_ps(y + n + 8, a1);  _mm256_storeu_ps(y + n + 16, a2); _mm256_storeu_ps(y + n + 24, a3);
    _mm256_storeu_ps(y + n + 32, a4); _mm256_storeu_ps(y + n + 40, a5); _mm256_storeu_ps(y + n + 48, a6); _mm256_storeu_ps(y + n + 56, a7);
  }
  for (; n < n_out; n++) { float s = 0; for (int k = 0; k < K; k++) s = fmaf(h[k], x[n + k], s); y[n] = s; }
}
#define NAME_INTR "AVX2 tile 8×8"
#endif

typedef void (*convf)(float *restrict, const float *restrict, const float *restrict, int, int);
static convf CUR;
static void run(void *p) { for (int r = 0; r < 10; r++) { CUR(Y, X, H, NOUT, K); asm volatile("" ::: "memory"); } }
typedef void (*convq)(int16_t *restrict, const int16_t *restrict, const int16_t *restrict, int, int);
static convq CURQ;
static void runq(void *p) { for (int r = 0; r < 10; r++) { CURQ(YQ, XQ, HQ, NOUT, K); asm volatile("" ::: "memory"); } }

int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  FILE *f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; }
  fseek(f, 0, SEEK_END); int n = ftell(f) / 4; fseek(f, 0, SEEK_SET);
  X = malloc(4 * (n + 64)); if (fread(X, 4, n, f) != (size_t)n) return 1; fclose(f);
  NOUT = n - K + 1;
  Y = malloc(4 * NOUT); REF = malloc(8 * NOUT);
  // bộ lọc thông dải 2.625–3.000 Hz ở 12 kHz: hiệu hai sinc thông thấp, cửa sổ Hamming
  const double fs = 12000, f1 = 2625, f2 = 3000;
  for (int k = 0; k < K; k++) {
    double t = k - (K - 1) / 2.0, w = 0.54 - 0.46 * cos(2 * M_PI * k / (K - 1));
    double lp2 = 2 * f2 / fs * (t == 0 ? 1 : sin(2 * M_PI * f2 / fs * t) / (2 * M_PI * f2 / fs * t));
    double lp1 = 2 * f1 / fs * (t == 0 ? 1 : sin(2 * M_PI * f1 / fs * t) / (2 * M_PI * f1 / fs * t));
    H[k] = (float)((lp2 - lp1) * w);
  }
  for (int i = 0; i < NOUT; i++) { double s = 0; for (int k = 0; k < K; k++) s += (double)H[k] * X[i + k]; REF[i] = s; }
  double rms = 0; for (int i = 0; i < NOUT; i++) rms += REF[i] * REF[i]; rms = sqrt(rms / NOUT);
  printf("%s: %d mẫu, K = %d, %d đầu ra; RMS đầu ra băng 2.625–3.000 Hz = %.4f\n", argv[2], n, K, NOUT, rms);
  struct { const char *name; convf fn; } V[] = {
    {"vô hướng (-fno-tree-vectorize), trực tiếp", conv_direct_s}, {"vô hướng, hệ số ngoài", conv_tapouter_s},
#if defined(__aarch64__)
    {"tự động NEON 128 bit, trực tiếp", conv_direct_v128}, {"tự động NEON 128 bit, hệ số ngoài", conv_tapouter_v128},
    {"tự động SVE, trực tiếp", conv_direct_vw}, {"tự động SVE, hệ số ngoài", conv_tapouter_vw},
    {"tự động NEON -Ofast, trực tiếp", conv_direct_fast},
#else
    {"tự động SSE2 128 bit, trực tiếp", conv_direct_v128}, {"tự động SSE2 128 bit, hệ số ngoài", conv_tapouter_v128},
    {"tự động AVX2 256 bit, trực tiếp", conv_direct_vw}, {"tự động AVX2 256 bit, hệ số ngoài", conv_tapouter_vw},
    {"tự động AVX2 -Ofast, trực tiếp", conv_direct_fast},
#endif
    {"intrinsics " NAME_INTR, conv_intr}};
  int nv = sizeof V / sizeof V[0];
  double base = 0;
  printf("  %-44s %9s %8s %8s %12s\n", "phiên bản", "chu kỳ/ra", "FLOP/ck", "tăng tốc", "sai số max");
  for (int v = 0; v < nv; v++) {
    CUR = V[v].fn;
    double c = bench_cycles(run, 0, 10.0 * NOUT);
    if (v == 0) base = c;
    double err = 0; for (int i = 0; i < NOUT; i++) { double e = fabs(Y[i] - REF[i]); if (e > err) err = e; }
    printf("  %-44s %9.2f %8.2f %7.2f× %12.2e\n", V[v].name, c, 2.0 * K / c, base / c, err);
  }
  printf("  (xung %.2f GHz; trần FMA lõi này: x86 P-core 32 FLOP/chu kỳ float — C3.6 §11.2)\n", bench_ghz);
  // Q15: lượng tử hóa tín hiệu và hệ số về Q15 (tỉ lệ cố định), so với bản vô hướng từng bit
  XQ = malloc(2 * (n + 64)); YQ = malloc(2 * NOUT); YQREF = malloc(2 * NOUT);
  float mx = 0; for (int i = 0; i < n; i++) if (fabsf(X[i]) > mx) mx = fabsf(X[i]);
  for (int i = 0; i < n; i++) XQ[i] = (int16_t)lrintf(X[i] / mx * 32767);
  for (int k = 0; k < K; k++) HQ[k] = (int16_t)lrintf(H[k] * 32767);
  conv_q15_s(YQREF, XQ, HQ, NOUT, K);
  struct { const char *name; convq fn; } Q[] = {{"Q15 vô hướng", conv_q15_s}, {"Q15 tự động 128 bit", conv_q15_v128}, {"Q15 tự động rộng nhất", conv_q15_vw}};
  double bq = 0;
  for (int v = 0; v < 3; v++) {
    CURQ = Q[v].fn; double c = bench_cycles(runq, 0, 10.0 * NOUT); if (v == 0) bq = c;
    int diff = 0; for (int i = 0; i < NOUT; i++) diff += YQ[i] != YQREF[i];
    printf("  %-44s %9.2f %8.2f %7.2f×  khác bản vô hướng: %d\n", Q[v].name, c, 2.0 * K / c, bq / c, diff);
  }
  return 0;
}
