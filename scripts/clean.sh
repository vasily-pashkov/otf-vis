#!/usr/bin/env bash

if [ -d latex ]; then
   rm -R latex
fi

if [ -d html ]; then
   rm -R html
fi

if [ -d pdf ]; then
   rm -R pdf
fi

if [ -f *.log ]; then
   rm *.log
fi

if [ -f *.sh~ ]; then
   rm *.sh~
fi

if [ -f *.out ]; then
   rm *.out
fi

