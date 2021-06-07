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

#include "hybrid_fbfcl_router.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cassert>
#include <limits>
#include <climits>
#include <algorithm>

#include "../../globals.hpp"
#include "../../random_utils.hpp"
#include "../../vc.hpp"
#include "../../routefunc.hpp"
#include "../../outputset.hpp"
#include "../../buffer.hpp"
#include "../../buffer_state.hpp"
#include "../../arbiters/roundrobin_arb.hpp"
#include "../../allocators/allocator.hpp"
//#include "../power/switch_monitor.hpp"
//#include "../power/buffer_monitor.hpp"

namespace Booksim
{


    HybridFBFCLRouter::HybridFBFCLRouter( Configuration const & config, Module *parent, 
            string const & name, int id, int inputs, int outputs )
    : Router( config, parent, name, id, inputs, outputs )
    {
        // TODO: Check that this router is being constructed with route_delay=0 and noq=0,
        //       required for lookahead routing


        // Number of Virtual channel per channel.
        _vcs = config.GetInt("num_vcs");

        // Set all the bypass paths to false -> not taken
        _bypass_path.resize(_inputs*_vcs, false);
        _vc_bypassing.resize(_inputs, -1);
        _pid_bypass.resize(_inputs*_vcs,-1);
        _output_strict_priority_vc.resize(_outputs, -1);
        // Dateline partition: to copy from LA to Flit
        _dateline_partition.resize(_inputs*_vcs, -1);

        // Routing function
        string const rf = config.GetStr("routing_function") + "_" + config.GetStr("topology");
        map<string, tRoutingFunction>::const_iterator rf_iter = gRoutingFunctionMap.find(rf);
        if(rf_iter == gRoutingFunctionMap.end()) {
            Error("Invalid routing function: " + rf);
        }
        _rf = rf_iter->second;

        // Allocate Virtual Channels VC's
        // TODO: Can be these shared buffers?
        _buf.resize(_inputs);
        for(int i = 0; i < _inputs; ++i) {
            ostringstream module_name;
            module_name << "buf_" << i;
            _buf[i] = new Buffer(config, _outputs, this, module_name.str( ) );
            module_name.str("");
        }

        // Allocate next VCs' buffer state
        _next_buf.resize(_outputs);
        for(int j = 0; j < _outputs; ++j) {
            ostringstream module_name;
            module_name << "next_vc_o" << j;
            _next_buf[j] = new BufferState( config, this, module_name.str( ) );
            module_name.str("");
        }

        // SA-I round robin arbiters
        // TODO: we could define the type of arbiter from the config file
        _switch_arbiter_input.resize(_inputs);
        for(int input=0; input < _inputs; input++) {
            ostringstream module_name;
            module_name << "SA-I_" << input;
            _switch_arbiter_input[input] = Arbiter::NewArbiter(this, module_name.str(), "round_robin", _vcs);
            //_switch_arbiter_input[input] = Arbiter::NewArbiter(this, module_name.str(), "matrix", _vcs);
        }

        _switch_arbiter_input_flits.resize(_inputs*_vcs, false);

        // SW-O allocator
        _switch_arbiter_output.resize(_outputs);
        for(int output=0; output < _outputs; output++) {
            ostringstream module_name;
            module_name << "SA-O_" << output;
            _switch_arbiter_output[output] = Arbiter::NewArbiter(this, module_name.str() , "matrix", _inputs);
        }

        // Output queues
        _output_buffer_size = config.GetInt("output_buffer_size");
        _output_buffer.resize(_outputs); 
        _credit_buffer.resize(_inputs); 
        _lookahead_buffer.resize(_inputs); 
        
        _proc_credits.resize(_outputs,NULL);

        // Option to deactivate bypass
        _disable_bypass = config.GetInt("disable_bypass");
        _regain_bypass = config.GetInt("regain_bypass");

        // Give priority to LA over flits: 1 or Flits over LA: 0
        _lookaheads_kill_flits = config.GetInt("lookaheads_kill_flits");
        
        // Policy to place a request in the input switch arbiter
        _switch_arbiter_input_policy = config.GetStr("switch_arbiter_input_policy");
        _switch_arbiter_input_winner.resize(_inputs,-1); // Used by round_robin_on_miss

#ifdef TRACK_FLOWS
        for(int c = 0; c < _classes; ++c) {
            _stored_flits[c].resize(_inputs, 0);
            _active_packets[c].resize(_inputs, 0);
        }
#endif
        // Guarantee message order
        _guarantee_order = config.GetInt("guarantee_order");
        // We track the number of packets that use a cer
        _buffered_packet_outputs.resize(_inputs, vector<int>(_outputs, 0));
    }

    HybridFBFCLRouter::~HybridFBFCLRouter( )
    {
        // Delete buffers
        for(int i = 0; i < _inputs; ++i) {
            delete _buf[i];
            delete _switch_arbiter_input[i];
        }

        // Delete next buffer state
        for(int j = 0; j < _outputs; ++j) {
            delete _next_buf[j];
            delete _switch_arbiter_output[j];
        }
    }

    // The following three methods are evaluated cycle by cycle
    // Read and prepare flits, credits and lookahead information for pipeline stages
    // FIXME
    void HybridFBFCLRouter::ReadInputs()
    {
        // Read incoming flits
        bool have_flits = _ReceiveFlits();
        // Read incoming credits
        bool have_credits = _ReceiveCredits();
        // Read incoming lookahead requests
        bool have_lookahead = _ReceiveLookahead();

        // Is the router active?
        _active = _active || have_flits || have_credits || have_lookahead;
    }

    void HybridFBFCLRouter::_InternalStep()
    {
        if(!_active){
            return;
        }
        // 3rd stage
        if(!_crossbar_flits.empty())
            _SwitchTraversal();
        // 2nd stage
        if(!_switch_arbiter_output_flits.empty())
            _SwitchArbiterOutput();
        if(!_lookahead_conflict_check_flits.empty() || !_lookahead_conflict_check_lookaheads.empty())
            _LookAheadConflictCheck();
        // 1st stage
        _SwitchArbiterInput();

        _active = !_crossbar_flits.empty() ||
                  !_switch_arbiter_output_flits.empty() ||
                  !_lookahead_conflict_check_flits.empty() ||
                  !_lookahead_conflict_check_lookaheads.empty();
    }

