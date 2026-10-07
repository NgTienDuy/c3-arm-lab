GCC=gcc
GCCOPTS=-D_GNU_SOURCE -Wall -std=gnu99 -O2 -pthread
LINKOPTS=
SRC=\
 MP.c\
 MP+dmb.st+po.c\
 MP+po+dmb.ld.c\
 MP+dmb.st+dmb.ld.c\
 MP+dmbs.c\
 MP+rel+acq.c\
 MP+rel+acqpc.c\
 MP+rel+po.c\
 MP+dmb.st+addr.c\
 MP+dmb.st+ctrl.c\
 MP+dmb.st+ctrlisb.c\
 SB.c\
 SB+dmbs.c\
 SB+rel+acq.c\
 SB+rel+acqpc.c\
 SB+swpal.c\
 LB.c\
 LB+datas.c\
 LB+ctrls.c\
 2+2W.c\
 2+2W+dmb.sts.c\
 S.c\
 S+dmb.st+data.c\
 R.c\
 R+dmbs.c\
 IRIW.c\
 IRIW+dmb.lds.c\
 IRIW+addrs.c\
 CoRR.c\

EXE=$(SRC:.c=.exe)
T=$(SRC:.c=.t)

all: $(EXE) $(T)

clean:
	/bin/rm -f *.o *.s *.t *.exe *~

cleansource:
	/bin/rm -f *.o *.c *.h *.s *~

affinity.o: affinity.c
	$(GCC) $(GCCOPTS) -O2 -c affinity.c

outs.o: outs.c
	$(GCC) $(GCCOPTS) -O2 -c outs.c

utils.o: utils.c
	$(GCC) $(GCCOPTS) -O2 -c utils.c

litmus_rand.o: litmus_rand.c
	$(GCC) $(GCCOPTS) -O2 -c litmus_rand.c

UTILS=affinity.o outs.o utils.o litmus_rand.o

%.exe:%.s $(UTILS)
	$(GCC) $(GCCOPTS) $(LINKOPTS) -o $@ $(UTILS) $<

%.s:%.c
	$(GCC) $(GCCOPTS) -S $<

%.t:%.s
	awk -f show.awk $< > $@

