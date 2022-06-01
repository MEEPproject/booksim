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
#define CONSUMPTION_TIME 0
#define NEW_PACKETS 15

using namespace std;

struct Message {
   int source;
   int destination;
   const char* message;
};

int main (int argc, char** argv)
{
   // local variables
   vector<Booksim::BooksimWrapper*>   booksim_wrapper;  // Booksim library pointer
   map<int, map<int, Message> >       packets_map;      // Booksim packet ID -> Message
   int pending_consumption = 0;
   int pending_new_packets = NEW_PACKETS;

   if (argc < 2 || argc > 2) {
      cerr << "You need to supply a configuration file name like: test.cfg" << endl;
      return (EXIT_FAILURE);
   }

   // Initialize wrapper
   for(int i=1; i < argc; ++i)
      booksim_wrapper.push_back(new Booksim::BooksimWrapper(argv[i]));
   assert(booksim_wrapper.size() == (argc-1));

   // BookSim wrapper 0 - testing backpressure
   Booksim::BookSimConfig booksim_config;
   booksim_config.ParseFile(argv[1]);
   // Classes checks
   const uint8_t classes = booksim_config.GetInt("classes");
   assert(classes >= 3);
   int packetId;
   // Generate a packet of class 0
   assert(booksim_wrapper[PRIO]->CheckInjectionQueue(0, 0) > 1);
   packetId = booksim_wrapper[PRIO]->GeneratePacket(0, 1, 1, 0, 0);
   packets_map[PRIO][packetId] = {0, 1, "NoC0-From-0-to-1-Size-1-Class-0-InjLatency-0"};
   cout << booksim_wrapper[PRIO]->GetSimTime() << " | Injection queue free size: " << booksim_wrapper[PRIO]->CheckInjectionQueue(0, 0) << endl;
   // packet of class 1
   assert(booksim_wrapper[PRIO]->CheckInjectionQueue(0, 0) > 1);
   packetId = booksim_wrapper[PRIO]->GeneratePacket(0, 1, 1, 0, 0);
   packets_map[PRIO][packetId] = {0, 1, "NoC0-From-0-to-1-Size-1-Class-0-InjLatency-0"};
   cout << booksim_wrapper[PRIO]->GetSimTime() << " | Injection queue free size: " << booksim_wrapper[PRIO]->CheckInjectionQueue(0, 0) << endl;
   // packet of class 2
   assert(booksim_wrapper[PRIO]->CheckInjectionQueue(0, 0) >= 1);
   packetId = booksim_wrapper[PRIO]->GeneratePacket(0, 1, 1, 0, 0);
   packets_map[PRIO][packetId] = {0, 1, "NoC0-From-0-to-1-Size-3-Class-0-InjLatency-0"};
   cout << booksim_wrapper[PRIO]->GetSimTime() << " | Injection queue free size: " << booksim_wrapper[PRIO]->CheckInjectionQueue(0, 0) << endl;

   // full injection queue
   assert(booksim_wrapper[PRIO]->CheckInjectionQueue(0, 0) == 0);

   bool next_cycle_should_be_executed;
   // Run simulation until there are no more packets nor credits
   do {
      next_cycle_should_be_executed = false;
      for(int wrap=0; wrap < argc-1; ++wrap) {
         Booksim::BooksimWrapper::RetiredPacket rpacket;
         rpacket.pid = -1;
         booksim_wrapper[wrap]->RunCycles(1);
         if (pending_consumption == 0) {
            rpacket = booksim_wrapper[wrap]->RetirePacket();
            cout << booksim_wrapper[wrap]->GetSimTime() << " | Trying to retire packet" << endl;
         } else {
            pending_consumption--;
            cout << booksim_wrapper[wrap]->GetSimTime() << " | Consuming packet" << endl;
         }
         if (rpacket.pid >= 0)
         {
            pending_consumption = CONSUMPTION_TIME;
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
         if (booksim_wrapper[wrap]->GetSimTime() > 10 && pending_new_packets > 0) {
            cout << booksim_wrapper[wrap]->GetSimTime() << " | Injection queue free size: " << booksim_wrapper[wrap]->CheckInjectionQueue(0, 0);
            if (booksim_wrapper[wrap]->CheckInjectionQueue(0, 0) >= 1)
            {
               packetId = booksim_wrapper[wrap]->GeneratePacket(0, 1, 1, 0, 0);
               packets_map[wrap][packetId] = {0, 1, "NoC0-From-0-to-1-Size-1-Class-0-InjLatency-0"};
               pending_new_packets--;
               cout << " - new packet generated." << endl;
            }
            else
               cout << " - it can not generate a new packet - backpressure!" << endl;
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