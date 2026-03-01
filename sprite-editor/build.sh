#!/bin/bash

rm -rf build
mkdir build
cd build
qmake6 ../SpriteEditor.pro
make -j4
