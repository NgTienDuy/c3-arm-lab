#!/bin/sh

date
LITMUSOPTS="${@:-$LITMUSOPTS}"
SLEEP=0
if [ ! -f MP.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | LDR W0,[X1] ;
 STR W0,[X1] | LDR W2,[X3] ;
 MOV W2,#1   |             ;
 STR W2,[X3] |             ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP.t
./MP.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+dmb.st+po.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+dmb.st+po.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+dmb.st+po

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | LDR W0,[X1] ;
 STR W0,[X1] | LDR W2,[X3] ;
 DMB ISHST   |             ;
 MOV W2,#1   |             ;
 STR W2,[X3] |             ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+dmb.st+po.t
./MP+dmb.st+po.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+po+dmb.ld.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+po+dmb.ld.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+po+dmb.ld

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | LDR W0,[X1] ;
 STR W0,[X1] | DMB ISHLD   ;
 MOV W2,#1   | LDR W2,[X3] ;
 STR W2,[X3] |             ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+po+dmb.ld.t
./MP+po+dmb.ld.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+dmb.st+dmb.ld.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+dmb.st+dmb.ld.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+dmb.st+dmb.ld

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | LDR W0,[X1] ;
 STR W0,[X1] | DMB ISHLD   ;
 DMB ISHST   | LDR W2,[X3] ;
 MOV W2,#1   |             ;
 STR W2,[X3] |             ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+dmb.st+dmb.ld.t
./MP+dmb.st+dmb.ld.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+dmbs.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+dmbs.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+dmbs

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | LDR W0,[X1] ;
 STR W0,[X1] | DMB ISH     ;
 DMB ISH     | LDR W2,[X3] ;
 MOV W2,#1   |             ;
 STR W2,[X3] |             ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+dmbs.t
./MP+dmbs.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+rel+acq.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+rel+acq.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+rel+acq

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0           | P1           ;
 MOV W0,#1    | LDAR W0,[X1] ;
 STR W0,[X1]  | LDR W2,[X3]  ;
 MOV W2,#1    |              ;
 STLR W2,[X3] |              ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+rel+acq.t
./MP+rel+acq.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+rel+acqpc.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+rel+acqpc.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+rel+acqpc

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0           | P1            ;
 MOV W0,#1    | LDAPR W0,[X1] ;
 STR W0,[X1]  | LDR W2,[X3]   ;
 MOV W2,#1    |               ;
 STLR W2,[X3] |               ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+rel+acqpc.t
./MP+rel+acqpc.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+rel+po.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+rel+po.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+rel+po

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0           | P1          ;
 MOV W0,#1    | LDR W0,[X1] ;
 STR W0,[X1]  | LDR W2,[X3] ;
 MOV W2,#1    |             ;
 STLR W2,[X3] |             ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+rel+po.t
./MP+rel+po.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+dmb.st+addr.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+dmb.st+addr.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+dmb.st+addr

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1                  ;
 MOV W0,#1   | LDR W0,[X1]         ;
 STR W0,[X1] | EOR W4,W0,W0        ;
 DMB ISHST   | LDR W2,[X3,W4,SXTW] ;
 MOV W2,#1   |                     ;
 STR W2,[X3] |                     ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+dmb.st+addr.t
./MP+dmb.st+addr.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+dmb.st+ctrl.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+dmb.st+ctrl.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+dmb.st+ctrl

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1           ;
 MOV W0,#1   | LDR W0,[X1]  ;
 STR W0,[X1] | CBNZ W0,LC00 ;
 DMB ISHST   | LC00:        ;
 MOV W2,#1   | LDR W2,[X3]  ;
 STR W2,[X3] |              ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+dmb.st+ctrl.t
./MP+dmb.st+ctrl.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f MP+dmb.st+ctrlisb.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for MP+dmb.st+ctrlisb.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 MP+dmb.st+ctrlisb

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1           ;
 MOV W0,#1   | LDR W0,[X1]  ;
 STR W0,[X1] | CBNZ W0,LC01 ;
 DMB ISHST   | LC01:        ;
 MOV W2,#1   | ISB          ;
 STR W2,[X3] | LDR W2,[X3]  ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat MP+dmb.st+ctrlisb.t
./MP+dmb.st+ctrlisb.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f SB.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for SB.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 SB

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | MOV W0,#1   ;
 STR W0,[X1] | STR W0,[X1] ;
 LDR W2,[X3] | LDR W2,[X3] ;

exists (0:X2=0 /\ 1:X2=0)
Generated assembler
EOF
cat SB.t
./SB.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f SB+dmbs.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for SB+dmbs.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 SB+dmbs

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | MOV W0,#1   ;
 STR W0,[X1] | STR W0,[X1] ;
 DMB ISH     | DMB ISH     ;
 LDR W2,[X3] | LDR W2,[X3] ;

exists (0:X2=0 /\ 1:X2=0)
Generated assembler
EOF
cat SB+dmbs.t
./SB+dmbs.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f SB+rel+acq.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for SB+rel+acq.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 SB+rel+acq

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0           | P1           ;
 MOV W0,#1    | MOV W0,#1    ;
 STLR W0,[X1] | STLR W0,[X1] ;
 LDAR W2,[X3] | LDAR W2,[X3] ;

exists (0:X2=0 /\ 1:X2=0)
Generated assembler
EOF
cat SB+rel+acq.t
./SB+rel+acq.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f SB+rel+acqpc.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for SB+rel+acqpc.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 SB+rel+acqpc

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0            | P1            ;
 MOV W0,#1     | MOV W0,#1     ;
 STLR W0,[X1]  | STLR W0,[X1]  ;
 LDAPR W2,[X3] | LDAPR W2,[X3] ;

exists (0:X2=0 /\ 1:X2=0)
Generated assembler
EOF
cat SB+rel+acqpc.t
./SB+rel+acqpc.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f SB+swpal.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for SB+swpal.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 SB+swpal

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0               | P1               ;
 MOV W0,#1        | MOV W0,#1        ;
 SWPAL W0,W5,[X1] | SWPAL W0,W5,[X1] ;
 LDR W2,[X3]      | LDR W2,[X3]      ;

exists (0:X2=0 /\ 1:X2=0)
Generated assembler
EOF
cat SB+swpal.t
./SB+swpal.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f LB.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for LB.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 LB

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 LDR W0,[X1] | LDR W0,[X1] ;
 MOV W2,#1   | MOV W2,#1   ;
 STR W2,[X3] | STR W2,[X3] ;

exists (0:X0=1 /\ 1:X0=1)
Generated assembler
EOF
cat LB.t
./LB.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f LB+datas.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for LB+datas.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 LB+datas

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0           | P1           ;
 LDR W0,[X1]  | LDR W0,[X1]  ;
 EOR W2,W0,W0 | EOR W2,W0,W0 ;
 ADD W2,W2,#1 | ADD W2,W2,#1 ;
 STR W2,[X3]  | STR W2,[X3]  ;

exists (0:X0=1 /\ 1:X0=1)
Generated assembler
EOF
cat LB+datas.t
./LB+datas.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f LB+ctrls.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for LB+ctrls.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 LB+ctrls

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0           | P1           ;
 LDR W0,[X1]  | LDR W0,[X1]  ;
 CBNZ W0,LC02 | CBNZ W0,LC03 ;
 LC02:        | LC03:        ;
 MOV W2,#1    | MOV W2,#1    ;
 STR W2,[X3]  | STR W2,[X3]  ;

exists (0:X0=1 /\ 1:X0=1)
Generated assembler
EOF
cat LB+ctrls.t
./LB+ctrls.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f 2+2W.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for 2+2W.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 2+2W

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | MOV W0,#1   ;
 STR W0,[X1] | STR W0,[X1] ;
 MOV W2,#2   | MOV W2,#2   ;
 STR W2,[X3] | STR W2,[X3] ;

exists ([x]=1 /\ [y]=1)
Generated assembler
EOF
cat 2+2W.t
./2+2W.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f 2+2W+dmb.sts.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for 2+2W+dmb.sts.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 2+2W+dmb.sts

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | MOV W0,#1   ;
 STR W0,[X1] | STR W0,[X1] ;
 DMB ISHST   | DMB ISHST   ;
 MOV W2,#2   | MOV W2,#2   ;
 STR W2,[X3] | STR W2,[X3] ;

exists ([x]=1 /\ [y]=1)
Generated assembler
EOF
cat 2+2W+dmb.sts.t
./2+2W+dmb.sts.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f S.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%
% Results for S.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 S

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#2   | LDR W0,[X1] ;
 STR W0,[X1] | MOV W2,#1   ;
 MOV W2,#1   | STR W2,[X3] ;
 STR W2,[X3] |             ;

exists (1:X0=1 /\ [x]=2)
Generated assembler
EOF
cat S.t
./S.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f S+dmb.st+data.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for S+dmb.st+data.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 S+dmb.st+data

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1           ;
 MOV W0,#2   | LDR W0,[X1]  ;
 STR W0,[X1] | EOR W2,W0,W0 ;
 DMB ISHST   | ADD W2,W2,#1 ;
 MOV W2,#1   | STR W2,[X3]  ;
 STR W2,[X3] |              ;

exists (1:X0=1 /\ [x]=2)
Generated assembler
EOF
cat S+dmb.st+data.t
./S+dmb.st+data.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f R.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%
% Results for R.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 R

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | MOV W0,#2   ;
 STR W0,[X1] | STR W0,[X1] ;
 MOV W2,#1   | LDR W2,[X3] ;
 STR W2,[X3] |             ;

exists ([y]=2 /\ 1:X2=0)
Generated assembler
EOF
cat R.t
./R.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f R+dmbs.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for R+dmbs.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 R+dmbs

{
 0:X1=x; 0:X3=y;
 1:X1=y; 1:X3=x;
}
 P0          | P1          ;
 MOV W0,#1   | MOV W0,#2   ;
 STR W0,[X1] | STR W0,[X1] ;
 DMB ISH     | DMB ISH     ;
 MOV W2,#1   | LDR W2,[X3] ;
 STR W2,[X3] |             ;

exists ([y]=2 /\ 1:X2=0)
Generated assembler
EOF
cat R+dmbs.t
./R+dmbs.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f IRIW.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for IRIW.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 IRIW

{
 0:X1=x;
 1:X1=y;
 2:X1=x; 2:X3=y;
 3:X1=y; 3:X3=x;
}
 P0          | P1          | P2          | P3          ;
 MOV W0,#1   | MOV W0,#1   | LDR W0,[X1] | LDR W0,[X1] ;
 STR W0,[X1] | STR W0,[X1] | LDR W2,[X3] | LDR W2,[X3] ;

exists (2:X0=1 /\ 2:X2=0 /\ 3:X0=1 /\ 3:X2=0)
Generated assembler
EOF
cat IRIW.t
./IRIW.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f IRIW+dmb.lds.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for IRIW+dmb.lds.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 IRIW+dmb.lds

{
 0:X1=x;
 1:X1=y;
 2:X1=x; 2:X3=y;
 3:X1=y; 3:X3=x;
}
 P0          | P1          | P2          | P3          ;
 MOV W0,#1   | MOV W0,#1   | LDR W0,[X1] | LDR W0,[X1] ;
 STR W0,[X1] | STR W0,[X1] | DMB ISHLD   | DMB ISHLD   ;
             |             | LDR W2,[X3] | LDR W2,[X3] ;

exists (2:X0=1 /\ 2:X2=0 /\ 3:X0=1 /\ 3:X2=0)
Generated assembler
EOF
cat IRIW+dmb.lds.t
./IRIW+dmb.lds.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f IRIW+addrs.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for IRIW+addrs.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 IRIW+addrs

{
 0:X1=x;
 1:X1=y;
 2:X1=x; 2:X3=y;
 3:X1=y; 3:X3=x;
}
 P0          | P1          | P2                  | P3                  ;
 MOV W0,#1   | MOV W0,#1   | LDR W0,[X1]         | LDR W0,[X1]         ;
 STR W0,[X1] | STR W0,[X1] | EOR W4,W0,W0        | EOR W4,W0,W0        ;
             |             | LDR W2,[X3,W4,SXTW] | LDR W2,[X3,W4,SXTW] ;

exists (2:X0=1 /\ 2:X2=0 /\ 3:X0=1 /\ 3:X2=0)
Generated assembler
EOF
cat IRIW+addrs.t
./IRIW+addrs.exe -q $LITMUSOPTS
fi
sleep $SLEEP

if [ ! -f CoRR.no ]; then
cat <<'EOF'
%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Results for CoRR.litmus %
%%%%%%%%%%%%%%%%%%%%%%%%%%%
AArch64 CoRR

{
 0:X1=x;
 1:X1=x;
}
 P0          | P1          ;
 MOV W0,#1   | LDR W0,[X1] ;
 STR W0,[X1] | LDR W2,[X1] ;

exists (1:X0=1 /\ 1:X2=0)
Generated assembler
EOF
cat CoRR.t
./CoRR.exe -q $LITMUSOPTS
fi
sleep $SLEEP

cat <<'EOF'
Revision exported, version 7.58
Command line: /tmp/claude-1000/-mnt-d-Projects-Huong-Dan/7ab31724-8eb3-4dae-bb84-ff7ccc681a3e/scratchpad/opam/herd/bin/litmus7 -a 4 -s 10k -r 1000 -affinity incr1 -o /tmp/claude-1000/-mnt-d-Projects-Huong-Dan/7ab31724-8eb3-4dae-bb84-ff7ccc681a3e/scratchpad/l7a64 MP.litmus MP+dmb.st+po.litmus MP+po+dmb.ld.litmus MP+dmb.st+dmb.ld.litmus MP+dmbs.litmus MP+rel+acq.litmus MP+rel+acqpc.litmus MP+rel+po.litmus MP+dmb.st+addr.litmus MP+dmb.st+ctrl.litmus MP+dmb.st+ctrlisb.litmus SB.litmus SB+dmbs.litmus SB+rel+acq.litmus SB+rel+acqpc.litmus SB+swpal.litmus LB.litmus LB+datas.litmus LB+ctrls.litmus 2+2W.litmus 2+2W+dmb.sts.litmus S.litmus S+dmb.st+data.litmus R.litmus R+dmbs.litmus IRIW.litmus IRIW+dmb.lds.litmus IRIW+addrs.litmus CoRR.litmus
Parameters
#define SIZE_OF_TEST 10000
#define NUMBER_OF_RUN 1000
#define AVAIL 4
#define STRIDE (-1)
#define MAX_LOOP 0
/* gcc options: -D_GNU_SOURCE -Wall -std=gnu99 -O2 -pthread */
/* barrier: user */
/* launch: changing */
/* affinity: incr1 */
/* memory: direct */
/* safer: write */
/* preload: random */
/* speedcheck: no */
/* alloc: dynamic */
/* proc used: 4 */
EOF
sed '2q;d' comp.sh
echo "LITMUSOPTS=$LITMUSOPTS"
date
