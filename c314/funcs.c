// funcs.c — ba hàm cùng thân, số vòng tỉ lệ 1 : 2 : 4 → thời gian đúng 1/7, 2/7, 4/7. perf record (lấy mẫu) có ra đúng tỉ lệ đó không?
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define BODY(k) { uint64_t x = s, y = 3; long n = (k); \
  asm volatile("1:\n\t.rept 8\n\tadd %1,%1,%2\n\t.endr\n\tsubs %0,%0,#1\n\tb.ne 1b" : "+r"(n), "+r"(x) : "r"(y) : "cc"); return x; }
__attribute__((noinline)) uint64_t f1(uint64_t s) BODY(1000)
__attribute__((noinline)) uint64_t f2(uint64_t s) BODY(2000)
__attribute__((noinline)) uint64_t f4(uint64_t s) BODY(4000)
int main(int c, char **v) { long reps = atol(v[1]); uint64_t s = 1; for (long i = 0; i < reps; i++) { s = f1(s); s = f2(s); s = f4(s); } printf("%llu\n", (unsigned long long)(s & 7)); return 0; }
