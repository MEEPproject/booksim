#!/usr/bin/bash

# Copyright (c) 2014-2020, University of Cantabria
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
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

# Author: Ivan Perez

# This script generates a CSV file of a given injection rate swept when using
# two packet classes (classes=2).
# If require more just increase the number of lines printed by the head
# commands according to the number of classes
# IMPORTANT NOTE: set BOOKSIM_HOME to the directory with your BookSim binary.

# Usage: ./injection_rate_swept_csv.sh <cfg_file> <inj_rate_list> <output_csv>

BOOKSIM_HOME=../../booksim2
TMP_OUTPUT=/tmp/booksim_outputs/
CONFIG_FILE=$1
INJECTION_RATES=$2
OUTPUT=$3

first="TRUE"

mkdir ${TMP_OUTPUT}

for inj_rate in ${INJECTION_RATES};
do
    ${BOOKSIM_HOME}/booksim ${CONFIG_FILE} injection_rate=${inj_rate} \
                            > ${TMP_OUTPUT}/output_${inj_rate}
    if [ ${first} = "TRUE" ];
    then
        tail -3 ${TMP_OUTPUT}/output_${inj_rate} | head -3 > ${OUTPUT}
        first="FALSE"
    else
        tail -2 ${TMP_OUTPUT}/output_${inj_rate} | head -2 >> ${OUTPUT}
    fi
done
