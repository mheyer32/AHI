#!/bin/bash

set -x


SQUIRT_HOST=${SQUIRT_HOST:=192.168.0.110}
SQUIRT_PATH=${SQUIRT_PATH:=~/squirt/build}
SQUIRT=${SQUIRT:=${SQUIRT_PATH}/squirt}
SQUIRT_EXEC=${SQUIRT_EXEC:=${SQUIRT_PATH}/squirt_exec}

squirt --dest boot:libs/picasso96 ${SQUIRT_HOST} $PWD/external/Prometheus/PrometheusCard/_bin/gcc/Prometheus.card
${SQUIRT} --dest boot:libs/ ${SQUIRT_HOST} $PWD/external/Prometheus/PromLib/_bin/gcc/prometheus.library
${SQUIRT} --dest boot:C/ ${SQUIRT_HOST} $PWD/external/Prometheus/PromLib/_bin/gcc/prometheus.exe
${SQUIRT} --dest boot:System/ ${SQUIRT_HOST} $PWD/_build/m68k-amigaos-ahiusr-6.0.zip

${SQUIRT_EXEC} ${SQUIRT_HOST} xadUnFile boot:system/m68k-amigaos-ahiusr-6.0.zip DEST RAM: OVERWRITE
${SQUIRT_EXEC} ${SQUIRT_HOST} copy RAM:m68k-amigaos-ahi/User/ Boot: ALL CLONE

