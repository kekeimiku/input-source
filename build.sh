#!/bin/bash

set -euo pipefail

clang -shared \
    -O3 \
    -flto \
    -fvisibility=hidden \
    -Wl,-x \
    mac-input-source.c \
    -I/Applications/Emacs.app/Contents/Resources/include \
    -framework Carbon \
    -o mac-input-source-core.dylib
