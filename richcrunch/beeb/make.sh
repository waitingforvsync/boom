#! /bin/sh
beebasm -i main.6502 -do crunch.ssd -v -opt 0 -title CRUNCH > list.txt && \
b2 -0 crunch.ssd

