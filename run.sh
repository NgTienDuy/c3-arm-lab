#!/bin/bash
# run10: C3.14 — bộ đếm hiệu năng trên runner ARM Neoverse-N2 (PMU thật, perf 6.17) + runner x86 (không PMU: chỉ đo thời gian).
mkdir -p out; A=$(uname -m)
sudo apt-get update -qq >/dev/null 2>&1
sudo apt-get install -y -qq linux-tools-common linux-tools-$(uname -r) binutils >/dev/null 2>&1 || true
sudo sysctl -w kernel.perf_event_paranoid=-1 >/dev/null
P="sudo perf"
cd c314
for f in kern funcs skid aslr gather offcpu; do gcc -O2 -o $f $f.c || echo "LỖI dịch $f"; done
gcc -O2 -pthread -o smt smt.c
objdump -d --no-show-raw-insn kern > ../out/c314-kern-objdump-$A.txt 2>&1
objdump -d --no-show-raw-insn skid | awk '/<loopload>:/,/ret/' > ../out/c314-skid-objdump-$A.txt; objdump -d --no-show-raw-insn skid | awk '/<loopdiv>:/,/ret/' >> ../out/c314-skid-objdump-$A.txt
set -x
{ uname -a; lscpu; nproc; cat /proc/sys/kernel/perf_event_paranoid; ls /sys/bus/event_source/devices/
  cat /sys/bus/event_source/devices/armv8_pmuv3_0/caps/slots 2>/dev/null; sudo dmesg | grep -i -E 'perfevents|PMU' | head
  grep -m3 -E 'CPU (implementer|variant|part|revision)' /proc/cpuinfo; grep -m1 'CPU revision' /proc/cpuinfo
  perf --version; $P list pmu 2>/dev/null | grep -c -E '^  [a-z0-9_]+$'
  for c in /sys/devices/system/cpu/cpu[0-9]*; do echo "$c: $(cat $c/topology/thread_siblings_list 2>/dev/null)"; done
} > ../out/c314-a-probe-$A.txt 2>&1
$P list pmu > ../out/c314-a-list-$A.txt 2>&1
if [ "$A" = aarch64 ]; then
# B. bộ đếm có đếm đúng không: số lệnh đo so với số lệnh biết trước (hiệu hai lần chạy N và N/2 khử phần khởi động)
{ for m in "chain" "indep" "fdiv" "spin" "chase 65536"; do set -- $m
    for n in 100000000 50000000; do $P stat -x, -e instructions:u,cycles:u -r 3 ./kern $1 $n $2 2>&1 >/dev/null | sed "s/^/$1 $n,/"; done; done
  for n in 1000000 500000; do $P stat -x, -e instructions:u,instructions:k,cycles:u,cycles:k ./kern sys $n 2>&1 >/dev/null | sed "s/^/sys $n,/"; done
} > ../out/c314-b-exact-$A.txt 2>&1
# B2. sự kiện "chung" của perf trên ARM là gì
{ $P stat -e cycles,instructions,cache-references,cache-misses,l1d_cache,l1d_cache_refill -- ./kern chase 20000000 1048576
  $P stat -e branches,branch-misses,br_pred,br_mis_pred,br_retired,br_mis_pred_retired -- ./kern branch 50000000 128
} > ../out/c314-b-generic-$A.txt 2>&1
# C. bảng mẫu hình: hai nhóm sự kiện, mỗi nhóm ≤ 6 bộ đếm lập trình được + bộ đếm chu kỳ riêng → KHÔNG ghép kênh
G1=cycles,instructions,l1d_cache_refill,l2d_cache_refill,ll_cache_miss_rd,br_mis_pred_retired,stall_backend_mem
G2=cpu_cycles,stall_slot,stall_slot_frontend,stall_slot_backend,op_spec,op_retired,br_mis_pred
{ while read -r tag m n arg; do for G in $G1 $G2 task-clock; do $P stat -x, -e $G ./kern $m $n $arg 2>&1 >/dev/null | sed "s/^/$tag,/"; done; done <<L
chain chain 100000000 0
indep indep 100000000 0
fdiv fdiv 30000000 0
chaseL1 chase 30000000 32768
chaseL2 chase 10000000 524288
chaseSLC chase 3000000 33554432
chaseDRAM chase 1000000 1073741824
stream stream 4 1073741824
branch50 branch 100000000 128
branch3 branch 100000000 8
spin spin 200000000 0
sys sys 2000000 0
L
} > ../out/c314-c-patterns-$A.txt 2>&1
# D. ghép kênh: 13 sự kiện trên 6 bộ đếm, chương trình hai pha (tính rồi đuổi con trỏ) — so với đếm riêng từng nhóm
E1=instructions,l1d_cache,l1d_cache_refill,l2d_cache,l2d_cache_refill,ll_cache_rd
E2=ll_cache_miss_rd,mem_access,br_retired,br_mis_pred,stall_backend_mem,stall_frontend
{ for r in 1 2 3; do $P stat -x, -e cycles,$E1,$E2 ./kern phase 400000000 2>&1 >/dev/null | sed "s/^/ghép_r$r,/"
    $P stat -x, -e cycles,$E1 ./kern phase 400000000 2>&1 >/dev/null | sed "s/^/riêng1_r$r,/"
    $P stat -x, -e cycles,$E2 ./kern phase 400000000 2>&1 >/dev/null | sed "s/^/riêng2_r$r,/"; done
  $P stat -I 250 -e cycles,instructions,l2d_cache_refill ./kern phase 400000000
} > ../out/c314-d-mux-$A.txt 2>&1
# E. lấy mẫu vs đếm: tỉ lệ 1:2:4 → 14,29 / 28,57 / 57,14 %
{ for opt in "-F 99" "-F 999" "-F 9999" "-c 1000003"; do $P record -q $opt -e cycles -o /tmp/f.data ./funcs 20000 >/dev/null 2>&1
    echo "== $opt"; $P report -i /tmp/f.data --stdio --sort sym 2>/dev/null | grep -E 'f[124]|Samples|Event count'; done
  for F in 0 1000 10000 50000; do
    if [ $F = 0 ]; then $P stat -e task-clock ./funcs 20000 2>&1 >/dev/null | grep -E 'task-clock|elapsed'
    else echo "== -F $F"; /usr/bin/time -f "%e s" $P record -q -F $F -e cycles -o /tmp/f.data ./funcs 20000 2>&1 >/dev/null | tail -1; fi; done
} > ../out/c314-e-sample-$A.txt 2>&1
# F. trượt (skid): mẫu rơi vào lệnh nào
{ for w in load fdiv; do $P record -q -e cycles -c 100003 -o /tmp/s.data ./skid $w 3000000 >/dev/null 2>&1
    echo "== $w"; $P annotate -i /tmp/s.data --stdio -s loop$w 2>/dev/null | grep -v '^\s*$' | head -40; done
  $P record -q -e cycles:pp -c 100003 -o /tmp/s2.data ./skid load 3000000; echo "mã thoát cycles:pp = $?"
  $P record -q -e cycles:ppp -c 100003 -o /tmp/s3.data ./skid load 300000; echo "mã thoát cycles:ppp = $?"
} > ../out/c314-f-skid-$A.txt 2>&1
# G. nhiễu: lặp, hàng xóm, nhân/người dùng
{ $P stat -r 20 -e cycles,instructions,task-clock ./kern chase 3000000 33554432 2>&1 >/dev/null
  $P stat -r 20 -e cycles,instructions,task-clock ./kern chain 50000000 2>&1 >/dev/null
  for nb in 0 1 3; do
    pids=""; for k in $(seq 1 $nb); do taskset -c $k ./kern stream 1000 1073741824 >/dev/null & pids="$pids $!"; done; sleep 1
    echo "== hàng xóm stream 1 GiB: $nb"
    $P stat -e cycles,instructions,l2d_cache_refill,ll_cache_miss_rd,stall_backend_mem,task-clock -- taskset -c 0 ./kern chase 3000000 33554432 2>&1 >/dev/null
    [ -n "$pids" ] && kill $pids 2>/dev/null; wait 2>/dev/null; done
} > ../out/c314-g-noise-$A.txt 2>&1
fi
# H. ASLR và vị trí ngăn xếp (cả hai kiến trúc)
{ echo "== ASLR bật"; for i in $(seq 30); do ./aslr 50000; done
  echo "== ASLR tắt (setarch -R)"; for i in $(seq 10); do setarch $(uname -m) -R ./aslr 50000; done
  echo "== ASLR tắt, đổi cỡ biến môi trường"; for pad in $(seq 0 64 4096); do echo -n "pad $pad: "; env -i PAD=$(head -c $pad /dev/zero | tr '\0' x) setarch $(uname -m) -R ./aslr 50000; done
} > ../out/c314-h-aslr-$A.txt 2>&1
# I. SMT: hàng xóm trên luồng anh em vs lõi khác (chỉ đo thời gian)
{ for c in /sys/devices/system/cpu/cpu[0-3]; do echo "$c: $(cat $c/topology/thread_siblings_list)"; done
  SIB=$(cut -d, -f2 /sys/devices/system/cpu/cpu0/topology/thread_siblings_list | cut -d- -f2); [ "$SIB" = 0 ] && SIB=-1
  OTHER=$(for c in 1 2 3; do grep -q -w -E "^0|,0$|-" /sys/devices/system/cpu/cpu$c/topology/thread_siblings_list && [ $c = "$SIB" ] && continue; echo $c; done | grep -v "^$SIB$" | head -1)
  echo "anh em của cpu0: $SIB; lõi khác: $OTHER"
  for m in alu mem; do ./smt 0 -1 $m; [ "$SIB" != -1 ] && ./smt 0 $SIB $m; ./smt 0 $OTHER $m; done
  [ "$SIB" != -1 ] && ./smt 0 $SIB alu mem && ./smt 0 $SIB mem alu
} > ../out/c314-i-smt-$A.txt 2>&1
if [ "$A" = aarch64 ]; then
# J. checkpoint: CPU, bộ nhớ hay I/O? Ba tải + can thiệp kiểm chứng
base64 -w 76 /dev/urandom | head -c 268435456 > /dev/shm/text; head -c 134217728 /dev/shm/text > /dev/shm/text_half
head -c 2147483648 /dev/urandom > $HOME/big.bin; sync
J1=task-clock,context-switches,cpu-migrations,major-faults,block:block_rq_issue,cycles,instructions,stall_backend_mem,ll_cache_miss_rd,l2d_cache_refill,mem_access
dc() { sync; echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null; }
{ while read -r tag cold cmd; do for G in $J1 $G2; do [ $cold = 1 ] && dc; echo "== $tag | $cmd"; $P stat -e $G -- $cmd 2>&1 >/dev/null; done; done <<L
W1 0 gzip -6 -c /dev/shm/text
W1half 0 gzip -6 -c /dev/shm/text_half
W2 0 ./gather 1024 100000000
W2small 0 ./gather 1 100000000
W3 1 wc -l $HOME/big.bin
W3warm 0 wc -l $HOME/big.bin
W4 0 ./offcpu 5
L
  for w in "./gather 1024 100000000" "./gather 1 100000000"; do $w; done
} > ../out/c314-j-ck-$A.txt 2>&1
fi
set +x
cat ../out/c314-a-probe-$A.txt | head -5
