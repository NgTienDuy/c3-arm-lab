#!/bin/bash
# run11: C3.14 — (a) sự kiện nào máy ảo thật sự cho đếm, (b) đếm đúng vùng bằng perf stat --control, (c) ghép kênh gặp chu kỳ pha,
# (d) chi phí lấy mẫu, (e) trượt (sửa tên hàm), (f) 4K aliasing quét mịn + 300 lần ASLR, (g) checkpoint với sự kiện đếm được, (h) cachegrind vs PMU
mkdir -p out; A=$(uname -m)
sudo apt-get update -qq >/dev/null 2>&1
sudo apt-get install -y -qq linux-tools-common linux-tools-$(uname -r) binutils valgrind >/dev/null 2>&1 || true
sudo sysctl -w kernel.perf_event_paranoid=-1 >/dev/null
P="sudo perf"
cd c314
for f in kern funcs skid aslr gather offcpu; do gcc -O2 -o $f $f.c 2>/dev/null || echo "không dịch được $f trên $A"; done
mkfifo /tmp/ctl /tmp/ack
R() { sudo KERN_CTL=/tmp/ctl KERN_ACK=/tmp/ack perf stat -D -1 --control fifo:/tmp/ctl,/tmp/ack "$@"; }   # chỉ đếm vùng kern đánh dấu
set -x
if [ "$A" = aarch64 ]; then
cat /sys/bus/event_source/devices/armv8_pmuv3_0/perf_event_mux_interval_ms > ../out/c314-k-muxint-$A.txt
# (a) khảo sát: mỗi sự kiện trong `perf list pmu`, đếm trên vùng đuổi con trỏ 64 MiB (trượt mọi tầng) — 0 hay khác 0
$P list pmu 2>/dev/null | grep -E '^  [a-z0-9_]+$' | tr -d ' ' > /tmp/evs
{ set +x; split -l 5 /tmp/evs /tmp/evg_
  for g in /tmp/evg_*; do E=$(paste -sd, $g); R -x, -e cycles,$E ./kern chase 300000 67108864 2>&1 >/dev/null | grep -v ',cycles,' ; done
  set -x; } > ../out/c314-k-survey-$A.txt 2>&1
# (b) bảng mẫu hình, CHỈ vùng đo, các sự kiện đếm được
G1=cycles,instructions,l1d_cache,l1d_cache_refill,br_mis_pred_retired,stall_frontend,stall_backend
G2=cpu_cycles,op_spec,op_retired,inst_spec,l1d_tlb_refill,l1i_cache_refill,exc_taken
{ while read -r tag m n arg; do for G in $G1 $G2; do R -x, -e $G ./kern $m $n $arg 2>&1 >/dev/null | sed "s/^/$tag,/"; done; done <<L
chain chain 100000000 0
indep indep 100000000 0
fdiv fdiv 30000000 0
chaseL1 chase 30000000 32768
chaseL2 chase 10000000 524288
chase32M chase 3000000 33554432
chase1G chase 1000000 1073741824
stream stream 4 1073741824
branch50 branch 100000000 128
branch3 branch 100000000 8
spin spin 200000000 0
sys sys 2000000 0
L
} > ../out/c314-l-patterns-$A.txt 2>&1
# so: cả chương trình vs chỉ vùng đo (chase 1 GiB)
{ $P stat -e cycles,instructions,l1d_cache_refill ./kern chase 1000000 1073741824 2>&1 >/dev/null
  R -e cycles,instructions,l1d_cache_refill ./kern chase 1000000 1073741824 2>&1 >/dev/null; } > ../out/c314-l-region-$A.txt 2>&1
