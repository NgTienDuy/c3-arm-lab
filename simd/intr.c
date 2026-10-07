// intr.c — intrinsics viết tay so với compiler, cho hai hàm mà C3.4 và C3.5 để lại:
//   sat16 (C3.4 §9.10): bão hòa int32 → int16 — x86 packssdw / ARM sqxtn
//   sum_ge (C3.5 §8.9): tổng các phần tử ≥ ngưỡng — AVX2 / NEON, cộng dồn 32 bit theo khối rồi mới mở rộng
#include "bench.h"
#include <stdint.h>
#include <string.h>
#if defined(__aarch64__)
#include <arm_neon.h>
#else
#include <immintrin.h>
#endif
#define N 32768
void sat_c_o2(int16_t *restrict, const int32_t *restrict, int);   // -O2 (C3.4 dùng -O2)
void sat_c_v(int16_t *restrict, const int32_t *restrict, int);    // -O3, vector rộng nhất của đích
long sum_ge_c_sse(const int *, int, int);                        // x86: -O3 (SSE2) · ARM: -O3 (NEON)
long sum_ge_c_v(const int *, int, int);                          // x86: -O3 -march=x86-64-v3 · ARM: -O3 +sve

static int32_t IN[N] __attribute__((aligned(64)));
static int16_t OUT[N] __attribute__((aligned(64))), REF[N] __attribute__((aligned(64)));
static int V[N] __attribute__((aligned(64)));

#if defined(__aarch64__)
__attribute__((noinline)) void sat_intr(int16_t *restrict out, const int32_t *restrict in, int n) {
  for (int i = 0; i < n; i += 8) {                    // sqxtn: thu hẹp có bão hòa, 4 làn mỗi lệnh
    int16x4_t lo = vqmovn_s32(vld1q_s32(in + i));
    int16x8_t r = vqmovn_high_s32(lo, vld1q_s32(in + i + 4));
    vst1q_s16(out + i, r);
  }
}
__attribute__((noinline)) void sat_intr_bad(int16_t *restrict out, const int32_t *restrict in, int n) { sat_intr(out, in, n); }
__attribute__((noinline)) long sum_ge_intr(const int *v, int n, int t) {
  int64x2_t s64 = vdupq_n_s64(0);
  int32x4_t tt = vdupq_n_s32(t);
  for (int b = 0; b < n; b += 1024) {                 // khối 1024: 256 phần tử mỗi làn × 2^20 < 2^31
    int32x4_t s0 = vdupq_n_s32(0), s1 = vdupq_n_s32(0);
    for (int i = b; i < b + 1024; i += 8) {
      int32x4_t x0 = vld1q_s32(v + i), x1 = vld1q_s32(v + i + 4);
      s0 = vaddq_s32(s0, vandq_s32(x0, vreinterpretq_s32_u32(vcgeq_s32(x0, tt))));
      s1 = vaddq_s32(s1, vandq_s32(x1, vreinterpretq_s32_u32(vcgeq_s32(x1, tt))));
    }
    s64 = vpadalq_s32(s64, vaddq_s32(s0, s1));         // cộng từng cặp, mở rộng lên 64 bit
  }
  return vaddvq_s64(s64);
}
#else
__attribute__((noinline)) void sat_intr(int16_t *restrict out, const int32_t *restrict in, int n) {
  for (int i = 0; i < n; i += 16) {                   // AVX2: packs làm việc TRONG từng nửa 128 bit → phải hoán vị lại
    __m256i a = _mm256_loadu_si256((const __m256i *)(in + i));
    __m256i b = _mm256_loadu_si256((const __m256i *)(in + i + 8));
    __m256i p = _mm256_packs_epi32(a, b);             // [a0..3 b0..3 | a4..7 b4..7]
    p = _mm256_permute4x64_epi64(p, 0xD8);            // [a0..3 a4..7 | b0..3 b4..7]
    _mm256_storeu_si256((__m256i *)(out + i), p);
  }
}
__attribute__((noinline)) void sat_intr_bad(int16_t *restrict out, const int32_t *restrict in, int n) {
  for (int i = 0; i < n; i += 16) {                   // quên hoán vị: đúng giá trị, SAI thứ tự
    __m256i a = _mm256_loadu_si256((const __m256i *)(in + i));
    __m256i b = _mm256_loadu_si256((const __m256i *)(in + i + 8));
    _mm256_storeu_si256((__m256i *)(out + i), _mm256_packs_epi32(a, b));
  }
}
__attribute__((noinline)) long sum_ge_intr(const int *v, int n, int t) {
  __m256i s64 = _mm256_setzero_si256(), tt = _mm256_set1_epi32(t - 1);
  for (int b = 0; b < n; b += 1024) {                 // khối 1024: 128 phần tử mỗi làn × 2^20 < 2^31
    __m256i s0 = _mm256_setzero_si256(), s1 = _mm256_setzero_si256();
    for (int i = b; i < b + 1024; i += 16) {
      __m256i x0 = _mm256_load_si256((const __m256i *)(v + i)), x1 = _mm256_load_si256((const __m256i *)(v + i + 8));
      s0 = _mm256_add_epi32(s0, _mm256_and_si256(x0, _mm256_cmpgt_epi32(x0, tt)));   // x > t-1  ⇔  x ≥ t
      s1 = _mm256_add_epi32(s1, _mm256_and_si256(x1, _mm256_cmpgt_epi32(x1, tt)));
    }
    __m256i s = _mm256_add_epi32(s0, s1);             // mở rộng lên 64 bit MỘT lần mỗi khối
    s64 = _mm256_add_epi64(s64, _mm256_cvtepi32_epi64(_mm256_castsi256_si128(s)));
    s64 = _mm256_add_epi64(s64, _mm256_cvtepi32_epi64(_mm256_extracti128_si256(s, 1)));
  }
  __m128i h = _mm_add_epi64(_mm256_castsi256_si128(s64), _mm256_extracti128_si256(s64, 1));
  return _mm_cvtsi128_si64(h) + _mm_extract_epi64(h, 1);
}
#endif

