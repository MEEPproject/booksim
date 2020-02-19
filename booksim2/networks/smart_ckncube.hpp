// $Id$

/*
 Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
 Copyright (c) 2014-2020, Trustees of The University of Cantabria
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

#ifndef _SMARTCKNCUBE_HPP_
#define _SMARTCKNCUBE_HPP_

#include "network.hpp"
#include "../routefunc.hpp"

namespace Booksim
{

    class SMARTCKNCube : public Network {

      bool _mesh;

      int _k;
      int _n;
      int _c;
      int _yr;
      int _xr;

      vector<int> _kVect;
      vector<int> _cVect;

      void _ComputeSize(const Configuration &config);
      void _BuildNet(const Configuration &config);

      int _LeftChannel(int node, int dim);
      int _RightChannel(int node, int dim);

      int _LeftRouter(int router, int dim);
      int _RightRouter(int router, int dim);

    public:
      SMARTCKNCube(const Configuration &config, const string & name, bool mesh);
      static void RegisterRoutingFunctions();

      int GetN() const;
      int GetK() const;

      double Capacity() const;

      void InsertRandomFaults(const Configuration &config);

      //virtual void ReadInputs();
      //void WriteOutputs();

      static vector<int> RouterIndex(int router);//Returns the router indexes given the router ID
      static vector<int> NodeIndex(int node);//Returns the node indexes given the node ID
      static int IndexToRouter(vector<int> router_index); //Given a vector with the index of a router it returns the router ID.
      static int NodeInRouter(int node); //Returns the router ID in which the given node ID is attached
      static int ConcentrationOffset(int node);//Returns the output port offset that connects to the node.

    };
} // namespace Booksim

#endif
