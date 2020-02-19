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

# Experiment launcher. I use the term experiment to refer to a set of
# simulations that shares some common configuration parameters and has one or
# more variable ones.
# NOTE: Take a look to the "experiment" directory to see some examples
# NOTE: to use this script you must add your user info in "host.py"

# Usage: ./launcher.py <experiment.json>

import sys
import json
import host
import os

def replace_inv_chars(name):
    temp_name = name.replace(" ", "_")
    temp_name = temp_name.replace("=","_")
    temp_name = temp_name.replace("\(","_")
    temp_name = temp_name.replace("\)","")
    temp_name = temp_name.replace("\\{","")
    temp_name = temp_name.replace("\\}","")
    temp_name = temp_name.replace(",","_")

    return temp_name


class Simulation():
    def __init__(self, cmd_line, name, sim_out, working_dir, injection_rates, classes):
        self.cmd_line = cmd_line
        self.name = name
        self.sim_out = sim_out
        self.working_dir = working_dir
        self.injection_rates = injection_rates
        self.classes = classes

if __name__ == '__main__':

    current_host = host.getCurrentHost()

    with open(sys.argv[1],'r') as f:
        data = json.load(f)

    name = data["name"]
    sim_out = data["output_dir"]
    working_dir = os.path.dirname(os.path.abspath(__file__))

    # Fixed parameters
    cmd = data["booksim_bin"] + " " + data["config_file"]
    for p in data["params"]:
        data_value = data["params"][p]
#        if "{" in p:
#            data_value = data_value.replace("{","//{")
#        if "}" in p:
#            data_value = data_value.replace("}","//}")
        cmd += " " + p + "=" + data_value
    print(cmd)

    if "classes" in data["params"]:
        classes = data["params"]["classes"]
    else:
        classes = 1


    if "injection_rate" in data:
        injection_rate = float(data["injection_rate"][0])
        maximum_injection_rate = float(data["injection_rate"][1])
        step_injection_rate = float(data["injection_rate"][2])

        if injection_rate == step_injection_rate:
            sys.exit("\n\nError: step injection rate cannot be equal to the initail injection rate")

        injection_rates_seq = str(injection_rate) + " "
        while injection_rate <= maximum_injection_rate:
            #print "injection_rate: ", injection_rate, " maximum_injection_rate: ", maximum_injection_rate

            if injection_rate == float(data["injection_rate"][0]):
                injection_rate = step_injection_rate
            else:
                injection_rate += step_injection_rate
            injection_rates_seq += str(injection_rate) + " "

    elif "injection_rate_classes" in data:
        injection_rate = float(data["injection_rate_classes"][0])
        maximum_injection_rate = float(data["injection_rate_classes"][1])
        step_injection_rate = float(data["injection_rate_classes"][2])
        proportion_injection_rate = [float(x) for x in data["injection_rate_classes"][3]]

        injection_rates_seq = '"{' + str(injection_rate*proportion_injection_rate[0])
        for class_ir in proportion_injection_rate[1:]:
            injection_rates_seq += ',' + str(injection_rate*class_ir)
        injection_rates_seq += '}" '
        while injection_rate <= maximum_injection_rate:
            if injection_rate == float(data["injection_rate_classes"][0]):
                injection_rate = step_injection_rate
            else:
                injection_rate += step_injection_rate
            injection_rates_seq += '"{' + str(injection_rate*proportion_injection_rate[0])
            for class_ir in proportion_injection_rate[1:]:
                injection_rates_seq += ',' + str(injection_rate*class_ir)
            injection_rates_seq += '}" '
        print(injection_rates_seq)

    elif "injection_rate_list" in data:
        injection_rates_seq = data["injection_rate_list"]


    #TODO: Multiples variables how could we combine them
    # Calc number of simulations
    simulations = 1
    variable_name = []
    for var in data["variables"]:
        simulations = simulations * len(data["variables"][var])

    for i in range(simulations):
        variable_name.append("")

    # Fill simulations vector
    factor = 1
    for index, var in enumerate(data["variables"]):
        if index==0:
            factor = 1
        else:
            factor = factor * len(data["variables"][old_var])
        old_var = var
        for i in range(simulations):
            stride = int(i/factor) % len(data["variables"][var])
            variable_name[i] = variable_name[i] + " " + var + "=" + data["variables"][var][stride]

    print("Number of simulations: " + str(len(variable_name)))

    sim_index = 0
    for var_name in variable_name:
        cmd_line = cmd + var_name
        sim = Simulation(cmd_line, name + replace_inv_chars(var_name),
                         sim_out, working_dir,
                         injection_rates_seq,
                         classes)
        sim_index += 1
        print("Job %d of %d" % (sim_index,len(variable_name)))
        current_host.run_simulation(sim)
