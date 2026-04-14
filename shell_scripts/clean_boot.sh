#!/bin/bash
sed -i '/^	print("BOOT:/ { s/^/\/\* MINIMAL BOOT: /; s/$/ \*\//; }' kernel/9front-pc64/main.c
sed -i '/^	uartputs("BOOT:/ { s/^/\/\* MINIMAL BOOT: /; s/$/ \*\//; }' kernel/9front-pc64/main.c
sed -i '/^	iprint("BOOT\[init0\]:/ { s/^/\/\* MINIMAL BOOT: /; s/$/ \*\//; }' kernel/9front-pc64/main.c
sed -i '/^	print("BOOT\[init0\]:/ { s/^/\/\* MINIMAL BOOT: /; s/$/ \*\//; }' kernel/9front-pc64/main.c
