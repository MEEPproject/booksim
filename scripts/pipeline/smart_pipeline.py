#!/usr/bin/python3

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

# Generates a CSV to track flits in the SMART networks.
# Columns represent cycles and rows router stages.
# I use the output file with an Excel like tool to highlight the packets of
# interest applying cell conditions.

# Usage: ./smart_pipeline.py --inputfile=<in> --outputfile=<out>
# NOTE: Simulate using watch_every_packet=1 watch_out=<output file>.
# NOTE: Compile with -DPIPELINE_DEBUG

import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument("inputfile",
                    help="input file generated from a BookSim simulation")
parser.add_argument("outputfile",
                    help="output file with a csv format representing \
                          the pipeline of the routers in the simulation")

args = parser.parse_args()

# NOTE: adjust the following values
# FIXME: get automatically these values from a configuration file
num_routers = 16
#num_routers = 8
#num_routers = 6
ports = 5
vc_buf_size = 5 
start_cycle = 600
end_cycle = 700 

# 2x2
#router_dict = {"network_0/router_0_0" : 0,
#               "network_0/router_1_0" : 1,    
#               "network_0/router_0_1" : 2,    
#               "network_0/router_1_1" : 3,    
#        }

# 4x4
router_dict = {"network_0/router_0_0" : 0,
               "network_0/router_1_0" : 1,    
               "network_0/router_2_0" : 2,    
               "network_0/router_3_0" : 3,    
               "network_0/router_0_1" : 4,    
               "network_0/router_1_1" : 5,    
               "network_0/router_2_1" : 6,    
               "network_0/router_3_1" : 7,    
               "network_0/router_0_2" : 8,    
               "network_0/router_1_2" : 9,    
               "network_0/router_2_2" : 10,    
               "network_0/router_3_2" : 11,    
               "network_0/router_0_3" : 12,    
               "network_0/router_1_3" : 13,    
               "network_0/router_2_3" : 14,    
               "network_0/router_3_3" : 15,    
        }

# 1x16
#router_dict = {"network_0/router_0_0" : 0,
#               "network_0/router_1_0" : 1,    
#               "network_0/router_2_0" : 2,    
#               "network_0/router_3_0" : 3,    
#               "network_0/router_4_0" : 4,    
#               "network_0/router_5_0" : 5,    
#               "network_0/router_6_0" : 6,    
#               "network_0/router_7_0" : 7,    
#               "network_0/router_8_0" : 8,    
#               "network_0/router_9_0" : 9,    
#               "network_0/router_10_0" : 10,    
#               "network_0/router_11_0" : 11,    
#               "network_0/router_12_0" : 12,    
#               "network_0/router_13_0" : 13,    
#               "network_0/router_14_0" : 14,    
#               "network_0/router_15_0" : 15,    
#        }

# 5x1
#router_dict = {"network_0/router_0_0" : 0,
#               "network_0/router_1_0" : 1,    
#               "network_0/router_2_0" : 2,    
#               "network_0/router_3_0" : 3,    
#               "network_0/router_4_0" : 4,    
#        }

# 5x5
#router_dict = {
#               "network_0/router_0_0" : 0,
#               "network_0/router_1_0" : 1,    
#               "network_0/router_2_0" : 2,    
#               "network_0/router_3_0" : 3,    
#               "network_0/router_4_0" : 4,    
#               "network_0/router_0_1" : 5,
#               "network_0/router_1_1" : 6,    
#               "network_0/router_2_1" : 7,    
#               "network_0/router_3_1" : 8,    
#               "network_0/router_4_1" : 9,    
#               "network_0/router_0_2" : 10,
#               "network_0/router_1_2" : 11,    
#               "network_0/router_2_2" : 12,    
#               "network_0/router_3_2" : 13,    
#               "network_0/router_4_2" : 14,    
#               "network_0/router_0_3" : 15,
#               "network_0/router_1_3" : 16,    
#               "network_0/router_2_3" : 17,    
#               "network_0/router_3_3" : 18,    
#               "network_0/router_4_3" : 19,    
#               "network_0/router_0_4" : 20,
#               "network_0/router_1_4" : 21,    
#               "network_0/router_2_4" : 22,    
#               "network_0/router_3_4" : 23,    
#               "network_0/router_4_4" : 24,    
#        }

# 3x3
#router_dict = {"network_0/router_0_0" : 0,
#               "network_0/router_1_0" : 1,    
#               "network_0/router_2_0" : 2,    
#               "network_0/router_0_1" : 3,    
#               "network_0/router_1_1" : 4,    
#               "network_0/router_2_1" : 5,    
#               "network_0/router_0_2" : 6,    
#               "network_0/router_1_2" : 7,    
#               "network_0/router_2_2" : 8,    
#        }

outport_dict = {"Output 0" : 0,
                "Output 1" : 1,
                "Output 2" : 2,
                "Output 3" : 3,
                "Output 4" : 4,
        }

inport_dict = {"Input 0" : 0,
               "Input 1" : 1,
               "Input 2" : 2,
               "Input 3" : 3,
               "Input 4" : 4,
        }