    void HybridFBFCLRouter::WriteOutputs()
    {
        _SendFlits();
        _SendCredits();
        _SendLookahead();
    }

    //------------------------------------------------------------------------------
    // read inputs
    //------------------------------------------------------------------------------

    bool HybridFBFCLRouter::_ReceiveFlits()
    {
        bool activity = false;

        // Check the input channels
        for(int input = 0; input < _inputs; ++input) { 
            Flit * const f = _input_channels[input]->Receive();
            if(f) {

#ifdef TRACK_FLOWS
                ++_received_flits[f->cl][input];
#endif

#ifdef FLIT_DEBUG
                if(f->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                << " | Stage: ReceiveFlits | Flit: " << f->id << " pid " << f->pid << " head? "
                                << f->head << " tail? " << f->tail << " | Input: " << input << " VC: " << f->vc
                                << " | Bypass? " << _bypass_path[input*_vcs+f->vc] << " for pid: " << _pid_bypass[input*_vcs+f->vc]
                                << std::endl;
                }
#endif
                // Bypass pipeline
                if(_bypass_path[input*_vcs+f->vc]) {

                    int const in_vc = f->vc;
                    // Read destination port and vc
                    Buffer * cur_buf = _buf[input];
                    int output_requested = cur_buf->GetOutputPort(in_vc);
                    f->vc = cur_buf->GetOutputVC(in_vc);
                    OutputSet const * const route_set = cur_buf->GetRouteSet(in_vc);

                    
                    // Copy lookahead route to control data of flit.
                    if(f->head) {
                        // Flit at destination (is this always true?)
                        bool is_router = _output_channels[output_requested]->GetSink() != NULL ? true : false;
                        if(route_set->GetSet().size() == 0 || !is_router) // The last part in which I compare the output requested is part of the memory leak fix for lookaheads. TODO: Move all of these to LookAheadConflictCheck avoiding the creation of the lookahead when the flit arrives to the last router (also avoiding routecomputation)
                            f->la_route_set.Clear();
                        else
                            f->la_route_set = * route_set;
                    }
                    
                    f->ph = _dateline_partition[input*_vcs+in_vc];

                    // Move flit to ST stage 
                    _crossbar_flits.push_back(make_pair(f, output_requested));
                        
                    _bypass_path[input*_vcs+in_vc] = false;

                    // Required for optimization FIXME: What is this for?
                    if(_output_strict_priority_vc[output_requested] != input*_vcs+in_vc) {
                        // Unblock output port
                        _pid_bypass[input*_vcs+in_vc] = -1;
                    }

                    // If this is the last flit of the packet => free VC:
                    if(f->tail) {
                        // Unblock output port
                        if(_output_strict_priority_vc[output_requested] == input*_vcs+in_vc) {
                            _output_strict_priority_vc[output_requested] = -1;
                        }
                        _pid_bypass[input*_vcs+in_vc] = -1;
                        // Clear input VC info
                        if (cur_buf->GetExpectedPID(in_vc) == f->pid) {
                            cur_buf->ReleaseVC(in_vc); // _expected_pid is only used for debbuging purposes
                        }
                        cur_buf->SetOutput(in_vc,-1,-1);
                        cur_buf->SetState(in_vc,VC::idle);
                    }

#ifdef FLIT_DEBUG
                    if(f->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName() 
                                    << " | Stage: Bypassing flit | Flit: " << f->id << " pid " << f->pid
                                    << " head? " << f->head << " tail? " << f->tail << " | Input: " << input
                                    << " | Bypass? " << _bypass_path[input*_vcs+f->vc] << " bypass output port "
                                    << output_requested << " dest vc " << f->vc 
                                    << " blocked_output? " << _output_strict_priority_vc[output_requested]
                                    << std::endl;
                    }
#endif
                }
                // Non-Bypass pipeline
                else {
                    // Write flit in input VC
                    _buffer_write_flits[input] = f;
                }
                activity = true;
            }
        }
        _BufferWrite();
        return activity;
    }

    bool HybridFBFCLRouter::_ReceiveCredits()
    {
        bool activity = false;

        // Check the output channels for credits
        for(int output = 0; output < _outputs; ++output) {  
            Credit * const c = _output_credits[output]->Receive();
            if(c)
            {
                BufferState * const dest_buf = _next_buf[output];
#ifdef CREDIT_DEBUG
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName() 
                            << " | Stage: ReceiveCredits| Credit: " << c->id << " Credit head? "
                            << c->head << " tail? " << c->tail << " | Output: " << output << " | Occupancy "
                            << dest_buf->Occupancy() << " VC: " << *(c->vc.begin()) << " VC Occupancy "
                            << dest_buf->OccupancyFor(*(c->vc.begin())) << std::endl;
#endif
                // Increase credit count
                dest_buf->ProcessCredit(c);

                // Delete credit
                c->Free();
                
                activity = true;
            }
        }
        return activity;
    }

    bool HybridFBFCLRouter::_ReceiveLookahead()
    {
        bool activity = false;

        // Check if topology/network supports lookahead signals.
        assert((int)_input_lookahead.size() == _inputs);

        // Read lookahead signals
        for(int input = 0; input < _inputs; ++input) {

            Lookahead * const la = _input_lookahead[input]->Receive();
            if(la) {

                // Delete lookaheads if bypass is disabled
                if(_disable_bypass == 1) {
                    la->Free();     
                    continue;
                }

#ifdef LOOKAHEAD_DEBUG 
                if(la->watch) 
                {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                << " | Stage: ReceiveLookahead | Lookahead: " << la->id << " head? "
                                << la->head << " tail? " << la->tail << " | Input: " << input << " VC: "
                                << la->vc << std::endl;
                }
#endif
                // Move lookaheads to LookAhead-ConflictCheck (LA-CC)
                _lookahead_conflict_check_lookaheads.push_back(make_pair(la, input));

                activity = true;
            }
        }

        return activity;
    }

