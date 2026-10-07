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
$GCC $GCCOPTS $LINKOPTS -o MP+dmb.st+po.exe affinity.o outs.o utils.o litmus_rand.o MP+dmb.st+po.c
$GCC $GCCOPTS -S MP+dmb.st+po.c && awk -f show.awk MP+dmb.st+po.s > MP+dmb.st+po.t && /bin/rm MP+dmb.st+po.s
$GCC $GCCOPTS $LINKOPTS -o MP+po+dmb.ld.exe affinity.o outs.o utils.o litmus_rand.o MP+po+dmb.ld.c
$GCC $GCCOPTS -S MP+po+dmb.ld.c && awk -f show.awk MP+po+dmb.ld.s > MP+po+dmb.ld.t && /bin/rm MP+po+dmb.ld.s
$GCC $GCCOPTS $LINKOPTS -o MP+dmb.st+dmb.ld.exe affinity.o outs.o utils.o litmus_rand.o MP+dmb.st+dmb.ld.c
$GCC $GCCOPTS -S MP+dmb.st+dmb.ld.c && awk -f show.awk MP+dmb.st+dmb.ld.s > MP+dmb.st+dmb.ld.t && /bin/rm MP+dmb.st+dmb.ld.s
$GCC $GCCOPTS $LINKOPTS -o MP+dmbs.exe affinity.o outs.o utils.o litmus_rand.o MP+dmbs.c
$GCC $GCCOPTS -S MP+dmbs.c && awk -f show.awk MP+dmbs.s > MP+dmbs.t && /bin/rm MP+dmbs.s
$GCC $GCCOPTS $LINKOPTS -o MP+rel+acq.exe affinity.o outs.o utils.o litmus_rand.o MP+rel+acq.c
$GCC $GCCOPTS -S MP+rel+acq.c && awk -f show.awk MP+rel+acq.s > MP+rel+acq.t && /bin/rm MP+rel+acq.s
$GCC $GCCOPTS $LINKOPTS -o MP+rel+acqpc.exe affinity.o outs.o utils.o litmus_rand.o MP+rel+acqpc.c
$GCC $GCCOPTS -S MP+rel+acqpc.c && awk -f show.awk MP+rel+acqpc.s > MP+rel+acqpc.t && /bin/rm MP+rel+acqpc.s
$GCC $GCCOPTS $LINKOPTS -o MP+rel+po.exe affinity.o outs.o utils.o litmus_rand.o MP+rel+po.c
$GCC $GCCOPTS -S MP+rel+po.c && awk -f show.awk MP+rel+po.s > MP+rel+po.t && /bin/rm MP+rel+po.s
$GCC $GCCOPTS $LINKOPTS -o MP+dmb.st+addr.exe affinity.o outs.o utils.o litmus_rand.o MP+dmb.st+addr.c
$GCC $GCCOPTS -S MP+dmb.st+addr.c && awk -f show.awk MP+dmb.st+addr.s > MP+dmb.st+addr.t && /bin/rm MP+dmb.st+addr.s
$GCC $GCCOPTS $LINKOPTS -o MP+dmb.st+ctrl.exe affinity.o outs.o utils.o litmus_rand.o MP+dmb.st+ctrl.c
$GCC $GCCOPTS -S MP+dmb.st+ctrl.c && awk -f show.awk MP+dmb.st+ctrl.s > MP+dmb.st+ctrl.t && /bin/rm MP+dmb.st+ctrl.s
$GCC $GCCOPTS $LINKOPTS -o MP+dmb.st+ctrlisb.exe affinity.o outs.o utils.o litmus_rand.o MP+dmb.st+ctrlisb.c
$GCC $GCCOPTS -S MP+dmb.st+ctrlisb.c && awk -f show.awk MP+dmb.st+ctrlisb.s > MP+dmb.st+ctrlisb.t && /bin/rm MP+dmb.st+ctrlisb.s
$GCC $GCCOPTS $LINKOPTS -o SB.exe affinity.o outs.o utils.o litmus_rand.o SB.c
$GCC $GCCOPTS -S SB.c && awk -f show.awk SB.s > SB.t && /bin/rm SB.s
$GCC $GCCOPTS $LINKOPTS -o SB+dmbs.exe affinity.o outs.o utils.o litmus_rand.o SB+dmbs.c
$GCC $GCCOPTS -S SB+dmbs.c && awk -f show.awk SB+dmbs.s > SB+dmbs.t && /bin/rm SB+dmbs.s
$GCC $GCCOPTS $LINKOPTS -o SB+rel+acq.exe affinity.o outs.o utils.o litmus_rand.o SB+rel+acq.c
$GCC $GCCOPTS -S SB+rel+acq.c && awk -f show.awk SB+rel+acq.s > SB+rel+acq.t && /bin/rm SB+rel+acq.s
$GCC $GCCOPTS $LINKOPTS -o SB+rel+acqpc.exe affinity.o outs.o utils.o litmus_rand.o SB+rel+acqpc.c
$GCC $GCCOPTS -S SB+rel+acqpc.c && awk -f show.awk SB+rel+acqpc.s > SB+rel+acqpc.t && /bin/rm SB+rel+acqpc.s
$GCC $GCCOPTS $LINKOPTS -o SB+swpal.exe affinity.o outs.o utils.o litmus_rand.o SB+swpal.c
$GCC $GCCOPTS -S SB+swpal.c && awk -f show.awk SB+swpal.s > SB+swpal.t && /bin/rm SB+swpal.s
$GCC $GCCOPTS $LINKOPTS -o LB.exe affinity.o outs.o utils.o litmus_rand.o LB.c
$GCC $GCCOPTS -S LB.c && awk -f show.awk LB.s > LB.t && /bin/rm LB.s
$GCC $GCCOPTS $LINKOPTS -o LB+datas.exe affinity.o outs.o utils.o litmus_rand.o LB+datas.c
$GCC $GCCOPTS -S LB+datas.c && awk -f show.awk LB+datas.s > LB+datas.t && /bin/rm LB+datas.s
$GCC $GCCOPTS $LINKOPTS -o LB+ctrls.exe affinity.o outs.o utils.o litmus_rand.o LB+ctrls.c
$GCC $GCCOPTS -S LB+ctrls.c && awk -f show.awk LB+ctrls.s > LB+ctrls.t && /bin/rm LB+ctrls.s
$GCC $GCCOPTS $LINKOPTS -o 2+2W.exe affinity.o outs.o utils.o litmus_rand.o 2+2W.c
$GCC $GCCOPTS -S 2+2W.c && awk -f show.awk 2+2W.s > 2+2W.t && /bin/rm 2+2W.s
$GCC $GCCOPTS $LINKOPTS -o 2+2W+dmb.sts.exe affinity.o outs.o utils.o litmus_rand.o 2+2W+dmb.sts.c
$GCC $GCCOPTS -S 2+2W+dmb.sts.c && awk -f show.awk 2+2W+dmb.sts.s > 2+2W+dmb.sts.t && /bin/rm 2+2W+dmb.sts.s
$GCC $GCCOPTS $LINKOPTS -o S.exe affinity.o outs.o utils.o litmus_rand.o S.c
$GCC $GCCOPTS -S S.c && awk -f show.awk S.s > S.t && /bin/rm S.s
$GCC $GCCOPTS $LINKOPTS -o S+dmb.st+data.exe affinity.o outs.o utils.o litmus_rand.o S+dmb.st+data.c
$GCC $GCCOPTS -S S+dmb.st+data.c && awk -f show.awk S+dmb.st+data.s > S+dmb.st+data.t && /bin/rm S+dmb.st+data.s
$GCC $GCCOPTS $LINKOPTS -o R.exe affinity.o outs.o utils.o litmus_rand.o R.c
$GCC $GCCOPTS -S R.c && awk -f show.awk R.s > R.t && /bin/rm R.s
$GCC $GCCOPTS $LINKOPTS -o R+dmbs.exe affinity.o outs.o utils.o litmus_rand.o R+dmbs.c
$GCC $GCCOPTS -S R+dmbs.c && awk -f show.awk R+dmbs.s > R+dmbs.t && /bin/rm R+dmbs.s
$GCC $GCCOPTS $LINKOPTS -o IRIW.exe affinity.o outs.o utils.o litmus_rand.o IRIW.c
$GCC $GCCOPTS -S IRIW.c && awk -f show.awk IRIW.s > IRIW.t && /bin/rm IRIW.s
$GCC $GCCOPTS $LINKOPTS -o IRIW+dmb.lds.exe affinity.o outs.o utils.o litmus_rand.o IRIW+dmb.lds.c
$GCC $GCCOPTS -S IRIW+dmb.lds.c && awk -f show.awk IRIW+dmb.lds.s > IRIW+dmb.lds.t && /bin/rm IRIW+dmb.lds.s
$GCC $GCCOPTS $LINKOPTS -o IRIW+addrs.exe affinity.o outs.o utils.o litmus_rand.o IRIW+addrs.c
$GCC $GCCOPTS -S IRIW+addrs.c && awk -f show.awk IRIW+addrs.s > IRIW+addrs.t && /bin/rm IRIW+addrs.s
$GCC $GCCOPTS $LINKOPTS -o CoRR.exe affinity.o outs.o utils.o litmus_rand.o CoRR.c
$GCC $GCCOPTS -S CoRR.c && awk -f show.awk CoRR.s > CoRR.t && /bin/rm CoRR.s
