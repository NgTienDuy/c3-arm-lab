// skid.c — vòng có MỘT lệnh đắt và 16 lệnh rẻ đã biết địa chỉ: mẫu lấy theo chu kỳ rơi vào lệnh nào?
//   skid load N  : ldr x1,[x1] (đuổi con trỏ trong 256 MiB → trượt DRAM) rồi 16 add độc lập, subs, b.ne
//   skid fdiv N  : 1 fdiv nối đuôi (độ trễ dài), rồi 16 add độc lập, subs, b.ne
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t *mkring(size_t bytes) { size_t n = bytes / 64; uint64_t *a = aligned_alloc(64, n * 64); size_t *p = malloc(n * sizeof *p);
  for (size_t i = 0; i < n; i++) p[i] = i; srand(1); for (size_t i = n - 1; i > 0; i--) { size_t j = ((size_t)rand() * 2654435761u + rand()) % (i + 1); size_t t = p[i]; p[i] = p[j]; p[j] = t; }
  for (size_t i = 0; i < n; i++) a[p[i] * 8] = (uint64_t)&a[p[(i + 1) % n] * 8]; free(p); return a; }
__attribute__((noinline)) uint64_t loopload(long n, uint64_t x1) { uint64_t z = 0;
  asm volatile(".p2align 6\n1:\n\tldr %1,[%1]\n\t.rept 16\n\tadd %2,%2,#1\n\t.endr\n\tsubs %0,%0,#1\n\tb.ne 1b" : "+r"(n), "+r"(x1), "+r"(z) :: "cc", "memory"); return x1 + z; }
__attribute__((noinline)) uint64_t loopdiv(long n) { double x = 1e300, y = 1.0000001; uint64_t z = 0;
  asm volatile(".p2align 6\n1:\n\tfdiv %d1,%d1,%d3\n\t.rept 16\n\tadd %2,%2,#1\n\t.endr\n\tsubs %0,%0,#1\n\tb.ne 1b" : "+r"(n), "+w"(x), "+r"(z) : "w"(y) : "cc"); return z + (x != 0); }
int main(int c, char **v) { long n = atol(v[2]); uint64_t r;
  if (!strcmp(v[1], "load")) { uint64_t *a = mkring(256ul << 20); r = loopload(n, (uint64_t)a); } else r = loopdiv(n);
  printf("%llu\n", (unsigned long long)(r & 7)); return 0; }
