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

#include "bypass_arb_fbfcl_router.hpp"

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

    BypassArbFBFCLRouter::BypassArbFBFCLRouter( Configuration const & config, Module *parent, 
            string const & name, int id, int inputs, int outputs )
    : Router( config, parent, name, id, inputs, outputs )
    {
        // TODO: Check that this router is being constructed with route_delay=0 and noq=0, required for lookahead routing

        // Number of Virtual channel per channel.
        _vcs = config.GetInt("num_vcs");

        // Set all the bypass paths to false -> not taken
        _bypass_path.resize(_inputs, false);
        // Dateline partition: to copy from LA to Flit
        _dateline_partition.resize(_inputs*_vcs, -1);

        // Routing
        string const rf = config.GetStr("routing_function") + "_" + config.GetStr("topology");
        map<string, tRoutingFunction>::const_iterator rf_iter = gRoutingFunctionMap.find(rf);
        if(rf_iter == gRoutingFunctionMap.end()) {
            Error("Invalid routing function: " + rf);
        }
        _rf = rf_iter->second;

        // Allocate Virtual Channels VC's
        // TODO: Can be these shared buffers?
        _buf.resize(_inputs);
        for(int i = 0; i < _inputs; ++i)
        {
            ostringstream module_name;
            module_name << "buf_" << i;
            _buf[i] = new Buffer(config, _outputs, this, module_name.str( ) );
            module_name.str("");
        }

        // Allocate next VCs' buffer state
        _next_buf.resize(_outputs);
        for(int j = 0; j < _outputs; ++j)
        {
            ostringstream module_name;
            module_name << "next_vc_o" << j;
            _next_buf[j] = new BufferState( config, this, module_name.str( ) );
            module_name.str("");
        }

        // SA-I round robin arbiters
        // TODO: we could define the type of arbiter from the config file
        _switch_arbiter_input.resize(_inputs);
        for(int input=0; input < _inputs; input++)
        {
            ostringstream module_name;
            module_name << "SA-I_" << input;
            _switch_arbiter_input[input] = Arbiter::NewArbiter(this, module_name.str(), "round_robin", _vcs);
            //_switch_arbiter_input[input] = Arbiter::NewArbiter(this, module_name.str(), "matrix", _vcs);
        }

        _switch_arbiter_input_flits.resize(_inputs*_vcs, false);

        // SW-O allocator
        _switch_arbiter_output.resize(_outputs);
        for(int output=0; output < _outputs; output++)
        {
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

        // Deprecated
                    
        // If a trail flit is stored the LAs of following flits are ignored
        //_lookahead_buffered_flits.resize(_inputs,-1);

        // Option to deactivate bypass
        _disable_bypass = config.GetInt("disable_bypass");
        
        // Optimization for single-flit packets
        _single_flit_optimization = config.GetInt("single_flit_optimization");
        if(config.GetStr("router") == "hybrid_simplified_fbfcl") {
            _single_flit_optimization = true;
        }

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
    }

    BypassArbFBFCLRouter::~BypassArbFBFCLRouter( )
    {
        // TODO: delete buffers
        for(int i = 0; i < _inputs; ++i)
        {
            delete _buf[i];
            // To Remove delete _bypass_path[i];
            delete _switch_arbiter_input[i];
        }

        // TODO: delete next buffer state
        for(int j = 0; j < _outputs; ++j)
        {
            delete _next_buf[j];
            delete _switch_arbiter_output[j];
        }
    }

    // The following three methods are evaluated cycle by cycle
    // Read and prepare flits, credits and lookahead information for pipeline stages
    // TODO: The same function than Hybrid etc
    void BypassArbFBFCLRouter::ReadInputs()
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

    // TODO: The same function than Hybrid etc
    void BypassArbFBFCLRouter::_InternalStep()
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

    // TODO: The same function than Hybrid etc
    void BypassArbFBFCLRouter::WriteOutputs()
    {
        _SendFlits();
        _SendCredits();
        _SendLookahead();
    }

    //------------------------------------------------------------------------------
    // read inputs
    //------------------------------------------------------------------------------

    bool BypassArbFBFCLRouter::_ReceiveFlits()
    {
        bool activity = false;
        // Check the input channels
        for(int input = 0; input < _inputs; ++input)
        { 
            Flit * const f = _input_channels[input]->Receive();
            // Two possible options:
            //    a) Take the bypass path
            //    b) Write the flit into the input buffer
            if(f)
            {

#ifdef TRACK_FLOWS
                ++_received_flits[f->cl][input];
#endif

#ifdef FLIT_DEBUG
                if(f->watch)
                {
                    *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: ReceiveFlits | Flit: " << f->id << " pid " << f->pid << " head? " << f->head << " tail? " << f->tail << " | Input: " << input << " VC: " << f->vc << " | Bypass? " << _bypass_path[input] << std::endl;
                }
#endif
                // Is the bypass path ready?
                if(_bypass_path[input])
                {

                
                    // Add flit to the ST stage 
                    // We have to know two things the output port and the output vc
                    // and we have to set the next hop route
                    int const in_vc = f->vc;
                    Buffer * cur_buf = _buf[input];
                    int output_requested = cur_buf->GetOutputPort(in_vc);
                    f->vc = cur_buf->GetOutputVC(in_vc);
                    OutputSet const * const route_set = cur_buf->GetRouteSet(in_vc);
                    if(f->head)
                    {
                        // Flit at destination (is this always true?)
                        bool is_router = _output_channels[output_requested]->GetSink() != NULL ? true : false;
                        if(route_set->GetSet().size() == 0 || !is_router) // The last part in which I compare the output requested is part of the memory leak fix for lookaheads. TODO: Move all of these to LookAheadConflictCheck avoiding the creation of the lookahead when the flit arrives to the last router (also avoiding routecomputation)
                        {
                            f->la_route_set.Clear();
                        }
                        else
                        {
                            f->la_route_set = * route_set;
                        }

                        f->ph = _dateline_partition[input*_vcs+in_vc];
                    }
                    _crossbar_flits.push_back(make_pair(f, output_requested));

                    _bypass_path[input] = false;

                    // If last flit of the packet free VC:
                    if(f->tail)
                    {
                        // FIXME: Is this necessary?
                        if (cur_buf->GetExpectedPID(in_vc) == f->pid) {
                            cur_buf->ReleaseVC(in_vc); // _expected_pid is only used for debbuging purposes
                        }
                        cur_buf->SetOutput(in_vc,-1,-1);
                        cur_buf->SetState(in_vc,VC::idle);
                    }

#ifdef FLIT_DEBUG
                if(f->watch)
                {
                    *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: Bypassing flit | Flit: " << f->id << " pid " << f->pid << " head? " << f->head << " tail? " << f->tail << " | Input: " << input << " | Bypass? " << _bypass_path[input] << " bypass output port " << output_requested << " dest vc " << f->vc << " la route output port: " << f->la_route_set.GetSet().begin()->output_port << " vc start " << f->la_route_set.GetSet().begin()->vc_start << " vc end " << f->la_route_set.GetSet().begin()->vc_end << ". WARNING this message is obsolete and don't show all possible routes" << std::endl;
                }
#endif
                }
                else
                {
                    _buffer_write_flits[input] = f;
                    _lookahead_buffered_flits[f->pid] = 1;

                    // Set VC to SA if this is a head flit
                    if(f->head)
                    {
                        Buffer * cur_buf = _buf[input];
                        int const in_vc = f->vc;
                        //assert(cur_buf->GetState(in_vc) == VC::idle); // VC should be idle
                        if(cur_buf->GetState(in_vc) == VC::idle) {
                            cur_buf->SetState(in_vc, VC::sa_input);
                            cur_buf->SetRouteSet(in_vc, &f->la_route_set);
                        }
                    }
                }
                
                activity = true;
            }
        }
        _BufferWrite();
        return activity;
    }

    bool BypassArbFBFCLRouter::_ReceiveCredits()
    {
        bool activity = false;
        // Check the output channels for credits
        for(int output = 0; output < _outputs; ++output) {  
            Credit * const c = _output_credits[output]->Receive();
            if(c)
            {
                BufferState * const dest_buf = _next_buf[output];
#ifdef CREDIT_DEBUG
                *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: ReceiveCredits| Credit: " << c->id << " Credit head? " << c->head << " tail? " << c->tail << " | Output: " << output << " | Occupancy " << dest_buf->Occupancy() << " VC: " << *(c->vc.begin()) << " VC Occupancy " << dest_buf->OccupancyFor(*(c->vc.begin())) << std::endl;
#endif
                // (arrival time, (credit, output))
                //_proc_credits[output] = c;
                
                dest_buf->ProcessCredit(c);
                c->Free();
                
                // State of destination input buffer
                activity = true;
            }
        }
        return activity;
    }

    bool BypassArbFBFCLRouter::_ReceiveLookahead()
    {
        bool activity = false;
        // Read lookahead signals
        for(int input = 0; input < _inputs; ++input) {  
            // FIXME: is this a good way of avoid the problem of choose a topology/network that doesn't set the lookahead channels?
            if((int)_input_lookahead.size() == _inputs){
                Lookahead * const la = _input_lookahead[input]->Receive();
                if(la)
                {
                    if(_disable_bypass == 1)
                    {
                       la->Free();     
                       continue;
                    }

#ifdef LOOKAHEAD_DEBUG 
                    if(la->watch) 
                    {
                        *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: ReceiveLookahead | Lookahead: " << la->id << " head? " << la->head << " tail? " << la->tail << " | Input: " << input << " VC: " << la->vc << std::endl;
                    }
#endif
                    // FIXME: I set a one cycle delay for lookahead signals. Parametizer this.
                    _lookahead_conflict_check_lookaheads.push_back(make_pair(la, input));
                    activity = true;
                }
            }
        }
        return activity;
    }

    void BypassArbFBFCLRouter::_SwitchTraversal( )
    {
        // (flit, output port))
        for(auto const iter : _crossbar_flits)
        {

            Flit * const f = iter.first;
            f->hops++;
            int const output = iter.second;

#ifdef FLIT_DEBUG
            if(f->watch)
            {
                *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: SwitchTraversal | Flit: " << f->id << " pid " << f->pid << " head? " << f->head << " tail? " << f->tail << " | Output: " << output << std::endl;
            }
#endif

            // Store the flit in the output buffer (It should have slot only for one flit)
            _output_buffer[output].push(f);
        }
        _crossbar_flits.clear();
    }

    void BypassArbFBFCLRouter::_SwitchArbiterOutput()
    {

#ifdef TRACK_STALLS
        enum ConflictStates {ei_crossbar_conflicts, ei_buffer_busy, ei_buffer_reserved, ei_buffer_full, ei_output_blocked, ei_winner};
        pair<int,int> conflicts_record[_inputs];
        ConflictStates cs = ei_crossbar_conflicts;
#endif

        // (expanded input)
        for(auto const iter : _switch_arbiter_output_flits)
        {
            int const input = iter.first;
            Flit const * const f = iter.second;
            assert(f);
            int const in_vc = f->vc;

            // Read output requested by the flit
            Buffer * const cur_buf = _buf[input];
            
            //assert(cur_buf->GetState(in_vc) == VC::sa_output || cur_buf->GetState(in_vc) == VC::active);
           

            // FIMXE: To improve performance we should allow interleaving between flits of different flits.
            

            if(f->head) // We have to allocate a VC
            {
                // VC should be in SA-O stage
#ifdef FLIT_DEBUG
                if(f->watch)
                    *gWatchOut << "Cycle " << GetSimTime() << " Name " << FullName() << " SA-O | Flit: " << f->id << " input " << input << " in vc " << in_vc << " cur_buf->GetState(in_vc) " << cur_buf->GetState(in_vc) << " VC::sa_output " << VC::sa_output << std::endl;
    //            assert(cur_buf->GetState(in_vc) == VC::sa_output);
#endif

                // Output port
                set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                int stop = false;

                for(auto iter : route)
                {        
                    int output_port = iter.output_port;

                    // Check that there is room for the flit
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
                    
    //                for(int dest_vc = iter.vc_start; dest_vc <= iter.vc_end; dest_vc++) {
#ifdef FLIT_DEBUG
                        if(f->watch)
                        {
                            *gWatchOut << "Cycle " << GetSimTime() << " Name " << FullName() << " SA-O | Flit: " << f->id << " input " << input << " in vc " << in_vc << " cur_buf->GetState(in_vc) " << cur_buf->GetState(in_vc) << " VC::sa_output " << VC::sa_output << " dest_vc " << dest_vc << std::endl;
                            dest_buf->Display(*gWatchOut);
                        }
                        //assert(cur_buf->GetState(in_vc) == VC::sa_output);
#endif
                        // Check if packet will change of dimension
                        bool dimension_change = DimensionChange(input, output_port);
                        if(dest_buf->IsAvailableFor(dest_vc) && (dimension_change ? dest_buf->AvailableFor(dest_vc) > f->packet_size : dest_buf->AvailableFor(dest_vc) > 0 ))
                        {

#ifdef TRACK_STALLS
                            cs = ei_crossbar_conflicts;
                            conflicts_record[input] = make_pair(cs,f->cl);
#endif

                            // TODO: Maybe we can use the position of the element in the _switch_arbiter_output vector to identify the winner quickly
                            _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, f->pri);
                    
                            // Set output for the case of being the winner
                            cur_buf->SetOutput(in_vc, output_port, dest_vc);
#ifdef FLIT_DEBUG
                            if(f->watch)
                                *gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " head Flit: " << f->id << " , sets VC's " << in_vc << " output" << std::endl;
#endif
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

                    if(stop)
                    {
                        break;
                    }
                }
            }
            else // body flits use the same VC as the head
            {

                //assert(cur_buf->GetState(in_vc) == VC::active);
                int output_port = cur_buf->GetOutputPort(in_vc);
                int priority = cur_buf->GetPriority(in_vc);
                int dest_vc = cur_buf->GetOutputVC(in_vc);
#ifdef FLIT_DEBUG
                if(f->watch)
                    *gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " flit takes info from input " << input << " VC's " << in_vc << " output" << ". Read (l: 414) output port " << output_port << " Flit " << f->id << " Flit head? " << f->head << " Flit pid " << f->pid << std::endl;
#endif //FLIT_DEBUG
                BufferState const * const dest_buf = _next_buf[output_port];
#ifdef FLIT_DEBUG
                if(f->watch)
                {
                    dest_buf->Display(*gWatchOut);
                    //*gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " Flit " << f->id << " dest_buf->IsAvailableFor(" << dest_vc << ") " << dest_buf->IsAvailableFor(dest_vc) << " dest_buf->AvailableFor(" << dest_vc << ") " << dest_buf->AvailableFor(dest_vc)  << std::endl;
                    *gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " Flit " << f->id << " dest_buf->IsAvailableFor(" << dest_vc << ") " << dest_buf->IsAvailableFor(dest_vc) << " > 0 && dest_buf->UsedBy(" << dest_vc << ") " << dest_buf->UsedBy(dest_vc) << " == f->pid " << f->pid  << std::endl;
                }
#endif
                //if(dest_buf->IsAvailableFor(dest_vc) && dest_buf->AvailableFor(dest_vc) > 0)
                if(dest_buf->AvailableFor(dest_vc) > 0 && dest_buf->UsedBy(dest_vc) == f->pid)
                {

#ifdef TRACK_STALLS
                            cs = ei_crossbar_conflicts;
                            conflicts_record[input] = make_pair(cs,f->cl);
#endif

#ifdef FLIT_DEBUG
                    if(f->watch)
                        *gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " Added Request to switch arbiter output. In vc " << in_vc << " output" << ". Read (l: 430) output port " << output_port << " Flit " << f->id << " Flit head? " << f->head << " Flit pid " << f->pid << std::endl;
#endif //FLIT_DEBUG
                    // Add Request
                    _switch_arbiter_output[output_port]->AddRequest(input, input*_vcs+f->vc, priority+1); // IMPORTANT NOTE: We add 1 to the priority of bodyflits because if preferible to generate lookaheads contiguosly than interleved (if a previous flit is buffered, the next ones also)
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
        }

        // Arbitration
        for(int output = 0; output < _outputs; output++)
        {
            //BSMOD: Change flit and packet id to long
            long expanded_input;
            int priority;
            if(_switch_arbiter_output[output]->Arbitrate(&expanded_input,&priority) > -1)
            {
                _lookahead_conflict_check_flits[output] = expanded_input;

#ifdef TRACKSTALLS
                Flit * fstall = _switch_arbiter_output_flits[input];
                cs = ei_winner;
                conflicts_record[input] = make_pair(cs,fstall->cl);
#endif

#ifdef FLIT_DEBUG
                int const input = expanded_input / _vcs;
                int const in_vc = expanded_input % _vcs;
                Flit * f = _switch_arbiter_output_flits[input];
            

                    if(f->watch)
                        *gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " SA-O Winner. In vc " << in_vc << " output" << ". Read (l: 452) output port " << output << " Flit " << f->id << " Flit head? " << f->head << " Flit pid " << f->pid << " VC " << f->vc << std::endl;
#endif //FLIT_DEBUG
            }
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

    // TODO: I don't like this code. Maybe we can improve it adding all the lookahead
    // requests to the arbiter prioritizing body over head lookaheads.
    void BypassArbFBFCLRouter::_LookAheadConflictCheck()
    {

#ifdef TRACK_STALLS
        enum ConflictStates {ei_crossbar_conflicts, ei_buffer_busy, ei_buffer_reserved, ei_buffer_full, ei_output_blocked, ei_sao_winner_killed, ei_ignore};
        pair<int,int> conflicts_record[_inputs];
        ConflictStates cs = ei_crossbar_conflicts;
#endif

        // ((Lookahead *, input), bypass_state)
        vector<pair<pair<Lookahead *, int>,BypassOutput>> lookahead_candidates;
        // number of lookaheads per output
        vector<int> lookahead_requests_per_output;
        lookahead_requests_per_output.resize(_outputs,0);

        // Dissable all bypass paths
        //_bypass_path.resize(_inputs,false);
        // (Lookahead *, input) 
        for(auto const iter : _lookahead_conflict_check_lookaheads)
        {
            Lookahead * la = iter.first;
            int const input = iter.second;

#ifdef LOOKAHEAD_DEBUG
            if(la->watch)
            {
                *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid " << la->pid << " head? " << la->head << " tail? " << la->tail << std::endl;
            }
#endif

            // Compute lookahead routing
            //_LookAheadRouteCompute(la);
        
#ifdef TRACK_STALLS
            cs = ei_crossbar_conflicts;
            conflicts_record[input] = make_pair(cs,la->cl);
#endif

            if(la->head)
            {
                // Check if input VC is in active state: if so that means that there are flits stored that are moving to the next router.
                Buffer * const cur_buf = _buf[input];
                if(!cur_buf->Empty(la->vc) || cur_buf->GetState(la->vc) == VC::active){
                    //if(_single_flit_optimization && la->packet_size > 1 || (cur_buf->GetState(la->vc) == VC::active)){ // Original working version
                    if(_single_flit_optimization && (la->packet_size > 1 || (cur_buf->GetState(la->vc) == VC::active))){
                        continue;
                    }
                    else if(!_single_flit_optimization){
                        continue;
                    }
                }

                set<OutputSet::sSetElement> const route = la->la_route_set.GetSet();
                bool stop = false;
                for(auto route_iter : route)
                {
                    BufferState const * const dest_buf = _next_buf[route_iter.output_port];

                    int dest_vc = dest_buf->GetAvailVCMinOccupancy(route_iter.vc_start, route_iter.vc_end);

#ifdef LOOKAHEAD_DEBUG
                    if(la->watch) {
                        *gWatchOut  << "(line " << __LINE__ << ") | Cycle: " << GetSimTime() << " | Router: " << FullName()
                            << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid "
                            << la->pid << " head? " << la->head << " tail? " << la->tail
                            << " dest_vc (-1, no available vc): " << dest_vc
                            << std::endl;
                        dest_buf->Display(*gWatchOut);
                    }
#endif

                    if(dest_vc > -1) {

                    //for(int dest_vc = route_iter.vc_start; dest_vc <= route_iter.vc_end; dest_vc++) {               {
                        // Check if there is room for the entire packet if there isn't a dimension change
                        // or the packet size +1 if there is
                        bool dimension_change = DimensionChange(input, route_iter.output_port);
                        if(dest_buf->IsAvailableFor(dest_vc) && (dimension_change ? dest_buf->AvailableFor(dest_vc) > la->packet_size : dest_buf->AvailableFor(dest_vc) > 0 )) {
                //            la->vc = dest_vc;

                            BypassOutput bypass_state;
                            bypass_state.output_port = route_iter.output_port;
                            bypass_state.dest_vc = dest_vc;
                            //bypass_state.pid = la->pid;
#ifdef LOOKAHEAD_DEBUG
                            if(la->watch)
                            {
                                *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: LookAhead Conflict Check | Lookahead: " << la->id << " pid " << la->pid << " head? " << la->head << " tail? " << la->tail << " Sets bypass state to: output port " << bypass_state.output_port << " dest_vc " << bypass_state.dest_vc << std::endl;
                            }
#endif
                            lookahead_candidates.push_back(make_pair(iter,bypass_state));
                            lookahead_requests_per_output[route_iter.output_port]++;

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
                            conflicts_record[input] = make_pair(cs,la->cl);
                        }
#endif
                    }
                    
                    if(stop)
                    {
                        break;
                    }
                }
            }
            else // Body flits
            {
                Buffer * const cur_buf = _buf[input];
                // Check if there is a flit in the packet buffered.
                // Checking the las pid buffered should be enough because two packets cannot be
                // interleaved over a single VC.
                //if(_lookahead_buffered_flits.find(la->pid) == _lookahead_buffered_flits.end())
                if(cur_buf->Empty(la->vc))
                {
                    BypassOutput bypass_state;
                    int const in_vc = la->vc;
                    int const output_port = cur_buf->GetOutputPort(in_vc);
                    int const dest_vc = cur_buf->GetOutputVC(in_vc);

                    bypass_state.output_port = output_port;
                    bypass_state.dest_vc = dest_vc;
                    //bypass_state.la_route_set = *cur_buf->GetRouteSet(in_vc);

                    BufferState const * const dest_buf = _next_buf[output_port];
                    //if(dest_buf->IsAvailableFor(dest_vc) && dest_buf->AvailableFor(dest_vc) > 0)
                    //if(dest_buf->IsEmptyFor(dest_vc) > 0 && dest_buf->UsedBy(dest_vc) == la->pid)
                    if(dest_buf->AvailableFor(dest_vc) > 0 && dest_buf->UsedBy(dest_vc) == la->pid)
                    {
                        //la->vc = dest_vc;

                        lookahead_candidates.push_back(make_pair(iter,bypass_state));
                        lookahead_requests_per_output[bypass_state.output_port]++;
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
                else
                {
                    _lookahead_buffered_flits.erase(la->pid);
                }
            }
        }
        
                    
        // To know which lookahead wins the arbitration
        map<int, pair<Lookahead*, BypassOutput>> la_id_to_ptr;

        vector<bool> output_granted;
        output_granted.resize(_outputs,false);

        // ((Lookahead *, input), bypass_state)
        for(auto const iter : lookahead_candidates) 
        {
            Lookahead * la = iter.first.first;
            int input = iter.first.second;
            BypassOutput const bypass_state = iter.second;

            int in_vc = la->vc;

            if(_lookaheads_kill_flits){
#ifdef TRACK_STALLS
                ++_la_sa_winners_killed[la->cl];
#endif
                _lookahead_conflict_check_flits.erase(bypass_state.output_port);
            } else if(_lookahead_conflict_check_flits.find(bypass_state.output_port) != _lookahead_conflict_check_flits.end()){
                continue;
            }

            if(lookahead_requests_per_output[bypass_state.output_port] == 1)
            {

                Lookahead * la_n = new Lookahead(la);
                //_lookahead_route_compute_lookaheads.pushback(make_pair(la_n,bypass_state.output_port));
                la_n->vc = bypass_state.dest_vc;
                
                _bypass_path[input] = true;
            
                // Compute lookahead routing
                if(la->head){
                    _LookAheadRouteCompute(la_n, bypass_state.output_port);
                    _dateline_partition[input*_vcs+in_vc] = la_n->ph;
                }
                
                // Decrement credit count
                BufferState * const dest_buf = _next_buf[bypass_state.output_port];
                if(la_n->head)
                {
                    dest_buf->TakeBuffer(la_n->vc, la_n->pid);
                    // Set VC
                    Buffer * const cur_buf = _buf[input];
                    cur_buf->SetState(in_vc, VC::active);
                    cur_buf->SetRouteSet(in_vc, &la_n->la_route_set);
                    cur_buf->SetOutput(in_vc,bypass_state.output_port, bypass_state.dest_vc);
                }


                bool dimension_change = DimensionChange(input, bypass_state.output_port);
                dest_buf->SendingFlit(la_n, dimension_change);

#ifdef TRACK_STALLS
                    cs = ei_ignore;
                    conflicts_record[input] = make_pair(cs,la_n->cl);
#endif
                    
                // Send credit, as the corresponding flit is not going to be stored
                if(_credit_buffer[input])
                {
                    _credit_buffer[input]->vc.insert(in_vc);
                }
                else
                {
                    Credit * c = Credit::New();
                    c->vc.insert(in_vc);
                    c->id = la_n->id;
                    _credit_buffer[input] = c;
                }

#ifdef LOOKAHEAD_DEBUG
                if(la->watch)
                {
                    *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: LookAhead Conflict Check Winner (only one LA for this output) | Lookahead: " << la->id << " pid " << la->pid << " head? " << la->head << " tail? " << la->tail << " Preparing credit: input " << input << " VC: " << in_vc << std::endl;
                }
#endif

                // Send Lookahead
                _lookahead_buffer[bypass_state.output_port] = la_n;

            }
            else
            {
                if(!output_granted[bypass_state.output_port])
                {
                    // If it is a body lookahead has priority over head lookaheads
                    //if(bypass_state.pid == la->pid)
                    //if(!la->head && bypass_state.pid == la->pid)
                    if(!la->head)
                    {
                        _bypass_path[input] = true;
                        Lookahead * la_n = new Lookahead(la);
                        la_n->vc = bypass_state.dest_vc;
                    
                        // Decrement credit count
                        BufferState * const dest_buf = _next_buf[bypass_state.output_port];
                        //dest_buf->TakeBuffer(bypass_state.dest_vc, la_n->pid);
                        bool dimension_change = DimensionChange(input, bypass_state.output_port);
                        dest_buf->SendingFlit(la_n, dimension_change);

#ifdef TRACK_STALLS
                    cs = ei_ignore;
                    conflicts_record[input] = make_pair(cs,la_n->cl);
#endif

                        // Send credit, as the corresponding flit is not going to be stored
                        if(_credit_buffer[input])
                        {
                            _credit_buffer[input]->vc.insert(in_vc);
                        }
                        else
                        {
                            Credit * c = Credit::New();
                            c->vc.insert(in_vc);
                            c->id = la_n->id;
                            _credit_buffer[input] = c;
                        }

#ifdef LOOKAHEAD_DEBUG
                if(la->watch)
                {
                    *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: LookAhead Conflict Check Winner (body lookahead) | Lookahead: " << la_n->id << " pid " << la->pid << " head? " << la->head << " tail? " << la->tail << " Preparing credit: input " << input << " VC: " << in_vc << " output port " << bypass_state.output_port << " la_n " << la_n << std::endl;
                }
#endif
                        
                        // Send Lookahead
                        _lookahead_buffer[bypass_state.output_port] = la_n;

                        output_granted[bypass_state.output_port] = true;
                        _switch_arbiter_output[bypass_state.output_port]->Clear();
                    }
                    else
                    {
                       // XXX: We use the same arbiters as in the SA-O stage instead of use vector priorities for each input and an epoch of 20 cycles
                        _switch_arbiter_output[bypass_state.output_port]->AddRequest(input, la->id , la->pri);
                        la_id_to_ptr[la->id] = make_pair(la,bypass_state);
                    }
                }
            }
        }
        
        // Arbitration
        for(int output = 0; output < _outputs; output++)
        {
            Lookahead * la;
            //BSMOD: Change flit and packet id to long
            long la_id;
            int input;
            int priority;
            input = _switch_arbiter_output[output]->Arbitrate(&la_id,&priority);
            if(input > -1)
            {
                _bypass_path[input] = true;

                la = la_id_to_ptr[la_id].first;
                BypassOutput bypass_state = la_id_to_ptr[la_id].second;

                int in_vc = la->vc;

                Lookahead * la_n = new Lookahead(la);
                la_n->vc = bypass_state.dest_vc;
                //_lookahead_route_compute_lookaheads.pushback(make_pair(la_n,output));
                // TODO: store the lookahead in the list of lookaheads to send. See _SendLookahead
            

                // Decrement credit count
                BufferState * const dest_buf = _next_buf[output];
                if(la->head)
                {
                    // Compute lookahead routing
                    _LookAheadRouteCompute(la_n, bypass_state.output_port);
                    _dateline_partition[input*_vcs+in_vc] = la_n->ph;
                    dest_buf->TakeBuffer(la_n->vc, la_n->pid);
                }
                
                bool dimension_change = DimensionChange(input, bypass_state.output_port);
                dest_buf->SendingFlit(la_n, dimension_change);

#ifdef TRACK_STALLS
                    cs = ei_ignore;
                    conflicts_record[input] = make_pair(cs,la_n->cl);
#endif
                
                // Set router's bypass state
                if(la->head)
                {
                    // Set VC
                    Buffer * const cur_buf = _buf[input];
                    cur_buf->SetState(in_vc, VC::active);
                    cur_buf->SetRouteSet(in_vc, &la_n->la_route_set);
                    cur_buf->SetOutput(in_vc,bypass_state.output_port, bypass_state.dest_vc);
                }
                
                // Send credit, as the corresponding flit is not going to be stored
                if(_credit_buffer[input])
                {
                    _credit_buffer[input]->vc.insert(in_vc);
                }
                else
                {
                    Credit * c = Credit::New();
                    c->vc.insert(in_vc);
                    c->id = la_n->id;
                    _credit_buffer[input] = c;
                }

#ifdef LOOKAHEAD_DEBUG
            if(la->watch)
            {
                *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: LookAhead Conflict Check Winner (arbitration) | Lookahead: " << la->id << " pid " << la->pid << " head? " << la->head << " tail? " << la->tail << " Preparing credit: input " << input << " VC: " << in_vc << " output port " << output << std::endl;
            }
#endif

                // Send Lookahead
                _lookahead_buffer[output] = la_n;
                _switch_arbiter_output[output]->Clear();
            }
        }

        // Grant SA-O that weren't killed
        // (output port, expanded_input)
        for(auto const iter : _lookahead_conflict_check_flits) 
        {
            int const output_port = iter.first;
            int const input = iter.second / _vcs;
            int const in_vc = iter.second % _vcs;
            Buffer * const cur_buf = _buf[input];
            //Flit * const f = cur_buf->FrontFlit(in_vc);
            
            if(_bypass_path[input]) {
                continue;
            }
            
            // We have to read this flit from SA-O input buffer
            Flit * const f = _switch_arbiter_output_flits[input];

            // Compute Lookahead route
            Flit * f_n = f; 
            if(f->head)
                _LookAheadRouteCompute(f_n, output_port);

            // FIXME: if this flit is a head we don't have the destination vc here yet. This should be transfer in _lookahead_conflict_check_flits
            f_n->vc = cur_buf->GetOutputVC(in_vc);
                
            Lookahead * la_n = new Lookahead(f_n);
            _lookahead_buffer[output_port] = la_n;

            // Move flit to ST
            _crossbar_flits.push_back(make_pair(f_n,output_port));
            // Switch Arbiter Policy (round_robin_on_miss)
            _switch_arbiter_input_winner[input] = in_vc;
            
            // Decrement credit count
            BufferState * const dest_buf = _next_buf[output_port];
            if(f->head)
            {
                dest_buf->TakeBuffer(f_n->vc,f_n->pid);
                // Set VC
                cur_buf->SetState(in_vc, VC::active);
                cur_buf->SetRouteSet(in_vc, &f->la_route_set);
                cur_buf->SetOutput(in_vc, output_port, f_n->vc);
            }
            bool dimension_change = DimensionChange(input, output_port);
            dest_buf->SendingFlit(f_n, dimension_change);
            
            // Send credit. This flit moves to SA-O input buffers
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
            //_switch_arbiter_output_flits[input] = NULL;
            //_switch_arbiter_output_flits.erase(input);
            if(cur_buf->Empty(in_vc))    
            {
                _switch_arbiter_input_flits[iter.second] = false;
            }

            if(f->tail)
            {
                // Clear buffer's output
#ifdef FLIT_DEBUG
                if(f->watch)
                    *gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " tail flit: " << f->id << " packet: " << f->pid << " clears VC: " << in_vc << " input: " << input << std::endl;
#endif
                cur_buf->SetOutput(in_vc, -1, -1);
                cur_buf->SetState(in_vc,VC::idle);
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
    void BypassArbFBFCLRouter::_LookAheadRouteCompute(Flit * f, int output_port) {
#ifdef FLIT_DEBUG
                if(f->watch)
                    *gWatchOut << "Cycle: " << GetSimTime() << " Name " << FullName() << " LookAheadRouteCompute of Flit/Lookahead (l: 835): " << *f << std::endl;
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
    void BypassArbFBFCLRouter::_BufferWrite()
    {
        // Read all flits in the input of the BW stage
        for(auto const iter : _buffer_write_flits)
        {
            // Add flit to the buffer 
            int const input = iter.first;
            Flit * const f = iter.second;

            int const vc = f->vc;

            Buffer * const cur_buf = _buf[input];

#ifdef TRACK_FLOWS
            ++_stored_flits[f->cl][input];
            if(f->head) ++_active_packets[f->cl][input];
#endif

#ifdef FLIT_DEBUG
            if(f->watch)
                *gWatchOut << "Cycle: " << GetSimTime() << " Router: " << FullName() << " BufferWrite, Adding flit from buffer input " << input << " in vc " << vc << " Flit: " << f << " ID " << f->id << " pid " << f->pid << std::endl;
#endif
            cur_buf->AddFlit(vc, f);
            /////////////////////////////////////
            int expanded_input = input * _vcs + vc;
                
            _switch_arbiter_input_flits[expanded_input] = true;
            
            //if(cur_buf->GetState(vc) == VC::idle && f->head)
            // Part of a packet can be bypassed
            if(cur_buf->GetState(vc) == VC::idle && f->head)
            {
                // Set VC state to SA-I and VC route
                cur_buf->SetState(vc, VC::sa_input);
            }


#ifdef FLIT_DEBUG
            if(f->watch)
            {
                *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: BufferWrite | Flit: " << f->id << " head? " << f->head << " tail? " << f->tail << " | Input: " << input << " | VC: " << vc << " VC state " << cur_buf->GetState(vc) << std::endl;
            }
#endif
        }

        // Clean inputs for next cycle
        _buffer_write_flits.clear();

    }

    void BypassArbFBFCLRouter::_SwitchArbiterInput()
    {

#ifdef TRACK_STALLS
        int sa_i_conflicts[_classes] = {0};
#endif

        for(int expanded_input = 0; expanded_input < _inputs*_vcs; expanded_input++)
        {
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
            if(f->watch)
              *gWatchOut << "Cycle: " << GetSimTime() << " SA-I adding request " << input << " in vc " << in_vc << " Flit: " << f << " ID " << f->id << " pid " << f->pid << std::endl;
#endif

#ifdef TRACK_STALLS
            ++sa_i_conflicts[f->cl];
#endif
            //// Add request to the input arbiter
            //_switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri);
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
                //_switch_arbiter_input[input]->AddRequest(in_vc, f->id, INT_MAX); // FIXME: This can be the cause of deadlocks in FBFCL
                _switch_arbiter_input[input]->AddRequest(in_vc, f->id, f->pri);
            }
            */
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
            if(f->watch)
                *gWatchOut << "Cycle: " << GetSimTime() << " SA-I, Removing flit from buffer input " << input << " in vc " << in_vc << " Flit: " << f << " ID " << f->id << " pid " << f->pid << std::endl;
#endif
            _switch_arbiter_output_flits[input] = f;
            
            if(cur_buf->GetState(in_vc) == VC::sa_input)
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

    void BypassArbFBFCLRouter::_SendFlits( )
    {
        for(int output = 0; output < _outputs; ++output)
        {
            if(!_output_buffer[output].empty())
            {
                // Read flit from output buffer
                Flit * const f = _output_buffer[output].front();
                _output_buffer[output].pop( );

                // Add flit to the output channel
                _output_channels[output]->Send(f);

#ifdef TRACK_FLOWS
                ++_sent_flits[f->cl][output];
#endif

#ifdef FLIT_DEBUG
                if(f->watch)
                {
                    BufferState * const dest_buf = _next_buf[output];
                    *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: SendFlits | Flit: " << f->id << " pid " << f->pid << " head? " << f->head << " tail? " << f->tail << " | Output: " << output << " Occupancy: " << dest_buf->Occupancy() << " VC: " << f->vc << " VC Occupancy: " << dest_buf->OccupancyFor(f->vc) << std::endl;
                }
#endif
            }
        }
    }

    void BypassArbFBFCLRouter::_SendCredits( )
    {
        for(int input = 0; input < _inputs; ++input)
        {
            if(_credit_buffer[input] != NULL)
            {
                // Read credit
                Credit * const c = _credit_buffer[input];
                _credit_buffer[input] = NULL;
                // Send credit
                _input_credits[input]->Send(c);
#ifdef CREDIT_DEBUG
                Buffer * const cur_buf = _buf[input];
                *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: SendCredits | Credit: " << c->id << " Credit head? " << c->head << " tail? " << c->tail << " | Input: " << input << " Input buffer occupancy: " << cur_buf->GetOccupancy() << " VC: " << *(c->vc.begin()) << " VC occupancy " << cur_buf->GetOccupancy(*(c->vc.begin())) << std::endl;
#endif
            }

        }
    }

    void BypassArbFBFCLRouter::_SendLookahead()
    {
        // IMPORTANT NOTE: Note that the loop doesn't iterate through the last output port which corresponds with the terminal node, because it doesn't handle lookaheads
        for(int output = 0; output < _outputs; ++output)
        {

            bool is_router = _output_channels[output]->GetSink() != NULL ? true : false;

            if(!is_router && _lookahead_buffer[output] != NULL){
                _lookahead_buffer[output]->Free();
                _lookahead_buffer[output] = NULL;
                continue;
            }

            if(_lookahead_buffer[output] != NULL)
            {
                // Read credit
                Lookahead * const la = _lookahead_buffer[output];
                _lookahead_buffer[output] = NULL;
                // Send credit
                _output_lookahead[output]->Send(la);

#ifdef LOOKAHEAD_DEBUG
                if(la->watch)
                {
                    *gWatchOut << "Cycle: " << GetSimTime() << " | Router: " << FullName() << " | Stage: SendLookaheads | Lookahead: " << la->id << " pid " << la->pid << " head? " << la->head << " tail? " << la->tail << " | Output: " << output << std::endl;
                }
#endif
            }
        }
    }

    //-----------------------------------------------------------
    // misc.
    // ----------------------------------------------------------
    void BypassArbFBFCLRouter::Display( ostream & os ) const
    {
      for(int input = 0; input < _inputs; ++input){
        for(int in_vc = 0; in_vc < _vcs; ++in_vc){
          os << " Router: " << Name() << " Input: " << input
             << " Input VC: " << in_vc
             << " Input channel: " << _input_channels[input]->Name();

          Buffer * cur_buf = _buf[input];
          Flit * const f = cur_buf->FrontFlit(in_vc);
          if(f){
            os << " Front Flit: " << f->id << " Packet: " << f->pid;
          }else{
            os << " Buffer Empty" << std::endl;
            continue;
          }

          int output_requested = cur_buf->GetOutputPort(in_vc);
          int out_vc = cur_buf->GetOutputVC(in_vc);
          if(output_requested > -1){
            os << " Output port: " << output_requested
               << " Output VC: " << out_vc
               << " Output channel: " << _output_channels[output_requested]->Name();
            bool is_router = _output_channels[output_requested]->GetSink() != NULL ? true : false;
            if(is_router){
              os << " Next router: " << _output_channels[output_requested]->GetSink()->Name();
            }
          }else{
            os << " Hasn't requested an output yet";
          }

          os << std::endl;




        }
          //_buf[input]->Display( os );
      }
    }
} // namespace Booksim
