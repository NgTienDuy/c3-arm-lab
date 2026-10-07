// smc.c — AArch64: mã tự sửa có cần dọn cache lệnh không? (trả tham chiếu C3.3 §5.5)
// Đọc CTR_EL0 (bit IDC 28: không cần dọn D-cache tới điểm hợp nhất; bit DIC 29: không cần vô hiệu I-cache),
// rồi N lần: ghi hai lệnh "movz w0,#k ; ret" vào một trang RWX, gọi nó, xem trả về k mới hay k cũ.
// Chế độ 0: không làm gì sau khi ghi · 1: __builtin___clear_cache · 2: chỉ isb · 3: chỉ dsb ish + isb
// build: gcc -O2 smc.c -o smc ; chạy: ./smc N
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <setjmp.h>
#include <signal.h>

static sigjmp_buf jb;
static void on_ill(int sig) { (void)sig; siglongjmp(jb, 1); }

static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }

int main(int argc, char **argv) {
  long n = argc > 1 ? atol(argv[1]) : 1000000;
  uint64_t ctr; asm volatile("mrs %0, ctr_el0" : "=r"(ctr));
  printf("CTR_EL0 = 0x%llx: DIC=%d IDC=%d IminLine=%d B DminLine=%d B\n", (unsigned long long)ctr,
         (int)(ctr >> 29 & 1), (int)(ctr >> 28 & 1), 4 << (ctr & 15), 4 << (ctr >> 16 & 15));
  fflush(stdout);
  signal(SIGILL, on_ill);
  uint32_t *code = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) { perror("mmap"); return 1; }
  int (*fn)(void) = (int (*)(void))code;
  // khởi tạo trang bằng mã hợp lệ "movz w0,#0xffff ; ret", dọn cache đúng quy trình một lần
  for (int i = 0; i < 1024; i += 2) { code[i] = 0x52800000u | (0xffffu << 5); code[i + 1] = 0xd65f03c0u; }
  __builtin___clear_cache((char *)code, (char *)(code + 1024));
  const char *mname[] = {"không làm gì", "__builtin___clear_cache", "chỉ isb", "dsb ish + isb"};
  for (int mode = 0; mode < 4; mode++) {
    volatile long stale = 0, ill = 0; double t0 = now();
    for (volatile long k = 0; k < n; k++) {
      if (sigsetjmp(jb, 1)) { ill++; continue; }
      int imm = (int)(k & 0xffff);
      code[0] = 0x52800000u | ((uint32_t)imm << 5);   // movz w0, #imm
      code[1] = 0xd65f03c0u;                         // ret
      if (mode == 1) __builtin___clear_cache((char *)code, (char *)(code + 2));
      else if (mode == 2) asm volatile("isb" ::: "memory");
      else if (mode == 3) asm volatile("dsb ish\n\tisb" ::: "memory");
      if (fn() != imm) stale++;
    }
    double dt = now() - t0;
    printf("  %-26s lần chạy mã cũ: %ld, lệnh không hợp lệ: %ld / %ld   %.1f ns mỗi vòng\n", mname[mode], stale, ill, n, dt / n * 1e9);
    fflush(stdout);
  }
  return 0;
}
