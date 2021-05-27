// $Id$

/*
 Copyright (c) 2014-2020, Trustees of The University of Cantabria
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.  Redistributions in binary
 form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with
 the distribution.

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

#include "smart_nebb_vct_opt_router.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cassert>
#include <limits>
#include <climits>
#include <algorithm>

#include "../../../globals.hpp"
#include "../../../random_utils.hpp"
#include "../../../vc.hpp"
#include "../../../routefunc.hpp"
#include "../../../outputset.hpp"
#include "../../../buffer.hpp"
#include "../../../buffer_state.hpp"
#include "../../../arbiters/roundrobin_arb.hpp"
#include "../../../allocators/allocator.hpp"

namespace Booksim
{

    SMARTNEBBVCTOPTRouter::SMARTNEBBVCTOPTRouter(Configuration const & config, Module *parent, string
            const & name, int id, int inputs, int outputs) : SMARTRouter( config, parent,
                name, id, inputs, outputs )
    {
        // NEBB-VCT
        _output_port_blocked.resize(_outputs, -1);
        _input_port_blocked.resize(_outputs, -1);

        _bypass_blocked.resize(_inputs, false);

        //_destination_credit_ps.resize(_outputs, make_pair(-1,-1));

    }

    SMARTNEBBVCTOPTRouter::~SMARTNEBBVCTOPTRouter() {
        // @XXX: Is there something to destroy?
    }

    void SMARTNEBBVCTOPTRouter::ReadInputs() {

        for (int input = 0; input < _inputs; ++input) { 

            // Read input channel
            Flit * const f = _input_channels[input]->Receive();

            // Proceed if there is a flit
            if (f) {
                 ++_active;
#ifdef TRACK_FLOWS
                 ++_received_flits[f->cl][input];
#endif
                //Perform Buffer Write
                BufferWrite(input, f);
            }
        }

        for (int input = 0; input < _inputs; input++) {
            if(!_flits_to_BW[input].empty()) {
                //BSMOD: Change time to long long
                pair<long long, Flit *> elem = _flits_to_BW[input].front();
                if (elem.second && elem.first == GetSimTime()) {
                    BufferWrite(input, elem.second);
                    _flits_to_BW[input].pop();
                }
            }
        }

        for (int output = 0; output < _outputs; output++) {
            if (!_destination_queue_credits[output].empty()) {
                //BSMOD: Change time to long long
                pair<long long, Credit *> elem =
                    _destination_queue_credits[output].front();
                if (elem.first >= GetSimTime()){
                    continue;
                }

                Credit * const c = elem.second;

                if (c) {
                    // @XXX: destination credits to reseemble Bluespec
                    _next_buf[output]->ProcessCredit(c, true);
#if defined(PIPELINE_DEBUG)
                    *gWatchOut  << GetSimTime() << " | " << FullName()
                        << " | Credit Reception | Flit " << c->id
                        << " | Output " <<  output << " Destination queue"
                        << std::endl;
#endif 
                    c->Free();
                }

                _destination_queue_credits[output].pop();
            }
        }

        for (int output = 0; output < _outputs; output++) {
            Credit * const c = _output_credits[output]->Receive();

            if (c) {
                 // FIXME: Esto es un fix muy guarro (mirar en VCManagement)
#if defined(PIPELINE_DEBUG)
				*gWatchOut  << GetSimTime() << " | " << FullName()
							<< " | Credit Reception | Flit " << c->id
							<< " | Output " <<  output << std::endl;
#endif 
                 if (output < _outputs - gC)
                 {
                     _next_buf[output]->ProcessCredit(c, true);
#if defined(PIPELINE_DEBUG)
                    *gWatchOut  << GetSimTime() << " | " << FullName()
                                << " | Credit Reception | Flit " << c->id
                                << " | Output " <<  output << std::endl;
#endif 
                 }
                 c->Free();
            }

            // FIXME: Chapuza para emular a bluespec
            //if (_destination_credit_ps[output].first > -1)
            //{
            //    Credit * c = Credit::New();
            //    c->id = 777;
            //    c->vc.insert(_destination_credit_ps[output].first);
            //    c->packet_size = _destination_credit_ps[output].second;
            //    _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
            //    _destination_credit_ps[output] = make_pair(-1,-1);
            //}

#ifdef PIPELINE_DEBUG
            *gWatchOut  << GetSimTime() << " | " << FullName()
                        << " | Credit availability | Output " 
                        << output << " | " << _next_buf[output]->Print()
                        << std::endl;
#endif

        }
        SwitchTraversal();
    }

    int SMARTNEBBVCTOPTRouter::FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance) {
        
        // Firstly, empty VCs
        if(f->head){
            for(int vc=vc_start; vc <= vc_end; vc++){
                if(_next_buf[output]->IsAvailableFor(vc) &&
                    _next_buf[output]->AvailableFor(vc) == _next_buf[output]->LimitFor(vc) &&
                    (distance == 0 || (distance > 0 && FreeLocalBuf(input, vc_start, vc_end)))
                  ){
                    return vc;
                }
            }
        }

        // Secondly, VCs with room for the whole packet and the next router has an idle VC
        for(int vc=vc_start; vc <= vc_end; vc++){
            if(f->head){ 
                if(_next_buf[output]->IsAvailableFor(vc)){
                    if(_next_buf[output]->AvailableFor(vc) >= f->packet_size){
                            return vc;
                    }
                }
            } else{
                if(_next_buf[output]->UsedBy(vc) == f->pid){
                    return vc;
                }
            }
        }
        
        return -1;
    }

    void SMARTNEBBVCTOPTRouter::ReadFlit(int input, Flit * f) {
#ifdef TRACK_FLOWS
        ++_received_flits[f->cl][input];
#endif
        ++_active;

        // Compute hop route
        OutputSet nos;
        _rf(this, f, input, &nos, false);
        f->la_route_set = nos;

        // @XXX: f ID shouldn't be used
        bool premat_stop = false;
        for (int vc = 0; vc < _vcs; vc ++) {
            if (_prematurely_stop[input*_vcs+vc] == f->pid) {
                premat_stop = true;
            }
        }

        // NEBB-VCT with VCT credits
        bool local_flit_between_sal_sag = false;

        bool idle_vc_for_bypass = f->head ? _buf[input]->GetState(f->vc) == VC::idle : true;
        bool body_flit_bypass_blocked = !f->head ? _bypass_blocked[input] : true;

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
        if (f->watch) {
            *gWatchOut  << GetSimTime() << " | " << FullName() << " | ReadFlit | Flit " << f->id
                << " | Input " <<  input << " | Output " << -1 << " | L | PID " << f-> pid
                << " Input VC: " << f->vc
                << " FreeLocalBuf: " << FreeLocalBuf(input, f->vc, f->vc)
                << " _bypass_path: " << _bypass_path[input]
                << " !premat_stop: " << !premat_stop
                << " idle_vc_for_bypass: " << idle_vc_for_bypass
                << " !local_flit_between_sal_sag: " << !local_flit_between_sal_sag
                << " body_flit_bypass_blocked: " << body_flit_bypass_blocked
                << std::endl;
        }
#endif 
        if (_bypass_path[input] == f->pid && !premat_stop && idle_vc_for_bypass && !local_flit_between_sal_sag && body_flit_bypass_blocked) {
            {
            
                int output = _dest_output[input];
                int in_vc = f->vc;
                f->vc = _dest_vc[input];
            
                if (f->head) {
                    if(_out_queue_smart_credits.count(input) == 0) {
                      _out_queue_smart_credits.insert(make_pair(input, Credit::New()));
                    }

                    _out_queue_smart_credits.find(input)->second->id = f->id;
                    _out_queue_smart_credits.find(input)->second->vc.insert(in_vc);
                    _out_queue_smart_credits.find(input)->second->packet_size = f->packet_size;


#if defined(PIPELINE_DEBUG)
                    if (f->watch) {
                        *gWatchOut  << GetSimTime() << " | " << FullName()
                                    << " | Credit Bypass | Flit " << f->id
                                    << " | Input " <<  input << " | vc "
                                    << in_vc << " | PID " << f-> pid
                                    << " | Packet size: " << f->packet_size
                                    << std::endl;
                    }
#endif 
                 }


                if (f->head) {
                    _next_buf[output]->TakeBuffer(f->vc, f->pid);
                    // XXX: We do this flit by flit because the credits are
                    // inmediatelly processed in Bluespec
                    if (output >= _outputs - gC)
                    {
                        //_destination_credit_ps[output] = make_pair(f->vc,1);
                        Credit * c = Credit::New();
                        c->id = f->id;
                        c->packet_size = f->packet_size;
                        c->vc.insert(f->vc);
                        _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
                    }
                }

                _next_buf[output]->SendingFlit(f, true); // Activate VCT flag
                _bypass_credit[output] = true;

                TransferFlit(input, output, f);
                // Decrement credit count and prepare backwards credit
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | ST+LT (Bypass) | Flit " << f->id
                                << " | Input " <<  input << " | Output " << output << " | B | PID " << f-> pid << std::endl;
                }
#endif 
                if (f->head) {
                    _bypass_blocked[input] = true;
                }

                if (f->tail) {
                    _bypass_path[input] = -1;
                    _bypass_blocked[input] = false;
                }
            }
        } else {
            _flits_to_BW[input].push(make_pair(GetSimTime()+1,f));
            _prematurely_stop[input*_vcs+f->vc] = f->tail ? -1 : f->pid;
        }
    }

    void SMARTNEBBVCTOPTRouter::TransferFlit(int input, int output, Flit * f) {
        //SMARTRouter* router = const_cast<SMARTRouter *>(GetNextRouter(output));
        SMARTNEBBVCTOPTRouter* router = (SMARTNEBBVCTOPTRouter *)GetNextRouter(output);
        int in_channel = GetOutputFlitChannel(output)->GetSinkPort();

        // Increment the number of hops
        f->hops++;
        f->hpc.back()++;

        if (router) { // Next hop is another router
            router->ReadFlit(in_channel, f);
        }else{ // Next hop is the destination router
            assert(output >= _outputs - gC);

            _output_channels[output]->Send(f);
        }

        // NEBB-VCT
        if (f->head) {
            _output_port_blocked[output] = f->pid;
            _input_port_blocked[input] = f->pid;
        }
        if (f->tail) {
            _output_port_blocked[output] = -1;
            _input_port_blocked[input] = -1;
        }
        --_active;
    }

    bool SMARTNEBBVCTOPTRouter::AddRequestSAG(SMARTRequest sr) {

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
        if (sr.f->watch) {
            *gWatchOut  << GetSimTime() << " | " << FullName()
                        << " | SSR | Flit " << sr.f->id
                        << " | Input " <<  sr.input_port 
                        << " | Output " << sr.output_port
                        << " | PID " << sr.f-> pid
                        << " | SSR distance " << sr.distance
                        << std::endl;
        }
#endif 

        if (sr.distance >= _hpc_max) {
            return true;
        }

        if (_smart_dimensions == "oneD" && sr.distance > 0 && (sr.input_port/2 != sr.output_port/2 || sr.output_port >= _outputs - gC)) {
            return true;
        }
        // Dest router bypass optimization
        // FIXME: This avoids low throughput in SMART++_nD (It removes the
        // last router bypass optimization described in the HPCA'13 paper. The
        // low throughput regards oneD is caused by the Bluespec fix
        // to match the timing of the credit generation in destinations. The
        // credit cannot be generated during SA-G like when transfering a
        // local flit because of bypass enabling false positives)
        //if (_smart_dimensions == "nD" && sr.distance > 0 && sr.output_port >=
        //        _outputs-gC) {
        //    return true;
        //}

        _sag_requests.push(sr);

        return false;
    }

    void SMARTNEBBVCTOPTRouter::FilterSAGRequests() {

        while ( !_sag_requests.empty()) {
            SMARTRequest sr = _sag_requests.front();
            _sag_requests.pop();

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName()
                            << " | SA-G (pre-processing) | Flit " << sr.f->id
                            << " | Input " <<  sr.input_port 
                            << " | Output " << sr.output_port
                            << " | PID " << sr.f-> pid
                            << " | SSR distance " << sr.distance
                            << " | _output_port_blocked[Output] " << _output_port_blocked[sr.output_port]
                            << " | _input_port_blocked[Output] " << _input_port_blocked[sr.input_port]
                            << " | bypass blocked for PID: " << _bypass_path[sr.input_port]
                            << std::endl;
            }
#endif 

            // NEBB-VCT: if output port in used by a packet ignore this SR.
            if (sr.f->head && _output_port_blocked[sr.output_port] > -1) {
                continue;
            }
            
            // NEBB-VCT: if bypass path is blocked for a packet for another
            // output ignore this SR.
            if (sr.f->head && _input_port_blocked[sr.input_port] > -1) {
                continue;
            }
            // NEBB-VCT: if the output for is not blocked for this packet
            // ignored. In a real implementation only SR of head flits makes
            // sense. 
            if (!sr.f->head &&
                    _output_port_blocked[sr.output_port] != sr.f->pid &&
                    _input_port_blocked[sr.input_port] != sr.f->pid
                ) {
                continue;
            }

            // NEBB-VCT: If the bypass is blocked ignore this SSR.
            if (_bypass_blocked[sr.input_port] && sr.distance > 0) {
                continue;
            }

            assert(sr.f->head ? _output_port_blocked[sr.output_port] == -1 : _output_port_blocked[sr.output_port] == sr.f->pid );

            // The expanded input is used to differenciate between local and
            // "global" SSRs.
            int expanded_input = sr.distance == 0 ? sr.input_port * 2 : sr.input_port * 2 + 1;

            if (!_sag_requestors[expanded_input].f || 
                !sr.f->head ||
                (_sag_requestors[expanded_input].f && _sag_requestors[expanded_input].distance > sr.distance &&
                 _sag_requestors[expanded_input].f->head)
               )
            {
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                SMARTRequest prev_sr = _sag_requestors[expanded_input];
                if (prev_sr.f && prev_sr.f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName()
                                << " | SA-G (pre-processing) | Flit " << prev_sr.f->id
                                << " | Input " <<  prev_sr.input_port 
                                << " | Output " << prev_sr.output_port
                                << " | PID " << prev_sr.f-> pid
                                << " | SSR distance " << prev_sr.distance
                                << " | Killed by SSR: " << sr.f->id
                                << " | Input " <<  sr.input_port 
                                << " | Output " << sr.output_port
                                << " | PID " << sr.f-> pid
                                << " | SSR distance " << sr.distance
                                << " | _output_port_blocked[Output] " << _output_port_blocked[sr.output_port]
                                << std::endl;
                } else if (sr.f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName()
                                << " | SA-G (pre-processing) | Flit " << sr.f->id
                                << " | Input " <<  sr.input_port 
                                << " | Output " << sr.output_port
                                << " | PID " << sr.f-> pid
                                << " | SSR distance " << sr.distance
                                << " | Pass Filter"
                                << std::endl;
                }
#endif 
                _sag_requestors[expanded_input] = sr;
            }
        }
    }

    void SMARTNEBBVCTOPTRouter::SwitchAllocationLocal() {
#ifdef TRACK_FLOWS
        bool increase_allocations = false;
#endif

        // Arbitrate among every flit in the front of a VC
        // TODO: change this to hold the VC until the whole packet is sent.
        for (int input = 0; input < _inputs; input++) {
            //assert(_sal_to_sag[input] == NULL);
            if (_sal_to_sag[input] != NULL) {
                // FIXME: For the moment this only works for single route routing algorithms.
                Flit *of = _sal_to_sag[input];

                set<OutputSet::sSetElement> const route = of->la_route_set.GetSet();
                // FIXME: pick last route's output port. With adaptive algorithms this
                // doesn't work.
                int o_output = -1;
                for (auto iter : route) {
                    o_output = iter.output_port;
                }
                assert(o_output > -1);
                vector<SMARTRequest> smart_requests = GetFlitRoute(of, input, o_output);
                // Place SMART Request to Switch Allocation Global of router X
                for (auto request : smart_requests) {
                    request.router->AddRequestSAG(request);
                }
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (of->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | SAL-2-SAG | Flit " << of->id
                                << " | Input " <<  input << " | Output " << -1 << " | PID " << of-> pid
                                << std::endl;
                }
#endif 
                continue;
            }
            int vc = _sal_next_vc[input];
            
            Buffer * cur_buf = _buf[input];
            
            int dest_vc = -1;
            int output = -1;
            int priority = -1;
            
            int end_vc_iter = _sal_next_vc[input];
            do{
                Flit * f = cur_buf->FrontFlit(vc);
                if (f) {

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                    if (f->watch) {
                        *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAL | Flit " << f->id
                            << " | Input " <<  input << " | Output " << -1 << " | PID " << f-> pid
                            << " vc: " << vc << " Active? " << (cur_buf->GetState(vc) == VC::active)
                            << " Active PID: " << cur_buf->GetActivePID(vc)
                            << std::endl;
                    }
#endif 

                    // The input VC is already sending a packet, increase VC until reaching the next body flit.
                    if (f->head && cur_buf->GetState(vc) == VC::active) {
                        vc = vc == _vcs-1 ? 0 : vc + 1;
                        continue;
                    }

                    // Check if there is an available destination in one of the routes of the route set.
                    set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                    // FIXME: pick last route's output port. With adaptive algorithms this
                    // doesn't work.
                    if (f->head) {
                        for (auto iter : route) {
                            output = iter.output_port;
                        }
                        dest_vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
                        priority = 0;
                    } else { // If it's a body flit, read the VC info.
                        output = cur_buf->GetOutputPort(vc);
                        dest_vc = cur_buf->GetOutputVC(vc);
                        priority = 1; // Priority to body flits so the packet can progress contiguously
                    }

                    // The input VC is already sending a packet, increase VC until reaching the next body flit.

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                    if (f->watch && output > -1) {
                        *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAL 1 | Flit " << f->id
                                    << " | Input " <<  input << " | Output " << output << " | PID " << f-> pid
                                    << " vc: " << vc << " Active? " << (cur_buf->GetState(vc) == VC::active)
                                    << " dest_vc: " << dest_vc;
                        if (dest_vc > -1) {
                            *gWatchOut
                                << " In use by: " << _next_buf[output]->UsedBy(dest_vc);

                        }
                        else {
                            *gWatchOut << " | Credit availability | "
                                       << _next_buf[output]->Print()
                                       << std::endl;
                        }
                        *gWatchOut  << " Output port blocked by: " << _output_port_blocked[output] 
                                    << std::endl;
                    }
#endif 

                    // If there is a free destination VC
                    //if (output > -1 && dest_vc > -1 && !_sal_o_winners[output]) {
                    if (dest_vc > -1 && output > -1) {

                        // NEBB-VCT
                        bool req = true;
                        if (f->head && _output_port_blocked[output] > -1) {
                          req = false;
                        } else if (!f->head && _output_port_blocked[output] != f->pid) {
                          req = false;
                        }

                        if (req) {
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                            if (f->watch && output > -1) {
                                *gWatchOut  << GetSimTime() << " | " << FullName()
                                    << " | Starting SAL 2 | Flit " << f->id
                                    << " | Input " <<  input << " | Output "
                                    << output << " | PID " << f-> pid
                                    << " vc: " << vc << " Active? "
                                    << (cur_buf->GetState(vc) == VC::active)
                                    << " dest_vc: " << dest_vc << " In use by: "
                                    << _next_buf[output]->UsedBy(0)
                                    << std::endl;
                            }
#endif 
                            //if (output > -1) {
                            _sw_allocator_local->AddRequest(input, output, vc,
                                    priority, priority);

                            // Finish the do-while loop
                            break;
                        }
                    }
                }

                // Pick next vc for the following iteration
                vc = vc == _vcs-1 ? 0 : vc + 1;
            } while (vc != end_vc_iter);
            
            // Increase VC for next cycle (Round-Robin)
            _sal_next_vc[input] = vc + 1;
            if (_sal_next_vc[input] >= _vcs) {
                _sal_next_vc[input] = 0;
            }
        }

        // Perform SA-L 
        _sw_allocator_local->Allocate();

        // Send SSR requests up to _hpc_max for the winners
        // FIXME: This has to be done in the next cycle
        // TODO: Generate a table with the smart_requests for each output port.
        // 		 Entries with NULL values are not considered.
        for (int output = 0; output < _outputs; output++) {

            // TODO: Should I create an additional method for the following
            // lines?
            int input = _sw_allocator_local->InputAssigned(output);


            // If the input pipeline register hasn't been dequeue go to the
            // next SA-L winner
            if (input == -1) {
                continue;
            }
            int vc = _sw_allocator_local->ReadRequest(input, output);
            Buffer * cur_buf = _buf[input];
            assert(!cur_buf->Empty(vc));
            Flit * f = cur_buf->FrontFlit(vc);

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName()
                            << " | SA-L | Flit " << f->id
                            << " | Input " <<  input
                            << " | Output " << output
                            << " | PID " << f-> pid
                            << std::endl;
            }
#endif 
        
            // FIXME: For the moment this only works for single route
            //        routing algorithms.
            vector<SMARTRequest> smart_requests = GetFlitRoute(f,
                                                               input,
                                                               output);

#ifdef TRACK_FLOWS
            increase_allocations = true;
#endif

            // Dequeue flit from input buffer and store it in the pipeline register
            cur_buf->RemoveFlit(vc);
            _sal_to_sag[input] = f;
            //_sal_o_winners[output] = true;
        
            // FIXME: delay should be 0 (or 1, I don't know for sure) to make this work
            //_input_credits[input]->Send(c);
            if (f->head) {  //&& f->packet_size == 1) {
                if(_out_queue_credits.count(input) == 0) {
                  _out_queue_credits.insert(make_pair(input, Credit::New()));
                }
                _out_queue_credits.find(input)->second->id = f->id;
                _out_queue_credits.find(input)->second->vc.insert(vc);
                _out_queue_credits.find(input)->second->packet_size = 1; // In SA-L we free only 1 slot

#if defined(PIPELINE_DEBUG)
                if (f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Local | Flit " << f->id
                                << " | Input " <<  input << " | vc " << vc << " | PID " << f-> pid << std::endl;
                }
#endif 
            }


            if (f->head || !f->tail) {
                _sal_next_vc[input] = vc;
            }
            if (f->tail) {
                _sal_next_vc[input] = vc+1;
                if (_sal_next_vc[input] >= _vcs) {
                    _sal_next_vc[input] = 0;
                }
            }
            
            // Take buffer and decrement credit count
            //VCManagement(input, vc, output, f);

            // Place SMART Request to Switch Allocation Global of router X
            for (auto request : smart_requests) {
                request.router->AddRequestSAG(request);
            }
        }
        // Reset _sw_allocator_local
        _sw_allocator_local->Clear();
#ifdef TRACK_FLOWS
        if (increase_allocations) {
            _sal_allocations++;
        }
#endif
    }

    void SMARTNEBBVCTOPTRouter::SwitchAllocationGlobal() {

        // NEBB-VCT: evaluation of the rules
        FilterSAGRequests();

        // Reset Bypass flags from previous cycle
        for (int input = 0; input < _inputs; input++) {
            if (!_bypass_blocked[input]) {
                _bypass_path[input] = -1;
            }
        }

#ifdef TRACK_FLOWS
        bool increase_allocations = false;
#endif

        for (int expanded_input = 0; expanded_input < _inputs*2; expanded_input++) {

            SMARTRouter::SMARTRequest sr = _sag_requestors[expanded_input];
            if (!sr.f) {
                continue;
            }
            // Check if there is a free VC
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (sr.f && sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAG | Flit " << sr.f->id
                            << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                            << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                            << std::endl;
            }
#endif 

            if(sr.output_port < _outputs && FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1) {
                // SMART NEBB-VCT: input VC is transfering another packet
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (sr.f && sr.f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAG 1 | Flit " << sr.f->id
                                << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                                << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                                << std::endl;
                }
#endif 
                if (sr.distance == 0 && sr.f->head && _buf[sr.input_port]->GetState(sr.f->vc) == VC::active) {
                    continue;
                }

                // Don't request SA-G if a previous flit of the packet has been
                // buffered.
                //
                bool premat_stop = false;
                for (int vc = 0; vc < _vcs; vc ++) {
                    if (_prematurely_stop[sr.input_port*_vcs+vc] == sr.f->pid && sr.distance > 0) {
                        premat_stop = true;
                    }
                }

                if (premat_stop) {
                    continue;
                }

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (sr.f && sr.f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAG 2 | Flit " << sr.f->id
                                << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                                << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                                << std::endl;
                }
#endif 


                // Priority to Local flits
                int priority = _hpc_max - sr.distance;
                // SMART-2D priorities
                // 4 priorities: dest (3), straight (2), turn left (1), turn right (0)
                if (_smart_dimensions == "nD") {
                    priority = priority*4; // 4 priorities

                    // Dest port
                    if (sr.output_port >= _outputs - gC){
                        priority = priority+3; // It doesn't matter the value
                    }
                    // Straigth
                    else if (sr.initial_output_port == sr.output_port) {
                        priority = priority+2;
                    } else {
                        bool same_dim = sr.initial_output_port/2 == sr.output_port/2;
                        assert(!same_dim);
                        // Turn: true (left), false (right)
                        bool turn_left = sr.initial_output_port%2 == sr.output_port%2;
                        if (turn_left) {
                            priority = priority + 1;
                        }
                        //else {
                        //    priority = priority;
                        //}
                    }
                }
                // Priority to body flits over head flits (VCT)
                priority = !sr.f->head ? priority*2 : priority;
                _sag_arbiters[sr.output_port]->AddRequest(expanded_input, sr.f->id, priority);
            }
        }


        // Setup crossbar, bypass path, bw selector, send flits and credits...
        for (int output = 0; output < _outputs; output++) {
            //_sal_o_winners[output] = false;
            int expanded_input = _sag_arbiters[output]->Arbitrate();
            _sag_arbiters[output]->Clear();

            if (expanded_input == -1) {
                // There isn't a request for "output"
                continue;
            }

            // Destination VC.
            SMARTRouter::SMARTRequest sr = _sag_requestors[expanded_input];

            int input = expanded_input / 2;

            assert(sr.f);

            // XXX: I have to store the class of packet in another structure
            //      to know the range of VCs the winner can use.
            int vc;
            if (sr.f->head) {
                vc = FreeDestVC(input, output, sr.vc_start,
                                sr.vc_end, sr.f, sr.distance);

            } else {
                vc = FreeDestVC(input, output, gBeginVCs[sr.f->cl],
                                gEndVCs[sr.f->cl], sr.f, sr.distance);
            }

            assert(vc > -1);

            assert(sr.f);

#ifdef TRACK_FLOWS
            increase_allocations = true;
#endif

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | SA-G | Flit " << sr.f->id
                            << " | Input " <<  input << " | Output " << output << " | PID " << sr.f-> pid << std::endl;
            }
#endif

            if (sr.distance > 0) {
                _dest_vc[input] = vc;
                _dest_output[input] = output;
                _bypass_path[input] = sr.f->pid;
            }
            else{
                Flit * f = _sal_to_sag[input];
                assert (f->id == sr.f->id);
                _sal_to_sag[input] = NULL;

                //if (f->watch) {
                // FIXME: I think the following line is wrong
                //f->vc = vc;
                assert(_crossbar_flits[input].second == NULL);

                _crossbar_flits[input] = make_pair(output, f);

                int in_vc = f->vc;
                f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
                VCManagement(input, in_vc, output, f);

                // FIXME: delay should be 0 (or 1, I don't know for sure) to make this work
                //_input_credits[input]->Send(c);
                if (f->head && f->packet_size > 1) {
                    if(_out_queue_credits.count(input) == 0) {
                      _out_queue_credits.insert(make_pair(input, Credit::New()));
                    }
                    _out_queue_credits.find(input)->second->id = f->id;
                    _out_queue_credits.find(input)->second->vc.insert(in_vc);
                    _out_queue_credits.find(input)->second->packet_size = f->packet_size - 1;
#if defined(PIPELINE_DEBUG)
                    if (f->watch) {
                        *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Local | Flit " << f->id
                                    << " | Input " <<  input << " | vc " << vc << " | PID " << f-> pid << std::endl;
                    }
#endif
                }

                // NEBB-VCT
                if (f->head) {
                    _output_port_blocked[output] = f->pid;
                }
                if (f->tail) {
                    _output_port_blocked[output] = -1;		
                }

            }
        }
        
        for (int expanded_input = 0; expanded_input < _inputs*2; expanded_input++) {
            _sag_requestors[expanded_input] = {NULL, NULL, -1, -1, -1, -1, -1, -1, -1, -1};
        }
        // Reset _sw_allocator_global
        //_sw_allocator_global->Clear();

#ifdef TRACK_FLOWS
        if (increase_allocations) {
            _sag_allocations++;
        }
#endif
    }

    int SMARTNEBBVCTOPTRouter::IdleLocalBuf(int input, int vc_start, int vc_end, Flit * f) {
     for (int vc=vc_start; vc <= vc_end; vc++) {
        
        bool flit_in_sag_reg = true;
        //if (_sal_to_sag[input]) {
        //	Flit * of = _sal_to_sag[input];
        //	flit_in_sag_reg = of->vc != vc;
        //}
        
        if (f->head && _buf[input]->GetState(vc) == VC::idle && flit_in_sag_reg) {
            return vc;
        } else if (!f->head && _buf[input]->GetState(vc) == VC::active && f->pid == _buf[input]->GetActivePID(vc)) {
            return vc;
        }
     } 
     return -1;
    }

    void SMARTNEBBVCTOPTRouter::VCManagement(int input, int in_vc, int output, Flit * f) {

        if (f->head) {
            // FIXME: Read first route
            set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
            int output = route.begin()->output_port;

            f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
            // Set input VC output for body flits
            //assert(_buf[input]->GetState(in_vc) == VC::idle);
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch && output > -1) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | VCManagement | Flit " << f->id
                                << " | Input " <<  input << " | Output " << output << " | PID " << f-> pid
                                << " in VC: " << in_vc 
                                << " dest VC: " << f->vc
                                << " Packet size: " << f->packet_size
                                << " Avail slots: "   << _next_buf[output]->AvailableFor(f->vc)
                                << std::endl;
                }
#endif 
            _buf[input]->SetOutput(in_vc, output, f->vc);
            if (!f->tail) {
                if (_buf[input]->GetState(in_vc) == VC::active) {
                    std::cout << "Error a head is setting a VC while the VC is Active. Router: " << _id
                        << " input: " << input << " VC: " << in_vc << " Flit: " << f->id << " pid: " << f->pid
                        << " Front Flit: " << _buf[input]->FrontFlit(in_vc)->id
                        << " pid: " << _buf[input]->FrontFlit(in_vc)->pid
                        << std::endl;
                }
                assert(_buf[input]->GetState(in_vc) == VC::idle);
                _buf[input]->SetState(in_vc, VC::active);
                _buf[input]->SetActivePID(in_vc, f->pid);
            }
        
            // Take next buffer
            // FIXME: Fix guarro
            //if (output < _outputs - gC)
            {
                _next_buf[output]->TakeBuffer(f->vc, f->pid);
            }
        }else{
            //f->vc = _buf[input]->GetOutputVC(in_vc);
            f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
            if (f->tail) {
                _buf[input]->SetOutput(in_vc, -1, -1);
                _buf[input]->SetState(in_vc, VC::idle);
                _buf[input]->SetActivePID(in_vc, -1);
            }
        }

        // Decrement credit count
        // FIXME: Esto es un fix guarro para evitar la latencia extra que introduce el LT al mandar el flit al nodo de destino.
        //SMARTRouter * router = const_cast<SMARTRouter *>(GetNextRouter(output));
        //if (router) {
        //if (output < _outputs - gC)
        
        _next_buf[output]->SendingFlit(f, true);
        if (f->head)
        {
            if (output >= _outputs - gC)
            {
                //_destination_credit_ps[output] = make_pair(f->vc, f->packet_size);
                Credit * c = Credit::New();
                c->id = f->id;
                c->packet_size = f->packet_size;
                c->vc.insert(f->vc);
                _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
            }
        }
    }
} // namespace Booksim
