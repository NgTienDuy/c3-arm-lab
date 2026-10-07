#!/bin/bash
# run6 (C3.8): ghi model CPU; ARM: fplat (sửa), bản -mcpu=neoverse-n2; x86: lặp lại để gắn nhãn CPU
set -u
mkdir -p out
A=$(uname -m)
cd simd
M=$(lscpu | grep -m1 "Model name" | sed 's/  */ /g')
echo "$M" > ../out/c38b-cpu-$A.txt; lscpu >> ../out/c38b-cpu-$A.txt
curl -sfL -o 130.mat https://engineering.case.edu/sites/default/files/130.mat && python3 mat2f32.py 130.mat X130_DE_time > /dev/null
gcc -O2 fplat.c -o fplat && { echo "$M"; ./fplat 0; ./fplat 1; } > ../out/c38b-fplat-$A.txt 2>&1
if [ "$A" = aarch64 ]; then
  gcc -O3 -fno-tree-vectorize widths.c -o w0 && gcc -O3 widths.c -o w1 && gcc -O3 -march=armv8.2-a+sve widths.c -o w2 && gcc -O3 -mcpu=neoverse-n2 widths.c -o w3
  { echo "$M"; ./w0 0 "vô hướng"; ./w1 0 "-O3 (NEON)"; ./w2 0 "-O3 -march=armv8.2-a+sve"; ./w3 0 "-O3 -mcpu=neoverse-n2"; } > ../out/c38b-widths-$A.txt 2>&1
  objdump -d --no-show-raw-insn w3 | awk '/<saxpy>:/,/^$/' > ../out/c38b-saxpy-n2-$A.txt
  gcc -O3 -fno-tree-vectorize -DSFX=_s -c conv_k.c -o c_s.o && gcc -O3 -DSFX=_v128 -c conv_k.c -o c_v128.o && gcc -O3 -mcpu=neoverse-n2 -DSFX=_vw -c conv_k.c -o c_vw.o && gcc -Ofast -mcpu=neoverse-n2 -DSFX=_fast -c conv_k.c -o c_fast.o
  gcc -O2 conv.c c_s.o c_v128.o c_vw.o c_fast.o -o conv -lm && { echo "$M (tự động rộng nhất = -O3 -mcpu=neoverse-n2)"; ./conv 0 X130_DE_time.f32; } > ../out/c38b-conv-n2-$A.txt 2>&1
  for f in conv_tapouter_vw conv_q15_vw conv_intr; do echo "## $f"; objdump -d --no-show-raw-insn conv | awk "/<$f>:/,/^\$/"; done > ../out/c38b-conv-dis-$A.txt
else
  gcc -O3 -fno-tree-vectorize widths.c -o w0 && gcc -O3 widths.c -o w1 && gcc -O3 -march=x86-64-v3 widths.c -o w2
  { echo "$M"; ./w0 0 "vô hướng"; ./w1 0 "-O3 (SSE2)"; ./w2 0 "-O3 -march=x86-64-v3"; } > ../out/c38b-widths-$A.txt 2>&1
  gcc -O2 -mavx2 align.c -o align && { echo "$M"; ./align 0; } > ../out/c38b-align-$A.txt 2>&1
  gcc -O2 dot8.c -o dot8 && { echo "$M"; ./dot8 0; } > ../out/c38b-dot8-$A.txt 2>&1
  gcc -O3 -fno-tree-vectorize -DSFX=_s -c conv_k.c -o c_s.o && gcc -O3 -DSFX=_v128 -c conv_k.c -o c_v128.o && gcc -O3 -march=x86-64-v3 -DSFX=_vw -c conv_k.c -o c_vw.o && gcc -Ofast -march=x86-64-v3 -DSFX=_fast -c conv_k.c -o c_fast.o
  gcc -O2 conv.c c_s.o c_v128.o c_vw.o c_fast.o -o conv -lm && { echo "$M"; ./conv 0 X130_DE_time.f32; } > ../out/c38b-conv-$A.txt 2>&1
  gcc -O2 -DSFX=_o2 -c kern_c.c -o k_o2.o && gcc -O3 -march=x86-64-v3 -DSFX=_v -c kern_c.c -o k_v.o && gcc -O3 -DSFX=_sse -c kern_c.c -o k_sse.o
  gcc -O3 -mavx2 intr.c k_o2.o k_v.o k_sse.o -o intr && { echo "$M"; ./intr 0; } > ../out/c38b-intr-$A.txt 2>&1
fi
cd ..
cat out/c38b-*-$A.txt | grep -v "^ *[0-9a-f]*:" | grep -v "^Flags" | head -120
