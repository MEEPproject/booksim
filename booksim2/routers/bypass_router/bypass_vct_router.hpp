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

#ifndef _BYPASS_VCT_ROUTER_HPP_
#define _BYPASS_VCT_ROUTER_HPP_

#include <string>
#include <deque>
#include <queue>
#include <set>
#include <map>

#include "router.hpp"
#include "../routefunc.hpp"
#include "../arbiters/arbiter.hpp"

namespace Booksim
{

    using namespace std;

    class VC;
    class Flit;
    class Credit;
    class Buffer;
    class BufferState;
    class Allocator;
    //class SwitchMonitor;
    //class BufferMonitor;


    class BypassVCTRouter : public Router {
        struct BypassOutput {
            int output_port;
            int dest_vc;
            OutputSet la_route_set;
            //int pid;
        };

        // Number of virtual channels per channel
        int _vcs;

        // Is the router active?
        bool _active;
        
        vector<Credit *> _proc_credits;

        // Stage communication
        vector<pair<Flit *, int> > _crossbar_flits;

        // SA-I input (expanded inputs with flits)
        vector<bool> _switch_arbiter_input_flits;
        
        map<int, Flit *> _buffer_write_flits; // (output port, expanded_input)

        // Lookahead Check Conflict
        vector<pair<Lookahead *, int> > _lookahead_conflict_check_lookaheads; // (Lookahead, input)
        map<int, int> _lookahead_conflict_check_flits; // (output port, expanded_input)
        // SA-O input
        map<int, Flit *> _switch_arbiter_output_flits; // input, Flit

        /*

        /////////////////////////
        // Stage communication //
        /////////////////////////
        // Input_Stage
        // (input, flit)
        map<int, Flit *> _buffer_write_flits;
        // (time, (credit, output))
        deque<pair<int, pair<Credit *, int> > > _proc_credits;
        // (expanded_input, time)
        map<int, int> _sa_input_vcs;

        // Second Stage
        // (expanded_input, time)
        map<int, int> _sa_output_vcs;
        // (time, (lookahead, input))
        map<int, Lookahead *> _sa_output_la_inputs;

        // Output Stage FIXME: addapt this to my new nomenclature
        // (time, (Flit, (input, output))
        deque<pair<int, pair<Flit *, pair<int, int>>>> _crossbar_flits;

        // LT???
        map<int, Credit *> _out_queue_credits;
        map<int, Lookahead *> _out_queue_lookahead;
        */


        // Input buffer (input VCs)
        vector<Buffer *> _buf;
        // output buffer state (output VCs)
        vector<BufferState *> _next_buf;

        // SA arbiters
        vector<Arbiter *> _switch_arbiter_input;
        vector<Arbiter *> _switch_arbiter_output;
        // Round robin for dest buf
        vector<int> _sao_last_dest_vc_output;
        vector<int> _lacc_last_dest_vc_output;


        /*
        // Allocators
        //    SW-I as many allocators as inputs
        vector<Allocator *> _sw_input_allocator;
        //    To implement round roby policy. NOTE: Maybe there is a better solution using the arbiter class
        vector<int> _sw_input_allocator_last_vc;

        Allocator *_sw_output_allocator;
        Allocator * _vc_allocator;
        */

        // Routing function
        tRoutingFunction _rf;

        // Bypass paths
        vector<bool> _bypass_path;
        vector<int>  _vc_bypassing;
        //BSMOD: Change flit and packet id to long
        vector<long>  _pid_bypass; // This is only used for debugging
        vector<int> _output_strict_priority_vc;
        // Only for dateline routing: used to copy ph field from LA to a Flit
        vector<int> _dateline_partition;
        
        // Give priority to LA over flits: 1 or Flits over LA: 0
        bool _lookaheads_kill_flits;
                    
        // Output buffer size and output buffer
        int _output_buffer_size;
        vector<queue<Flit *> > _output_buffer;

        vector<Credit *> _credit_buffer;

        vector<Lookahead *> _lookahead_buffer;

        // Reads incoming flits
        bool _ReceiveFlits();
        // Reads incoming credits 
        bool _ReceiveCredits();
        // Reads incoming lookahead information
        bool _ReceiveLookahead();

        // Internal step (called by Network::Evaluate())
        virtual void _InternalStep();

        void _BufferWrite();
        void _SwitchArbiterInput();
        void _SwitchArbiterOutput();
        void _LookAheadConflictCheck();
        void _LookAheadRouteCompute(Flit *f, int output_port);

        // Output_Stage: ST
        void _Output_Stage();
        void _SwitchTraversal();

        // pre-LT
        void _OutputQueuing();

        // Used in WriteOuputs()
        void _SendFlits();
        void _SendCredits();
        void _SendLookahead();

        // disables bypass
        int _disable_bypass;
        bool _regain_bypass;

        // guarantee message order
        bool _guarantee_order;
        std::vector<std::vector<int>>_buffered_packet_outputs;


        public:

        BypassVCTRouter( Configuration const & config,
                Module *parent, string const & name, int id,
                int inputs, int outputs );

        virtual ~BypassVCTRouter( );

        virtual void ReadInputs( );
        virtual void WriteOutputs( );
        
        void Display( ostream & os = cout ) const;

        // FIXME: What is this shit.
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

    };
} // namespace Booksim

#endif
