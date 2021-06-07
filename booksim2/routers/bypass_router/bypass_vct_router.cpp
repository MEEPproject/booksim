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

#include "bypass_vct_router.hpp"

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


    BypassVCTRouter::BypassVCTRouter( Configuration const & config, Module *parent, 
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
        
        // Give priority to LA over flits: 1 or Flits over LA: 0
        _lookaheads_kill_flits = config.GetInt("lookaheads_kill_flits");
    }

    BypassVCTRouter::~BypassVCTRouter( )
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
    // TODO: Same function than HybridRouter's
    void BypassVCTRouter::ReadInputs()
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

    // TODO: Same function than HybridRouter's
    void BypassVCTRouter::_InternalStep()
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

    // TODO: Same function than HybridRouter's
    void BypassVCTRouter::WriteOutputs()
    {
        _SendFlits();
        _SendCredits();
        _SendLookahead();
    }

    //------------------------------------------------------------------------------
    // read inputs
    //------------------------------------------------------------------------------

    bool BypassVCTRouter::_ReceiveFlits()
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
                                << " | Stage: ReceiveFlits.0 | Flit: " << f->id << " pid " << f->pid << " head? "
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

                    // Copy lookahead route to control data of flit.
                    OutputSet const * const route_set = cur_buf->GetRouteSet(in_vc);
#ifdef FLIT_DEBUG
                if(f->watch) {
                    set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                    for(auto route_iter : route) {

                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                    << " | Stage: ReceiveFlits.1 | Flit: " << f->id << " pid " << f->pid << " head? "
                                    << f->head << " tail? " << f->tail << " | Input: " << input << " VC: " << in_vc
                                    << " | Bypass? " << _bypass_path[input*_vcs+f->vc] << " for pid: " << _pid_bypass[input*_vcs+f->vc]
                                    << " | Next hop ouput port: " << route_iter.output_port
                                    << std::endl;
                    }
                }
#endif

                    if(f->head) {
                        // Flit at destination (is this always true?)
                        bool is_router = _output_channels[output_requested]->GetSink() != NULL ? true : false;
                        if(route_set->GetSet().size() == 0 || !is_router) {// The last part in which I compare the output requested is part of the memory leak fix for lookaheads. TODO: Move all of these to LookAheadConflictCheck avoiding the creation of the lookahead when the flit arrives to the last router (also avoiding routecomputation)
                            f->la_route_set.Clear();
                        }
                        else {
                            assert(route_set != NULL);
                            f->la_route_set = * route_set;
                        }
                    }

                    // Torus dateline (copy from LA routing info)
                    f->ph = _dateline_partition[input*_vcs+in_vc];

                    // Move flit to ST stage 
                    _crossbar_flits.push_back(make_pair(f, output_requested));
                    
                    _bypass_path[input*_vcs+in_vc] = false;

                    // If this is the last flit of the packet => free VC:
                    if(f->tail) {
                        // FIXME: The following two variables should be ignored in this implementation
                        // Unblock bypass
                        // Unblock output port
                        _output_strict_priority_vc[output_requested] = -1;
                        _pid_bypass[input*_vcs+in_vc] = -1;
                        // Clear input VC info
                        cur_buf->SetOutput(in_vc,-1,-1);
                        cur_buf->SetState(in_vc,VC::idle);
                    }
        

