#!/bin/bash
# run16: C3.14 — alias4.c: vòng ĐÃ VECTOR HÓA (restrict, -O3; AVX2 trên x86, NEON trên AArch64), tỉ số kẹp tham chiếu, thêm off lệch 32 B xa vùng 4K
mkdir -p out; A=$(uname -m)
cd c314
if [ "$A" = x86_64 ]; then gcc -O3 -mavx2 -o alias4 alias4.c; else gcc -O3 -o alias4 alias4.c; fi
objdump -d --no-show-raw-insn alias4 | awk '/<kF>:/,/ret/; /<kD>:/,/ret/' > ../out/c314-af-alias4-objdump-$A.txt
{ lscpu | grep -E "Model name"; taskset -c 1 ./alias4; echo "== lần 2"; taskset -c 2 ./alias4; } > ../out/c314-af-alias4-$A.txt 2>&1