    void HybridFBFCLRouter::_SwitchTraversal( )
    {
        // Read ready flits (flit, output port))
        for(auto const iter : _crossbar_flits) {

            Flit * const f = iter.first;
            f->hops++;
            int const output = iter.second;

#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                            << " | Stage: SwitchTraversal | Flit: " << f->id << " pid " << f->pid
                            << " head? " << f->head << " tail? " << f->tail << " | Output: " << output << std::endl;
            }
#endif

            // Store the flit in the output buffer (It should have slot only for one flit)
            _output_buffer[output].push(f);
        }
        // Clear flits from ST input registers
        _crossbar_flits.clear();
    }

    void HybridFBFCLRouter::_SwitchArbiterOutput()
    {

#ifdef TRACK_STALLS
        enum ConflictStates {ei_crossbar_conflicts, ei_buffer_busy, ei_buffer_reserved, ei_buffer_full, ei_output_blocked, ei_winner};
        pair<int,int> conflicts_record[_inputs];
        ConflictStates cs = ei_crossbar_conflicts;
#endif

        // Read flits that are in SA-O (expanded input)
        for(auto const iter : _switch_arbiter_output_flits) {

            // Read input and input vc
            int const input = iter.first;
            Flit const * const f = iter.second;
            assert(f);
            int const in_vc = f->vc;

            // Read output requested by the flit
            Buffer * const cur_buf = _buf[input];
            
            if(cur_buf->GetState(in_vc) == VC::idle && f->head)
            {
                cur_buf->SetState(in_vc, VC::sa_output);
            }
            
            assert(cur_buf->GetState(in_vc) == VC::sa_output || cur_buf->GetState(in_vc) == VC::active);
           
            // VA: Check if there is a destination VC free and with room for the flit
            if(f->head) {

#ifdef FLIT_DEBUG
                if(f->watch) {
                    *gWatchOut  << "Cycle " << GetSimTime() << " Name " << FullName() << " SA-O | Flit: "
                                << f->id << " input " << input << " in vc " << in_vc << " cur_buf->GetState(in_vc) "
                                << cur_buf->GetState(in_vc) << " VC::sa_output " << VC::sa_output << std::endl;
                }
#endif

                
                // FIXME: This assert could be important
                if(cur_buf->GetState(in_vc) == VC::active)
                {
                    continue;
                }
                assert(cur_buf->GetState(in_vc) == VC::sa_output);

                // Read packet route from input VC
                set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                
                bool stop = false; // Used to point out an available destination VC

                // Iterate through all the posible routes (output ports and destinations VCs)
                for(auto iter : route) {        
                    int output_port = iter.output_port;

                    // Check if there is room for the flit
                    BufferState const * const dest_buf = _next_buf[output_port];
                    
                    int dest_vc = dest_buf->GetAvailVCMinOccupancy(iter.vc_start, iter.vc_end);
                    if(dest_vc > -1) {
                    //for(int dest_vc = iter.vc_start; dest_vc <= iter.vc_end; dest_vc++) {

#ifdef FLIT_DEBUG
                        if(f->watch) {
                            *gWatchOut  << "Cycle " << GetSimTime() << " Name " << FullName() << " SA-O | Flit: "
                                        << f->id << " input " << input << " in vc " << in_vc
                                        << " cur_buf->GetState(in_vc) " << cur_buf->GetState(in_vc)
                                        << " VC::sa_output " << VC::sa_output << " dest_vc " << dest_vc
                                        << " output " << output_port
                                        << dest_buf->AvailableFor(dest_vc) << std::endl;
                                        
                            dest_buf->Display(*gWatchOut);
                        }
#endif
                        // Check if packet will change of dimension
                        bool dimension_change = DimensionChange(input, output_port);
                        // FIXME: With the new method GetAvailVCMinOccupancy(start_vc, end_vc) this is not required
                        if(dest_buf->IsAvailableFor(dest_vc) &&
                           (dimension_change ? dest_buf->AvailableFor(dest_vc) > f->packet_size :
                                               dest_buf->AvailableFor(dest_vc) > 0)
                        ) {

#ifdef TRACK_STALLS
                            cs = ei_crossbar_conflicts;
                            conflicts_record[input] = make_pair(cs,f->cl);
#endif
                            //// Add request to SA-O
                            //if(f->head) {
                                _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, f->pri);
                            //} else {
                            //    _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, INT_MAX);
                            //}
                    
                            // Set output for the case of being the winner,
                            // in other case it will be override as this input VC is used by this packet
                            cur_buf->SetOutput(in_vc, output_port, dest_vc);
#ifdef FLIT_DEBUG
                            if(f->watch) {
                                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " head Flit: "
                                            << f->id << " , sets VC's " << in_vc << " output" << std::endl;
                            }
#endif
                            // Point out that destination VC found, stop loop through output ports
                            stop = true;
                            break;
                        }
#ifdef TRACK_STALLS
                        else
                        {
                            if(!dest_buf->IsAvailableFor(dest_vc)) {
                                cs = ei_buffer_busy;
                                if(!(dest_buf->AvailableFor(dest_vc) > 0))
                                {
                                    cs = ei_buffer_full;
                                }else if(!dest_buf->IsFull())
                                {
                                    cs = ei_buffer_reserved;
                                }
                            }
                            conflicts_record[input] = make_pair(cs,f->cl);
                        }
#endif
                        
                    }

                    // A destionation VC was found, stop loop through output ports
                    if(stop)
                    {
                        break;
                    }
                }
            }
            // Body flits check only if there is room for the flit (do not to perform VA)
            else {

                // Read output port and destination VC from info stored in input VC
                assert(cur_buf->GetState(in_vc) == VC::active);
                int output_port = cur_buf->GetOutputPort(in_vc);
                int dest_vc = cur_buf->GetOutputVC(in_vc);

                BufferState const * const dest_buf = _next_buf[output_port];

#ifdef FLIT_DEBUG
                if(f->watch)
                {
                    dest_buf->Display(*gWatchOut);
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " SA-O | Flit (Body) " << f->id
                                << " dest_buf->AvailableFor(" << dest_vc << ") "
                                << dest_buf->AvailableFor(dest_vc) << " > 0 && dest_buf->UsedBy("
                                << dest_vc << ") " << dest_buf->UsedBy(dest_vc) << " == f->pid "
                                << f->pid 
                                << std::endl;
                }
#endif

                if(dest_buf->AvailableFor(dest_vc) > 0)
                {

#ifdef FLIT_DEBUG
                    if(f->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName()
                                    << " Added Request to switch arbiter output. In vc " << in_vc
                                    << " output port " << output_port << " Flit " << f->id
                                    << " Flit head? " << f->head << " Flit pid " << f->pid << std::endl;
                    }
#endif //FLIT_DEBUG

                    // Add Request to the output port SA-O arbiter
                    _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, INT_MAX);
                }
            }
        }

        // Perform SA-O arbitration
        for(int output = 0; output < _outputs; output++)
        {
            // Read expanded input of SA-O winner
            //BSMOD: Change flit and packet id to long
            long expanded_input;
            int priority;
            if(_switch_arbiter_output[output]->Arbitrate(&expanded_input,&priority) > -1) {

                // Move flit to LA-CC stage
                _lookahead_conflict_check_flits[output] = expanded_input;

#ifdef TRACK_STALLS
                int const input = expanded_input/_vcs;
                Flit * fstall = _switch_arbiter_output_flits[input];
                cs = ei_winner;
                conflicts_record[input] = make_pair(cs,fstall->cl);
#endif

#ifdef FLIT_DEBUG
                int const input = expanded_input / _vcs;
                int const in_vc = expanded_input % _vcs;
                Flit * f = _switch_arbiter_output_flits[input];
                if(f->watch) {
                    *gWatchOut << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " SA-O Winner. In vc "
                        << in_vc << " output" << ". Read (l: 514) output port " << output << " Flit "
                        << f->id << " Flit head? " << f->head << " Flit pid " << f->pid << " VC "
                        << f->vc << std::endl;
                }
#endif //FLIT_DEBUG
            }
            // Clear request from output arbiter
            _switch_arbiter_output[output]->Clear();
        }
        // Cannot be more winners than outputs
        assert((int)_lookahead_conflict_check_flits.size() <= _outputs);

