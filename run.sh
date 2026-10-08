#!/bin/bash
# run7 (C3.12): số học trên ARM thật (Neoverse N2) và x86 của runner — đối chiếu với QEMU và [i7]
set -u
mkdir -p out
A=$(uname -m); M=$(lscpu | grep -m1 "Model name" | sed 's/  */ /g')
{ echo "$M"; lscpu; gcc --version | head -1; ldd --version | head -1; } > out/c312-cpu-$A.txt
cd c312
( mkdir -p cwru && cd cwru && for n in 97 98 99 100 105 106 107 108 118 119 120 121 130 131 132 133; do
    v=$(printf "X%03d_DE_time" $n); curl -sfL -o $n.mat https://engineering.case.edu/sites/default/files/$n.mat && python3 ../mat2f32.py $n.mat $v > /dev/null; done )
gcc -O2 int/conv.c -o conv && gcc -O2 int/shift.c -o shift && gcc -O2 fp/fp754.c -o fp754 -lm && gcc -O2 fp/nan.c -o nan -lm
{ echo "$M"; echo "== conv"; ./conv; echo "== shift"; ./shift; echo "== fp754 (NaN)"; ./fp754 | grep -E "0/0|sqrtf|Inf - Inf|nanf"; echo "== nan -O2"; ./nan; } > ../out/c312-intfp-$A.txt 2>&1
{ echo "$M"; for f in "-O2" "-O2 -ffp-contract=off" "-O2 -march=native"; do gcc $f fp/fma.c -o fma_t -lm && echo "== gcc $f" && ./fma_t; done; } > ../out/c312-fma-$A.txt 2>&1
gcc -O2 fp/libm_probe.c -o libm_probe -lm && { echo "$M"; ./libm_probe ../out/c312-libm-$A.bin; } > ../out/c312-libm-$A.txt 2>&1
if [ "$A" = aarch64 ]; then gcc -O2 fp/denorm_a64.c -o denorm -lm; else gcc -O2 -mavx2 -mfma fp/denorm.c -o denorm -lm; fi
{ echo "$M"; ./denorm 1; } > ../out/c312-denorm-$A.txt 2>&1
gcc -O2 -fopenmp num/omp_sum.c -o omp_sum -lm && { echo "$M ($(nproc) CPU)"; ./omp_sum cwru; } > ../out/c312-omp-$A.txt 2>&1
cd ..
cat out/c312-*-$A.txt | head -150
