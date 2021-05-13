#!/usr/bin/python

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

import sys
from statistics import mean

def timeIntervals(cycles):
    intervals = list()
    prev_cycle = 0
    for cycle in cycles:
        interval = cycle - prev_cycle
        intervals.append(interval)
        prev_cycle = cycle

    return intervals

def main():
    in_files = sys.argv[1:]

    for f in in_files:
        in_file = open(f, 'r')
        lines = in_file.readlines()
        cycles = list()
        for line in lines:
            if " | " in line:
                cycle = int(line.split(" | ")[0])
                cycles.append(cycle)
        in_file.close()

        intervals = timeIntervals(cycles)

        print("Time avg: {} min: {} max: {}".format(
                    mean(intervals),
                    min(intervals),
                    max(intervals)
            ))


if __name__ == "__main__":
    main()