#ifdef TRACK_STALLS
        for(int i = 0; i < _inputs; i++) {
            switch(conflicts_record[i].first) {
                case ei_buffer_busy:
                    ++_buffer_busy_stalls[conflicts_record[i].second];
                    break;
                case ei_buffer_reserved:
                    ++_buffer_reserved_stalls[conflicts_record[i].second];
                    break;
                case ei_buffer_full:
                    ++_buffer_full_stalls[conflicts_record[i].second];
                    break;
                case ei_crossbar_conflicts:
                    ++_crossbar_conflict_stalls[conflicts_record[i].second];
                    break;
                case ei_output_blocked:
                    ++_output_blocked_stalls[conflicts_record[i].second];
                    break;


            }
        }
#endif
    }

    void HybridFBFCLRouter::_LookAheadConflictCheck()
    {

#ifdef LOOKAHEAD_DEBUG
        bool watch_arbiter = false;
#endif

#ifdef TRACK_STALLS
        enum ConflictStates {ei_crossbar_conflicts, ei_buffer_busy, ei_buffer_reserved, ei_buffer_full, ei_output_blocked, ei_ignore};
        pair<int,int> conflicts_record[_inputs];
        ConflictStates cs = ei_crossbar_conflicts;
#endif

        // Used to take lookahead winners from arbitration easily
        map<int, pair<Lookahead*, BypassOutput>> la_id_to_ptr;

        // Add lookahead request for destination VC if possible (Lookahead *, input) 
        for(auto const iter : _lookahead_conflict_check_lookaheads) {

            Lookahead * la = iter.first;
            int const input = iter.second;
            int in_vc = la->vc;

#ifdef LOOKAHEAD_DEBUG
            if(la->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                    << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id
                    << " pid " << la->pid << " head? " << la->head << " tail? " << la->tail << std::endl;
                //watch_arbiter = true;
            }
#endif

#ifdef TRACK_STALLS
            cs = ei_crossbar_conflicts;
            conflicts_record[input] = make_pair(cs,la->cl);
#endif

            // This router works with lookaheads of packets (i.e. head flits)
            if(la->head) {

                Buffer * const cur_buf = _buf[input];
                if(cur_buf->GetState(in_vc) == VC::active) {
                    continue; // Input VC has a packet that is moving
                }

                // Check if there is destination VC available from the route list 
                set<OutputSet::sSetElement> const route = la->la_route_set.GetSet();

                bool stop = false; // To point out that a destination VC was found

                for(auto route_iter : route) {

#ifdef LOOKAHEAD_DEBUG
                    if(la->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                            << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                            << la->pid << " head? " << la->head << " tail? " << la->tail
                            << " route - output:" << route_iter.output_port << " vc_start: " << route_iter.vc_start
                            << " vc_end: " << route_iter.vc_end << " Blocked output? " << _output_strict_priority_vc[route_iter.output_port]
                            << std::endl;
                    }
#endif

                    // Check if packet could potentially break message order
                    if(_guarantee_order && 
                       _buffered_packet_outputs[input][route_iter.output_port] > 0) {
                        continue;
                    }
            
                    if(_output_strict_priority_vc[route_iter.output_port] > -1 && !cur_buf->Empty(in_vc)) { // If output is being used to bypass another packet ignore this lookahead to avoid conflict between body LA.
                        continue;
                    }

                    BufferState const * const dest_buf = _next_buf[route_iter.output_port];

                    int dest_vc = dest_buf->GetAvailVCMinOccupancy(route_iter.vc_start, route_iter.vc_end);
                    if(dest_vc > -1) {
                        
                        bool dimension_change = DimensionChange(input, route_iter.output_port);
                        
                        bool same_dimension_cond = (cur_buf->Empty(in_vc) && dest_buf->AvailableFor(dest_vc) > 0 ) ||
                                              (!cur_buf->Empty(in_vc) && dest_buf->AvailableFor(dest_vc) >= la->packet_size);

                        if(dest_buf->IsAvailableFor(dest_vc) &&
                            ( dimension_change ? dest_buf->AvailableFor(dest_vc) > la->packet_size :
                                                 same_dimension_cond )
                        ){

                    //for(int dest_vc = route_iter.vc_start; dest_vc <= route_iter.vc_end; dest_vc++)
                    //{

                            // Save output port and destination for arbitration, if required
                            BypassOutput bypass_state;
                            bypass_state.output_port = route_iter.output_port;
                            bypass_state.dest_vc = dest_vc;

                            // Add lookahead request to Output arbiter
                            if(cur_buf->Empty(in_vc)) {
                                _switch_arbiter_output[bypass_state.output_port]->AddRequest(input, la->id , la->pri+100);
                            } else {
                                _switch_arbiter_output[bypass_state.output_port]->AddRequest(input, la->id , la->pri);
                            }

                            la_id_to_ptr[la->id] = make_pair(la,bypass_state);

                            // Point out to the output port loop that a destination VC was found
                            stop = true;
#ifdef LOOKAHEAD_DEBUG
                            if(la->watch) {
                                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                    << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                                    << la->pid << " head? " << la->head << " tail? " << la->tail
                                    << " Sets bypass state to: output port " << bypass_state.output_port << " dest_vc "
                                    << bypass_state.dest_vc << std::endl;
                                watch_arbiter = true;
                            }
#endif
                            // Stop destination VC loop
                            break;
                        }
#ifdef TRACK_STALLS
                        else
                        {
                            if(!dest_buf->IsAvailableFor(dest_vc)) {
                                cs = ei_buffer_busy;
                                if(!(dest_buf->AvailableFor(dest_vc) > 0))
                                {
                                    cs = ei_buffer_full;
                                }else if(!dest_buf->IsFull())
                                {
                                    cs = ei_buffer_reserved;
                                }
                            }
                            conflicts_record[input] = make_pair(cs,la->cl);
                        }
#endif
                    }

                    // A destination VC was found, stop search
                    if(stop) {
                        break;
                    }
                }
            }
            else { // Check if this body lookahead has to decrease credit count

                Buffer * const cur_buf = _buf[input];

                int dest_vc = cur_buf->GetOutputVC(la->vc);
                int output = cur_buf->GetOutputPort(la->vc);

#ifdef LOOKAHEAD_DEBUG
                if(la->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                        << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                        << la->pid << " head? " << la->head << " tail? " << la->tail
                        << " Input: " << input << " in VC: " << la->vc << " Bypass? " << _bypass_path[input*_vcs+la->vc]
                        << " Cur Buf Empty? " << cur_buf->Empty(la->vc) << " Dest VC: " << dest_vc << " Output " << output
                << " _output_strict_priority_vc " << _output_strict_priority_vc[output]
                        //<< " Blocked Output? " << _blocked_output[output]
                        << std::endl;
                }
#endif
                
                // Check if output is set
                if(output == -1) {
                    continue;
                }
                
                BufferState const * const dest_buf = _next_buf[output];
                if(dest_buf->AvailableFor(dest_vc) < 1 && _output_strict_priority_vc[output] != input*_vcs+in_vc) {
                    continue;
                }

#ifdef LOOKAHEAD_DEBUG
                if(la->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                        << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                        << la->pid << " head? " << la->head << " tail? " << la->tail
                        << " Dest VC Available For: " << dest_buf->AvailableFor(dest_vc)
                        << " Dest VC in use by PID: " << dest_buf->UsedBy(dest_vc)
                        << " Blocked Output? " << _output_strict_priority_vc[output]
                        << std::endl;
                }
#endif


                // Add lookahead request to Output arbiter
                if(_output_strict_priority_vc[output] == input*_vcs+la->vc) {
                    _switch_arbiter_output[output]->AddRequest(input, la->id , INT_MAX);
                } else if(cur_buf->Empty(la->vc)) {
                    _switch_arbiter_output[output]->AddRequest(input, la->id , INT_MAX-1);
                } else {
                    continue;
                }
                
                // Save output port and destination for arbitration, if required
                BypassOutput bypass_state;
                bypass_state.output_port = output;
                bypass_state.dest_vc = dest_vc;

                la_id_to_ptr[la->id] = make_pair(la,bypass_state);
#ifdef LOOKAHEAD_DEBUG
                if(la->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                        << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                        << la->pid << " head? " << la->head << " tail? " << la->tail
                        << " Sets bypass state to: output port " << bypass_state.output_port << " dest_vc "
                        << bypass_state.dest_vc << std::endl;
                    watch_arbiter = true;
                }
#endif
            }
        }
                    
        // Arbitration among Lookahead requests
        for(int output = 0; output < _outputs; output++)
        {

            Lookahead * la;
            //BSMOD: Change flit and packet id to long
            long la_id;
            int input;
            int priority;
            // Read winner
            input = _switch_arbiter_output[output]->Arbitrate(&la_id,&priority);
            if(input > -1) {
                
                la = la_id_to_ptr[la_id].first;
                /*
                // Kill SA-O winner
                if(_lookahead_conflict_check_flits.find(output) != _lookahead_conflict_check_flits.end()){
                    int const f_expanded_input = _lookahead_conflict_check_flits[output];
                    int const f_input = f_expanded_input / _vcs;
                    int const f_in_vc = f_expanded_input % _vcs;
                    Buffer * const f_cur_buf = _buf[f_input];
                    if(f_cur_buf->FrontFlit(f_in_vc)->head && _lookaheads_kill_flits || !la->head){
                        _lookahead_conflict_check_flits.erase(output);
                    }else{
                        // Ignore this winner because SA-O's winner has priority.
                        // Clear requests to switch arbiter
                        _switch_arbiter_output[output]->Clear();
                        continue;
                    }
                }
                */
                /*
                if(_lookaheads_kill_flits){
#ifdef TRACK_STALLS
                    ++_la_sa_winners_killed[la->cl];
#endif
                    _lookahead_conflict_check_flits.erase(output);
                } else if(_lookahead_conflict_check_flits.find(output) != _lookahead_conflict_check_flits.end()){
                    // Clear requests to switch arbiter
                    _switch_arbiter_output[output]->Clear();
                    continue;
                }
                */
                // Kill SA-O winner
                if(_lookahead_conflict_check_flits.find(output) != _lookahead_conflict_check_flits.end()){
                    if(_lookaheads_kill_flits || (!la->head && _output_strict_priority_vc[output] == input*_vcs+la->vc)){
                        _lookahead_conflict_check_flits.erase(output);
                    }else{
                        // Ignore this winner because SA-O's winner has priority.
                        // Clear requests to switch arbiter
                        _switch_arbiter_output[output]->Clear();
                        continue;
                    }
                }

                BypassOutput bypass_state = la_id_to_ptr[la_id].second;

#ifdef FLIT_DEBUG
                Flit * const f = _switch_arbiter_output_flits[input];
                if(f) {
                    if(f->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " flit : "
                                    << f-> id << " won SA-O and killed in LA-CC by LA " << la->id << " in vc " 
                                    << la->vc << " output" << std::endl;
                    }
                }
#endif


                int in_vc = la->vc;

                BufferState * const dest_buf = _next_buf[output];
                    
                Lookahead * la_n = new Lookahead(la);
                la_n->vc = bypass_state.dest_vc;
                
                Buffer * const cur_buf = _buf[input];
                        
                bool vct_request = false;
                
                if(la->head){
            
            // Compute lookahead routing
                    _LookAheadRouteCompute(la_n, bypass_state.output_port);
                    _dateline_partition[input*_vcs+in_vc] = la_n->ph;

                    // Change input VC state to active
                    cur_buf->SetState(in_vc, VC::active);
                    // Save route and output in the input VC
                    cur_buf->SetRouteSet(in_vc, &la_n->la_route_set);
                    cur_buf->SetOutput(in_vc,bypass_state.output_port, bypass_state.dest_vc);
                    
                    
                    // Take destination buffer
                    dest_buf->TakeBuffer(la_n->vc, la_n->pid);
                    
                    if(!cur_buf->Empty(in_vc)) {
                        _output_strict_priority_vc[bypass_state.output_port] = input*_vcs+in_vc;
                        vct_request = true;
                    }
                    _pid_bypass[input*_vcs+in_vc] = la->pid;
                }
                    
                _bypass_path[input*_vcs+in_vc] = true;
            
            // For body flits:
            if ( _output_strict_priority_vc[bypass_state.output_port] == input*_vcs+in_vc) {
                vct_request = true;
            }
                
                // Decrease credit count
                bool dimension_change = DimensionChange(input, bypass_state.output_port);
                dest_buf->SendingFlit(la_n, vct_request || dimension_change);

#ifdef TRACK_STALLS
                    cs = ei_ignore;
                    conflicts_record[input] = make_pair(cs,la_n->cl);
#endif
                    
#ifdef LOOKAHEAD_DEBUG
                if(la->watch || watch_arbiter) {
                    *gWatchOut << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                        << " | Stage: LookAhead Conflict Check Winner (arbitration) | Lookahead: " << la->id << " pid "
                        << la->pid << " head? " << la->head << " tail? " << la->tail << " Preparing credit: input "
                        << input << " VC: " << in_vc << " output port " << output << std::endl;
                }
#endif
                // Send credit back, as the corresponding flit is not going to be stored
                if(_credit_buffer[input]) {
                    _credit_buffer[input]->vc.insert(in_vc);
                }
                else {
                    Credit * c = Credit::New();
                    c->vc.insert(in_vc);
                    c->id = la->id;
                    _credit_buffer[input] = c;
#ifdef CREDIT_DEBUG
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                    << " | Credit: " << c->id << " creation ID ReceiveFlits-Bypass (l:251)"
                                    << " input " << input << " in vc " << in_vc << std::endl;
#endif

                }
                // Send Lookahead to the next router
                _lookahead_buffer[output] = la_n;
            } // REMOVE ME

                // Clear requests to switch arbiter
            _switch_arbiter_output[output]->Clear();
        }

        // Grant SA-O that weren't killed (output port, expanded_input)
        for(auto const iter : _lookahead_conflict_check_flits) 
        {

            int const output_port = iter.first;
            int const input = iter.second / _vcs;
            int const in_vc = iter.second % _vcs;
            Buffer * const cur_buf = _buf[input];
            
            // We have to read this flit from SA-O input buffer
            Flit * const f = _switch_arbiter_output_flits[input];

            // ignore if input is going to bypass a flit next cycle
            bool bypass_next_cycle = false;
            for(int vc=0; vc < _vcs; vc++) {
                if(_bypass_path[input*_vcs+vc]) bypass_next_cycle = true;
            }
            if(bypass_next_cycle) {
#ifdef TRACK_STALLS
               ++_la_sa_winners_killed[f->cl];
#endif
                continue;
            }


            // Get destination VC
            Flit * f_n = f; 
            f_n->vc = cur_buf->GetOutputVC(in_vc);

            // Move flit to ST
            _crossbar_flits.push_back(make_pair(f_n,output_port));
            // Switch Arbiter Policy (round_robin_on_miss)
            _switch_arbiter_input_winner[input] = in_vc;
            
            // Take destination VC and set ouput port, route and destination vc
            BufferState * const dest_buf = _next_buf[output_port];

            if(f->head) {
                dest_buf->TakeBuffer(f_n->vc,f_n->pid);
                cur_buf->SetState(in_vc, VC::active);
                cur_buf->SetRouteSet(in_vc, &f->la_route_set);
                cur_buf->SetOutput(in_vc, output_port, f_n->vc);
            }

#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " flit : "
                            << f-> id << " won SA-O and LA-CC input " << input << " in vc " 
                            << in_vc << " output" << std::endl;
            }
#endif
            
            // Compute Lookahead route
            if(f->head){
                _LookAheadRouteCompute(f_n, output_port);
                _dateline_partition[input*_vcs+in_vc] = f_n->ph;
            }
            
            if(_regain_bypass){
                Lookahead * la_n = new Lookahead(f_n);
                // Send Lookahead to the next router
                _lookahead_buffer[output_port] = la_n;
            }

            // Decrement destination VC credit count
            bool dimension_change = DimensionChange(input, output_port);
            dest_buf->SendingFlit(f_n, dimension_change);
            
            // Send credit.
            if(_credit_buffer[input])
            {
                _credit_buffer[input]->vc.insert(in_vc);
            }
            else
            {
                Credit * c = Credit::New();
                c->vc.insert(in_vc);
                c->id = f->id;
                _credit_buffer[input] = c;
            }

            // Remove flit from SA-O input buffer
            cur_buf->RemoveFlit(in_vc);
            
            // Message order
            if(_guarantee_order) {
                _buffered_packet_outputs[input][output_port]--;
            }
            
            if(cur_buf->Empty(in_vc))    
            {
                _switch_arbiter_input_flits[iter.second] = false;
            }

            if(f->tail)
            {
                // Clear input VC info
                cur_buf->SetOutput(in_vc, -1, -1);
                cur_buf->SetState(in_vc,VC::idle);
#ifdef FLIT_DEBUG
                if(f->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " tail flit clears VC's: "
                                << in_vc << " output" << std::endl;
                }
#endif
            }
        }

