// $Id$

/*
 Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
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

#ifndef _FLIT_HPP_
#define _FLIT_HPP_

#include <iostream>
#include <stack>
#include <vector>

#include "booksim.hpp"
#include "outputset.hpp"

namespace Booksim
{

    class Flit {

        public:

            Flit();
            ~Flit() {}

            // Current Virtual Channel
            int vc;

            // Message class 
            int cl;

            // Is this a head flit?
            bool head;
            // Is this a tail flit?
            bool tail;

            //BSMOD: Change time to long long
            // Creation time
            long long  ctime;
            // Injection time
            long long  itime;
            // Arrival time
            long long  atime;

            //BSMOD: Change flit and packet id to long
            // Flit ID
            long  id;
            // Packet ID
            long  pid;

            // Used in SMART routers
            int router_id;
            int port_id;

            // ???
            bool record;

            // Packet source
            int  src;
            // Packet destination
            int  dest;
            //BSMOD: Add AcmeVectorMemoryTrafficPattern
            // Packet chain auxiliar field
            // Change to a LIFO stack if multiple chain hops are required
            int chain_aux;

            // Packet priority
            //BSMOD: Change time to long long
            long  pri;

            // Hops done
            int  hops;
            // Only used by SMART routers. HPC - hops per cycle.
            // Vector lenght: number of hops
            // Vector elements: number of routers traverse in the hop
            vector<int> hpc;

            // Is this flit being watched? (watch flag)
            bool watch;

            //(I) packet_size used in Bubble flow control. Maybe isn't the best way to give this info to the
            // BufferPolicies class but for the moment is a solution
            int packet_size;

            //Gem5 Interface:
            int subnetwork;
            int rubydest;

            // intermediate destination (if any)
            mutable int intm;

            // phase in multi-phase algorithms
            mutable int ph;

            // Fields for arbitrary data
            void* data ;

            // Lookahead route info
            OutputSet la_route_set;

            void Reset();

            static Flit * New();
            void Free();
            static void FreeAll();

        private:


            static stack<Flit *> _all;
            static stack<Flit *> _free;

    };

    ostream& operator<<( ostream& os, const Flit& f );
} // namespace Booksim

#endif
