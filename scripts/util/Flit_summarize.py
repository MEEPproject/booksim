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

# Usage: ./Flit_summarize.py <SMART log file>

import sys
g_event_list = ["BW",
                "SA-L",
                #"Starting SA-G 0",
                #"Starting SA-G 1",
                #"Starting SA-G 2",
                #"Starting SA-G 3",
                #"Starting SA-G 4",
                "SA-G",
                "SSR",
                "SSR (Kill)",
                "SSR (Eval)",
                "ST+LT (Local)",
                "ST+LT (Bypass)",
                "ST+LT (Spec-Bypass)",
               ]

# TODO: this could be made easily with a dictionary
def event_format(event):
    event_formated = ""
    if "BW" == event:
        event_formated = "\033[1;41m" + event + "\033[0m"
    if "SA-L" == event:
        event_formated = "\033[1;42m" + event + "\033[0m"
    if "SA-G" == event:
        event_formated = "\033[1;46m" + event + "\033[0m"
    if "Starting SA-G" in event:
        event_formated = "\033[1;43m" + event + "\033[0m"
    if "SSR" in event:
        event_formated = "\033[1;44m" + event + "\033[0m"
    if "ST+LT" in event:
        event_formated = "\033[1;45m" + event + "\033[0m"

    return event_formated


def format_text(text):
    lines = text.split("\n")

    new_text = ""
    for line in lines:
        for event in g_event_list:
            if event in line:
                new_text += line.replace(event, event_format(event)) + "\n"
    return new_text


def filter_line(line):
    fields = line.rstrip("\n").split(" | ")

    if len(fields) == 1:
        return ""

    if fields[2] in g_event_list:
        return line
    else:
        return ""


def table_format(lines):

    current_cycle = -1
    table = "cycle \t| Events\n"
    events = ""
    for line in lines:
        if line == "":
            continue

        fields = line.split(" | ")
        cycle = int(fields[0])
        router = fields[1].split("/")[-1]
        event = event_format(fields[2])

        if cycle != current_cycle :
            table += "\n{} \t| {}: {} |".format(cycle, router, event)
            current_cycle = cycle
        else:
            table += " {}: {} |".format(router, event)

    return table


def main():
    in_files = sys.argv[1:]

    for f in in_files:
        filtered_lines = list()
        in_file = open(f, 'r')
        lines = in_file.readlines()
        for line in lines:
            new_line = filter_line(line)
            #new_line = format_line(new_line)
            if line != "":
                #outf.write(new_line)
                filtered_lines.append(new_line)
        in_file.close()

        table = table_format(filtered_lines)
        #table = format_text(table)
        out_file = open(f + "_table", 'w')
        out_file.write(table)
        out_file.close()


if __name__ == "__main__":
    main()
