#!/bin/sh

# makeinfo in guise of mkguide
 make -C ./external/mkguide/src all -B

# flexcat
make -C ./external/flexcat bootstrap OS=unix
PATH=${PATH}:$(pwd)/external/flexcat/src/bin_unix
make -C ./external/flexcat OS=unix bin_unix/flexcat

#robodoc
make -C ./external/robodoc
