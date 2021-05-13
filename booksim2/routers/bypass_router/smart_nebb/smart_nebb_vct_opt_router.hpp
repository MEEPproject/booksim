// $Id$

/*
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

#ifndef _SMART_NEBB_VCT_OPT_ROUTER_HPP_
#define _SMART_NEBB_VCT_OPT_ROUTER_HPP_

#include <string>
#include <deque>
#include <queue>
#include <set>
#include <map>

#include "../smart_router.hpp"

namespace Booksim
{

    using namespace std;

    class VC;
    class Flit;
    class Credit;
    class Buffer;
    class BufferState;
    class Allocator;

    class SMARTNEBBVCTOPTRouter : public SMARTRouter {

      public:

        SMARTNEBBVCTOPTRouter( Configuration const & config,
                Module *parent, string const & name, int id,
                int inputs, int outputs );

        virtual ~SMARTNEBBVCTOPTRouter();
        
        void ReadInputs();

        virtual int FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance);

        virtual void ReadFlit(int input, Flit * f);
        virtual void TransferFlit(int input, int output, Flit * f);
        
        virtual bool AddRequestSAG(SMARTRequest sr);
        void FilterSAGRequests();
        
        virtual void SwitchAllocationLocal();
        virtual void SwitchAllocationGlobal();

        virtual int IdleLocalBuf(int input, int vc_start, int vc_end, Flit * f);
        
        //virtual const SMARTNEBBVCTOPTRouter * GetNextRouter(int next_output_port) const;
        
        virtual void VCManagement(int input, int in_vc, int output, Flit *f);

      protected:
        vector<int> _output_port_blocked;
        vector<bool> _bypass_blocked;
        vector<int> _input_port_blocked;
        queue<SMARTRequest> _sag_requests;
        
        //vector<pair<int, int>> _destination_credit_ps;
    };

} // namespace Booksim

#endif
