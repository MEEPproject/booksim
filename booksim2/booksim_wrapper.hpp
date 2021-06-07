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

#ifndef _BOOKSIMWRAPPER_HPP_
#define _BOOKSIMWRAPPER_HPP_

#include <string>

namespace Booksim
{
    class TrafficManagerWrapper;

    class BooksimWrapper {

        private:
            TrafficManagerWrapper * _traffic_manager;

        public:
            BooksimWrapper(std::string const & config_file);
            virtual ~BooksimWrapper();

            // NOTE: if you require additional stats, add more fields. If it
            // grows considerably, consider the creation of a STATS structure.
            struct RetiredPacket {
                //BSMOD: Change flit and packet id to long
                long pid; // packet ID
                int src; // source
                int dst; // destination
                int c; // packet class
                int ps; // packet size
                int plat; // packet latency
                int nlat; // network latency
                int hops; // number of hops done
                int shops; // number of SMART hops done
                int br; // bypassed routers
            };

            //! Returns the number of free slots in the injection queue
            //Note: implement this method if you set finite inj. queues in
            //BookSim. In gem5 there are already injection queues so we use
            // inf. or huge queues.
            //unsigned int CheckInjectionQueue();
            /**
             Generate a new packet. Returns the packet ID (-1 if not possible).
             \todo {Possible overflow in Full System simulations}
            */
            //BSMOD: Change flit and packet id to long
            long GeneratePacket(int source, int dest, int size, int cl,
                               long time);

            //! Run "cycles" internal cycles
            void RunCycles(const unsigned int cycles);

            //! Check if there are packets in the ejection queue.
            //! Get first packet in the consumption queue
            RetiredPacket RetirePacket();

            //! Checks if there are flits inside the network
            bool CheckInFlightPackets();
            
	    //! Checks if there are flits inside the network
            int CheckInjectionQueue(int source, int cl);

            //! Updates the cycle counter
            //Note: use this function if you want to synchonize the timestamps
            //between simulators. This is usefull if you want to plot the 
            //evolution of a metric regards execution time.
            void UpdateSimTime(int cycles);

            //! Print statistics. This function updates the statistics up to this cycle and prints them.
            void PrintStats(std::ostream & os);

            //! Reset statistics
            void ResetStats();
    };
} // namespace Booksim
#endif
