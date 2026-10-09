#!/bin/bash
# run15: C3.14 — 4K aliasing bằng tỉ số kẹp tham chiếu (alias3.c), hai vòng (double, float), cả hai runner
mkdir -p out; A=$(uname -m)
cd c314
if [ "$A" = x86_64 ]; then gcc -O2 -mavx2 -o alias3 alias3.c; else gcc -O2 -o alias3 alias3.c; fi
objdump -d --no-show-raw-insn alias3 | awk '/<runF>:/,/ret/; /<runD>:/,/ret/' > ../out/c314-ae-alias3-objdump-$A.txt
{ lscpu | grep -E "Model name"; taskset -c 1 ./alias3; } > ../out/c314-ae-alias3-$A.txt 2>&1
