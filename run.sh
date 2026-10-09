#!/bin/bash
# run13: C3.14 — (a) lần ngược ngăn xếp khi KHÔNG có gọi đuôi; (b) perf stat mặc định bị lỗi → bộ sự kiện tự chọn; (c) bộ dự đoán
# nhánh học chuỗi ngẫu nhiên lặp lại; (d) đếm theo từng CPU khi "không có gì chạy"
mkdir -p out; A=$(uname -m)
sudo apt-get update -qq >/dev/null 2>&1
sudo apt-get install -y -qq linux-tools-common linux-tools-$(uname -r) binutils >/dev/null 2>&1 || true
sudo sysctl -w kernel.perf_event_paranoid=-1 >/dev/null
P="sudo perf"
cd c314
[ "$A" = aarch64 ] || exit 0
gcc -O2 -o kern kern.c
for v in fp nofp; do o=$([ $v = fp ] && echo -fno-omit-frame-pointer || echo -fomit-frame-pointer)
  gcc -O2 $o -fno-optimize-sibling-calls -o cg_${v}_nosib cg.c; done
objdump -d --no-show-raw-insn cg_fp_nosib | awk '/<leaf>:/,/ret/' > ../out/c314-y-leaf-objdump-$A.txt
mkfifo /tmp/ctl /tmp/ack
R() { sudo KERN_CTL=/tmp/ctl KERN_ACK=/tmp/ack perf stat -D -1 --control fifo:/tmp/ctl,/tmp/ack "$@"; }
set -x
{ for b in cg_fp_nosib cg_nofp_nosib; do for cgm in fp dwarf; do
    $P record -q --call-graph $cgm -F 999 -o /tmp/cg.data ./$b 300 >/dev/null 2>&1
    echo "== $b --call-graph $cgm ($(sudo du -k /tmp/cg.data | cut -f1) KiB)"
    $P report -i /tmp/cg.data --stdio --no-children --sort sym -G 2>/dev/null | grep -v '^#' | grep -v '^$' | head -24; done; done
} > ../out/c314-y-callgraph-$A.txt 2>&1
base64 -w 76 /dev/urandom | head -c 67108864 > /dev/shm/t64
{ $P stat -e task-clock,context-switches,cpu-migrations,page-faults,cycles,instructions,branches,branch-misses -- gzip -6 -c /dev/shm/t64 2>&1 >/dev/null
  $P stat --version; $P stat -- true; echo "mã thoát perf stat mặc định: $?"
} > ../out/c314-z-default-$A.txt 2>&1
{ for thr in 8 32 64 128; do for m in branch branchx; do R -x, -e cycles,instructions,br_retired,br_mis_pred_retired ./kern $m 50000000 $thr 2>&1 >/dev/null | sed "s/^/$m,$thr,/"; done; done
} > ../out/c314-za-branch-$A.txt 2>&1
{ sudo perf stat -a -A -e cycles,instructions,context-switches -- sleep 2; } > ../out/c314-zb-percpu-$A.txt 2>&1
set +x
