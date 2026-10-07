// dot8.c — tích vô hướng int8 (kích hoạt u8 × trọng số s8 → tổng s32), hạt nhân của suy luận lượng tử hóa.
// x86: vô hướng · AVX2 vpmaddubsw+vpmaddwd · AVX-VNNI vpdpbusd   ARM: vô hướng · NEON smull/sadalp · NEON usdot (i8mm)
// Kiểm đúng trên dữ liệu ngẫu nhiên VÀ dữ liệu "xấu nhất" (a = 255, w = 127): vpmaddubsw bão hòa ở 16 bit.
#include "bench.h"
#include <stdint.h>
#include <string.h>
#if defined(__aarch64__)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif
#define N 4096
static uint8_t A[N] __attribute__((aligned(64)));
static int8_t Wt[N] __attribute__((aligned(64)));
static volatile int32_t sink;

__attribute__((noinline, optimize("no-tree-vectorize"))) int32_t dot_scalar(const uint8_t *a, const int8_t *w, int n) {
  int32_t s = 0;
  for (int i = 0; i < n; i++) s += a[i] * w[i];
  return s;
}
#if defined(__aarch64__)
__attribute__((noinline)) int32_t dot_simd(const uint8_t *a, const int8_t *w, int n) {   // NEON v8.0: mở rộng 8→16, nhân dài 16→32
  int32x4_t s0 = vdupq_n_s32(0), s1 = vdupq_n_s32(0);
  for (int i = 0; i < n; i += 16) {
    int16x8_t a0 = vreinterpretq_s16_u16(vmovl_u8(vld1_u8(a + i))), a1 = vreinterpretq_s16_u16(vmovl_high_u8(vld1q_u8(a + i)));
    int16x8_t w0 = vmovl_s8(vld1_s8(w + i)), w1 = vmovl_high_s8(vld1q_s8(w + i));
    s0 = vmlal_s16(s0, vget_low_s16(a0), vget_low_s16(w0)); s1 = vmlal_high_s16(s1, a0, w0);
    s0 = vmlal_s16(s0, vget_low_s16(a1), vget_low_s16(w1)); s1 = vmlal_high_s16(s1, a1, w1);
  }
  return vaddvq_s32(vaddq_s32(s0, s1));
}
__attribute__((target("arch=armv8.6-a+i8mm"), noinline)) int32_t dot_dp(const uint8_t *a, const int8_t *w, int n) {   // usdot: 4 tích u8×s8 → 1 s32
  int32x4_t s0 = vdupq_n_s32(0), s1 = vdupq_n_s32(0);
  int32x4_t s2 = vdupq_n_s32(0), s3 = vdupq_n_s32(0);
  for (int i = 0; i < n; i += 64) {
    s0 = vusdotq_s32(s0, vld1q_u8(a + i), vld1q_s8(w + i));
    s1 = vusdotq_s32(s1, vld1q_u8(a + i + 16), vld1q_s8(w + i + 16));
    s2 = vusdotq_s32(s2, vld1q_u8(a + i + 32), vld1q_s8(w + i + 32));
    s3 = vusdotq_s32(s3, vld1q_u8(a + i + 48), vld1q_s8(w + i + 48));
  }
  return vaddvq_s32(vaddq_s32(vaddq_s32(s0, s1), vaddq_s32(s2, s3)));
}
#define dot_wide dot_simd
#define NAME_WIDE "NEON smull/smlal (mở rộng 16 bit)"
#define NAME_SIMD "NEON smull/smlal (mở rộng 16 bit)"
#define NAME_DP "NEON usdot (i8mm)"
#else
__attribute__((target("avx2"), noinline)) int32_t dot_simd(const uint8_t *a, const int8_t *w, int n) {
  const __m256i ones = _mm256_set1_epi16(1);
  __m256i s0 = _mm256_setzero_si256(), s1 = _mm256_setzero_si256();
  for (int i = 0; i < n; i += 128) {
#define MB(s, o) s = _mm256_add_epi32(s, _mm256_madd_epi16(_mm256_maddubs_epi16(_mm256_load_si256((const __m256i *)(a + i + o)), \
                                         _mm256_load_si256((const __m256i *)(w + i + o))), ones))   /* cặp s16 → s32 */
    MB(s0, 0); MB(s1, 32); MB(s0, 64); MB(s1, 96);
  }
  __m256i s = _mm256_add_epi32(s0, s1);
  __m128i h = _mm_add_epi32(_mm256_castsi256_si128(s), _mm256_extracti128_si256(s, 1));
  h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0x4E)); h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0xB1));
  return _mm_cvtsi128_si32(h);
}
__attribute__((target("avx2,avxvnni"), noinline)) int32_t dot_dp(const uint8_t *a, const int8_t *w, int n) {
  __m256i s0 = _mm256_setzero_si256(), s1 = _mm256_setzero_si256();
  __m256i s2 = _mm256_setzero_si256(), s3 = _mm256_setzero_si256();   // 4 bộ cộng dồn: vpdpbusd có độ trễ nhiều chu kỳ
  for (int i = 0; i < n; i += 128) {
#define VDP(s, o) s = _mm256_dpbusd_avx_epi32(s, _mm256_load_si256((const __m256i *)(a + i + o)), _mm256_load_si256((const __m256i *)(w + i + o)))
    VDP(s0, 0); VDP(s1, 32); VDP(s2, 64); VDP(s3, 96);
  }
  __m256i s = _mm256_add_epi32(_mm256_add_epi32(s0, s1), _mm256_add_epi32(s2, s3));
  __m128i h = _mm_add_epi32(_mm256_castsi256_si128(s), _mm256_extracti128_si256(s, 1));
  h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0x4E)); h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0xB1));
  return _mm_cvtsi128_si32(h);
}
__attribute__((target("avx2"), noinline)) int32_t dot_wide(const uint8_t *a, const int8_t *w, int n) {   // mở rộng lên 16 bit trước: không bão hòa
  __m256i s0 = _mm256_setzero_si256(), s1 = _mm256_setzero_si256();
  for (int i = 0; i < n; i += 32) {
    __m256i a0 = _mm256_cvtepu8_epi16(_mm_load_si128((const __m128i *)(a + i))), a1 = _mm256_cvtepu8_epi16(_mm_load_si128((const __m128i *)(a + i + 16)));
    __m256i w0 = _mm256_cvtepi8_epi16(_mm_load_si128((const __m128i *)(w + i))), w1 = _mm256_cvtepi8_epi16(_mm_load_si128((const __m128i *)(w + i + 16)));
    s0 = _mm256_add_epi32(s0, _mm256_madd_epi16(a0, w0)); s1 = _mm256_add_epi32(s1, _mm256_madd_epi16(a1, w1));
  }
  __m256i s = _mm256_add_epi32(s0, s1);
  __m128i h = _mm_add_epi32(_mm256_castsi256_si128(s), _mm256_extracti128_si256(s, 1));
  h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0x4E)); h = _mm_add_epi32(h, _mm_shuffle_epi32(h, 0xB1));
  return _mm_cvtsi128_si32(h);
}
#define NAME_WIDE "AVX2 mở rộng 16 bit + vpmaddwd"
#define NAME_SIMD "AVX2 vpmaddubsw+vpmaddwd"
#define NAME_DP "AVX-VNNI vpdpbusd"
#endif
static void b0(void *p) { for (int r = 0; r < 20000; r++) sink = dot_scalar(A, Wt, N); }
static void b1(void *p) { for (int r = 0; r < 20000; r++) sink = dot_simd(A, Wt, N); }
static int32_t (*DP)(const uint8_t *, const int8_t *, int);
static void b2(void *p) { for (int r = 0; r < 20000; r++) sink = DP(A, Wt, N); }
static void b3(void *p) { for (int r = 0; r < 20000; r++) sink = dot_wide(A, Wt, N); }

