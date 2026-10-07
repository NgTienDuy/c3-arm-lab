// litmus.c — bộ chạy litmus test tối giản cho x86-64 và AArch64.
// Mỗi phép thử: các luồng ghim vào CPU riêng, mỗi lượt gặp nhau ở một hàng rào quay (barrier),
// rồi chạy thân phép thử trên một cặp ô nhớ MỚI (mỗi ô một dòng cache). Thân viết bằng asm
// nội tuyến để compiler không đổi thứ tự lệnh: thứ tự ta thấy trong mã = thứ tự lệnh máy.
//
// build: gcc -O2 -pthread litmus.c -o litmus           (x86-64 hoặc AArch64, chạy native)
// chạy : ./litmus TEST BIẾN-THỂ SỐ-LƯỢT cpu0 cpu1 [cpu2 cpu3]
//        ./litmus list
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct { int v; char pad[60]; } __attribute__((aligned(64))) cell;

#define NI 4096                     // số "bản" (instance) mỗi mẻ; mỗi bản có ô x, y riêng
static cell *X, *Y;
static int *OUT[4];                 // OUT[t][i*4 + k] = thanh ghi k của luồng t ở bản i

// ---------- các lệnh nhớ, viết bằng asm để giữ đúng thứ tự ----------
#if defined(__aarch64__)
#define ARCH "aarch64"
#define ST(p, v)   asm volatile("str %w1, [%0]" :: "r"(p), "r"(v) : "memory")
#define LD(p)      ({ int _r; asm volatile("ldr %w0, [%1]" : "=r"(_r) : "r"(p) : "memory"); _r; })
#define LDA(p)     ({ int _r; asm volatile("ldar %w0, [%1]" : "=r"(_r) : "r"(p) : "memory"); _r; })
#define LDAPR(p)   ({ int _r; asm volatile(".arch armv8.3-a\n\tldapr %w0, [%1]" : "=r"(_r) : "r"(p) : "memory"); _r; })
#define STL(p, v)  asm volatile("stlr %w1, [%0]" :: "r"(p), "r"(v) : "memory")
#define FULL()     asm volatile("dmb ish"   ::: "memory")
#define FLD()      asm volatile("dmb ishld" ::: "memory")
#define FST()      asm volatile("dmb ishst" ::: "memory")
#define ISB()      asm volatile("isb"       ::: "memory")
// phụ thuộc địa chỉ: địa chỉ lần nạp thứ hai = p + (r ^ r), luôn bằng p nhưng CPU phải chờ r
#define LD_ADDR(r, p) ({ int _t; asm volatile("eor %w0, %w1, %w1\n\tldr %w0, [%2, %w0, sxtw]" \
                          : "=&r"(_t) : "r"(r), "r"(p) : "memory"); _t; })
// phụ thuộc dữ liệu: giá trị ghi = (r ^ r) + v
#define ST_DATA(r, p, v) ({ int _t; asm volatile("eor %w0, %w1, %w1\n\tadd %w0, %w0, %w3\n\tstr %w0, [%2]" \
                          : "=&r"(_t) : "r"(r), "r"(p), "r"(v) : "memory"); })
// phụ thuộc điều khiển: nhánh có điều kiện theo r, hai đích trùng nhau
#define CTRL(r)    asm volatile("cbnz %w0, 1f\n1:" :: "r"(r) : "memory")
#define SWP(p, v)  asm volatile("swpal %w1, wzr, [%0]" :: "r"(p), "r"(v) : "memory")
#else
#define ARCH "x86-64"
#define ST(p, v)   asm volatile("movl %1, (%0)" :: "r"(p), "r"(v) : "memory")
#define LD(p)      ({ int _r; asm volatile("movl (%1), %0" : "=r"(_r) : "r"(p) : "memory"); _r; })
#define LDA(p)     LD(p)            // x86: nạp thường đã có ngữ nghĩa acquire
#define LDAPR(p)   LD(p)
#define STL(p, v)  ST(p, v)         // x86: ghi thường đã có ngữ nghĩa release
#define FULL()     asm volatile("mfence" ::: "memory")
#define FLD()      asm volatile("" ::: "memory")
#define FST()      asm volatile("" ::: "memory")
#define ISB()      asm volatile("" ::: "memory")
#define LD_ADDR(r, p) ({ int _t; asm volatile("movl %1, %0\n\txorl %1, %0\n\tmovslq %0, %q0\n\tmovl (%2,%q0), %0" \
                          : "=&r"(_t) : "r"(r), "r"(p) : "memory"); _t; })
