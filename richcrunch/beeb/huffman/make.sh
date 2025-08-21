#! /bin/sh
beebasm -i main_huffman.6502 -do huffman.ssd -v -opt 0 -title HUFFMAN > huffman.txt && \
b2 -0 huffman.ssd