# (c) ghép kênh: 13 sự kiện (đều đếm được) trên 6 bộ đếm, chương trình xen hai pha với chu kỳ khác nhau
M12=instructions,l1d_cache,l1d_cache_refill,br_retired,br_mis_pred,br_pred,op_spec,op_retired,stall_frontend,inst_spec,l1d_cache_rd,l1i_cache
{ for ph in 100 1000 4000 10000 100000; do for r in 1 2; do
    R -x, -e cycles,$M12 ./kern alt 3000 $ph 2>&1 >/dev/null | sed "s/^/ghép_$ph,/"
    R -x, -e cycles,l1d_cache_refill,instructions ./kern alt 3000 $ph 2>&1 >/dev/null | sed "s/^/riêng_$ph,/"; done; done
} > ../out/c314-m-mux-$A.txt 2>&1
# (d) lấy mẫu: tỉ lệ và chi phí, chạy dài hơn (funcs 200000 ~ 3,3 s)
{ for F in 1 99 999 9999 49999; do /usr/bin/time -f "-F $F: %e s, %U s người dùng, %S s nhân" $P record -q -F $F -e cycles -o /tmp/f$F.data ./funcs 200000 >/dev/null
    $P report -i /tmp/f$F.data --stdio --sort sym 2>/dev/null | grep -E 'f[124]$|Samples'; done; } > ../out/c314-n-sample-$A.txt 2>&1
# (e) trượt: hai vòng, và perf có thật sự dùng precise_ip không
{ for w in load div; do $P record -q -e cycles -c 100003 -o /tmp/s.data ./skid $([ $w = load ] && echo load || echo fdiv) 3000000 >/dev/null 2>&1
    echo "== loop$w"; $P annotate -i /tmp/s.data --stdio -s loop$w 2>/dev/null | grep -E '^\s+[0-9.]+ :' | head -24; done
  $P record -q -e cycles:pp -c 100003 -o /tmp/s2.data ./skid load 3000000 >/dev/null; echo "mã thoát $?"
  $P evlist -v -i /tmp/s2.data 2>&1 | head -3
  $P annotate -i /tmp/s2.data --stdio -s loopload 2>/dev/null | grep -E '^\s+[0-9.]+ :' | sed -n '17,20p'
} > ../out/c314-o-skid-$A.txt 2>&1
# (h) cachegrind (mô phỏng) vs PMU: số lần trượt L1D trên cùng vùng
{ for m in "chase 2000000 1048576" "stream 1 67108864"; do set -- $m
    R -e cycles,instructions,l1d_cache,l1d_cache_refill ./kern $1 $2 $3 2>&1 >/dev/null
    valgrind --tool=cachegrind --cache-sim=yes --D1=65536,4,64 --LL=1048576,8,64 --cachegrind-out-file=/dev/null ./kern $1 $2 $3 2>&1 | grep -E "D +refs|D1 +misses|LLd misses|I +refs"; done
} > ../out/c314-p-cachegrind-$A.txt 2>&1
fi
# (f) 4K aliasing: quét mịn và 300 lần ASLR bật
{ echo "== ASLR bật, 300 lần"; for i in $(seq 300); do ./aslr 20000; done
  echo "== ASLR tắt, quét cỡ biến môi trường bước 8"; for pad in $(seq 2400 8 2880); do echo -n "pad $pad: "; env -i PAD=$(head -c $pad /dev/zero | tr '\0' x) setarch $(uname -m) -R ./aslr 20000; done
} > ../out/c314-q-aslr-$A.txt 2>&1
if [ "$A" = aarch64 ]; then
# (g) checkpoint: CPU, bộ nhớ hay I/O? — sự kiện đếm được
base64 -w 76 /dev/urandom | head -c 268435456 > /dev/shm/text; head -c 134217728 /dev/shm/text > /dev/shm/text_half
head -c 2147483648 /dev/urandom > $HOME/big.bin; sync
J=task-clock,context-switches,cpu-migrations,major-faults,block:block_rq_issue,cycles,instructions,l1d_cache,l1d_cache_refill,stall_backend,stall_frontend,op_spec,op_retired
dc() { sync; echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null; }
{ for rep in 1 2; do while read -r tag cold cmd; do [ $cold = 1 ] && dc; echo "== $tag r$rep | $cmd"; $P stat -e $J -- $cmd 2>&1 >/dev/null; done <<L
W1 0 gzip -6 -c /dev/shm/text
W1half 0 gzip -6 -c /dev/shm/text_half
W2 0 ./gather 1024 100000000
W2small 0 ./gather 1 100000000
W3 1 wc -l $HOME/big.bin
W3warm 0 wc -l $HOME/big.bin
W4 0 ./offcpu 5
L
done; } > ../out/c314-r-ck-$A.txt 2>&1
fi
set +x