#ifdef FLIT_DEBUG
                    if(f->watch) {
                        string name_dest =  _output_channels[output_requested]->GetSink() != NULL ? _output_channels[output_requested]->GetSink()->FullName() : _output_channels[output_requested]->FullName();
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Source Router: " << FullName() 
                                    << " | Destination Router: " << name_dest
                                    << " | Stage: Bypassing flit | Flit: " << f->id << " pid " << f->pid
                                    << " head? " << f->head << " tail? " << f->tail << " | Input: " << input
                                    << " | Bypass? " << _bypass_path[input*_vcs+f->vc] << " bypass output port "
                                    << output_requested << " dest vc " << f->vc << " la route output port: "
                                    << f->la_route_set.GetSet().begin()->output_port << " vc start "
                                    << f->la_route_set.GetSet().begin()->vc_start << " vc end "
                                    << f->la_route_set.GetSet().begin()->vc_end
                                    << ". WARNING this message don't show all possible routes" << std::endl;
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

    bool BypassVCTRouter::_ReceiveCredits()
    {
        bool activity = false;

        // Check the output channels for credits
        for(int output = 0; output < _outputs; ++output) {  
            Credit * const c = _output_credits[output]->Receive();
            if(c)
            {
                BufferState * const dest_buf = _next_buf[output];
#ifdef CREDIT_DEBUG
                if(c->watch){
                    string name_dest =  _output_channels[output]->GetSink() != NULL ? _output_channels[output]->GetSink()->FullName() : _output_channels[output]->FullName();
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Source Router: " << FullName() 
                                << " | Destination Router: " << name_dest
                                << " | Stage: ReceiveCredits| Credit: " << c->id << " Credit head? "
                                << c->head << " tail? " << c->tail << " | Output: " << output << " | Occupancy "
                                << dest_buf->Occupancy() << " VC: " << *(c->vc.begin()) << " VC Occupancy "
                                << dest_buf->OccupancyFor(*(c->vc.begin())) << std::endl;
                    dest_buf->Display(*gWatchOut);
                }
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

    bool BypassVCTRouter::_ReceiveLookahead()
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
                    *gWatchOut  << "(line " <<  __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
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

    void BypassVCTRouter::_SwitchTraversal( )
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

    void BypassVCTRouter::_SwitchArbiterOutput()
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
            
            // Change VC state to SA-O
            if(cur_buf->GetState(in_vc) == VC::idle && f->head)
            {
                cur_buf->SetState(in_vc, VC::sa_output);
            }
            
            assert(cur_buf->GetState(in_vc) == VC::sa_output || cur_buf->GetState(in_vc) == VC::active);
           
            // VA: Check if there is a destination VC free and with room for the flit
            if(f->head) {

#ifdef FLIT_DEBUG
                if(f->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name: " << FullName() << " SA-O | Flit: "
                                << f->id << " input: " << input << " in vc: " << in_vc << " cur_buf->GetState(in_vc): "
                                << cur_buf->GetState(in_vc) << " VC::sa_output: " << VC::sa_output << std::endl;
                }
#endif

                
                // FIXME: Does this assert mean that the input VC obtained a dest VC for another packet that is being bypassed?
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

#ifdef FLIT_DEBUG
                    if(f->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name: " << FullName() << " SA-O | Flit: "
                                    << f->id << " input: " << input << " in vc: " << in_vc
                                    << " dest_vc (-1 => can't advance): " << dest_vc
                                    << " output: " << output_port
                                    << std::endl;                                
                        dest_buf->Display(*gWatchOut);
                    }
#endif

                    if(dest_vc > -1) {
                    //for(int dest_vc = iter.vc_start; dest_vc <= iter.vc_end; dest_vc++) {

#ifdef FLIT_DEBUG
                        if(f->watch) {
                            *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name: " << FullName() << " SA-O | Flit: "
                                        << f->id << " input: " << input << " in vc: " << in_vc
                                        << " cur_buf->GetState(in_vc): " << cur_buf->GetState(in_vc)
                                        << " VC::sa_output: " << VC::sa_output << " dest_vc: " << dest_vc
                                        << " output: " << output_port << " Available room: "
                                        << dest_buf->AvailableFor(dest_vc) << std::endl;
                                        
                            dest_buf->Display(*gWatchOut);
                        }
#endif
                        // FIXME: With the new method GetAvailVCMinOccupancy(start_vc, end_vc) this is not required
                        if((f->head && dest_buf->IsAvailableFor(dest_vc) && dest_buf->AvailableFor(dest_vc) >= f->packet_size) ||
                           (!f->head && dest_buf->IsAvailableFor(dest_vc) && dest_buf->AvailableFor(dest_vc) > 0)
                        ) {

#ifdef TRACK_STALLS
                            cs = ei_crossbar_conflicts;
                            conflicts_record[input] = make_pair(cs,f->cl);
#endif
                            // Add request to SA-O
                            if(f->head) {
                                _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, f->pri);
                            } else {
                                _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, INT_MAX);
                            }
                    
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
            // Body flits check only if there is room for the flit (do not perform VA)
            else {

                // Read output port and destination VC from info stored in input VC
                assert(cur_buf->GetState(in_vc) == VC::active);
                int output_port = cur_buf->GetOutputPort(in_vc);
                int dest_vc = cur_buf->GetOutputVC(in_vc);
                int priority = cur_buf->GetPriority(in_vc);

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
                
                // FIXME: I believe that in this architecture it is not necessary to check if the destination buffer
                // is being used by the current packet, as body flits will be bypassed if head one is.
    //            if(dest_buf->AvailableFor(dest_vc) > 0 && dest_buf->UsedBy(dest_vc) == f->pid)
                if(dest_buf->AvailableFor(dest_vc) > 0)
                {

#ifdef FLIT_DEBUG
                    if(f->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName()
                                    << " Added Request to switch arbiter output. In vc " << in_vc << " output"
                                    << ". Read (l: 430) output port " << output_port << " Flit " << f->id
                                    << " Flit head? " << f->head << " Flit pid " << f->pid << std::endl;
                    }
#endif //FLIT_DEBUG

                    // Add Request to the output port SA-O arbiter
                    _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, priority);
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
                        << in_vc << " output" << output << " Flit "
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

    void BypassVCTRouter::_LookAheadConflictCheck()
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
                watch_arbiter = true;
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

                    /*
                    if(_blocked_output[route_iter.output_port]) { // If output is being used to bypass another packet ignore this lookahead to avoid conflict between body LA.
                        continue;
                    }
                    */
                    
                    // Check if packet could potentially break message order
                    if(_guarantee_order && 
                       _buffered_packet_outputs[input][route_iter.output_port] > 0) {
                        continue;
                    }

                    if(_output_strict_priority_vc[route_iter.output_port] > -1) {
                        continue;
                    }

                    BufferState const * const dest_buf = _next_buf[route_iter.output_port];

                    int dest_vc = dest_buf->GetAvailVCMinOccupancy(route_iter.vc_start, route_iter.vc_end);

#ifdef LOOKAHEAD_DEBUG
                    if(la->watch) {
                        string name_dest =  _output_channels[route_iter.output_port]->GetSink() != NULL ? _output_channels[route_iter.output_port]->GetSink()->FullName() : _output_channels[route_iter.output_port]->FullName();
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Source Router: " << FullName()
                            << " | Destination Router: " << name_dest
                            << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                            << la->pid << " head? " << la->head << " tail? " << la->tail
                            << " dest_vc (-1, no available vc): " << dest_vc
                            << std::endl;
                        dest_buf->Display(*gWatchOut);
                    }
#endif

                    if(dest_vc > -1) {

                        //for(int dest_vc = route_iter.vc_start; dest_vc <= route_iter.vc_end; dest_vc++)
                        if(dest_buf->IsAvailableFor(dest_vc) && dest_buf->AvailableFor(dest_vc) >= la->packet_size ) {

                            // Save output port and destination for arbitration, if required
                            BypassOutput bypass_state;
                            bypass_state.output_port = route_iter.output_port;
                            bypass_state.dest_vc = dest_vc;

                            // Add lookahead request to Output arbiter
                            _switch_arbiter_output[bypass_state.output_port]->AddRequest(input, la->id , la->pri);

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
            // FIXME: This has to be removed. In this router this is not required
            // With the new version of BufferState, credits for VCT can be decrease for the packet size directly.
            else { // Check if this body lookahead has to decrease credit count

                Buffer * const cur_buf = _buf[input];
                int dest_vc = cur_buf->GetOutputVC(la->vc);
                int output = cur_buf->GetOutputPort(la->vc);

#ifdef LOOKAHEAD_DEBUG
                if(la->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                        << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                        << la->pid << " head? " << la->head << " tail? " << la->tail
                        << " output " << output << " priority expanded input " << _output_strict_priority_vc[output]
                        << std::endl;
                }
#endif
                
                if(output==-1){
                    continue;
                }
                if(_output_strict_priority_vc[output] != input*_vcs+la->vc) {
                    continue;
                }

                // Save output port and destination for arbitration, if required
                BypassOutput bypass_state;
                bypass_state.output_port = output;
                bypass_state.dest_vc = dest_vc;

                // Add lookahead request to Output arbiter
                _switch_arbiter_output[output]->AddRequest(input, la->id , INT_MAX);

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

                // Kill SA-O winner
                if(_lookahead_conflict_check_flits.find(output) != _lookahead_conflict_check_flits.end()){
                    int const f_expanded_input = _lookahead_conflict_check_flits[output];
                    int const f_input = f_expanded_input / _vcs;
                    int const f_in_vc = f_expanded_input % _vcs;
                    Buffer * const f_cur_buf = _buf[f_input];
                    if((f_cur_buf->FrontFlit(f_in_vc)->head && _lookaheads_kill_flits) || !la->head){
                        _lookahead_conflict_check_flits.erase(output);
                    }else{
                        // Ignore this winner because SA-O's winner has priority.
                        // Clear requests to switch arbiter
                        _switch_arbiter_output[output]->Clear();
                        continue;
                    }
                }


#ifdef FLIT_DEBUG
                Flit * const f = _switch_arbiter_output_flits[input];
                if(f) {
                    if(f->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " flit : "
                            << f->id << " won SA-O - Killed in LA-CC by LA: " << la->id << ". input" << input << " in vc " 
                            << la->vc << " output" << std::endl;
                    }
                }
#endif
                BypassOutput bypass_state = la_id_to_ptr[la_id].second;

                int in_vc = la->vc;

                BufferState * const dest_buf = _next_buf[output];

                Lookahead * la_n = new Lookahead(la);
                la_n->vc = bypass_state.dest_vc;

                Buffer * const cur_buf = _buf[input];

                if(la->head){

                    // Compute lookahead routing
                    _LookAheadRouteCompute(la_n, bypass_state.output_port);
                    _dateline_partition[input*_vcs+in_vc] = la_n->ph;

                    // Change input VC state to active
                    cur_buf->SetState(in_vc, VC::active);
                    // Save route and output in the input VC
                    //REMOVEME
                    cur_buf->SetRouteSet(in_vc, &la_n->la_route_set);
                    cur_buf->SetOutput(in_vc,bypass_state.output_port, bypass_state.dest_vc);

                    // Take destination buffer
                    dest_buf->TakeBuffer(la_n->vc, la_n->pid);

                    _output_strict_priority_vc[bypass_state.output_port] = input*_vcs+in_vc;

#ifdef LOOKAHEAD_DEBUG
                    if(la->watch || watch_arbiter) {
                        set<OutputSet::sSetElement> const route = cur_buf->GetRouteSet(in_vc)->GetSet();
                        for(auto route_iter : route) {
                        *gWatchOut << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                            << " | Stage: LookAhead Conflict Check Winner (arbitration) | Lookahead: " << la->id << " pid "
                            << " Setting stric priority VC for output: " << output << " to expanded input "
                            << input*_vcs+in_vc
                            << " | Saved LA route in buffer: " << input << "." << in_vc
                            << " | Next hop output port: " << route_iter.output_port
                            << " | Dest buf available for: " << dest_buf->AvailableFor(bypass_state.dest_vc)
                            << std::endl;
                        }
                    }
#endif
                    _pid_bypass[input*_vcs+in_vc] = la->pid;
                   
                }
                _bypass_path[input*_vcs+in_vc] = true;
                dest_buf->SendingFlit(la_n, true);

                // Decrease credit count
#ifdef LOOKAHEAD_DEBUG
                    if(la->watch || watch_arbiter) {
                        //set<OutputSet::sSetElement> const route = la_n->la_route_set.GetSet();
                        *gWatchOut << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName() << " Sending Flit (LookAhead)"
                                   << std::endl;
                    }
#endif


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
                    if(la->watch){
                        string name_dest =  _output_channels[input]->GetSink() != NULL ? _output_channels[input]->GetSink()->FullName() : _output_channels[input]->FullName();
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Source Router: " << FullName()
                            << " | Destination Router: " << name_dest
                            << " | Credit: " << c->id << " creation ID ReceiveFlits-Bypass (l:251)"
                            << " input " << input << " in vc " << in_vc << std::endl;
                        c->watch = true;
                    }
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

#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " flit : "
                    << f->id << " won SA-O - Checking if must be killed in LA-CC. input" << input << " in vc " 
                    << in_vc << " output" << std::endl;
            }
#endif

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

            // Take destination VC and set ouput port, route and destination vc
            BufferState * const dest_buf = _next_buf[output_port];

            if(f->head) {
                dest_buf->TakeBuffer(f_n->vc,f_n->pid);
                cur_buf->SetState(in_vc, VC::active);
                cur_buf->SetRouteSet(in_vc, &f->la_route_set);
                cur_buf->SetOutput(in_vc, output_port, f_n->vc);

#ifdef FLIT_DEBUG
                if(f->watch) {
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << "  | Source Router " << FullName() << " Flit : "
                        << f->id << " won SA-O and LA-CC input " << input << " in vc " 
                        << in_vc << " output"
                        << " | Takes destinaton buffer | Sets Output port to: " << output_port
                        << " | Destintion VC: " << f_n->vc
                        << std::endl;
                }
#endif
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
#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " Name " << FullName() << " flit : "
                    << f-> id << " Sending Flit (Flit)"
                    << std::endl;
            }
#endif
            dest_buf->SendingFlit(f_n,true);


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
#ifdef CREDIT_DEBUG
                if(f->watch)
                    c->watch = true;
#endif
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
    void BypassVCTRouter::_LookAheadRouteCompute(Flit * f, int output_port) {

#ifdef FLIT_DEBUG
                if(f->watch) {
                    string name_dest =  _output_channels[output_port]->GetSink() != NULL ? _output_channels[output_port]->GetSink()->FullName() : _output_channels[output_port]->FullName();
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Source Router: " << FullName() 
                                << " | Destination Router: " << name_dest
                                << " | LookAheadRouteCompute of Flit/Lookahead (l: 835): " << f->id << std::endl;
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

#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut << "HOLA: LINE" << __LINE__ << std::endl;
                set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                for(auto route_iter : route) {
                    string name_dest =  router->GetOutputChannel(route_iter.output_port)->GetSink() != NULL ? router->GetOutputChannel(route_iter.output_port)->GetSink()->FullName() : router->GetOutputChannel(route_iter.output_port)->FullName();
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Source Router: " << FullName() 
                                << " | Next Hop Destination Router: " << name_dest
                                << " | Next output port: " << route_iter.output_port
                                << " | LookAheadRouteCompute of Flit/Lookahead: " << f->id << std::endl;
                }
            }
#endif
        }
    }

    // Buffer Write
    void BypassVCTRouter::_BufferWrite()
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
                            << " Flit: " << f << " ID " << f->id << " pid " << f->pid << " Front flit buffer "
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
                *gWatchOut << "HOLA: LINE" << __LINE__ << std::endl;
                set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();

                for(auto route_iter : route) {

                    *gWatchOut << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                               << " | Stage: BufferWrite | Flit: " << f->id << " head? " << f->head << " tail? "
                               << f->tail << " | Input: " << input << " | VC: " << in_vc << " VC state "
                               << cur_buf->GetState(in_vc)
                               << " | Next output port: " << route_iter.output_port
                               << std::endl;
                }
            }
#endif
        }

        // Clean inputs for next cycle
        _buffer_write_flits.clear();

    }

    void BypassVCTRouter::_SwitchArbiterInput()
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

#ifdef FLIT_DEBUG
            if(f->watch) {
                *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " SA-I adding request " << input << " in vc "
                            << in_vc << " Flit: " << f << " ID " << f->id << " pid " << f->pid << std::endl;
            }
#endif

#ifdef TRACK_STALLS
            ++sa_i_conflicts[f->cl];
#endif
            
            // We give full priority to body flits: once a packet is moving all the packet moves to the next hop in consecutive cycles.
            int priority = 0;
            if(f->head){
                priority = f->pri;
            }else{
                priority = INT_MAX;
            }
            _switch_arbiter_input[input]->AddRequest(in_vc, f->id, priority);
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

            // Continue if there aren't winners
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

    void BypassVCTRouter::_SendFlits( )
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
                    string name_dest =  _output_channels[output]->GetSink() != NULL ? _output_channels[output]->GetSink()->FullName() : _output_channels[output]->FullName();
                    BufferState * const dest_buf = _next_buf[output];
                
                *gWatchOut << "HOLA: LINE" << __LINE__ << std::endl;
                    set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                    for(auto route_iter : route) {

                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Source Router: " << FullName()
                                    << " Destination Router: " << name_dest
                                    << " | Stage: SendFlits | Flit: " << f->id << " pid " << f->pid << " head? "
                                    << f->head << " tail? " << f->tail << " | Output: " << output << " Occupancy: "
                                    << dest_buf->Occupancy() << " VC: " << f->vc << " VC Occupancy: "
                                    << dest_buf->OccupancyFor(f->vc)
                                    << " | Next Hop Output Port: " << route_iter.output_port
                                    << std::endl;
                    }
                }
#endif
            }
        }
    }

    void BypassVCTRouter::_SendCredits( )
    {
        for(int input = 0; input < _inputs; ++input) {
            if(_credit_buffer[input] != NULL) {
                // Read credit
                Credit * const c = _credit_buffer[input];
                _credit_buffer[input] = NULL;
                // Send credit
                _input_credits[input]->Send(c);
#ifdef CREDIT_DEBUG
                if(c->watch){
                    Buffer * const cur_buf = _buf[input];
                    *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                                << " | Stage: SendCredits | Credit: " << c->id << " Credit head? " << c->head
                                << " tail? " << c->tail << " | Input: " << input << " Input buffer occupancy: "
                                << cur_buf->GetOccupancy() << " VC: " << *(c->vc.begin()) << " VC occupancy "
                                << cur_buf->GetOccupancy(*(c->vc.begin())) << std::endl;
                }
#endif
            }

        }
    }

    void BypassVCTRouter::_SendLookahead()
    {
        // IMPORTANT NOTE: Note that the loop doesn't iterate through the last output port which corresponds with the terminal node, because it doesn't handle lookaheads
        for(int output = 0; output < _outputs; ++output) {

            bool is_router = _output_channels[output]->GetSink() != NULL ? true : false;

            //if(output > _outputs-gC-1 && _lookahead_buffer[output] != NULL){
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
    void BypassVCTRouter::Display( ostream & os ) const
    {
          for ( int input = 0; input < _inputs; ++input ) {
                  _buf[input]->Display( os );
                    }
    }
} // namespace Booksim
