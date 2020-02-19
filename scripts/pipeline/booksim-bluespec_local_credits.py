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

# TODO: script description
# TODO: usage

import argparse
import re

import numpy as np
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument("booksim", help="input file generated from a BookSim simulation")
parser.add_argument("bluespec", help="input file generated from a BookSim simulation")
parser.add_argument("direction", help="Four options: up, right, down, left")

args = parser.parse_args()

num_routers_x = 16
num_routers_y = 1

dir_map = { "up" : (2, 0), "right" : (0, 1), "down" : (3, 2), "left" : (1, 3)}

booksim_dir, bluespec_dir = dir_map[args.direction]

booksim_flits = [[0 for x in range(num_routers_y)] for y in range(num_routers_x)]
bluespec_flits = [[0 for x in range(num_routers_y)] for y in range(num_routers_x)]

f = open(args.booksim, 'r') 
for line in f.readlines():
    if "Credit Local" in line and "Input {}".format(booksim_dir)  in line:
        spl = line.split(" ")
        #print(["{} : {}".format(x,y) for x, y in enumerate(spl)])
        spl = spl[2].replace("network_0/router_","")
        x,y = spl.split("_")
        x = int(x)
        y = int(y)
        assert x < num_routers_x, "Router X index read greater than total number of routers in X" 
        assert y < num_routers_y, "Router X index read greater than total number of routers in X" 
        booksim_flits[x][y] += 1
f.close()

f = open(args.bluespec, 'r') 
for line in f.readlines():
    if "Credit Local" in line and "Input{}".format(bluespec_dir) in line.replace(" ",""):
        spl = line.split(" | ")
        #print(["{} : {}".format(x,y) for x, y in enumerate(spl)])
        spl = spl[1].replace("top.meshNtk.routers_","")
        y,x = spl.split("_")
        x = int(x)
        y = int(y)
        assert x < num_routers_x, "Router X index read greater than total number of routers in X" 
        assert y < num_routers_y, "Router Y index read greater than total number of routers in Y" 
        bluespec_flits[x][y] += 1
f.close()

print("BookSim:  {}".format(booksim_flits))
print("Bluespec: {}".format(bluespec_flits))


ind = np.arange(num_routers_x*num_routers_y)  # the x locations for the groups
width = 0.35  # the width of the bars

booksim_flits_list = list()
bluespec_flits_list = list()
labels = list()

for src in range(num_routers_x):
    for dst in range(num_routers_y):
        booksim_flits_list.append(booksim_flits[src][dst])
        bluespec_flits_list.append(bluespec_flits[src][dst])
        labels.append("src: {}\ndst: {}".format(src,dst))


fig, ax = plt.subplots()
rects1 = ax.bar(ind - width/2, booksim_flits_list, width, label='BookSim')
rects2 = ax.bar(ind + width/2, bluespec_flits_list, width, label='Bluespec', color='IndianRed')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Number of flits')
ax.set_title('Flit distribution')
ax.set_xticks(ind)
ax.set_xticklabels(labels)
ax.legend()
plt.show()
