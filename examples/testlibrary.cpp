/*
 Copyright (c) 2021, Barcelona Supercomputing Center
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

/*
 * File: examples/testlibrary.cpp
 * Description: Demonstrates the use of Booksim as a library
 * Author: Mariano Benito <mariano.benito1@bsc.es>
*/

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cassert>

#include "../booksim2/booksim_wrapper.hpp"
#include "../booksim2/booksim_config.hpp"

#define PRIO 0
//#define MULTI 1

using namespace std;

struct Message {
   int source;
   int destination;
   const char* message;
};

int main (int argc, char** argv)
{
   /* This test library file assumes a first config file with, at least, 8 nodes and 3 classes of traffic */
   // Fails due to global variables:   and a second topology. if it used, with only 4 nodes */

   // local variables
   vector<Booksim::BooksimWrapper*>   booksim_wrapper;  // Booksim library pointer
   map<int, map<int, Message> >       packets_map;      // Booksim packet ID -> Message

   if (argc < 2 || argc > 2) {//argc > 3) {
      cerr << "You need to supply a configuration file name like: 8x8_mesh_IQ.cfg" << endl;
      return (EXIT_FAILURE);
   }

   // Initialize wrapper
   for(int i=1; i < argc; ++i)
      booksim_wrapper.push_back(new Booksim::BooksimWrapper(argv[i]));
   assert(booksim_wrapper.size() == (argc-1));

   // BookSim wrapper 0 - testing priorities
   Booksim::BookSimConfig booksim_config;
   booksim_config.ParseFile(argv[PRIO+1]);
   // Classes checks
   const uint8_t classes = booksim_config.GetInt("classes");
   assert(classes >= 3);
   int packetId;
   // Generate a packet of class 0
   packetId = booksim_wrapper[PRIO]->GeneratePacket(0, 7, 1, 0, 0);
   packets_map[PRIO][packetId] = {0, 7, "NoC0-From-0-to-7-Size-1-Class-0-InjLatency-0"};
   // packet of class 1
   packetId = booksim_wrapper[PRIO]->GeneratePacket(0, 7, 1, 1, 0);
   packets_map[PRIO][packetId] = {0, 7, "NoC0-From-0-to-7-Size-1-Class-1-InjLatency-0"};
   // packet of class 2
   packetId = booksim_wrapper[PRIO]->GeneratePacket(0, 7, 1, 2, 0);
   packets_map[PRIO][packetId] = {0, 7, "NoC0-From-0-to-7-Size-1-Class-2-InjLatency-0"};

   // BookSim wrapper 1 - testing multiple config files
   /*if(argc > 2) {
      // Generate a packet from 
      packetId = booksim_wrapper[MULTI]->GeneratePacket(0, 3, 1, 0, 0);
      packets_map[MULTI][packetId] = {0, 3, "NoC1-From-0-to-3-Size-1-Class-0-InjLatency-0"};
   }*/

   bool next_cycle_should_be_executed;
   // Run simulation until there are no more packets or credits
   do {
      next_cycle_should_be_executed = false;
      for(int wrap=0; wrap < argc-1; ++wrap) {
         Booksim::BooksimWrapper::RetiredPacket rpacket;
         rpacket.pid = -1;
         booksim_wrapper[wrap]->RunCycles(1);
         rpacket = booksim_wrapper[wrap]->RetirePacket();
         if(rpacket.pid >= 0) {
            assert(packets_map[wrap][rpacket.pid].source == rpacket.src);
            assert(packets_map[wrap][rpacket.pid].destination == rpacket.dst);
            cout << booksim_wrapper[wrap]->GetSimTime() << " | Packet with id " << rpacket.pid << " has been retired from NoC " << wrap << 
            "\n\t message: " << packets_map[wrap][rpacket.pid].message <<
            "\n\t class: " << rpacket.c <<
            "\n\t size: " << rpacket.ps <<
            "\n\t packet latency: " << rpacket.plat <<
            "\n\t network latency: " << rpacket.nlat <<
            "\n\t hops: " << rpacket.hops <<
            "\n\t smart hops: " << rpacket.shops <<
            "\n\t bypassed routers: " << rpacket.br << endl;
            packets_map[wrap].erase(rpacket.pid);
         }
         next_cycle_should_be_executed |= booksim_wrapper[wrap]->CheckInFlightPackets() || booksim_wrapper[wrap]->CheckInFlightCredits();
      }  
   } while(next_cycle_should_be_executed);

   // Finishing
   cout << "There are no more packets in flight - Finish simulation" << endl;

   for(int i=1; i < argc; ++i)
      delete booksim_wrapper[i];

   return (EXIT_SUCCESS);
}