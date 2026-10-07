#!/bin/bash
# run2: coh, fencecost, smc, ring, litmus dài, litmus7, ngôn ngữ
set -u
mkdir -p out
A=$(uname -m)
log() { echo "== $(date -u +%H:%M:%S) $*"; }
cd src
gcc -O2 -Wall -pthread coh.c -o coh && gcc -O2 -Wall fencecost.c -o fencecost && gcc -O2 -Wall -pthread litmus.c -o litmus
for v in 0 1 2 3 4 5 6 7; do gcc -O2 -Wall -pthread -DV=$v ring.c -o ring$v; done
for v in 0 2 3 4; do objdump -d --no-show-raw-insn ring$v | awk '/<producer>:/,/^$/' > ../out/ring$v-producer-$A.txt; objdump -d --no-show-raw-insn ring$v | awk '/<main>:/,/^$/' > ../out/ring$v-main-$A.txt; done
log coh;  { ./coh 0 1 2 256 201; ./coh 0 2 3 256 201; ./coh 1 3 0 256 201; } > ../out/coh-$A.txt 2>&1
log fencecost; { ./fencecost 0; ./fencecost 1; } > ../out/fencecost-$A.txt 2>&1
if [ "$A" = aarch64 ]; then
  gcc -O2 -Wall smc.c -o smc && log smc && { ./smc 1000000; ./smc 1000000; } > ../out/smc-$A.txt 2>&1
  gcc -O2 -static smc.c -o smc_static && objdump -d --no-show-raw-insn smc_static | awk '/<__aarch64_sync_cache_range>:/,/^$/' > ../out/sync_cache_range-$A.txt
fi
log ring
{ for v in 0 1 2 3 4 5 6 7; do ./ring$v 100000000 0 1; done
  ./ring0 100000000 1 2; ./ring0 100000000 2 3; } > ../out/ring-$A.txt 2>&1
log litmus-long
{ for t in "LB po" "S po" "2+2W po" "MP rel+po" "MP po+dmbld"; do ./litmus $t 100000000 0 1; done
  ./litmus IRIW po 100000000 0 1 2 3
  if [ "$A" = x86_64 ]; then for v in lockadd sfence lfence; do ./litmus SB $v 10000000 0 1; done; fi
} > ../out/litmus-long-$A.txt 2>&1
cd ..
if [ "$A" = aarch64 ]; then log litmus7; (cd l7 && make -j4 > /dev/null 2>&1 && sh run.sh) > out/litmus7-$A.txt 2>&1; fi
log lang
cd src/lang
{ java -version 2>&1 | head -1; go version; rustc --version || true
  javac MP.java && for v in plain volatile relacq opaque; do java -XX:+UseParallelGC MP $v 10000000; done
  go build -o mpgo mp.go && for v in plain atomic; do ./mpgo $v 10000000; done
  if command -v rustc >/dev/null; then rustc -O mp.rs -o mprs && for v in relaxed relacq seqcst; do ./mprs $v 10000000; done; fi
} > ../../out/lang-$A.txt 2>&1
cd ../..
log done
cat out/*-$A.txt | head -400
