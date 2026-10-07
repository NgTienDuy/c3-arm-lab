#!/bin/sh
GCC=gcc
GCCOPTS="-D_GNU_SOURCE -Wall -std=gnu99 -O2 -pthread"
LINKOPTS=""
/bin/rm -f *.exe *.s
$GCC $GCCOPTS -O2 -c affinity.c
$GCC $GCCOPTS -O2 -c outs.c
$GCC $GCCOPTS -O2 -c utils.c
$GCC $GCCOPTS -O2 -c litmus_rand.c
$GCC $GCCOPTS $LINKOPTS -o MP.exe affinity.o outs.o utils.o litmus_rand.o MP.c
$GCC $GCCOPTS -S MP.c && awk -f show.awk MP.s > MP.t && /bin/rm MP.s
$GCC $GCCOPTS $LINKOPTS -o MP+dmb.st+dmb.ld.exe affinity.o outs.o utils.o litmus_rand.o MP+dmb.st+dmb.ld.c
$GCC $GCCOPTS -S MP+dmb.st+dmb.ld.c && awk -f show.awk MP+dmb.st+dmb.ld.s > MP+dmb.st+dmb.ld.t && /bin/rm MP+dmb.st+dmb.ld.s
$GCC $GCCOPTS $LINKOPTS -o MP+dmb.st+ctrl.exe affinity.o outs.o utils.o litmus_rand.o MP+dmb.st+ctrl.c
$GCC $GCCOPTS -S MP+dmb.st+ctrl.c && awk -f show.awk MP+dmb.st+ctrl.s > MP+dmb.st+ctrl.t && /bin/rm MP+dmb.st+ctrl.s
$GCC $GCCOPTS $LINKOPTS -o SB.exe affinity.o outs.o utils.o litmus_rand.o SB.c
$GCC $GCCOPTS -S SB.c && awk -f show.awk SB.s > SB.t && /bin/rm SB.s
$GCC $GCCOPTS $LINKOPTS -o SB+rel+acq.exe affinity.o outs.o utils.o litmus_rand.o SB+rel+acq.c
$GCC $GCCOPTS -S SB+rel+acq.c && awk -f show.awk SB+rel+acq.s > SB+rel+acq.t && /bin/rm SB+rel+acq.s
$GCC $GCCOPTS $LINKOPTS -o SB+rel+acqpc.exe affinity.o outs.o utils.o litmus_rand.o SB+rel+acqpc.c
$GCC $GCCOPTS -S SB+rel+acqpc.c && awk -f show.awk SB+rel+acqpc.s > SB+rel+acqpc.t && /bin/rm SB+rel+acqpc.s
$GCC $GCCOPTS $LINKOPTS -o LB.exe affinity.o outs.o utils.o litmus_rand.o LB.c
$GCC $GCCOPTS -S LB.c && awk -f show.awk LB.s > LB.t && /bin/rm LB.s
$GCC $GCCOPTS $LINKOPTS -o S.exe affinity.o outs.o utils.o litmus_rand.o S.c
$GCC $GCCOPTS -S S.c && awk -f show.awk S.s > S.t && /bin/rm S.s
$GCC $GCCOPTS $LINKOPTS -o 2+2W.exe affinity.o outs.o utils.o litmus_rand.o 2+2W.c
$GCC $GCCOPTS -S 2+2W.c && awk -f show.awk 2+2W.s > 2+2W.t && /bin/rm 2+2W.s
$GCC $GCCOPTS $LINKOPTS -o R.exe affinity.o outs.o utils.o litmus_rand.o R.c
$GCC $GCCOPTS -S R.c && awk -f show.awk R.s > R.t && /bin/rm R.s
$GCC $GCCOPTS $LINKOPTS -o IRIW.exe affinity.o outs.o utils.o litmus_rand.o IRIW.c
$GCC $GCCOPTS -S IRIW.c && awk -f show.awk IRIW.s > IRIW.t && /bin/rm IRIW.s
$GCC $GCCOPTS $LINKOPTS -o CoRR.exe affinity.o outs.o utils.o litmus_rand.o CoRR.c
$GCC $GCCOPTS -S CoRR.c && awk -f show.awk CoRR.s > CoRR.t && /bin/rm CoRR.s
