#!/bin/bash

TARGET=x86_64-pc-ethos
MAKE=make

# Don't build again if the lock exists
if [[ -f cc/.lock ]]
then
    echo "cc/.lock exists, skipping toolchain build"
    exit 1
fi

# Build binutils and patch gcc
cd cc/toolchain/
bash build.sh

# Prep the build directory
cd ../
mkdir -p gcc
cd gcc/

# Configure gcc
../toolchain/gcc-patched/configure --target=$TARGET \
    --prefix=$(pwd) --with-sysroot=$(pwd)/../../root/ \
    --disable-nls --enable-languages=c --disable-multilib

# Build gcc
$MAKE all-gcc
$MAKE install-gcc

# Lock the directory
cd ../
touch .lock
