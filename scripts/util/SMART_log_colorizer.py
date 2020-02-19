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

# Script to add ASCII color codes to the debug log files of SMART routers.
# See -DSMART_PIPELINE compilation flag in BookSim's source code.

# Usage: ./SMART_log_colorized.py <SMART log file>

import sys

def formatLine(line):
    fields = line.rstrip("\n").split(" | ")
    
    if len(fields) == 1:
        return ""

    fields[0] = "\033[1;91m" + fields[0] + "\033[0m"

    if "network" in fields[1] and "router" in fields[1]:
        fields[1] = fields[1].replace("router_","\033[7mrouter_") + "\033[0m"
    
    if "BW" == fields[2]:
        fields[2] = "\033[1;41m" + fields[2] + "\033[0m"
    if "SA-L" == fields[2]:
        fields[2] = "\033[1;42m" + fields[2] + "\033[0m"
    if "SA-G" == fields[2]:
        fields[2] = "\033[1;43m" + fields[2] + "\033[0m"
    if "SSR" == fields[2]:
        fields[2] = "\033[1;44m" + fields[2] + "\033[0m"
    if "ST+LT" == fields[2]:
        fields[2] = "\033[1;45m" + fields[2] + "\033[0m"

    new_line = ""
    for field in fields:
        new_line = new_line + field + " | "
    new_line += "\n"
    return new_line


    

def main():
    in_files = sys.argv[1:]
    
    for f in in_files:
        inf = open(f, 'r')
        outf = open(f + "_color", 'w')
        lines = inf.readlines()
        for line in lines:
            new_line = formatLine(line)
            if line != "":
                outf.write(new_line)


if __name__ == "__main__":
    main()
