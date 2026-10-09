#!/bin/bash
# run14: C3.14 — 4K aliasing có kiểm soát (alias2.c) trên cả hai runner; nhánh ngẫu nhiên xorshift ở 8 giá trị p (bất đối xứng p / 1−p,
# giá một lần đoán sai); mọi thứ ghi vào out/
mkdir -p out; A=$(uname -m)
sudo apt-get update -qq >/dev/null 2>&1
sudo apt-get install -y -qq linux-tools-common linux-tools-$(uname -r) >/dev/null 2>&1 || true
sudo sysctl -w kernel.perf_event_paranoid=-1 >/dev/null
cd c314
gcc -O2 -o alias2 alias2.c
objdump -d --no-show-raw-insn alias2 | awk '/<run>:/,/ret/' > ../out/c314-ac-alias2-objdump-$A.txt
set -x
{ lscpu | grep -E "Model name|MHz"; taskset -c 1 ./alias2; } > ../out/c314-ac-alias2-$A.txt 2>&1
if [ "$A" = aarch64 ]; then
gcc -O2 -o kern kern.c; mkfifo /tmp/ctl /tmp/ack
R() { sudo KERN_CTL=/tmp/ctl KERN_ACK=/tmp/ack perf stat -D -1 --control fifo:/tmp/ctl,/tmp/ack "$@"; }
{ for r in 1 2; do for thr in 3 13 26 51 77 128 179 205 230 243 253; do R -x, -e cycles,instructions,br_retired,br_mis_pred_retired ./kern branchx 50000000 $thr 2>&1 >/dev/null | sed "s/^/branchx,$thr,$r,/"; done; done
} > ../out/c314-ad-branchp-$A.txt 2>&1
fi
set +x
