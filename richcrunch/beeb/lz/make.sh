#! /bin/sh
beebasm -i main_lz.6502 -do lz.ssd -v -opt 0 -title LZ > lz.txt && \
b2 -0 lz.ssd
