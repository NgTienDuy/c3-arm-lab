// kern.c — vi nhân có số lệnh biết trước (AArch64, asm nội tuyến) để KIỂM bộ đếm PMU và làm mẫu cho các mẫu hình IPC/trượt/đoán sai.
//   kern MODE N [ARG]      (N = số vòng; số lệnh mỗi vòng ghi cạnh từng chế độ)
//   chain   : 8 add phụ thuộc nối đuôi + subs + b.ne                         = 10 lệnh/vòng (giới hạn bởi độ trễ: ~8 chu kỳ)
//   indep   : 8 add độc lập (8 thanh ghi) + subs + b.ne                       = 10 lệnh/vòng (giới hạn bởi số cổng ALU)
//   fdiv    : 4 fdiv d phụ thuộc + subs + b.ne                                =  6 lệnh/vòng (lõi bận, không đụng bộ nhớ)
//   chase   : 8 ldr x1,[x1] nối đuôi + subs + b.ne trên hoán vị vòng ngẫu nhiên ARG byte (mỗi nút một dòng 64 B) = 10 lệnh/vòng
//   stream  : cộng dồn tuần tự ARG byte (C, -O2) — băng thông, bộ tiền nạp che độ trễ
//   branch  : nhánh theo byte ngẫu nhiên, xác suất rẽ ARG/256 (asm: ldrb, cmp, b.lo, add, subs, b.ne)
//   spin    : vòng chờ một cờ không bao giờ bật (ldr, cbnz, subs, b.ne = 4 lệnh/vòng) — "IPC cao mà không làm gì"
//   sys     : N lần getppid() — lệnh chạy trong nhân
//   phase   : N vòng chain rồi N/8 vòng chase 256 MiB — hai pha để thấy lỗi ngoại suy khi ghép kênh bộ đếm
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
static uint64_t *mkring(size_t bytes, unsigned seed) {        // hoán vị vòng ngẫu nhiên, mỗi nút 64 B
  size_t n = bytes / 64; uint64_t *a = aligned_alloc(64, n * 64); size_t *p = malloc(n * sizeof *p);
  for (size_t i = 0; i < n; i++) p[i] = i;
  srand(seed); for (size_t i = n - 1; i > 0; i--) { size_t j = ((size_t)rand() * 2654435761u + rand()) % (i + 1); size_t t = p[i]; p[i] = p[j]; p[j] = t; }
  for (size_t i = 0; i < n; i++) a[p[i] * 8] = (uint64_t)&a[p[(i + 1) % n] * 8];
  free(p); return a;
}
int main(int argc, char **argv) {
  if (argc < 3) { fprintf(stderr, "kern MODE N [ARG]\n"); return 1; }
  const char *m = argv[1]; long n = atol(argv[2]); long arg = argc > 3 ? atol(argv[3]) : 0; uint64_t r = 0;
#ifdef __aarch64__
  if (!strcmp(m, "chain")) { uint64_t x1 = 1, x2 = 3;
    asm volatile("1:\n\t.rept 8\n\tadd %1,%1,%2\n\t.endr\n\tsubs %0,%0,#1\n\tb.ne 1b" : "+r"(n), "+r"(x1) : "r"(x2) : "cc"); r = x1; }
  else if (!strcmp(m, "indep")) { uint64_t a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8, k = 3;
    asm volatile("1:\n\tadd %1,%1,%9\n\tadd %2,%2,%9\n\tadd %3,%3,%9\n\tadd %4,%4,%9\n\tadd %5,%5,%9\n\tadd %6,%6,%9\n\tadd %7,%7,%9\n\tadd %8,%8,%9\n\t"
                 "subs %0,%0,#1\n\tb.ne 1b" : "+r"(n), "+r"(a), "+r"(b), "+r"(c), "+r"(d), "+r"(e), "+r"(f), "+r"(g), "+r"(h) : "r"(k) : "cc");
    r = a + b + c + d + e + f + g + h; }
  else if (!strcmp(m, "fdiv")) { double x = 1e300, y = 1.0000001;
    asm volatile("1:\n\t.rept 4\n\tfdiv %d1,%d1,%d2\n\t.endr\n\tsubs %0,%0,#1\n\tb.ne 1b" : "+r"(n), "+w"(x) : "w"(y) : "cc"); r = (uint64_t)(x != 0); }
  else if (!strcmp(m, "chase") || !strcmp(m, "phase")) {
    size_t bytes = !strcmp(m, "phase") ? (256ul << 20) : (size_t)arg; uint64_t *a = mkring(bytes, 1); uint64_t x1 = (uint64_t)a;
    for (size_t i = 0; i < bytes / 64 * 2; i++) x1 = *(uint64_t *)x1;              // làm nóng: đi hai vòng
    if (!strcmp(m, "phase")) { uint64_t y = 1, z = 3; long k = n;
      asm volatile("1:\n\t.rept 8\n\tadd %1,%1,%2\n\t.endr\n\tsubs %0,%0,#1\n\tb.ne 1b" : "+r"(k), "+r"(y) : "r"(z) : "cc"); n /= 8; r = y; }
    asm volatile("1:\n\t.rept 8\n\tldr %1,[%1]\n\t.endr\n\tsubs %0,%0,#1\n\tb.ne 1b" : "+r"(n), "+r"(x1) :: "cc", "memory"); r += x1; }
  else if (!strcmp(m, "branch")) { size_t len = 1 << 16; uint8_t *b = malloc(len); srand(7); for (size_t i = 0; i < len; i++) b[i] = rand() & 255;
    uint64_t cnt = 0, thr = arg; long left = n;
    while (left > 0) { long k = left < (long)len ? left : (long)len; const uint8_t *p = b; long kk = k;
      asm volatile("1:\n\tldrb w9,[%1],#1\n\tcmp x9,%3\n\tb.hs 2f\n\tadd %2,%2,#1\n2:\n\tsubs %0,%0,#1\n\tb.ne 1b"
                   : "+r"(kk), "+r"(p), "+r"(cnt) : "r"(thr) : "x9", "cc", "memory"); left -= k; }
    r = cnt; }
  else if (!strcmp(m, "spin")) { volatile uint32_t flag = 0; const volatile uint32_t *f = &flag;
    asm volatile("1:\n\tldr w9,[%1]\n\tcbnz w9,2f\n\tsubs %0,%0,#1\n\tb.ne 1b\n2:" : "+r"(n) : "r"(f) : "x9", "cc", "memory"); r = flag; }
  else
#endif
  if (!strcmp(m, "stream")) { size_t w = (size_t)arg / 8; uint64_t *a = aligned_alloc(64, w * 8); for (size_t i = 0; i < w; i++) a[i] = i * 3;
    uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for (long k = 0; k < n; k++) for (size_t i = 0; i < w; i += 4) { s0 += a[i]; s1 += a[i + 1]; s2 += a[i + 2]; s3 += a[i + 3]; }
    r = s0 + s1 + s2 + s3; }
  else if (!strcmp(m, "sys")) { for (long i = 0; i < n; i++) r += syscall(SYS_getppid); }
  else { fprintf(stderr, "chế độ lạ hoặc không có trên kiến trúc này: %s\n", m); return 1; }
  printf("%s %ld %ld -> %llu\n", m, atol(argv[2]), arg, (unsigned long long)(r & 0xff));
  return 0;
}
