#!/bin/bash
# run4: tần suất MP phụ thuộc luồng nào giữ các dòng (ai đặt lại bộ nhớ)
set -u
mkdir -p out
A=$(uname -m)
cd src && gcc -O2 -Wall -pthread litmus.c -o litmus
{ for r in 0 1; do for t in "MP po" "MP dmbst+po" "MP po+dmbld" "MP dmbst+ctrl" "MP rel+po" "2+2W po" "S po" "R po"; do RESET=$r ./litmus $t 10000000 0 1; done; done; } > ../out/litmus-reset-$A.txt 2>&1
cd ..; cat out/*-$A.txt
