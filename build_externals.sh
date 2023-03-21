#!/bin/sh 

#echo on
set -x

# makeinfo in guise of mkguide
make -C ./external/mkguide/src all

# flexcat
make -C ./external/flexcat bootstrap OS=unix
PATH=${PATH}:$(pwd)/external/flexcat/src/bin_unix
make -C ./external/flexcat OS=unix bin_unix/flexcat

#robodoc
make -C ./external/robodoc

#OpenPCI
make -C ./external/OpenPCI -f makefile_m68k

#Prometheus.card
make -C ./external/Prometheus/PrometheusCard -f makefiles/gcc/makefile
cp ./external/Prometheus/PrometheusCard/_bin/gcc/Prometheus.card /media/sf_Amiga/

#Prometheus.library / exe
make -C ./external/Prometheus/PromLib -f makefiles/gcc/makefile
cp ./external/Prometheus/PromLib/_bin/gcc/prometheus.library /media/sf_Amiga/
cp ./external/Prometheus/PromLib/_bin/gcc/prometheus.exe /media/sf_Amiga/
