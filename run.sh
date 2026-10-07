#!/bin/bash
# run5 (C3.8 SIMD): độ rộng, độ trễ FP, tự vector hóa, intrinsics, căn lề, phép giảm, int8, checkpoint convolution
set -u
mkdir -p out
A=$(uname -m)
log() { echo "== $(date -u +%H:%M:%S) $*"; }
cd simd
curl -sfL -o 130.mat https://engineering.case.edu/sites/default/files/130.mat && python3 mat2f32.py 130.mat X130_DE_time > /dev/null
grep -m1 -E "Features|flags" /proc/cpuinfo > ../out/c38-features-$A.txt
if [ "$A" = aarch64 ]; then
  V128="-O3"; VW="-O3 -march=armv8.2-a+sve"; FAST="-Ofast"; NAT="-mcpu=neoverse-n2"
  gcc -O2 -march=armv8.2-a+sve sve.c -o sve && ./sve > ../out/c38-sve-$A.txt 2>&1
  objdump -d --no-show-raw-insn sve | awk '/<saxpy_sve>:/,/^$/' > ../out/c38-sve-dis-$A.txt
else
  V128="-O3"; VW="-O3 -march=x86-64-v3"; FAST="-Ofast -march=x86-64-v3"; NAT="-march=native"
fi
log widths
gcc -O3 -fno-tree-vectorize widths.c -o w0 && gcc $V128 widths.c -o w1 && gcc $VW widths.c -o w2
{ ./w0 0 "$A vô hướng"; ./w1 0 "$A $V128"; ./w2 0 "$A $VW"; } > ../out/c38-widths-$A.txt 2>&1
for b in w1 w2; do objdump -d --no-show-raw-insn $b | awk '/<saxpy>:/,/^$/' > ../out/c38-saxpy-$b-$A.txt; done
gcc -O2 fplat.c -o fplat && { ./fplat 0; ./fplat 1; } > ../out/c38-fplat-$A.txt 2>&1
log autovec
gcc -O3 -fno-tree-vectorize autovec.c -o av0 -lm && gcc $VW autovec.c -o av1 -lm && gcc $FAST autovec.c -o av2 -lm
gcc $VW -fopt-info-vec-all autovec.c -o /dev/null -lm 2> ../out/c38-autovec-optinfo-$A.txt
{ echo "-- vô hướng"; ./av0 0; echo "-- $VW"; ./av1 0; echo "-- $FAST"; ./av2 0; } > ../out/c38-autovec-$A.txt 2>&1
for f in k01_sum_f32 k07_gather k10_find k11_sinf k12_dot_u8s8; do echo "## $f av1"; objdump -d --no-show-raw-insn av1 | awk "/<$f>:/,/^\$/"; echo "## $f av2"; objdump -d --no-show-raw-insn av2 | awk "/<$f>:/,/^\$/"; done > ../out/c38-autovec-dis-$A.txt
log intr
gcc -O2 -DSFX=_o2 -c kern_c.c -o k_o2.o && gcc $VW -DSFX=_v -c kern_c.c -o k_v.o && gcc $V128 -DSFX=_sse -c kern_c.c -o k_sse.o
if [ "$A" = aarch64 ]; then gcc -O3 intr.c k_o2.o k_v.o k_sse.o -o intr; else gcc -O3 -mavx2 intr.c k_o2.o k_v.o k_sse.o -o intr; fi
./intr 0 > ../out/c38-intr-$A.txt 2>&1
for f in sat_c_o2 sat_c_v sat_intr sum_ge_c_sse sum_ge_c_v sum_ge_intr; do echo "## $f"; objdump -d --no-show-raw-insn intr | awk "/<$f>:/,/^\$/"; done > ../out/c38-intr-dis-$A.txt
log align
if [ "$A" = aarch64 ]; then gcc -O2 align.c -o align; else gcc -O2 -mavx2 align.c -o align; fi
{ ./align 0; ./align 1; } > ../out/c38-align-$A.txt 2>&1
log reduce
if [ "$A" = aarch64 ]; then gcc -O2 reduce.c -o reduce -lm; gcc -O2 reduce_overlap.c -o reduce_ov -lm; else gcc -O2 -mavx2 -mfma reduce.c -o reduce -lm; gcc -O2 -mavx2 -mfma reduce_overlap.c -o reduce_ov -lm; fi
{ ./reduce 0 X130_DE_time.f32; echo "-- không nối chuỗi giữa các lần gọi"; ./reduce_ov 0 | head -7; } > ../out/c38-reduce-$A.txt 2>&1
log dot8
gcc -O2 dot8.c -o dot8 && ./dot8 0 > ../out/c38-dot8-$A.txt 2>&1
objdump -d --no-show-raw-insn dot8 | awk '/<dot_dp>:/,/^$/' > ../out/c38-dot8-dis-$A.txt
log conv
gcc -O3 -fno-tree-vectorize -DSFX=_s -c conv_k.c -o c_s.o && gcc $V128 -DSFX=_v128 -c conv_k.c -o c_v128.o && gcc $VW -DSFX=_vw -c conv_k.c -o c_vw.o && gcc $FAST -DSFX=_fast -c conv_k.c -o c_fast.o
gcc $VW -DSFX=_vw -fopt-info-vec-all -c conv_k.c -o /dev/null 2> ../out/c38-conv-optinfo-$A.txt
gcc -O2 conv.c c_s.o c_v128.o c_vw.o c_fast.o -o conv -lm && { ./conv 0 X130_DE_time.f32; ./conv 1 X130_DE_time.f32; } > ../out/c38-conv-$A.txt 2>&1
for f in conv_direct_s conv_direct_v128 conv_direct_vw conv_direct_fast conv_tapouter_v128 conv_tapouter_vw conv_q15_v128 conv_q15_vw conv_intr; do echo "## $f"; objdump -d --no-show-raw-insn conv | awk "/<$f>:/,/^\$/"; done > ../out/c38-conv-dis-$A.txt
cd ..
log done
cat out/c38-*-$A.txt | grep -v "^ *[0-9a-f]*:" | head -200
