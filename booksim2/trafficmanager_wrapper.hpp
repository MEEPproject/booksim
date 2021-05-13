// $Id$

/*
 Copyright (c) 2014-2020, Trustess of The University of Cantabria
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

#ifndef _TRAFFICMANAGER_WRAPPER_HPP_
#define _TRAFFICMANAGER_WRAPPER_HPP_

// TODO: Include necessary headers
#include <vector>
#include <queue>
#include "trafficmanager.hpp"
//#include "stats.hpp"

namespace Booksim
{

    class TrafficManagerWrapper : public TrafficManager
    {

        protected:
            // Pure virtual method used to generate packets.
            // In this case packets are generated outside BookSim
            virtual void _Inject() { }
            // Pure virtual method called by Run() in main.cpp
            virtual bool _SingleSim( ) { return true; }

        private:
            vector<vector<queue<pair<Flit,Flit>>>> _ejection_queue;

            virtual void _RetirePacket(Flit * head, Flit * tail);

            long _last_print;

            long _sample_period;	

        public:
            TrafficManagerWrapper(const Configuration &config,
                                  const vector<Network *> & net
                                  );
            ~TrafficManagerWrapper();

            // Interface to internal methods of TrafficManager
            int GeneratePacket(int source,
                               int dest,
                               int size,
                               int cl,
                               long time);
            bool CheckEjectionQueue();
            // TODO: check type of returning data
            //int RetirePacket();
            pair<Flit,Flit> RetirePacket();
            void RunCycles(int cycles);
            bool CheckInFlightPackets();
            void ClearStats();
            void UpdateSimTime(int cycles);
            int CheckInjectionQueue(int source, int cl);
    };
} // namespace Booksim


#endif
