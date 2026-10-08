// omp_sum.c CWRU_DIR — Σx² (năng lượng rung) trên toàn bộ 16 bản ghi CWRU nối nhau, float32, bằng nhiều cách song song.
// Câu hỏi: kết quả có giống nhau từng bit khi đổi số luồng / chạy lại không, và làm thế nào để có.
//   (a) tuần tự  (b) OpenMP reduction, lịch tĩnh, T luồng  (c) lịch động — chạy lại 20 lần
//   (d) chia khối CỐ ĐỊNH (256 khối), cộng các tổng khối theo thứ tự khối — không phụ thuộc T
//   (e) bộ cộng dồn NGUYÊN: mỗi x² đổi sang điểm cố định 2^-90 trong __int128, cộng nguyên (kết hợp được) → không phụ thuộc thứ tự
#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static float *v; static long n;
static uint32_t bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float red_static(int T) { float s = 0;
#pragma omp parallel for num_threads(T) schedule(static) reduction(+ : s)
  for (long i = 0; i < n; i++) s += v[i];
  return s; }
static float red_dynamic(int T) { float s = 0;
#pragma omp parallel for num_threads(T) schedule(dynamic, 4096) reduction(+ : s)
  for (long i = 0; i < n; i++) s += v[i];
  return s; }
static float red_fixed(int T) { enum { K = 256 }; float part[K];
#pragma omp parallel for num_threads(T) schedule(dynamic, 1)
  for (int k = 0; k < K; k++) { long a = n * k / K, b = n * (k + 1) / K; float s = 0; for (long i = a; i < b; i++) s += v[i]; part[k] = s; }
  float s = 0; for (int k = 0; k < K; k++) s += part[k];
  return s; }
static double red_int(int T) { __int128 s = 0;
#pragma omp parallel num_threads(T)
  { __int128 my = 0;
#pragma omp for schedule(dynamic, 4096) nowait
    for (long i = 0; i < n; i++) my += (__int128)ldexp((double)v[i], 90);   // x² ≥ 0, < 2^37 ⇒ vừa 128 bit; phần dưới 2^-90 bị cắt (tất định)
#pragma omp critical
    s += my; }
  return ldexp((double)s, -90); }
int main(int argc, char **argv) {
  const int rec[] = {97, 98, 99, 100, 105, 106, 107, 108, 118, 119, 120, 121, 130, 131, 132, 133};
  v = malloc(sizeof(float) * 4000000); n = 0;
  for (int r = 0; r < 16; r++) {
    char p[512]; snprintf(p, sizeof p, "%s/X%03d_DE_time.f32", argv[1], rec[r]);
    FILE *f = fopen(p, "rb"); float x; while (fread(&x, 4, 1, f) == 1) { v[n++] = x * x; } fclose(f);
  }
  double ref = 0; long double refl = 0; for (long i = 0; i < n; i++) refl += v[i]; ref = (double)refl;
  printf("n = %ld mẫu; tham chiếu (long double) Σx² = %.6f\n", n, ref);
  float s = 0; for (long i = 0; i < n; i++) s += v[i];
  printf("  (a) tuần tự float                 %.6f  0x%08X  sai %+.2e\n", s, bits(s), (s - ref) / ref);
  for (int T = 1; T <= 10; T++) { float r = red_static(T);
    printf("  (b) OpenMP tĩnh, %2d luồng          %.6f  0x%08X  sai %+.2e\n", T, r, bits(r), (r - ref) / ref); }
  uint32_t seen[20]; int ns = 0;
  for (int k = 0; k < 20; k++) { uint32_t b = bits(red_dynamic(8)); int d = 1; for (int j = 0; j < ns; j++) d &= seen[j] != b; if (d) seen[ns++] = b; }
  printf("  (c) OpenMP động, 8 luồng, 20 lần chạy: %d kết quả khác nhau:", ns);
  for (int j = 0; j < ns && j < 8; j++) { float f; memcpy(&f, &seen[j], 4); printf(" %.4f", f); } printf("%s\n", ns > 8 ? " …" : "");
  for (int T = 1; T <= 10; T += 3) { float r = red_fixed(T);
    printf("  (d) 256 khối cố định, %2d luồng     %.6f  0x%08X  sai %+.2e\n", T, r, bits(r), (r - ref) / ref); }
  for (int T = 1; T <= 10; T += 3) { double r = red_int(T);
    printf("  (e) cộng nguyên 128 bit, %2d luồng  %.6f  (double)  sai %+.2e\n", T, r, (r - ref) / ref); }
  return 0;
}