#define ST_DATA(r, p, v) ({ int _t; asm volatile("movl %1, %0\n\txorl %1, %0\n\taddl %3, %0\n\tmovl %0, (%2)" \
                          : "=&r"(_t) : "r"(r), "r"(p), "r"(v) : "memory"); })
#define CTRL(r)    asm volatile("testl %0, %0\n\tjnz 1f\n1:" :: "r"(r) : "memory")
#define SWP(p, v)  ({ int _v = (v); asm volatile("xchgl %0, (%1)" : "+r"(_v) : "r"(p) : "memory"); })
#endif

#define x (&X[i].v)
#define y (&Y[i].v)
#define R(k) OUT[t][i * 4 + (k)]
typedef void (*body_fn)(int t, int i);

// ---------- các phép thử ----------
// MP (message passing): T0 ghi dữ liệu x rồi cờ y; T1 đọc cờ rồi dữ liệu. Yếu: thấy cờ, không thấy dữ liệu.
static void mp_w_po(int t, int i)    { ST(x, 1); ST(y, 1); }
static void mp_w_dmbst(int t, int i) { ST(x, 1); FST(); ST(y, 1); }
static void mp_w_dmb(int t, int i)   { ST(x, 1); FULL(); ST(y, 1); }
static void mp_w_rel(int t, int i)   { ST(x, 1); STL(y, 1); }
static void mp_r_po(int t, int i)    { int a = LD(y); int b = LD(x); R(0) = a; R(1) = b; }
static void mp_r_dmbld(int t, int i) { int a = LD(y); FLD(); int b = LD(x); R(0) = a; R(1) = b; }
static void mp_r_dmb(int t, int i)   { int a = LD(y); FULL(); int b = LD(x); R(0) = a; R(1) = b; }
static void mp_r_acq(int t, int i)   { int a = LDA(y); int b = LD(x); R(0) = a; R(1) = b; }
static void mp_r_acqpc(int t, int i) { int a = LDAPR(y); int b = LD(x); R(0) = a; R(1) = b; }
static void mp_r_addr(int t, int i)  { int a = LD(y); int b = LD_ADDR(a, x); R(0) = a; R(1) = b; }
static void mp_r_ctrl(int t, int i)  { int a = LD(y); CTRL(a); int b = LD(x); R(0) = a; R(1) = b; }
static void mp_r_ctrlisb(int t, int i) { int a = LD(y); CTRL(a); ISB(); int b = LD(x); R(0) = a; R(1) = b; }

// SB (store buffering): mỗi luồng ghi biến của mình rồi đọc biến của luồng kia. Yếu: cả hai đọc 0.
static void sb0_po(int t, int i)  { ST(x, 1); int a = LD(y); R(0) = a; }
static void sb1_po(int t, int i)  { ST(y, 1); int a = LD(x); R(0) = a; }
static void sb0_dmb(int t, int i) { ST(x, 1); FULL(); int a = LD(y); R(0) = a; }
static void sb1_dmb(int t, int i) { ST(y, 1); FULL(); int a = LD(x); R(0) = a; }
static void sb0_ra(int t, int i)  { STL(x, 1); int a = LDA(y); R(0) = a; }
static void sb1_ra(int t, int i)  { STL(y, 1); int a = LDA(x); R(0) = a; }
static void sb0_rapc(int t, int i) { STL(x, 1); int a = LDAPR(y); R(0) = a; }
static void sb1_rapc(int t, int i) { STL(y, 1); int a = LDAPR(x); R(0) = a; }
static void sb0_swp(int t, int i) { SWP(x, 1); int a = LD(y); R(0) = a; }
static void sb1_swp(int t, int i) { SWP(y, 1); int a = LD(x); R(0) = a; }

