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
Command line: /tmp/claude-1000/-mnt-d-Projects-Huong-Dan/7ab31724-8eb3-4dae-bb84-ff7ccc681a3e/scratchpad/opam/herd/bin/litmus7 -a 4 -s 10k -r 1000 -affinity incr1 -o /tmp/claude-1000/-mnt-d-Projects-Huong-Dan/7ab31724-8eb3-4dae-bb84-ff7ccc681a3e/scratchpad/l7a64 MP.litmus MP+dmb.st+dmb.ld.litmus MP+dmb.st+ctrl.litmus SB.litmus SB+rel+acq.litmus SB+rel+acqpc.litmus LB.litmus S.litmus 2+2W.litmus R.litmus IRIW.litmus CoRR.litmus
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
