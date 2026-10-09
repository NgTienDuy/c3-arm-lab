#!/bin/bash
# run12: C3.14 — (a) ghép kênh: so ước lượng với giá trị THẬT trong CÙNG lần chạy (nhóm ghim :D), quét chu kỳ pha và chu kỳ xoay;
# (b) lặp lại các offset "chậm" của 4K aliasing theo thứ tự ngẫu nhiên; (c) lần ngược ngăn xếp fp vs dwarf; (d) perf stat mặc định, -d;
# (e) IPC -O0 vs -O2
mkdir -p out; A=$(uname -m)
sudo apt-get update -qq >/dev/null 2>&1
sudo apt-get install -y -qq linux-tools-common linux-tools-$(uname -r) binutils >/dev/null 2>&1 || true
sudo sysctl -w kernel.perf_event_paranoid=-1 >/dev/null
P="sudo perf"
cd c314
for f in kern aslr funcs; do gcc -O2 -o $f $f.c 2>/dev/null || echo "không dịch được $f trên $A"; done
gcc -O2 -fno-omit-frame-pointer -o cg_fp cg.c; gcc -O2 -fomit-frame-pointer -o cg_nofp cg.c
gcc -O0 -o ipc_O0 ipc.c; gcc -O2 -o ipc_O2 ipc.c
mkfifo /tmp/ctl /tmp/ack
R() { sudo KERN_CTL=/tmp/ctl KERN_ACK=/tmp/ack perf stat -D -1 --control fifo:/tmp/ctl,/tmp/ack "$@"; }
set -x
lscpu | head -20 > ../out/c314-s-lscpu-$A.txt
if [ "$A" = aarch64 ]; then
MX=instructions,l1d_cache,l1d_cache_refill,br_retired,br_mis_pred,br_pred,op_spec,op_retired,stall_frontend,stall_backend,inst_spec,inst_retired
{ for mi in 1 4 16; do echo $mi | sudo tee /sys/bus/event_source/devices/armv8_pmuv3_0/perf_event_mux_interval_ms >/dev/null
    for ph in 50 300 1000 3000 10000 100000; do [ $mi != 1 ] && [ $ph != 1000 ] && [ $ph != 300 ] && continue
      for r in 1 2 3; do R -x, -e '{cycles,l1d_cache_refill}:D' -e $MX ./kern alt 2000 $ph 2>&1 >/dev/null | sed "s/^/mux$mi,$ph,$r,/"; done; done; done
  echo 1 | sudo tee /sys/bus/event_source/devices/armv8_pmuv3_0/perf_event_mux_interval_ms >/dev/null
} > ../out/c314-t-mux2-$A.txt 2>&1
{ for b in cg_fp cg_nofp; do for cgm in fp dwarf; do
    $P record -q --call-graph $cgm -F 999 -o /tmp/cg.data ./$b 300 >/dev/null 2>&1
    echo "== $b --call-graph $cgm ($(sudo du -k /tmp/cg.data | cut -f1) KiB)"
    $P report -i /tmp/cg.data --stdio --no-children --sort sym -G 2>/dev/null | grep -v '^#' | grep -v '^$' | head -16; done; done
} > ../out/c314-u-callgraph-$A.txt 2>&1
{ $P stat -- gzip -6 -c /dev/shm/x 2>&1 >/dev/null || true; } > /dev/null 2>&1
base64 -w 76 /dev/urandom | head -c 67108864 > /dev/shm/t64
{ $P stat -- gzip -6 -c /dev/shm/t64 2>&1 >/dev/null; $P stat -d -- gzip -6 -c /dev/shm/t64 2>&1 >/dev/null; } > ../out/c314-v-default-$A.txt 2>&1
{ for o in O0 O2; do $P stat -e cycles,instructions,task-clock ./ipc_$o 20000 2>&1 >/dev/null; done; } > ../out/c314-w-ipc-$A.txt 2>&1
fi
# (b) 4K aliasing: các offset nghi ngờ và đối chứng, mỗi offset 10 lần, thứ tự xáo trộn (pad = 3248 − offset)
{ for i in $(seq 10); do for off in 32 48 112 128 560 576 624 640; do echo "$off"; done; done | shuf --random-source=<(yes) > /tmp/order
  while read off; do pad=$(( (3248 - off + 4096) % 4096 )); echo -n "muốn $off pad $pad: "; env -i PAD=$(head -c $pad /dev/zero | tr '\0' x) setarch $(uname -m) -R ./aslr 20000; done < /tmp/order
} > ../out/c314-x-alias-$A.txt 2>&1
set +x