// LB (load buffering): mỗi luồng đọc biến của luồng kia rồi ghi biến của mình. Yếu: cả hai đọc 1.
static void lb0_po(int t, int i)   { int a = LD(x); ST(y, 1); R(0) = a; }
static void lb1_po(int t, int i)   { int a = LD(y); ST(x, 1); R(0) = a; }
static void lb0_data(int t, int i) { int a = LD(x); ST_DATA(a, y, 1); R(0) = a; }
static void lb1_data(int t, int i) { int a = LD(y); ST_DATA(a, x, 1); R(0) = a; }
static void lb0_ctrl(int t, int i) { int a = LD(x); CTRL(a); ST(y, 1); R(0) = a; }
static void lb1_ctrl(int t, int i) { int a = LD(y); CTRL(a); ST(x, 1); R(0) = a; }

// 2+2W: hai luồng ghi hai biến theo thứ tự ngược nhau. Yếu: cuối cùng x == 1 và y == 1.
static void w0_po(int t, int i)  { ST(x, 1); ST(y, 2); }
static void w1_po(int t, int i)  { ST(y, 1); ST(x, 2); }
static void w0_st(int t, int i)  { ST(x, 1); FST(); ST(y, 2); }
static void w1_st(int t, int i)  { ST(y, 1); FST(); ST(x, 2); }

// S: T0 ghi x=2 rồi y=1; T1 đọc y rồi ghi x=1. Yếu: T1 đọc y=1 nhưng cuối cùng x == 2.
static void s0_po(int t, int i)  { ST(x, 2); ST(y, 1); }
static void s1_po(int t, int i)  { int a = LD(y); ST(x, 1); R(0) = a; }
static void s0_st(int t, int i)  { ST(x, 2); FST(); ST(y, 1); }
static void s1_data(int t, int i) { int a = LD(y); ST_DATA(a, x, 1); R(0) = a; }

// R: T0 ghi x=1 rồi y=1; T1 ghi y=2 rồi đọc x. Yếu: cuối cùng y == 2 và T1 đọc x == 0.
static void r0_po(int t, int i)  { ST(x, 1); ST(y, 1); }
static void r1_po(int t, int i)  { ST(y, 2); int a = LD(x); R(0) = a; }
static void r0_dmb(int t, int i) { ST(x, 1); FULL(); ST(y, 1); }
static void r1_dmb(int t, int i) { ST(y, 2); FULL(); int a = LD(x); R(0) = a; }

// IRIW: hai người ghi độc lập, hai người đọc theo thứ tự ngược nhau. Yếu: hai người đọc bất đồng thứ tự.
static void iw0(int t, int i)  { ST(x, 1); }
static void iw1(int t, int i)  { ST(y, 1); }
static void ir2_po(int t, int i)  { int a = LD(x); int b = LD(y); R(0) = a; R(1) = b; }
static void ir3_po(int t, int i)  { int a = LD(y); int b = LD(x); R(0) = a; R(1) = b; }
static void ir2_ld(int t, int i)  { int a = LD(x); FLD(); int b = LD(y); R(0) = a; R(1) = b; }
static void ir3_ld(int t, int i)  { int a = LD(y); FLD(); int b = LD(x); R(0) = a; R(1) = b; }
static void ir2_addr(int t, int i) { int a = LD(x); int b = LD_ADDR(a, y); R(0) = a; R(1) = b; }
static void ir3_addr(int t, int i) { int a = LD(y); int b = LD_ADDR(a, x); R(0) = a; R(1) = b; }

