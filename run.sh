#!/bin/bash
# Mỗi lần đẩy: chạy thí nghiệm hiện tại, ghi vào out/
set -u
mkdir -p out
A=$(uname -m)
{
echo "== $(date -u) $A"; uname -a; nproc; lscpu; cat /proc/cpuinfo | head -30
for c in /sys/devices/system/cpu/cpu0/cache/index*; do echo "$c: L$(cat $c/level) $(cat $c/type) $(cat $c/size) ways=$(cat $c/ways_of_associativity 2>/dev/null) line=$(cat $c/coherency_line_size)"; done
cat /sys/devices/system/cpu/cpu*/regs/identification/midr_el1 2>/dev/null
gcc --version | head -1; free -m; cat /proc/meminfo | head -3
} > out/sysinfo-$A.txt 2>&1
gcc -O2 -Wall -pthread src/litmus.c -o litmus
objdump -d litmus | awk '/<(mp_w_po|mp_r_po|mp_r_addr|mp_r_ctrl|sb0_ra|sb0_rapc|lb0_data)>:/,/ret/' > out/litmus-dis-$A.txt
N=${N:-10000000}
for t in "MP po" "MP dmbst+po" "MP po+dmbld" "MP dmbst+dmbld" "MP dmb+dmb" "MP rel+acq" "MP rel+acqpc" "MP rel+po" "MP dmbst+addr" "MP dmbst+ctrl" "MP dmbst+ctrlisb" \
         "SB po" "SB dmb" "SB rel+acq" "SB rel+acqpc" "SB swp" "LB po" "LB data" "LB ctrl" "2+2W po" "2+2W dmbst" "S po" "S dmbst+data" "R po" "R dmb" "CoRR po"; do
  ./litmus $t $N 0 1
done > out/litmus-$A.txt 2>&1
./litmus IRIW po $N 0 1 2 3 >> out/litmus-$A.txt 2>&1
./litmus IRIW dmbld $N 0 1 2 3 >> out/litmus-$A.txt 2>&1
./litmus IRIW addr $N 0 1 2 3 >> out/litmus-$A.txt 2>&1
cat out/litmus-$A.txt