credits_avail = [[vc_buf_size for x in range(ports)] for y in range(num_routers)]
credits_local = [[vc_buf_size for x in range(ports)] for y in range(num_routers)]
credits_bypass = [[vc_buf_size for x in range(ports)] for y in range(num_routers)]
credits_reception = [[vc_buf_size for x in range(ports)] for y in range(num_routers)]
BW = [["X" for x in range(ports)] for y in range(num_routers)]
sal = [["X" for x in range(ports)] for y in range(num_routers)]
sal_fail = [["X" for x in range(ports)] for y in range(num_routers)]
sag = [["X" for x in range(ports)] for y in range(num_routers)]
st_lt = [["X" for x in range(ports)] for y in range(num_routers)]

cycle = -1

output_stream = list()

temp = ""
# Read file
f = open(args.inputfile, 'r') 
for line in f.readlines():

    line_split = line.split(" | ")
    
    if len(line_split) < 3:
        continue

    if cycle != int(line_split[0]):
        if cycle >= start_cycle and cycle <= end_cycle:
            temp = list()

            for r in range(num_routers):
                for i in range(ports):
                    temp.append(BW[r][i])
                    temp.append(credits_local[r][i])
                    temp.append(credits_bypass[r][i])
                for o in range(ports):
                    temp.append(sal[r][o])
                    temp.append(sal_fail[r][o])
                    temp.append(sag[r][o])
                    temp.append(st_lt[r][o])
                    temp.append(credits_reception[r][o])
                    temp.append(credits_avail[r][o])
            output_stream.append(temp)

            credits_avail = [[vc_buf_size for x in range(ports)] for y in range(num_routers)]
            credits_local = [["X" for x in range(ports)] for y in range(num_routers)]
            credits_bypass = [["X" for x in range(ports)] for y in range(num_routers)]
            credits_reception = [["X" for x in range(ports)] for y in range(num_routers)]
            BW = [["X" for x in range(ports)] for y in range(num_routers)]
            sal = [["X" for x in range(ports)] for y in range(num_routers)]
            sal_fail = [["X" for x in range(ports)] for y in range(num_routers)]
            sag = [["X" for x in range(ports)] for y in range(num_routers)]
            st_lt = [["X" for x in range(ports)] for y in range(num_routers)]

        cycle = int(line_split[0])

    if line_split[2] == "Credit availability":
        credits_avail[router_dict[line_split[1]]][int(outport_dict[line_split[3]])] = line_split[4].replace(" \n","")
    
    if line_split[2] == "Credit Local":
        credits_local[router_dict[line_split[1]]][inport_dict[line_split[4]]] = line_split[3].replace("Flit ", "")
    
    if line_split[2] == "Credit Bypass":
        credits_bypass[router_dict[line_split[1]]][inport_dict[line_split[4]]] = line_split[3].replace("Flit ", "")
    
    if line_split[2] == "Credit Reception":
        credits_reception[router_dict[line_split[1]]][outport_dict[line_split[4].replace("\n","")]] = line_split[3].replace("Flit ", "")
    
    if line_split[2] == "BW":
        BW[router_dict[line_split[1]]][inport_dict[line_split[4]]] = line_split[3].replace("Flit ", "")

    if line_split[2] == "SA-L":
        sal[router_dict[line_split[1]]][outport_dict[line_split[5]]] = line_split[3].replace("Flit ", "")

    if line_split[2] == "No free VC":
        #sal_fail[router_dict[line_split[1]]][int(line_split[5].replace("Output","").replace(" ",""))] = line_split[3].replace("Flit", "").replace(" ","")
        sal_fail[router_dict[line_split[1]]][int(line_split[5].replace("Output","").replace(" ",""))] = "-1"
    
    if line_split[2] == "SA-G":
        sag[router_dict[line_split[1]]][outport_dict[line_split[5]]] = line_split[3].replace("Flit ", "")
    
    if line_split[2] == "ST+LT":
        st_lt[router_dict[line_split[1]]][outport_dict[line_split[5]]] = line_split[3].replace("Flit ", "")

f.close()

len_y = len(output_stream[0]) 
len_x = len(output_stream)

cycle = start_cycle
temp = "Router; "
for y in range(len_x):
    temp += str(cycle) + "; "
    cycle += 1
print(temp)
            
header = list()
for r in range(num_routers):
    for i in range(ports):
        header.append("R{} BW I{}".format(r, i))
        header.append("R{} Crd L I{}".format(r, i))
        header.append("R{} Crd B I{}".format(r, i))
    for o in range(ports):
        header.append("R{} SA-L O{}".format(r, o))
        header.append("R{} SA-L (MISS) O{}".format(r, o))
        header.append("R{} SA-G O{}".format(r, o))
        header.append("R{} ST+LT O{}".format(r, o))
        header.append("R{} Crd Rec O{}".format(r, o))
        header.append("R{} Credits O{}".format(r, o))

for y in range(len_y):
    temp = header[y] + "; " 
    for x in range(len_x):
        temp += str(output_stream[x][y]) + "; "
    print(temp)

