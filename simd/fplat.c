// fplat.c — độ trễ và thông lượng của phép cộng/FMA vector: chuỗi phụ thuộc (độ trễ) và 8 chuỗi độc lập (thông lượng)
#include "bench.h"
static void lat_add(void *p) {
#if defined(__aarch64__)
  asm volatile("movi v0.4s, #0\n\tmovi v1.4s, #0\n\tmov x9, #100000\n1:\n\t.rept 100\n\tfadd v0.4s, v0.4s, v1.4s\n\t.endr\n\tsubs x9, x9, #1\n\tb.ne 1b" ::: "v0", "v1", "x9", "cc");
#else
  asm volatile("vxorps %%ymm0,%%ymm0,%%ymm0\n\tvxorps %%ymm1,%%ymm1,%%ymm1\n\tmov $100000, %%ecx\n1:\n\t.rept 100\n\tvaddps %%ymm1,%%ymm0,%%ymm0\n\t.endr\n\tdec %%ecx\n\tjnz 1b" ::: "xmm0", "xmm1", "ecx", "cc");
#endif
}
static void lat_fma(void *p) {
#if defined(__aarch64__)
  asm volatile("movi v0.4s, #0\n\tmovi v1.4s, #0\n\tmov x9, #100000\n1:\n\t.rept 100\n\tfmla v0.4s, v1.4s, v1.4s\n\t.endr\n\tsubs x9, x9, #1\n\tb.ne 1b" ::: "v0", "v1", "x9", "cc");
#else
  asm volatile("vxorps %%ymm0,%%ymm0,%%ymm0\n\tvxorps %%ymm1,%%ymm1,%%ymm1\n\tmov $100000, %%ecx\n1:\n\t.rept 100\n\tvfmadd231ps %%ymm1,%%ymm1,%%ymm0\n\t.endr\n\tdec %%ecx\n\tjnz 1b" ::: "xmm0", "xmm1", "ecx", "cc");
#endif
}
static void thr_add(void *p) {
#if defined(__aarch64__)
  asm volatile("mov x9, #100000\n1:\n\t.rept 25\n\tfadd v0.4s, v0.4s, v8.4s\n\tfadd v1.4s, v1.4s, v8.4s\n\tfadd v2.4s, v2.4s, v8.4s\n\tfadd v3.4s, v3.4s, v8.4s\n\t.endr\n\tsubs x9, x9, #1\n\tb.ne 1b" ::: "v0", "v1", "v2", "v3", "v8", "x9", "cc");
#else
  asm volatile("mov $100000, %%ecx\n1:\n\t.rept 25\n\tvaddps %%ymm8,%%ymm0,%%ymm0\n\tvaddps %%ymm8,%%ymm1,%%ymm1\n\tvaddps %%ymm8,%%ymm2,%%ymm2\n\tvaddps %%ymm8,%%ymm3,%%ymm3\n\t.endr\n\tdec %%ecx\n\tjnz 1b" ::: "xmm0", "xmm1", "xmm2", "xmm3", "xmm8", "ecx", "cc");
#endif
}
int main(int argc, char **argv) {
  bench_pin(argc > 1 ? atoi(argv[1]) : 0);
  double a = bench_cycles(lat_add, 0, 1e7), f = bench_cycles(lat_fma, 0, 1e7), t = bench_cycles(thr_add, 0, 1e7);
  printf("độ trễ cộng vector %.2f chu kỳ · độ trễ FMA %.2f · thông lượng cộng (4 chuỗi) %.2f chu kỳ/lệnh  (xung %.2f GHz)\n", a, f, t, bench_ghz);
  return 0;
}
