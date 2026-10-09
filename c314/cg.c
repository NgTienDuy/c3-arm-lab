// cg.c — lần ngược ngăn xếp cho profiler: leaf() nóng, được a() gọi 1 phần và b() gọi 3 phần → đúng ra 25% / 75% thời gian của leaf
// thuộc về a / b. Dịch hai cách (-fno-omit-frame-pointer / -fomit-frame-pointer) và ghi bằng hai cách (--call-graph fp / dwarf).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
__attribute__((noinline)) uint64_t leaf(uint64_t s, long n) { for (long i = 0; i < n; i++) { s = s * 6364136223846793005ull + 1442695040888963407ull; asm volatile("" : "+r"(s)); } return s; }
__attribute__((noinline)) uint64_t a(uint64_t s) { volatile char pad[64]; pad[0] = (char)s; return leaf(s + pad[0], 100000); }
__attribute__((noinline)) uint64_t b(uint64_t s) { volatile char pad[64]; pad[0] = (char)s; return leaf(s + pad[0], 300000); }
int main(int c, char **v) { long r = atol(v[1]); uint64_t s = 1; for (long i = 0; i < r; i++) { s = a(s); s = b(s); } printf("%llu\n", (unsigned long long)(s & 7)); return 0; }