// CoRR: một người ghi, một người đọc cùng ô hai lần. Yếu (mọi kiến trúc đều cấm): đọc 1 rồi 0.
static void co0(int t, int i)    { ST(x, 1); }
static void co1(int t, int i)    { int a = LD(x); int b = LD(x); R(0) = a; R(1) = b; }

// ---------- bảng phép thử ----------
enum kind { K_MP, K_SB, K_LB, K_2W, K_S, K_R, K_IRIW, K_CO };
typedef struct { const char *test, *variant; enum kind k; int nt; body_fn f[4]; const char *what; } litmus;

static const litmus T[] = {
  {"MP", "po",         K_MP, 2, {mp_w_po, mp_r_po}, "ghi x;ghi y || đọc y;đọc x"},
  {"MP", "dmbst+po",   K_MP, 2, {mp_w_dmbst, mp_r_po}, "chỉ rào phía ghi"},
  {"MP", "po+dmbld",   K_MP, 2, {mp_w_po, mp_r_dmbld}, "chỉ rào phía đọc"},
  {"MP", "dmbst+dmbld",K_MP, 2, {mp_w_dmbst, mp_r_dmbld}, "rào cả hai phía (tối thiểu)"},
  {"MP", "dmb+dmb",    K_MP, 2, {mp_w_dmb, mp_r_dmb}, "rào đầy đủ cả hai phía"},
  {"MP", "rel+acq",    K_MP, 2, {mp_w_rel, mp_r_acq}, "stlr cờ / ldar cờ"},
  {"MP", "rel+acqpc",  K_MP, 2, {mp_w_rel, mp_r_acqpc}, "stlr cờ / ldapr cờ"},
  {"MP", "rel+po",     K_MP, 2, {mp_w_rel, mp_r_po}, "release không có acquire đi kèm"},
  {"MP", "dmbst+addr", K_MP, 2, {mp_w_dmbst, mp_r_addr}, "phụ thuộc địa chỉ phía đọc"},
  {"MP", "dmbst+ctrl", K_MP, 2, {mp_w_dmbst, mp_r_ctrl}, "phụ thuộc điều khiển phía đọc (KHÔNG đủ)"},
  {"MP", "dmbst+ctrlisb", K_MP, 2, {mp_w_dmbst, mp_r_ctrlisb}, "điều khiển + isb phía đọc"},
  {"SB", "po",         K_SB, 2, {sb0_po, sb1_po}, "ghi mình;đọc người kia"},
  {"SB", "dmb",        K_SB, 2, {sb0_dmb, sb1_dmb}, "rào đầy đủ giữa ghi và đọc"},
  {"SB", "rel+acq",    K_SB, 2, {sb0_ra, sb1_ra}, "stlr rồi ldar (RCsc)"},
  {"SB", "rel+acqpc",  K_SB, 2, {sb0_rapc, sb1_rapc}, "stlr rồi ldapr (RCpc)"},
  {"SB", "swp",        K_SB, 2, {sb0_swp, sb1_swp}, "ghi bằng lệnh nguyên tử hoán đổi"},
  {"LB", "po",         K_LB, 2, {lb0_po, lb1_po}, "đọc người kia;ghi mình"},
  {"LB", "data",       K_LB, 2, {lb0_data, lb1_data}, "giá trị ghi phụ thuộc giá trị đọc"},
  {"LB", "ctrl",       K_LB, 2, {lb0_ctrl, lb1_ctrl}, "lệnh ghi nằm sau nhánh theo giá trị đọc"},
  {"2+2W", "po",       K_2W, 2, {w0_po, w1_po}, "ghi x;ghi y || ghi y;ghi x"},
  {"2+2W", "dmbst",    K_2W, 2, {w0_st, w1_st}, "dmb ishst giữa hai lần ghi"},
  {"S", "po",          K_S, 2, {s0_po, s1_po}, "ghi x;ghi y || đọc y;ghi x"},
  {"S", "dmbst+data",  K_S, 2, {s0_st, s1_data}, "rào phía ghi + phụ thuộc dữ liệu"},
  {"R", "po",          K_R, 2, {r0_po, r1_po}, "ghi x;ghi y || ghi y;đọc x"},
  {"R", "dmb",         K_R, 2, {r0_dmb, r1_dmb}, "rào đầy đủ cả hai"},
  {"IRIW", "po",       K_IRIW, 4, {iw0, iw1, ir2_po, ir3_po}, "hai người ghi, hai người đọc ngược thứ tự"},
  {"IRIW", "dmbld",    K_IRIW, 4, {iw0, iw1, ir2_ld, ir3_ld}, "dmb ishld ở người đọc"},
  {"IRIW", "addr",     K_IRIW, 4, {iw0, iw1, ir2_addr, ir3_addr}, "phụ thuộc địa chỉ ở người đọc"},
  {"CoRR", "po",       K_CO, 2, {co0, co1}, "đọc cùng ô hai lần"},
};
#define NT (sizeof T / sizeof T[0])

