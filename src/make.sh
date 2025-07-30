#! /bin/sh
beebasm -i doom.6502 -do doom.ssd -v -opt 2 -title DOOM > list.txt && \
beebjit -rom B roms/DDoc1.09.rom -0 doom.ssd -writeable -mutable -autoboot

