#!/bin/bash
clang-format -i $(find src/ -type f \( -iname \*.cpp -o -iname \*.h \))
