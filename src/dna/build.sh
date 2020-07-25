#!/bin/bash

flex -Pdnayy -o dna_lpp.cpp dna.lpp
bison -y -d -p dnayy -o dna_ypp.cpp dna.ypp
