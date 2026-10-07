// MP.java — phép thử MP trong Java: trường thường / volatile / VarHandle release-acquire / opaque.
// Mỗi lượt hai luồng gặp nhau ở một hàng rào quay; người ghi: x = 1; y = 1; người đọc: r0 = y; r1 = x.
// Kết quả yếu: r0 == 1 && r1 == 0.  chạy: java MP <biến-thể> <số lượt>
import java.lang.invoke.MethodHandles;
import java.lang.invoke.VarHandle;
import java.util.concurrent.atomic.AtomicInteger;

public class MP {
    static final class C { int v; }                 // trường thường
    static final class VC { volatile int v; }       // trường volatile
    static final int B = 4096;                      // số bản mỗi mẻ
    static final VarHandle VH;
    static { try { VH = MethodHandles.lookup().findVarHandle(C.class, "v", int.class); }
             catch (Exception e) { throw new RuntimeException(e); } }

    static C[] X = new C[B], Y = new C[B];
    static VC[] VY = new VC[B];
    static final int[] res = new int[B];
    static final AtomicInteger bar = new AtomicInteger();

    static void barrier(int k) {                    // hai luồng, lượt thứ k
        bar.incrementAndGet();
        while (bar.get() < 2 * k) Thread.onSpinWait();
    }

    public static void main(String[] a) throws Exception {
        String var = a[0]; long n = Long.parseLong(a[1]);
        long batches = (n + B - 1) / B, weak = 0, total = batches * B;
        long[] hist = new long[4];
        int k = 0;
        for (long b = 0; b < batches; b++) {
            for (int i = 0; i < B; i++) { X[i] = new C(); }
            for (int i = 0; i < B; i++) { Y[i] = new C(); VY[i] = new VC(); }
            final int k0 = k;
            Thread w = new Thread(() -> {
                int kk = k0;
                for (int i = 0; i < B; i++) {
                    barrier(++kk);
                    switch (var) {
                        case "plain"   -> { X[i].v = 1; Y[i].v = 1; }
                        case "volatile"-> { X[i].v = 1; VY[i].v = 1; }
                        case "relacq"  -> { X[i].v = 1; VH.setRelease(Y[i], 1); }
                        case "opaque"  -> { VH.setOpaque(X[i], 1); VH.setOpaque(Y[i], 1); }
                    }
                }
            });
            w.start();
            int kk = k0;
            for (int i = 0; i < B; i++) {
                barrier(++kk);
                int r0 = 0, r1 = 0;
                switch (var) {
                    case "plain"   -> { r0 = Y[i].v; r1 = X[i].v; }
                    case "volatile"-> { r0 = VY[i].v; r1 = X[i].v; }
                    case "relacq"  -> { r0 = (int) VH.getAcquire(Y[i]); r1 = X[i].v; }
                    case "opaque"  -> { r0 = (int) VH.getOpaque(Y[i]); r1 = (int) VH.getOpaque(X[i]); }
                }
                res[i] = r0 * 2 + r1;
            }
            w.join();
            k = kk;
            for (int i = 0; i < B; i++) { hist[res[i]]++; if (res[i] == 2) weak++; }
        }
        System.out.printf("java %s %-8s n=%d  yếu: %d (%.3g /triệu)  r0r1: 00=%d 01=%d 10=%d* 11=%d%n",
            System.getProperty("os.arch"), var, total, weak, 1e6 * weak / total, hist[0], hist[1], hist[2], hist[3]);
    }
}
