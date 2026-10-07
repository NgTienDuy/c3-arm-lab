// sve.c — SVE trên lõi thật: độ dài vector (đọc lúc chạy, không biết lúc biên dịch) và vòng lặp có vị từ (predicate)
#include <arm_sve.h>
#include <stdio.h>
#define N 1000
static float x[N], y[N];
__attribute__((noinline)) void saxpy_sve(float *y, const float *x, float a, int n) {
  for (int i = 0; i < n; i += svcntw()) {               // svcntw(): số float trong một thanh ghi SVE
    svbool_t pg = svwhilelt_b32(i, n);                   // vị từ: làn j bật nếu i + j < n — phần đuôi không cần vòng riêng
    svfloat32_t vx = svld1(pg, x + i), vy = svld1(pg, y + i);
    svst1(pg, y + i, svmla_x(pg, vy, vx, a));            // vy + vx * a
  }
}
int main(void) {
  for (int i = 0; i < N; i++) { x[i] = i; y[i] = 1; }
  saxpy_sve(y, x, 2.0f, N);
  double err = 0; for (int i = 0; i < N; i++) err += y[i] - (1 + 2.0f * i);
  printf("SVE: VL = %lu bit (%lu float mỗi thanh ghi); saxpy %d phần tử, tổng sai %.0f\n",
         svcntb() * 8, svcntw(), N, err);
  return 0;
}