#ifdef TRACK_STALLS
        for(int i = 0; i < _inputs; i++) {
            switch(conflicts_record[i].first) {
                case ei_buffer_busy:
                    ++_la_buffer_busy[conflicts_record[i].second];
                    break;
                case ei_buffer_reserved:
                    ++_la_buffer_reserved[conflicts_record[i].second];
                    break;
                case ei_buffer_full:
                    ++_la_buffer_full[conflicts_record[i].second];
                    break;
                case ei_crossbar_conflicts:
                    ++_la_crossbar_conflict[conflicts_record[i].second];
                    break;
                case ei_output_blocked:
                    ++_la_output_blocked[conflicts_record[i].second];
                    break;
            }
        }
#endif

        // Free lookahead memory
        for(auto iter : _lookahead_conflict_check_lookaheads) 
        {
            iter.first->Free();
        }

        // Clear LA-CC inputs
        _lookahead_conflict_check_flits.clear();
        _lookahead_conflict_check_lookaheads.clear();
        _switch_arbiter_output_flits.clear();
    }

    // Next Hop Routing
    void HybridFBFCLRouter::_LookAheadRouteCompute(Flit * f, int output_port) {

#ifdef FLIT_DEBUG
                if(f->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() 
                                << " LookAheadRouteCompute of Flit/Lookahead (l: 835): " << f->id << std::endl;
                }
#endif

        // Read current route 
        assert(f);
        f->la_route_set.Clear();
        // Load output channel and get next router
        const FlitChannel * channel = _output_channels[output_port];
        const Router * router = channel->GetSink();
        // If the output terminal of the channel is a router:
        if(router) {
            int in_channel = channel->GetSinkPort();
            OutputSet nos;
            // Computes next hope route
            _rf(router, f, in_channel, &nos, false);
            f->la_route_set = nos;
            // FIXME: add route
        }
    }

    // Buffer Write
    void HybridFBFCLRouter::_BufferWrite()
    {
        // Read all flits in the input of the BW stage
        for(auto const iter : _buffer_write_flits) {
        
            // Read flit to write 
            int const input = iter.first;
            Flit * const f = iter.second;

            int const in_vc = f->vc;

            // Message order
            if(_guarantee_order) {
                int output_port = -1;
                
                //FIXME: we are supposing that there is only one possibility.
                set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                for(auto iter : route) {
                    output_port = iter.output_port;
                }
                _buffered_packet_outputs[input][output_port]++;
            }

            Buffer * const cur_buf = _buf[input];

#ifdef TRACK_FLOWS
            ++_stored_flits[f->cl][input];
            if(f->head) ++_active_packets[f->cl][input];
#endif

#ifdef FLIT_DEBUG
            if(f->watch) {
                //BSMOD: Change flit and packet id to long
                long front_id = cur_buf->FrontFlit(in_vc) ? cur_buf->FrontFlit(in_vc)->id : -1;
                long front_pid = cur_buf->FrontFlit(in_vc) ? cur_buf->FrontFlit(in_vc)->pid : -1;
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Router: " << FullName() 
                            << " BufferWrite, Adding flit from buffer input " << input << " in vc " << in_vc
                            << " Flit: " << f->id << " pid " << f->pid << " Front flit buffer "
                            << front_id << " (pid) " << front_pid
                            << std::endl;
            }
#endif
            // Store flit in input VC
            cur_buf->AddFlit(in_vc, f);

            int expanded_input = input * _vcs + in_vc;
            // Point out that there are flits ready for SA-I (if there were flits already doesn't matter)
            _switch_arbiter_input_flits[expanded_input] = true;

            if(cur_buf->GetState(in_vc) == VC::idle && f->head) {
                // Set VC state to SA-I and VC route
                cur_buf->SetState(in_vc, VC::sa_input);
            }

#ifdef FLIT_DEBUG
            if(f->watch)
            {
                *gWatchOut << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                           << " | Stage: BufferWrite | Flit: " << f->id << " head? " << f->head << " tail? "
                           << f->tail << " | Input: " << input << " | VC: " << in_vc << " VC state "
                           << cur_buf->GetState(in_vc) << std::endl;
            }
#endif
        }

        // Clean inputs for next cycle
        _buffer_write_flits.clear();

    }

    void HybridFBFCLRouter::_SwitchArbiterInput()
    {

#ifdef TRACK_STALLS
        int sa_i_conflicts[_classes] = {0};
#endif

        // Read inputs in SA-I
        for(int expanded_input = 0; expanded_input < _inputs*_vcs; expanded_input++) {

            int const input = expanded_input / _vcs;
            int const in_vc = expanded_input % _vcs;

            // If there are no flits in the input, next vc or input
            Buffer * const cur_buf = _buf[input];
            if(!_switch_arbiter_input_flits[expanded_input])
            {
                assert(cur_buf->Empty(in_vc));
                continue;
            }

            Flit * const f = cur_buf->FrontFlit(in_vc);
            assert(f);
            /*

            if(f->head && cur_buf->GetState(in_vc) == VC::active) {
                continue; // This vc is bypassing a packet
            }
            */

#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " SA-I adding request " << input << " in vc "
                            << in_vc << " Flit: " << f << " ID " << f->id << " pid " << f->pid << std::endl;
            }
#endif
            
            // Add request to the input arbiter
            
            if(_switch_arbiter_input_policy == "strict_round_robin"){
                _switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri);
            } else if (_switch_arbiter_input_policy == "credit_based"){
                set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();

                if(f->head){
                //FIXME: Only working for determinist routing (1 port per hop) We are taking the first available output port
                    for(auto iter : route)
                    {
                        int output_port = iter.output_port;

                        // Check that there is room for the flit
                        BufferState const * const dest_buf = _next_buf[output_port];

                        int dest_vc = dest_buf->GetAvailVCMinOccupancy(iter.vc_start, iter.vc_end);

                        if(dest_vc > -1) {
                            if(dest_buf->IsAvailableFor(dest_vc)){
                                _switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri + dest_buf->AvailableFor(dest_vc));
                                break;
                            }
                        }
                    }
                } else {
                    int output_port = cur_buf->GetOutputPort(in_vc);
                    BufferState const * const dest_buf = _next_buf[output_port];
                    int dest_vc = cur_buf->GetOutputVC(in_vc);
                    _switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri + dest_buf->AvailableFor(dest_vc));
                }
            } else if(_switch_arbiter_input_policy == "round_robin_on_miss") {
                if(!f->head && _switch_arbiter_input_winner[input] == in_vc) {
                    _switch_arbiter_input[input]->AddRequest(in_vc, f->id, INT_MAX);
                    _switch_arbiter_input_winner[input] = -1;
                } else {
                    _switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri);
                }
            } else {
                if(f->head) {
                    _switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri);
                } else {
                    _switch_arbiter_input[input]->AddRequest(in_vc, f->id, INT_MAX); // FIXME: This can be the cause of deadlocks in FBFCL
                }
            }

            /*
            if(f->head) {
             _switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri);
            } else {
                _switch_arbiter_input[input]->AddRequest(in_vc, f->id, INT_MAX);
            }
            */

