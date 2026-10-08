// shift.c — dịch bit: số lượng dịch >= độ rộng (UB trong C), dịch phải số âm, chia vs dịch.
#include <stdint.h>
#include <stdio.h>
__attribute__((noinline)) uint32_t shl(uint32_t x, int n) { return x << n; }
__attribute__((noinline)) int32_t sar(int32_t x, int n) { return x >> n; }
__attribute__((noinline)) int32_t div2(int32_t x) { return x / 2; }
__attribute__((noinline)) int32_t shr1(int32_t x) { return x >> 1; }
int main(void) {
  volatile int n31 = 31, n32 = 32, n33 = 33, n40 = 40;
  printf("1u << 31 = %u | 1u << 32 = %u | 1u << 33 = %u | 1u << 40 = %u\n",
         shl(1, n31), shl(1, n32), shl(1, n33), shl(1, n40));
  printf("-7 >> 1 = %d | -7 / 2 = %d | -7 %% 2 = %d | -7 >> 2 = %d | -7 / 4 = %d\n",
         sar(-7, 1), div2(-7), -7 % 2, sar(-7, 2), -7 / 4);
  printf("-1 >> 31 = %d | INT32_MIN >> 31 = %d\n", sar(-1, n31), sar(INT32_MIN, n31));
  return 0;
}