int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  DP = dot_dp;
  uint32_t s = 3;
  for (int i = 0; i < N; i++) { s = s * 1664525u + 1013904223u; A[i] = s >> 24; Wt[i] = (int8_t)(s >> 16); }
#if !defined(__aarch64__)
  __builtin_cpu_init();
  if (!__builtin_cpu_supports("avxvnni")) { printf("(CPU không có AVX-VNNI: thay bản vpdpbusd bằng bản mở rộng 16 bit)\n"); DP = dot_wide; }
#endif
  printf("kiểm, dữ liệu ngẫu nhiên:   vô hướng %d · %s %d · %s %d\n", dot_scalar(A, Wt, N), NAME_SIMD, dot_simd(A, Wt, N), NAME_DP, DP(A, Wt, N));
  int bad = 0;                                         // từng khối 128 phần tử, dữ liệu ngẫu nhiên
  for (int i = 0; i < N; i += 128) bad += dot_simd(A + i, Wt + i, 128) != dot_scalar(A + i, Wt + i, 128);
  printf("                            khối 128 phần tử sai ở %s: %d / %d\n", NAME_SIMD, bad, N / 128);
  static uint8_t A2[128] __attribute__((aligned(64))); static int8_t W2[128] __attribute__((aligned(64)));
  memset(A2, 255, 128); memset(W2, 127, 128);
  printf("kiểm, a = 255, w = 127:     vô hướng %d · %s %d · %s %d\n", dot_scalar(A2, W2, 128), NAME_SIMD, dot_simd(A2, W2, 128), NAME_DP, DP(A2, W2, 128));
  int bad7 = 0;                                        // "reduce_range": trọng số 7 bit [-64, 63]
  static int8_t W7[N] __attribute__((aligned(64)));
  for (int i = 0; i < N; i++) W7[i] = Wt[i] >> 1;
  for (int i = 0; i < N; i += 128) bad7 += dot_simd(A + i, W7 + i, 128) != dot_scalar(A + i, W7 + i, 128);
  memset(W2, 63, 128);
  printf("trọng số 7 bit [-64, 63]:   khối sai ở %s: %d / %d; a = 255, w = 63: %d (đúng: %d)\n", NAME_SIMD, bad7, N / 128,
         dot_simd(A2, W2, 128), dot_scalar(A2, W2, 128));
  printf("kiểm %s: %d (đúng %d)\n", NAME_WIDE, dot_wide(A, Wt, N), dot_scalar(A, Wt, N));
  double u = 20000.0 * N;
  double c0 = bench_cycles(b0, 0, u), c1 = bench_cycles(b1, 0, u), c2 = bench_cycles(b2, 0, u), c3 = bench_cycles(b3, 0, u);
  printf("phép nhân-cộng int8 mỗi chu kỳ: vô hướng %.2f · %s %.1f · %s %.1f · %s %.1f  (xung %.2f GHz)\n", 1 / c0,
         NAME_WIDE, 1 / c3, NAME_SIMD, 1 / c1, NAME_DP, 1 / c2, bench_ghz);
  return 0;
}
