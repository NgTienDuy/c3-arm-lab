// reduce.c — phép giảm (reduction): (1) thông lượng theo số bộ cộng dồn vector độc lập; (2) độ chính xác
// của tổng bình phương (năng lượng / RMS) theo thứ tự cộng, trên tín hiệu rung thật (CWRU X130_DE_time).
// chạy: ./reduce cpu file.f32
#include "bench.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#if defined(__aarch64__)
#include <arm_neon.h>
#define LANES 4
typedef float32x4_t vec;
#define LD(p) vld1q_f32(p)
#define ADDV(a, b) vaddq_f32(a, b)
#define FMAV(a, b, c) vfmaq_f32(c, a, b)
#define ZERO() vdupq_n_f32(0)
#define BCAST(s) vdupq_n_f32(s)
static float hsum(vec v) { return vaddvq_f32(v); }
#else
#include <immintrin.h>
#define LANES 8
typedef __m256 vec;
#define LD(p) _mm256_loadu_ps(p)
#define ADDV(a, b) _mm256_add_ps(a, b)
#define FMAV(a, b, c) _mm256_fmadd_ps(a, b, c)
#define ZERO() _mm256_setzero_ps()
#define BCAST(s) _mm256_set1_ps(s)
static float hsum(vec v) {   // cộng ngang: 8 → 4 → 2 → 1 (thứ tự cố định)
  __m128 a = _mm_add_ps(_mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1));
  a = _mm_add_ps(a, _mm_movehl_ps(a, a));
  a = _mm_add_ss(a, _mm_shuffle_ps(a, a, 1));
  return _mm_cvtss_f32(a);
}
#endif

#define NL1 2048                       // phần (1): mảng trong L1
static float X1[NL1] __attribute__((aligned(64)));
static volatile float sink;

// tổng với K bộ cộng dồn vector độc lập (K = 1..16), mỗi vòng K lần nạp + K lần cộng.
// Giá trị đầu = (kết quả lần gọi trước) × 0: các lần gọi nối thành MỘT chuỗi phụ thuộc, nên lõi ngoài thứ tự
// không chồng được lần gọi sau lên lần gọi trước (§6.1 cho thấy chuyện gì xảy ra nếu không làm vậy).
#define DEF(K)                                                               \
  __attribute__((noinline)) float sumK##K(const float *x, int n, float init) { \
    vec acc[K];                    /* unroll hết: acc[] phải nằm trong thanh ghi */ \
    _Pragma("GCC unroll 16") for (int j = 0; j < K; j++) acc[j] = ZERO();    \
    acc[0] = BCAST(init);                                                    \
    for (int i = 0; i < n; i += K * LANES)                                   \
      _Pragma("GCC unroll 16") for (int j = 0; j < K; j++) acc[j] = ADDV(acc[j], LD(x + i + j * LANES)); \
    _Pragma("GCC unroll 16") for (int j = 1; j < K; j++) acc[0] = ADDV(acc[0], acc[j]); \
    return hsum(acc[0]);                                                     \
  }                                                                          \
  static void b##K(void *p) { float s = 0; for (int r = 0; r < 20000; r++) s = sumK##K(X1, NL1, s * 0.0f); sink = s; }
DEF(1) DEF(2) DEF(4) DEF(8) DEF(16)

// --- phần (2): năng lượng Σ x² theo nhiều thứ tự cộng ---
static double sq_double(const float *x, int n) { double s = 0; for (int i = 0; i < n; i++) s += (double)x[i] * x[i]; return s; }
__attribute__((noinline)) static float sq_seq(const float *x, int n) { float s = 0; for (int i = 0; i < n; i++) s = fmaf(x[i], x[i], s); return s; }
__attribute__((noinline)) static float sq_vec(const float *x, int n, int K) {   // K bộ cộng dồn vector + FMA
  vec acc[16];
  for (int j = 0; j < K; j++) acc[j] = ZERO();
  int i = 0;
  for (; i + K * LANES <= n; i += K * LANES)
    for (int j = 0; j < K; j++) { vec v = LD(x + i + j * LANES); acc[j] = FMAV(v, v, acc[j]); }
  for (int j = 1; j < K; j++) acc[0] = ADDV(acc[0], acc[j]);
  float s = hsum(acc[0]);
  for (; i < n; i++) s = fmaf(x[i], x[i], s);
  return s;
}
static float sq_pair(const float *x, int n) {            // cộng theo cặp (đệ quy), lá ≤ 8
  if (n <= 8) { float s = 0; for (int i = 0; i < n; i++) s = fmaf(x[i], x[i], s); return s; }
  return sq_pair(x, n / 2) + sq_pair(x + n / 2, n - n / 2);
}
static float sq_kahan(const float *x, int n) {           // cộng bù Kahan, float
  float s = 0, c = 0;
  for (int i = 0; i < n; i++) { float y = x[i] * x[i] - c; float t = s + y; c = (t - s) - y; s = t; }
  return s;
}

int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  for (int i = 0; i < NL1; i++) X1[i] = 1.0f + i * 1e-6f;
  printf("(1) tổng mảng float %d phần tử (L1), %d làn mỗi vector — chu kỳ mỗi VECTOR nạp-và-cộng:\n", NL1, LANES);
  void (*B[])(void *) = {b1, b2, b4, b8, b16}; int Ks[] = {1, 2, 4, 8, 16};
  for (int k = 0; k < 5; k++)
    printf("  K = %2d bộ cộng dồn: %.3f\n", Ks[k], bench_cycles(B[k], 0, 20000.0 * NL1 / LANES));
  printf("  (xung %.2f GHz)\n", bench_ghz);
  if (argc < 3) return 0;
  FILE *f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; }
  fseek(f, 0, SEEK_END); long n = ftell(f) / 4; fseek(f, 0, SEEK_SET);
  float *x = malloc(n * 4); if (fread(x, 4, n, f) != (size_t)n) return 1; fclose(f);
  double ref = sq_double(x, n);   // double: sai số tương đối ~1e-16 · n, đủ làm tham chiếu cho float
  printf("(2) Σ x² trên %s (%ld mẫu); tham chiếu (double) = %.9g\n", argv[2], n, ref);
  struct { const char *name; float v; } R[] = {
    {"tuần tự, float", sq_seq(x, n)},
    {"vector, 1 bộ cộng dồn", sq_vec(x, n, 1)},
    {"vector, 4 bộ cộng dồn", sq_vec(x, n, 4)},
    {"vector, 16 bộ cộng dồn", sq_vec(x, n, 16)},
    {"theo cặp (đệ quy)", sq_pair(x, n)},
    {"Kahan, float", sq_kahan(x, n)}};
  for (int k = 0; k < 6; k++)
    printf("  %-26s %.9g  sai số tương đối %+.2e\n", R[k].name, R[k].v, (R[k].v - ref) / ref);
  return 0;
}