static int T_; static long sink;
static void b_sat_o2(void *p)  { for (int r = 0; r < 2000; r++) { sat_c_o2(OUT, IN, N); asm volatile("" ::: "memory"); } }
static void b_sat_v(void *p)   { for (int r = 0; r < 2000; r++) { sat_c_v(OUT, IN, N); asm volatile("" ::: "memory"); } }
static void b_sat_i(void *p)   { for (int r = 0; r < 2000; r++) { sat_intr(OUT, IN, N); asm volatile("" ::: "memory"); } }
static void b_sum_sse(void *p) { for (int r = 0; r < 2000; r++) sink += sum_ge_c_sse(V, N, T_); }
static void b_sum_v(void *p)   { for (int r = 0; r < 2000; r++) sink += sum_ge_c_v(V, N, T_); }
static void b_sum_i(void *p)   { for (int r = 0; r < 2000; r++) sink += sum_ge_intr(V, N, T_); }

int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  uint32_t s = 7;
  for (int i = 0; i < N; i++) { s = s * 1664525u + 1013904223u; IN[i] = (int32_t)s >> 13; V[i] = (int)(s >> 12); }  // IN ∈ ±2^18, V ∈ [0, 2^20)
  int bad = 0, badpos = 0;
  sat_c_o2(REF, IN, N); sat_intr(OUT, IN, N); for (int i = 0; i < N; i++) bad += OUT[i] != REF[i];
  sat_intr_bad(OUT, IN, N); for (int i = 0; i < N; i++) badpos += OUT[i] != REF[i];
  long r0 = sum_ge_c_sse(V, N, 1 << 19), r1 = sum_ge_c_v(V, N, 1 << 19), r2 = sum_ge_intr(V, N, 1 << 19);
  printf("kiểm: sat16 intrinsics sai %d / %d; bản quên hoán vị sai %d / %d · sum_ge: %ld %ld %ld\n", bad, N, badpos, N, r0, r1, r2);
  double u = 2000.0 * N;
  printf("sat16    C -O2 %.3f | C vector %.3f | intrinsics %.3f  chu kỳ/phần tử\n",
         bench_cycles(b_sat_o2, 0, u), bench_cycles(b_sat_v, 0, u), bench_cycles(b_sat_i, 0, u));
  for (int k = 0; k < 3; k++) {
    T_ = (int[]){0, 1 << 19, 1 << 20}[k];
    printf("sum_ge p=%.1f  C 128 bit %.3f | C vector rộng %.3f | intrinsics %.3f  chu kỳ/phần tử\n", 1.0 - k * 0.5,
           bench_cycles(b_sum_sse, 0, u), bench_cycles(b_sum_v, 0, u), bench_cycles(b_sum_i, 0, u));
  }
  printf("(xung %.2f GHz)\n", bench_ghz);
  return 0;
}
