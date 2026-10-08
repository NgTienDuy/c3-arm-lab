// libm_probe.c OUT — tính sinf, cosf, expf, logf, powf, tanhf trên 10^6 đầu vào tất định (xorshift), ghi bit kết quả ra OUT.
// Cùng mã nguồn dịch cho Linux (glibc), Windows (mingw-w64), AArch64 (glibc); bản CUDA ở libm_probe.cu dùng đúng dãy đầu vào.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
static double now(void) { LARGE_INTEGER c, f; QueryPerformanceCounter(&c); QueryPerformanceFrequency(&f); return c.QuadPart * 1e9 / f.QuadPart; }
#else
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e9 + t.tv_nsec; }
#endif
#define N 1000000
static uint64_t s = 0x9E3779B97F4A7C15ull;
static double u01(void) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (s >> 11) * 0x1.0p-53; }
int main(int argc, char **argv) {
  static float in[6][N], in2[N], out[6][N];
  for (int i = 0; i < N; i++) {
    in[0][i] = (float)(-100 + 200 * u01());            // sinf
    in[1][i] = (float)(-100 + 200 * u01());            // cosf
    in[2][i] = (float)(-80 + 160 * u01());             // expf
    in[3][i] = (float)exp(-20 + 34 * u01());           // logf: 2e-9 … 6e5
    in[4][i] = (float)(0.1 + 9.9 * u01()); in2[i] = (float)(-20 + 40 * u01());   // powf
    in[5][i] = (float)(-10 + 20 * u01());              // tanhf
  }
  const char *nm[6] = {"sinf", "cosf", "expf", "logf", "powf", "tanhf"};
  printf("ns mỗi lời gọi (tốt nhất của 5 lần × 10^6):");
  for (int k = 0; k < 6; k++) {
    double best = 1e30;
    for (int r = 0; r < 5; r++) {
      double t0 = now();
      switch (k) {
        case 0: for (int i = 0; i < N; i++) out[0][i] = sinf(in[0][i]); break;
        case 1: for (int i = 0; i < N; i++) out[1][i] = cosf(in[1][i]); break;
        case 2: for (int i = 0; i < N; i++) out[2][i] = expf(in[2][i]); break;
        case 3: for (int i = 0; i < N; i++) out[3][i] = logf(in[3][i]); break;
        case 4: for (int i = 0; i < N; i++) out[4][i] = powf(in[4][i], in2[i]); break;
        case 5: for (int i = 0; i < N; i++) out[5][i] = tanhf(in[5][i]); break;
      }
      double t = (now() - t0) / N; if (t < best) best = t;
    }
    printf(" %s %.1f", nm[k], best);
  }
  printf("\n");
  FILE *f = fopen(argv[1], "wb");
  fwrite(in, sizeof in, 1, f); fwrite(in2, sizeof in2, 1, f); fwrite(out, sizeof out, 1, f); fclose(f);
  return 0;
}
