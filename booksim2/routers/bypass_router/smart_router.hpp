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

#ifndef _SMART_ROUTER_HPP_
#define _SMART_ROUTER_HPP_

#include <string>
#include <deque>
#include <queue>
#include <set>
#include <map>

#include "../router.hpp"
#include "../../routefunc.hpp"
#include "../../arbiters/arbiter.hpp"

namespace Booksim
{

    using namespace std;

    class VC;
    class Flit;
    class Credit;
    class Buffer;
    class BufferState;
    class Allocator;

    class SMARTRouter : public Router {
      public:
        struct SMARTRequest {
          SMARTRouter * router;
          Flit * f;
          int initial_port;
          int initial_output_port; // Used in SMART_nD
          int input_port;
          int vc; //current VC
          int output_port;
          int vc_start;
          int vc_end;
          int distance;
          int hops;
          bool last_ssr; // used to produce speculative SSRs
          bool dim_change; // used to produce speculative SSRs
          bool spec; // speculative SSR or not
          bool dest_ssr; // used to bypass destination (nebb_vct_la)
          long sag_cycle; // used by nebb_vct_la to avoid speculation SSR creation in the same cycle
          int agregated_dist; // used by nebb_vct_la to implement the SSR-propagation based idea
        };

      protected:

        int _vcs;

        int _hpc_max;

        string _smart_dimensions;

        string _smart_priority;

        bool _smart_dest_bypass;

        // FIXME: Are any of the following structures used?
        vector<bool> _bw_enabled;
        enum BwState {bw_bypass, bw_local};
        vector<BwState> _bw_selector;
        enum XbarState {xbar_bypass, xbar_local};
        vector<XbarState> _xbar_selector;

        // Tracks if a flit has stopped prematurely, so the next body flit is stored too.
        vector<int> _prematurely_stop;

        vector<Buffer *> _buf;
        // FIXME: Probably these are not going to be used.
        vector<BufferState *> _next_buf;

        // Bypass Path. It stores the flit ID, if it matches then the flit can take the bypass
        vector<int> _bypass_path;
        // FIXME: Replace this with _cur_buf state info when using multi-flit packets
        vector<int> _dest_vc;
        vector<int> _dest_output;

        Allocator * _sw_allocator_local;
        vector<int> _sal_next_vc;
        vector<int> _sal_next_vc_counter;
        vector<Flit *> _sal_to_sag;
        vector<bool> _sal_o_winners;
        vector<bool> _bypass_credit;

        //Allocator * _sw_allocator_global;
        vector<Arbiter *> _sag_arbiters;
        vector<SMARTRequest> _sag_requestors;
        // SMART Request for each input port
        //vector<vector<SMARTRequest>> _sr_list;
        // FIXME: Unused for the moment.
        Allocator * _vc_allocator;
        //          output, FLIT
        vector<pair<int, Flit *>> _crossbar_flits;

        // processing cycle, flit
        vector<queue<pair<long, Flit *>>> _flits_to_BW;

        virtual void _InternalStep();

        tRoutingFunction   _rf;

        // Credit output buffer
        map<int, Credit *> _out_queue_credits;
        vector<queue<Credit *> > _credit_buffer;
        map<int, Credit *> _out_queue_smart_credits;
        vector<queue<Credit *> > _smart_credit_buffer;
        // FIXME: Hack to emulate OpenSMART consumption latency
        vector<queue<pair<long, Credit *>>> _destination_queue_credits;
        //vector<int> _destination_credit;

        // Avoid computation when there aren't flits in the router
        unsigned int _active;

        virtual void _OutputQueuing( );
        virtual void _SendCredits( );
      public:

        SMARTRouter( Configuration const & config,
                Module *parent, string const & name, int id,
                int inputs, int outputs );

        virtual ~SMARTRouter();

        virtual void SwitchAllocationLocal();

        // FIXME: should I make the following method private?
        void BufferWrite(int input, Flit * f);
        void SwitchTraversal();
        // FIXME: Not necessary, ST and LT are performed in the same cycle.
        //void LinkTraversal();
        virtual void TransferFlit(int input, int output, Flit * f);
        virtual void ReadFlit(int input, Flit * f);
        
        // Check if flit is latched or bypassed in next router.
        void EvaluateFlitNextRouter();
        // Setup SMART request in the next router
        void SMARTSetupRequest();
        // Arbitrate among SMART request in the next router
        virtual bool AddRequestSAG(SMARTRequest sr);
        // FIXME: Change the name of this function (?). This determines if there is an available VC (> -1)
        // or not (-1)
        virtual int FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance);
        virtual void VCManagement(int input, int in_vc, int output, Flit *f);
        bool FreeLocalBuf(int input, int vc_start, int vc_end);
        virtual int IdleLocalBuf(int input, int vc_start, int vc_end, Flit * f);

        virtual void SwitchAllocationGlobal();

        void ReadInputs();
        //void Evaluate();
        void WriteOutputs();

        // FIXME: This is dirty way of obtaining the whole
        // route path, so we can send SSR without modifying
        // the flit/packet route info.
        vector<SMARTRequest> GetFlitRoute(Flit * f, int input_port, int output_port);

        // Used to obtain the whole route path.
        virtual SMARTRouter * GetNextRouter(int next_output_port);
        const FlitChannel * GetOutputFlitChannel(int next_output_port) const;
       
        // TODO: The following methods are useless stuff, wtf are they purely virtual
        virtual int GetUsedCredit(int o) const { return 0;}
        virtual int GetUsedCreditVC(int o, int vc) const {return 0;} //(I)
        virtual int GetBufferOccupancy(int i) const {return 0;}

#ifdef TRACK_BUFFERS
        virtual int GetUsedCreditForClass(int output, int cl) const {return 0;};
        virtual int GetBufferOccupancyForClass(int input, int cl) const {return 0;};
#endif
        
        virtual vector<int> UsedCredits() const { vector<int> result; return result;}
        virtual vector<int> FreeCredits() const { vector<int> result; return result;}
        virtual vector<int> MaxCredits() const { vector<int> result; return result;}
        
        void Display( ostream & os = cout ) const;
    };
} // namespace Booksim


#endif
