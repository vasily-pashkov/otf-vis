#!/usr/bin/env bash

doxygen

# pdf-file compilation
cd latex
pdflatex refman.tex
cd ..

# make pdf directory
if [ ! -d pdf ]; then
   mkdir pdf
fi

# copy to pdf directory and rename pdf-file
if [ -f latex/refman.pdf ]; then
   cp latex/refman.pdf pdf/
   mv pdf/refman.pdf pdf/Heartbeat_Service.pdf;
fi

#remove latex directory
if [ -d latex ]; then
   rm -R latex
fi

