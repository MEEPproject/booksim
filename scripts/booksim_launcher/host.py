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
# Used by launcher.py
# NOTE: addecuate batchcode in run_simulation to your system. In this exmaple
# Host class is meant to launch the simulation like a normal bash script while
# the children of Host are configurations for server computers with SLURM 
# job queues.
# TODO: Replace the personal information with a template.

import os

class Host:
    hostname=['Host']
    name='Host'

    @staticmethod
    def run_simulation(simulation):
        batchcode=r"""#!/bin/bash
export BookSim=%(working_dir)s
cd $BookSim

first="TRUE"
classes="%(classes)s"
INJECTION_RATES=( %(injection_rates)s )

mkdir -p %(sim_out)s

for inj_rate in ${INJECTION_RATES[@]};
do
    if [ ${first} = "TRUE" ];
    then
        { %(code)s injection_rate=${inj_rate} write_config_file=%(sim_out)s/%(name)s.cfg | tail -$(($classes+1)); } >%(sim_out)s/%(name)s.csv 2>%(sim_out)s/%(name)s.err
        first="FALSE"
    else
        { %(code)s injection_rate=${inj_rate} | tail -$classes; } >>%(sim_out)s/%(name)s.csv 2>>%(sim_out)s/%(name)s.err
    fi
done


""" % {"code"    : simulation.cmd_line,
       "name"    : simulation.name,
       "sim_out" : simulation.sim_out,
       "working_dir" : simulation.working_dir,
       "injection_rates" : simulation.injection_rates,
       "classes" : simulation.classes}


        f=open('%s.job'%simulation.name, 'w')
        f.write(batchcode)
        f.close()

        os.system("bash %s.job"%simulation.name)

#XXX: Slurm example. Set values between <>
class Server(Host):

    hostname=['<hostname>', '<hosname.domain>']
    name='<Name>'

    @staticmethod
    def run_simulation(simulation):
        #Create the needed stuff
        batchcode=r"""#!/bin/bash
#SBATCH -J %(name)s
#SBATCH --cpus-per-task=1
#SBATCH -D /home/<user>
#SBATCH -o /home/<user>/sim/booksim/%(name)s.out
#SBATCH -e /home/<user>/sim/booksim/%(name)s.err

export BookSim=%(working_dir)s
cd $BookSim

first="TRUE"
classes="%(classes)s"
INJECTION_RATES=( %(injection_rates)s )

mkdir -p %(sim_out)s

for inj_rate in ${INJECTION_RATES[@]};
do
    if [ ${first} = "TRUE" ];
    then
        { %(code)s injection_rate=${inj_rate} write_config_file=%(sim_out)s/%(name)s.cfg | tail -$(($classes+1)); } >%(sim_out)s/%(name)s.csv
        first="FALSE"
    else
        { %(code)s injection_rate=${inj_rate} | tail -$classes; } >>%(sim_out)s/%(name)s.csv
    fi
done

""" % {"code"    : simulation.cmd_line,
       "name"    : simulation.name,
       "sim_out" : simulation.sim_out,
       "working_dir" : simulation.working_dir,
       "injection_rates"  : simulation.injection_rates,
       "classes" : simulation.classes}

        f=open(simulation.name + '.job', 'w')
        print("Launching: " + simulation.name)
        f.write(batchcode)
        f.close()
        os.system(r"""
            sleep 0.5
            sbatch %(name)s.job
            """ % {"name" : simulation.name})

#Hosts that we currently are trcking
def getCurrentHost():
    hosts={ }
    import socket
    import sys
    import inspect

    #Iterate through all the members of this class

    current_module = sys.modules[__name__]
    for name, obj in inspect.getmembers(sys.modules[__name__]):
        if inspect.isclass(obj):
            for host in obj.hostname:
                hosts[host]=obj

    curr_host = socket.gethostname()
    print("Hostname: {}".format(curr_host))
    if curr_host in hosts:
        host=hosts[socket.gethostname()]()
    else:
        host = Host()
    return host
