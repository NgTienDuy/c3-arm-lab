#!/bin/bash
# run9: C3.14 — khảo sát thêm trên runner ARM (PMU thật) và x86 (không PMU): metric top-down, SPE, dao động giữa các lần chạy
set -x
mkdir -p out; A=$(uname -m)
sudo apt-get update -qq >/dev/null 2>&1
sudo apt-get install -y -qq linux-tools-common linux-tools-$(uname -r) >/dev/null 2>&1 || true
sudo sysctl -w kernel.perf_event_paranoid=-1 >/dev/null
cat > /tmp/loop.c <<'C'
#include <stdio.h>
#include <stdlib.h>
int main(int c, char **v) { long n = atol(v[1]); volatile double s = 0; for (long i = 0; i < n; i++) s += i * 0.5; printf("%f\n", s); return 0; }
C
gcc -O2 -o /tmp/loop /tmp/loop.c
{
perf list --details 2>/dev/null | grep -c -E "^\s+[a-z0-9_]+\s+\[" 
perf list pmu 2>&1 | head -80
perf list metricgroup 2>&1 | head -40
perf list metric 2>&1 | head -80
ls /sys/bus/event_source/devices/
ls /sys/bus/event_source/devices/*/ 2>/dev/null | head -40
cat /sys/bus/event_source/devices/armv8_pmuv3_0/caps/slots 2>/dev/null
grep -m1 -i "cpu part\|CPU implementer" /proc/cpuinfo; grep -m3 -i "cpu part\|CPU implementer\|CPU variant" /proc/cpuinfo
} > out/c314-list-$A.txt 2>&1
{
perf stat -M TopdownL1 -- /tmp/loop 300000000
perf stat --topdown -- /tmp/loop 300000000
perf stat -r 10 -e cycles,instructions -- /tmp/loop 100000000
perf stat -e cycles:pp -- /tmp/loop 10000000
perf record -e arm_spe// -o /tmp/spe.data -- /tmp/loop 10000000
} > out/c314-metrics-$A.txt 2>&1
cat out/*