// ---------- hàng rào quay giữa các luồng ----------
static _Atomic unsigned bar_cnt, bar_gen;
static int nthreads;
static void barrier(void) {
  unsigned g = atomic_load(&bar_gen);
  if (atomic_fetch_add(&bar_cnt, 1) == (unsigned)nthreads - 1) {
    atomic_store(&bar_cnt, 0);
    atomic_store(&bar_gen, g + 1);
  } else
    while (atomic_load(&bar_gen) == g) ;
}

static const litmus *cur;
static long batches;
static int cpus[4];
static unsigned char *DLY[4];       // độ trễ ngẫu nhiên trước thân phép thử
static int dmax = 0;

static void pin(int c) {
  cpu_set_t s; CPU_ZERO(&s); CPU_SET(c, &s);
  if (pthread_setaffinity_np(pthread_self(), sizeof s, &s)) { perror("affinity"); exit(1); }
}

static long hist[16];               // đếm kết quả theo mã
static long weak;

static int classify(int i) {        // trả mã kết quả; đặt cờ yếu
  int *o0 = OUT[0] + i * 4, *o1 = OUT[1] + i * 4;
  switch (cur->k) {
  case K_MP:  return o1[0] * 2 + o1[1];                        // (cờ, dữ liệu)
  case K_SB:  return o0[0] * 2 + o1[0];
  case K_LB:  return o0[0] * 2 + o1[0];
  case K_2W:  return (X[i].v - 1) * 2 + (Y[i].v - 1);          // (x, y) ∈ {1,2}²
  case K_S:   return o1[0] * 2 + (X[i].v - 1);                 // (r0, x)
  case K_R:   return (Y[i].v - 1) * 2 + o1[0];                 // (y, r0)
  case K_IRIW: { int *o2 = OUT[2] + i * 4, *o3 = OUT[3] + i * 4;
                 return o2[0] * 8 + o2[1] * 4 + o3[0] * 2 + o3[1]; }
  case K_CO:  return o1[0] * 2 + o1[1];
  }
  return 0;
}
static int is_weak(int code) {
  switch (cur->k) {
  case K_MP: return code == 2;      // cờ = 1, dữ liệu = 0
  case K_SB: return code == 0;      // 0, 0
  case K_LB: return code == 3;      // 1, 1
  case K_2W: return code == 0;      // x = 1, y = 1
  case K_S:  return code == 3;      // r0 = 1, x = 2
  case K_R:  return code == 2;      // y = 2, r0 = 0
  case K_IRIW: return code == 10;   // T2: x=1,y=0; T3: y=1,x=0
  case K_CO: return code == 2;      // 1 rồi 0
  }
  return 0;
}

