// coh.c — giá của một lần nạp / một lần RMW theo TRẠNG THÁI nhất quán của dòng trước đó.
// Luồng đo B đuổi con trỏ qua K dòng (thứ tự ngẫu nhiên, mỗi lần nạp phụ thuộc lần trước),
// sau khi luồng A (và C) đã đưa các dòng vào một trạng thái định trước: M ở A, E ở A, S ở A+C,
// chỉ ở DRAM (đã xóa khỏi mọi cache), hoặc trong cache của chính B.
// build: gcc -O2 -pthread coh.c -o coh   |   mingw: $MINGW -O2 coh.c -o coh.exe
// chạy : ./coh lpB lpA lpC [K] [R]
#include "plat.h"
#include <stdatomic.h>
#include <string.h>

typedef struct line { struct line *_Atomic next; long val; char pad[48]; } __attribute__((aligned(64))) line;

static line *L;
static int K = 512, R = 101;
enum { C_NONE, C_READ, C_WRITE, C_QUIT };
typedef struct { _Atomic int cmd; _Atomic int ack; int lp; } __attribute__((aligned(64))) worker;
static worker WA, WC;

static void read_all(void) { long s = 0; for (int i = 0; i < K; i++) s += *(volatile long *)&L[i].val; (void)s; }
static void write_all(void) { for (int i = 0; i < K; i++) *(volatile long *)&L[i].val += 1; }
static void flush_all(void) { for (int i = 0; i < K; i++) flush_line(&L[i]); flush_fence(); }

static void *work(void *p) {
  worker *w = p; pin(w->lp);
  for (int seen = 0;;) {
    int c;
    while ((c = atomic_load(&w->cmd)) == C_NONE) ;
    if (c == C_QUIT) return NULL;
    if (c == C_READ) read_all(); else if (c == C_WRITE) write_all();
    atomic_store(&w->cmd, C_NONE);
    atomic_store(&w->ack, ++seen);
  }
}
static void order(worker *w, int c) { int a = atomic_load(&w->ack); atomic_store(&w->cmd, c); while (atomic_load(&w->ack) == a) ; }

static line *chase(line *p) {                  // K lần nạp phụ thuộc
  for (int i = 0; i < K; i++) p = atomic_load_explicit(&p->next, memory_order_relaxed);
  return p;
}
static line *chase_rmw(line *p) {              // K lần RMW phụ thuộc (fetch_add 0 trên con trỏ)
  for (int i = 0; i < K; i++) p = (line *)atomic_fetch_add_explicit((_Atomic uintptr_t *)&p->next, 0, memory_order_relaxed);
  return p;
}

typedef enum { S_LOCAL, S_MA, S_EA, S_SAC, S_DRAM, W_LOCAL, W_SAB, W_MA, W_EA } scen;
static const char *name[] = {
  "nạp: dòng đã ở cache của chính B",
  "nạp: dòng M ở lõi A (A vừa ghi)",
  "nạp: dòng E ở lõi A (A vừa đọc, chỉ A có)",
  "nạp: dòng S ở lõi A và C",
  "nạp: dòng chỉ ở DRAM (đã xóa khỏi mọi cache)",
  "RMW: dòng của chính B (B vừa ghi)",
  "RMW: dòng S ở A và B (B nâng cấp lên M)",
  "RMW: dòng M ở lõi A",
  "RMW: dòng E ở lõi A",
};

static volatile line *sink;
static double run(scen s) {
  double *v = malloc(R * sizeof *v);
  for (int r = 0; r < R; r++) {
    switch (s) {
    case S_LOCAL: case W_LOCAL: flush_all(); write_all(); break;          // B giữ dòng ở M
    case S_MA: case W_MA:       order(&WA, C_WRITE); break;
    case S_EA: case W_EA:       flush_all(); order(&WA, C_READ); break;
    case S_SAC:                 flush_all(); order(&WA, C_READ); order(&WC, C_READ); break;
    case W_SAB:                 flush_all(); order(&WA, C_READ); read_all(); break;
    case S_DRAM:                flush_all(); break;
    }
    double t0 = now_ns();
    line *p = (s >= W_LOCAL) ? chase_rmw(&L[0]) : chase(&L[0]);
    double t1 = now_ns();
    sink = p;
    v[r] = (t1 - t0) / K;
  }
  double m = median(v, R); free(v); return m;
}

int main(int argc, char **argv) {
  if (argc < 4) { fprintf(stderr, "dùng: %s lpB lpA lpC [K] [R]\n", argv[0]); return 1; }
  int lpB = atoi(argv[1]); WA.lp = atoi(argv[2]); WC.lp = atoi(argv[3]);
  if (argc > 4) K = atoi(argv[4]);
  if (argc > 5) R = atoi(argv[5]);
  pin(lpB);
  L = amalloc(64, K * sizeof(line)); memset(L, 0, K * sizeof(line));
  int *perm = malloc(K * sizeof *perm);                       // vòng ngẫu nhiên qua K dòng
  for (int i = 0; i < K; i++) perm[i] = i;
  srand(7); for (int i = K - 1; i > 0; i--) { int j = rand() % (i + 1), t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
  for (int i = 0; i < K; i++) atomic_store(&L[perm[i]].next, &L[perm[(i + 1) % K]]);
  thr_t ta = thr_start(work, &WA), tc = thr_start(work, &WC);
  printf("%s  B=LP%d  A=LP%d  C=LP%d  K=%d dòng  R=%d lần (trung vị)\n", ARCH, lpB, WA.lp, WC.lp, K, R);
  for (int s = S_LOCAL; s <= W_EA; s++) printf("  %-48s %7.1f ns\n", name[s], run(s));
  atomic_store(&WA.cmd, C_QUIT); atomic_store(&WC.cmd, C_QUIT);
  thr_join(ta); thr_join(tc);
  return 0;
}
