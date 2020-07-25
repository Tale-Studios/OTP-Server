@echo off
win_flex --wincompat -Pdnayy -o dna_lpp.cpp dna.lpp
win_bison -y -d -p dnayy -o dna_ypp.cpp dna.ypp
pause