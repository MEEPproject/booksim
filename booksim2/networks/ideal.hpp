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

#ifndef _IDEAL_HPP_
#define _IDEAL_HPP_

#include "network.hpp"
#include "routefunc.hpp"
#include <vector>
#include <queue>

namespace Booksim
{

    class Ideal : public Network {

      int _k;
      int _n;
      std::vector<std::queue<Flit *>> _output_buffer;
      std::vector<std::queue<Credit *>> _input_buffer_credits;



      void _ComputeSize( const Configuration &config );
      void _BuildNet( const Configuration &config );
      void _Alloc(); 
     
    public:
      Ideal( const Configuration &config, const string & name );
      // ~Ideal();

      void WriteFlit(Flit *f, int source);
      Flit *ReadFlit(int dest);

      void WriteCredit(Credit *c, int dest);
      Credit *ReadCredit(int source);

      static void RegisterRoutingFunctions();
    };
    void routing_ideal( const Router *r, const Flit *f, int in_channel, 
              OutputSet *outputs, bool inject );

} // namespace Booksim

#endif 
