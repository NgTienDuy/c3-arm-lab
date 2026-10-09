// ipc.c N — tích vô hướng 4096 phần tử float lặp N lần. Dịch -O0 và -O2: bản nào IPC cao hơn, bản nào xong trước?
#include <stdio.h>
#include <stdlib.h>
float x[4096], y[4096];
int main(int c, char **v) { long n = atol(v[1]); float s = 0; for (int i = 0; i < 4096; i++) { x[i] = i * 1e-3f; y[i] = 1.0f / (i + 1); }
  for (long r = 0; r < n; r++) for (int i = 0; i < 4096; i++) s += x[i] * y[i];
  printf("%f\n", s); return 0; }
