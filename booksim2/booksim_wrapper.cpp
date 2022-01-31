// $Id$

/*
 Copyright (c) 2014-2020, University of Cantabria
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this 
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "booksim_wrapper.hpp"
#include "booksim_config.hpp"
#include "globals.hpp"
#include "trafficmanager_wrapper.hpp"
#include "credit.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Booksim
{
    class Stats;

    // TODO: Move this to another file to include it here and in main.cpp

    //////////////////////
    //Global declarations
    //////////////////////

     /* the current traffic manager instance */
    TrafficManager * trafficManager = NULL;

    //BSMOD: Change time to long long
    long long GetSimTime() {
      return trafficManager->getTime();
    }


    Stats * GetStats(const std::string & name) {
      Stats* test =  trafficManager->getStats(name);
      if(test == 0){
        cout<<"warning statistics "<<name<<" not found"<<endl;
      }
      return test;
    }

    /* printing activity factor*/
    bool gPrintActivity;

    int gK;//radix
    int gN;//dimension
    /**
    concentration. 
    XXX: By default 1 instead of 0. It is relevant in lookahead bypass router models
    to avoid mem leaks.
     */
    int gC = 1;
    vector<int> gKvector;
    vector<int> gCvector;//concentration

    int gNodes;

    // SynFull Network Interface
    NetworkInterface * ni;

    //generate nocviewer trace
    bool gTrace;

    ostream * gWatchOut;


    BooksimWrapper::BooksimWrapper(string const & config_file)
    {

        BookSimConfig config;

        //TODO: error message if file doesn't exist or isn't defined
        config.ParseFile(config_file);// FIXME: char * * argv

        /*
         * initialize routing, traffic, injection functions
        */
        InitializeRoutingMap(config);

        //gPrintActivity = (config.GetInt("print_activity") > 0);
        //gTrace = (config.GetInt("viewer_trace") > 0);
        //
        string watch_out_file = config.GetStr("watch_out");

        if (watch_out_file == "") {
            gWatchOut = NULL;
        } else if (watch_out_file == "-") {
            gWatchOut = &cout;
        } else {
            gWatchOut = new ofstream(watch_out_file.c_str());
        }

        // Create the network
        ////////////////////////////////////////////////////////////////////////////
        vector<Network *> net;

        int subnets = config.GetInt("subnets");
        net.resize(subnets);
        for (int i = 0; i < subnets; ++i)
        {
            ostringstream name;
            name << "network_" << i;
            net[i] = Network::New(config, name.str());
        }
        ////////////////////////////////////////////////////////////////////////////

        // Create traffic manager
        _traffic_manager = new TrafficManagerWrapper(config, net) ;
        // global assignment
        trafficManager = _traffic_manager;
    }


    BooksimWrapper::~BooksimWrapper()
    {
        delete _traffic_manager;
    }

    //BSMOD: Change flit and packet id to long
    long //! Packet ID
    BooksimWrapper::GeneratePacket(int source,
                                   int dest,
                                   int size,
                                   int cl,
                                   long time)
    {
        return _traffic_manager->GeneratePacket(source ,dest, size, cl, time);
    }

    // XXX: carefull, we are not taking packets from the ejection queues.
    // Safe call with a value of 1 for cycles parameter
    void
    BooksimWrapper::RunCycles(const unsigned int cycles)
    {
        _traffic_manager->RunCycles(cycles);
    }


    // Call it until returning null
    BooksimWrapper::RetiredPacket
    BooksimWrapper::RetirePacket()
    {

        pair<Flit,Flit> rp = _traffic_manager->RetirePacket();
        Flit head = rp.first;
        Flit tail = rp.second;
        RetiredPacket p = {head.pid,
                            head.src,
                            head.dest,
                            head.cl,
                            head.packet_size,
                            (int)(tail.atime-head.ctime),
                            (int)(tail.atime-head.itime),
                            head.hops,
                            (int)head.hpc.size(),
                            0};
        return p;
    }


    bool
    BooksimWrapper::CheckInFlightPackets()
    {
        return _traffic_manager->CheckInFlightPackets();
    }

    //BSMOD: Check in flight credits
    bool
    BooksimWrapper::CheckInFlightCredits()
    {
        return Credit::OutStanding()!=0;
    }
    
    int
    BooksimWrapper::CheckInjectionQueue(int source, int cl)
    {
        return _traffic_manager->CheckInjectionQueue(source, cl);
    }

    // when calling _Step() _time is increased by 1.
    void
    BooksimWrapper::UpdateSimTime(int cycles)
    {
        assert(!CheckInFlightPackets());
        _traffic_manager->UpdateSimTime(cycles);
    }

    //BSMOD: Add statistics management
    void
    BooksimWrapper::PrintStats(std::ostream & os)
    {
        _traffic_manager->UpdateStats();
        _traffic_manager->DisplayStats(os);
    }

    void
    BooksimWrapper::ResetStats()
    {
        _traffic_manager->ClearStats();
    }

    //BSMOD: Add the posibility to retrieve the simulation cycle of BookSim
    long
    BooksimWrapper::GetSimTime()
    {
        return _traffic_manager->getTime();
    }
} // namespace Booksim
