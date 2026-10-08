#!/bin/bash
# run8: C3.14 — khảo sát PMU trên runner: perf có đọc được bộ đếm phần cứng không?
set -x
mkdir -p out
{
uname -a; lscpu | head -25
cat /proc/sys/kernel/perf_event_paranoid
ls /sys/bus/event_source/devices/
sudo dmesg 2>/dev/null | grep -i -E "pmu|perf|PMUv3" | head -20
grep -o -w -E "arch_perfmon|pdcm|perfmon" /proc/cpuinfo | sort | uniq -c
} > out/c314-probe-$(uname -m).txt 2>&1
sudo apt-get update -qq >/dev/null 2>&1
sudo apt-get install -y -qq linux-tools-common linux-tools-$(uname -r) >/dev/null 2>&1 || sudo apt-get install -y -qq linux-tools-azure >/dev/null 2>&1 || true
sudo sysctl -w kernel.perf_event_paranoid=-1
{
which perf; perf --version
perf list hw cache 2>&1 | head -40
cat > /tmp/loop.c <<'C'
#include <stdio.h>
#include <stdlib.h>
int main(int c, char **v) { long n = atol(v[1]); volatile double s = 0; for (long i = 0; i < n; i++) s += i * 0.5; printf("%f\n", s); return 0; }
C
gcc -O2 -o /tmp/loop /tmp/loop.c
perf stat -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses -- /tmp/loop 300000000
perf stat -e task-clock,context-switches,cpu-migrations,page-faults -- /tmp/loop 100000000
perf record -e cycles -o /tmp/p.data -- /tmp/loop 200000000 && perf report -i /tmp/p.data --stdio | head -20
} > out/c314-perf-$(uname -m).txt 2>&1
cat out/*
