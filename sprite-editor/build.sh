#!/bin/bash

rm -rf build
mkdir build
cd build
qmake ../SpriteEditor.pro
make -j4