static void *worker(void *arg) {
  int t = (int)(intptr_t)arg;
  pin(cpus[t]);
  body_fn f = cur->f[t];
  unsigned char *d = DLY[t];
  for (long b = 0; b < batches; b++) {
    barrier();                       // luồng 0 vừa đặt lại bộ nhớ
    for (int i = 0; i < NI; i++) {
      barrier();
      for (int k = d[i]; k > 0; k--) asm volatile("");
      f(t, i);
    }
    barrier();                       // mọi luồng xong mẻ
    if (t == 0) {
      for (int i = 0; i < NI; i++) { int c = classify(i); hist[c]++; weak += is_weak(c); }
      for (int i = 0; i < NI; i++) X[i].v = Y[i].v = 0;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2 || !strcmp(argv[1], "list")) {
    for (size_t k = 0; k < NT; k++) printf("%-5s %-14s %d luồng  %s\n", T[k].test, T[k].variant, T[k].nt, T[k].what);
    return 0;
  }
  if (argc < 5) { fprintf(stderr, "dùng: %s TEST BIẾN-THỂ SỐ-LƯỢT cpu0 cpu1 [cpu2 cpu3]  (DMAX=n: trễ ngẫu nhiên)\n", argv[0]); return 1; }
  for (size_t k = 0; k < NT; k++)
    if (!strcmp(T[k].test, argv[1]) && !strcmp(T[k].variant, argv[2])) cur = &T[k];
  if (!cur) { fprintf(stderr, "không có phép thử %s %s\n", argv[1], argv[2]); return 1; }
  long n = atol(argv[3]);
  nthreads = cur->nt;
  if (argc < 4 + nthreads) { fprintf(stderr, "cần %d CPU\n", nthreads); return 1; }
  for (int t = 0; t < nthreads; t++) cpus[t] = atoi(argv[4 + t]);
  if (getenv("DMAX")) dmax = atoi(getenv("DMAX"));
  batches = (n + NI - 1) / NI;
  X = aligned_alloc(64, NI * sizeof(cell)); Y = aligned_alloc(64, NI * sizeof(cell));
  memset(X, 0, NI * sizeof(cell)); memset(Y, 0, NI * sizeof(cell));
  srand(12345);
  for (int t = 0; t < 4; t++) {
    OUT[t] = calloc(NI * 4, sizeof(int));
    DLY[t] = malloc(NI);
    for (int i = 0; i < NI; i++) DLY[t][i] = dmax ? rand() % dmax : 0;
  }
  struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);
  pthread_t th[4];
  for (int t = 1; t < nthreads; t++) pthread_create(&th[t], NULL, worker, (void *)(intptr_t)t);
  worker((void *)0);
  for (int t = 1; t < nthreads; t++) pthread_join(th[t], NULL);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  long total = batches * NI;
  printf("%s %-5s %-14s n=%ld cpu", ARCH, cur->test, cur->variant, total);
  for (int t = 0; t < nthreads; t++) printf("%c%d", t ? ',' : '=', cpus[t]);
  printf(" dmax=%d  yếu: %ld (%.3g /triệu)  %.1fs\n", dmax, weak, 1e6 * weak / total,
         (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec));
  printf("  kết quả:");
  for (int c = 0; c < 16; c++) if (hist[c]) {
    char lab[32];
    switch (cur->k) {
    case K_MP:   snprintf(lab, sizeof lab, "cờ=%d,dl=%d", c >> 1, c & 1); break;
    case K_2W:   snprintf(lab, sizeof lab, "x=%d,y=%d", (c >> 1) + 1, (c & 1) + 1); break;
    case K_S:    snprintf(lab, sizeof lab, "r0=%d,x=%d", c >> 1, (c & 1) + 1); break;
    case K_R:    snprintf(lab, sizeof lab, "y=%d,r0=%d", (c >> 1) + 1, c & 1); break;
    case K_IRIW: snprintf(lab, sizeof lab, "T2=%d%d,T3=%d%d", c >> 3, (c >> 2) & 1, (c >> 1) & 1, c & 1); break;
    default:     snprintf(lab, sizeof lab, "r0=%d,r1=%d", c >> 1, c & 1);
    }
    printf("  %s:%ld%s", lab, hist[c], is_weak(c) ? "*" : "");
  }
  printf("\n");
  return 0;
}
