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

#ifndef _IQ_ROUTER_HPP_
#define _IQ_ROUTER_HPP_

#include <string>
#include <deque>
#include <queue>
#include <set>
#include <map>

#include "router.hpp"
#include "../routefunc.hpp"

namespace Booksim
{

    using namespace std;

    class VC;
    class Flit;
    class Credit;
    class Buffer;
    class BufferState;
    class Allocator;
    class SwitchMonitor;
    class BufferMonitor;

    class IQRouter : public Router {

        // Number of virtual channels per channel
        int _vcs;

        // Configurable options
        bool _vc_busy_when_full;
        bool _vc_prioritize_empty;
        bool _vc_shuffle_requests;

        bool _speculative;
        bool _spec_check_elig;
        bool _spec_check_cred;
        bool _spec_mask_by_reqs;

        // Is the router active?
        bool _active;

        // Stage delays
        int _routing_delay;
        int _vc_alloc_delay;
        int _sw_alloc_delay;

        // Stage communication
        map<int, Flit *> _in_queue_flits;
        //BSMOD: Change time to long long
        deque<pair<long long, pair<Credit *, int> > > _proc_credits;
        //BSMOD: Change time to long long
        deque<pair<long long, pair<int, int> > > _route_vcs;
        deque<pair<long long, pair<pair<int, int>, int> > > _vc_alloc_vcs;  
        deque<pair<long long, pair<pair<int, int>, int> > > _sw_hold_vcs;
        deque<pair<long long, pair<pair<int, int>, int> > > _sw_alloc_vcs;
        //BSMOD: Change time to long long
        deque<pair<long long, pair<Flit *, pair<int, int> > > > _crossbar_flits;

        map<int, Credit *> _out_queue_credits;

        // Input buffer (input VCs)
        vector<Buffer *> _buf;
        // Output buffer state (output VCs)
        vector<BufferState *> _next_buf;

        // Allocators
        Allocator *_vc_allocator;
        Allocator *_sw_allocator;
        Allocator *_spec_sw_allocator;

        // ???
        vector<int> _vc_rr_offset;
        vector<int> _sw_rr_offset;

        // Routing function
        tRoutingFunction   _rf;

        // Output buffer size and output buffer
        int _output_buffer_size;
        vector<queue<Flit *> > _output_buffer;

        vector<queue<Credit *> > _credit_buffer;

        // Hold switch for all the flits of a packet
        bool _hold_switch_for_packet;
        vector<int> _switch_hold_in;
        vector<int> _switch_hold_out;
        vector<int> _switch_hold_vc;

        // Lookahead routing???????
        bool _noq;
        vector<vector<int> > _noq_next_output_port;
        vector<vector<int> > _noq_next_vc_start;
        vector<vector<int> > _noq_next_vc_end;

#ifdef TRACK_FLOWS
        vector<vector<queue<int> > > _outstanding_classes;
#endif

        bool _ReceiveFlits( );
        bool _ReceiveCredits( );

        // Router class methods
        virtual void _InternalStep( );

        // Pipeline stages
        bool _SWAllocAddReq(int input, int vc, int output);

        void _InputQueuing( );

        void _RouteEvaluate( );
        void _VCAllocEvaluate( );
        void _SWHoldEvaluate( );
        void _SWAllocEvaluate( );
        void _SwitchEvaluate( );

        void _RouteUpdate( );
        void _VCAllocUpdate( );
        void _SWHoldUpdate( );
        void _SWAllocUpdate( );
        void _SwitchUpdate( );

        void _OutputQueuing( );

        void _SendFlits( );
        void _SendCredits( );

        void _UpdateNOQ(int input, int vc, Flit const * f);

        // ----------------------------------------
        //
        //   Router Power Modellingyes
        //
        // ----------------------------------------

        SwitchMonitor * _switchMonitor ;
        BufferMonitor * _bufferMonitor ;

        public:

        IQRouter( Configuration const & config,
                Module *parent, string const & name, int id,
                int inputs, int outputs );

        virtual ~IQRouter( );

        virtual void AddOutputChannel(FlitChannel * channel, CreditChannel * backchannel);

        virtual void ReadInputs( );
        virtual void WriteOutputs( );

        void Display( ostream & os = cout ) const;

        //virtual int GetUsedCredit(int o) const {return 0;}
        //virtual int GetUsedCreditVC(int o, int vc) const {return 0;} //(I)
        //virtual int GetBufferOccupancy(int i) const {return 0;}
        virtual int GetUsedCredit(int o) const;
        virtual int GetUsedCreditVC(int o, int vc) const;
        virtual int GetBufferOccupancy(int i) const;

#ifdef TRACK_BUFFERS
        virtual int GetUsedCreditForClass(int output, int cl) const {return 0;};
        virtual int GetBufferOccupancyForClass(int input, int cl) const {return 0;};
#endif

        virtual vector<int> UsedCredits() const;
        virtual vector<int> FreeCredits() const;
        virtual vector<int> MaxCredits() const;
    //    virtual vector<int> UsedCredits() const {
    //        vector<int> result(1,0);
    //        return result;
    //    }
    //    virtual vector<int> FreeCredits() const{
    //        vector<int> result(1,0);
    //        return result;
    //    }
    //    virtual vector<int> MaxCredits() const{
    //        vector<int> result(1,0);
    //        return result;
    //    }

        SwitchMonitor const * GetSwitchMonitor() const {return _switchMonitor;}
        BufferMonitor const * GetBufferMonitor() const {return _bufferMonitor;}

    };
} // namespace Booksim

#endif
