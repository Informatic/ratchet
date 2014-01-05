#!/bin/sh
# $ sh build-baudtable.h > baudtable.h

echo "#ifndef __RATCHET_BAUDTABLE_H"
echo "#define __RATCHET_BAUDTABLE_H"
echo ""
echo "#include <termios.h>"
echo ""
echo "struct baudentry {"
echo "\tint speed, symbol;"
echo "};"
echo ""
echo "static struct baudentry baudtable[] = {"
for n in 230400 115200 57600 38400 19200 9600 4800 2400 1800 1200 600 300 200 150 134 110 75 50 0; do
    echo "#ifdef B$n"
    echo "\t{ $n, B$n },"
    echo "#endif"
done
echo "\t{ -1, -1 }"
echo "};"
