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

#include "../booksim2/booksim_wrapper.hpp"

using namespace std;

struct Message {
   int source;
   int destination;
   const char* message;
};

int main (int argc, char** argv)
{
   // local variables
   Booksim::BooksimWrapper*   booksim_wrapper;  // Booksim library pointer
   map<int, Message>          packets_map;      // Booksim packet ID -> Message

   if (argc < 2 || argc > 2) {
      cerr << "You only need to supply a configuration file name like: 8x8_mesh_IQ.cfg" << endl;
      return (EXIT_FAILURE);
   }

   // Initialize wrapper
   booksim_wrapper = new Booksim::BooksimWrapper(argv[1]);

   // Generate a packet of class 0
   int packetId1 = booksim_wrapper->GeneratePacket(0, 7, 1, 0, 0);
   string str1 = "From-0-to-7-Size-1-Class-0-InjLatency-0";
   Message msg1 = {0, 7, str1.c_str()};
   packets_map[packetId1] = msg1;
   // packet of class 1
   int packetId2 = booksim_wrapper->GeneratePacket(0, 7, 1, 1, 0);
   string str2 = "From-0-to-7-Size-1-Class-1-InjLatency-0";
   Message msg2 = {0, 7, str2.c_str()};
   packets_map[packetId2] = msg2;
   // packet of class 2
   int packetId3 = booksim_wrapper->GeneratePacket(0, 7, 1, 2, 0);
   string str3 = "From-0-to-7-Size-1-Class-2-InjLatency-0";
   Message msg3 = {0, 7, str3.c_str()};
   packets_map[packetId3] = msg3;

   // Run simulation until packet arrival
   while(booksim_wrapper->CheckInFlightPackets()) {
      Booksim::BooksimWrapper::RetiredPacket rpacket;
      rpacket.pid = -1;
      do {
         booksim_wrapper->RunCycles(1);
         rpacket = booksim_wrapper->RetirePacket();
      } while(rpacket.pid < 0);
      cout << "Packet with id " << rpacket.pid << " has been retired:" <<
         "\n\t message: " << packets_map[rpacket.pid].message <<
         "\n\t class: " << rpacket.c <<
         "\n\t size: " << rpacket.ps <<
         "\n\t packet latency: " << rpacket.plat <<
         "\n\t network latency: " << rpacket.nlat <<
         "\n\t hops: " << rpacket.hops <<
         "\n\t smart hops: " << rpacket.shops <<
         "\n\t bypassed routers: " << rpacket.br << endl;
      packets_map.erase(rpacket.pid);
   }

   // Finishing
   cout << "There are no more packets in flight - Finish simulation" << endl;

   delete booksim_wrapper;

   return (EXIT_SUCCESS);
}