#ifdef TRACK_STALLS
            ++sa_i_conflicts[f->cl];
#endif
            //_switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri);
        }

        // Arbitrate each SA-I arbiter requests
        for(int input = 0; input < _inputs; input++)
        {
            // Arbitrate
            int in_vc, pri;
            //BSMOD: Change flit and packet id to long
            long pid;
            in_vc = _switch_arbiter_input[input]->Arbitrate(&pid, &pri);

            // Update and Clear switch for next cycles:
            _switch_arbiter_input[input]->UpdateState();
            _switch_arbiter_input[input]->Clear();

            // Continue if there no winners
            if(in_vc < 0)
            {
                continue;
            }
            
            // Move winner to input register/buffer of SA-O stage
            Buffer * const cur_buf = _buf[input];
            Flit * const f = cur_buf->FrontFlit(in_vc);
            assert(f);

#ifdef TRACK_STALLS
            --sa_i_conflicts[f->cl];
#endif

#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " SA-I, Removing flit from buffer input " << input
                            << " in vc " << in_vc << " Flit: " << f << " ID " << f->id << " pid "
                            << f->pid << std::endl;
            }
#endif
            

            _switch_arbiter_output_flits[input] = f;
            
            if(cur_buf->GetState(in_vc) == VC::sa_input || cur_buf->GetState(in_vc) == VC::idle)
            {
                cur_buf->SetState(in_vc, VC::sa_output);
            }
        }

