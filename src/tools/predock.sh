#!/bin/bash
#
# Copyright (c) 2025 Ian Marco Moffett and Ethos0 engineers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the project nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#
# Description: Checks if 'bay-0' is OK to merge with 'main'
# Author: Ian Marco Moffett
#
set -e

# Directories to check
SYSTEM=$(find sys/ -type f -name "[!.]*")
LIB=$(find lib/ -type f -name "[!.]*")
TOOLS=$(find tools/ -type f -name "[!.]*")

fail() {
    echo "!! TEST FAILURE !!"
    exit 1
}

#
# $1: List of files
#
check_folder() {
    for file in $1
    do
        echo "predock: check $file..."
        if [[ ! -f $file ]]
        then
            echo "skip $file"
            continue
        fi
        tools/checknl.pl $file || fail
        echo "OK"
    done
}

echo "predock: checking bay..."
echo "predock: nuking artifacts..."
make distclean || fail
./build.sh || fail

echo "predock: CHECK SYSTEM"
check_folder "$SYSTEM"
echo "predock: OK, CHECK LIB..."
check_folder "$LIB"
echo "predock: OK, CHECK TOOLS..."
check_folder "$TOOLS"
echo "---------------------------------------"
echo "predock: ALL CHECKS PASSED"
echo "predock: IT IS SAFE TO DOCK BAY WITH 'main'"
git status -s
