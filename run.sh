#!/bin/bash
# run3: smc sửa, coh LL/SC vs LSE, ring C11 với ldapr, đột biến dài, IRIW có rào 10^8, litmus7 đầy đủ
set -u
mkdir -p out
A=$(uname -m)
log() { echo "== $(date -u +%H:%M:%S) $*"; }
cd src
gcc -O2 -Wall -pthread litmus.c -o litmus
for v in 0 3 5 6 7; do gcc -O2 -Wall -pthread -DV=$v ring.c -o ring$v; done
if [ "$A" = aarch64 ]; then
  gcc -O2 -Wall smc.c -o smc && log smc && { ./smc 1000000; ./smc 1000000; } > ../out/smc-$A.txt 2>&1
  gcc -O2 -static smc.c -o smc_static && objdump -d --no-show-raw-insn smc_static | awk '/<__aarch64_sync_cache_range>:/,/^$/' > ../out/sync_cache_range-$A.txt
  gcc -O2 -pthread -march=armv8.1-a coh.c -o coh_lse && gcc -O2 -pthread -march=armv8-a -mno-outline-atomics coh.c -o coh_llsc
  objdump -d --no-show-raw-insn coh_lse | awk '/<chase_rmw>:/,/^$/' > ../out/coh_lse-rmw-$A.txt
  objdump -d --no-show-raw-insn coh_llsc | awk '/<chase_rmw>:/,/^$/' > ../out/coh_llsc-rmw-$A.txt
  log coh; { ./coh_lse 0 1 2 256 201; ./coh_llsc 0 1 2 256 201; ./coh_lse 0 2 3 256 201; ./coh_llsc 0 2 3 256 201; } > ../out/coh-llsc-$A.txt 2>&1
  gcc -O2 -Wall -pthread -DV=3 -mcpu=neoverse-n2 ring.c -o ring3n2
  objdump -d --no-show-raw-insn ring3n2 | awk '/<producer>:/,/^$/' > ../out/ring3n2-producer-$A.txt
  objdump -d --no-show-raw-insn ring3n2 | awk '/<main>:/,/^$/' > ../out/ring3n2-main-$A.txt
  log ring
  { ./ring3 100000000 0 1; ./ring3n2 100000000 0 1; ./ring3n2 100000000 2 3
    for p in "0 1" "1 2" "2 3" "3 0"; do ./ring0 100000000 $p; done
    ./ring5 1000000000 2 3; ./ring6 100000000 2 3; ./ring7 1000000000 2 3; } > ../out/ring3-$A.txt 2>&1
  log litmus
  { ./litmus IRIW dmbld 100000000 0 1 2 3; ./litmus IRIW addr 100000000 0 1 2 3; ./litmus LB data 100000000 0 1; } > ../out/litmus3-$A.txt 2>&1
  cd ..; log litmus7
  (cd l7 && make GCCOPTS="-D_GNU_SOURCE -Wall -std=gnu99 -O2 -pthread -march=armv8.4-a" -j4 > ../out/litmus7-build.txt 2>&1 && sh run.sh) > out/litmus7-$A.txt 2>&1
else
  log ring; { ./ring0 1000000000 0 2; ./ring3 100000000 0 2; } > ../out/ring3-$A.txt 2>&1
  cd ..
fi
log done
cat out/*-$A.txt | head -300