#ifdef TRACK_STALLS
        for(int cl=0; cl < _classes; cl++)
        {
            _switch_arbiter_input_stalls[cl] += sa_i_conflicts[cl];
        }
#endif
    }

    //------------------------------------------------------------------------------
    // write outputs
    //------------------------------------------------------------------------------

    void HybridFBFCLRouter::_SendFlits( )
    {
        for(int output = 0; output < _outputs; ++output) {
            if(!_output_buffer[output].empty()) {
                // Read flit from output buffer
                Flit * const f = _output_buffer[output].front();
                _output_buffer[output].pop();

                // Add flit to the output channel
                _output_channels[output]->Send(f);

#ifdef TRACK_FLOWS
                ++_sent_flits[f->cl][output];
#endif

#ifdef FLIT_DEBUG
                if(f->watch) {
                    BufferState * const dest_buf = _next_buf[output];

                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                << " | Stage: SendFlits | Flit: " << f->id << " pid " << f->pid << " head? "
                                << f->head << " tail? " << f->tail << " | Output: " << output << " Occupancy: "
                                << dest_buf->Occupancy() << " VC: " << f->vc << " VC Occupancy: "
                                << dest_buf->OccupancyFor(f->vc) << std::endl;
                }
#endif
            }
        }
    }

    void HybridFBFCLRouter::_SendCredits( )
    {
        for(int input = 0; input < _inputs; ++input) {
            if(_credit_buffer[input] != NULL) {
                // Read credit
                Credit * const c = _credit_buffer[input];
                _credit_buffer[input] = NULL;
                // Send credit
                _input_credits[input]->Send(c);
#ifdef CREDIT_DEBUG
                Buffer * const cur_buf = _buf[input];
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                            << " | Stage: SendCredits | Credit: " << c->id << " Credit head? " << c->head
                            << " tail? " << c->tail << " | Input: " << input << " Input buffer occupancy: "
                            << cur_buf->GetOccupancy() << " VC: " << *(c->vc.begin()) << " VC occupancy "
                            << cur_buf->GetOccupancy(*(c->vc.begin())) << std::endl;
#endif
            }

        }
    }

    void HybridFBFCLRouter::_SendLookahead()
    {
        // IMPORTANT NOTE: Note that the loop doesn't iterate through the last output port which corresponds with the terminal node, because it doesn't handle lookaheads
        for(int output = 0; output < _outputs; ++output) {

            bool is_router = _output_channels[output]->GetSink() != NULL ? true : false;

            if(!is_router && _lookahead_buffer[output] != NULL){
                _lookahead_buffer[output]->Free();
                _lookahead_buffer[output] = NULL;
                continue;
            }

            if(_lookahead_buffer[output] != NULL) {
                // Read credit
                Lookahead * const la = _lookahead_buffer[output];
                _lookahead_buffer[output] = NULL;
                // Send credit
                _output_lookahead[output]->Send(la);

#ifdef LOOKAHEAD_DEBUG
                if(la->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                << " | Stage: SendLookaheads | Lookahead: " << la->id << " pid " << la->pid
                                << " head? " << la->head << " tail? " << la->tail << " | Output: " << output
                                << std::endl;
                }
#endif
            }
        }
    }

    //-----------------------------------------------------------
    // misc.
    // ----------------------------------------------------------
    void HybridFBFCLRouter::Display( ostream & os ) const
    {
          for ( int input = 0; input < _inputs; ++input ) {
                  _buf[input]->Display( os );
                    }
    }
} // namespace Booksim
