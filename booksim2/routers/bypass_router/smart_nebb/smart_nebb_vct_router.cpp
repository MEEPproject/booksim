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

#include "smart_nebb_vct_router.hpp"

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

    SMARTNEBBVCTRouter::SMARTNEBBVCTRouter(Configuration const & config, Module *parent, string
            const & name, int id, int inputs, int outputs) : SMARTRouter( config, parent,
                name, id, inputs, outputs )
    {
        // NEBB-VCT
        _output_port_blocked.resize(_outputs, -1);

    }

    SMARTNEBBVCTRouter::~SMARTNEBBVCTRouter() {
        //std::cout << "~SMARTNEBBVCTRouter not implemented" << std::endl;
        //SMARTRouter::~SMARTRouter();
    }

    int SMARTNEBBVCTRouter::FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance) {
        
        // Firstly, empty VCs
        if(f->head){
            for(int vc=vc_start; vc <= vc_end; vc++){
                if(_next_buf[output]->IsAvailableFor(vc) && _next_buf[output]->AvailableFor(vc) == _next_buf[output]->LimitFor(vc) && (distance == 0 || (distance > 0 && FreeLocalBuf(input, vc_start, vc_end)))){
                    return vc;
                }
            }
        }

        // Secondly, VCs with room for the whole packet and the next router has an idle VC
        for(int vc=vc_start; vc <= vc_end; vc++){
            if(f->head){ 

                SMARTNEBBVCTRouter * next_router = (SMARTNEBBVCTRouter *)GetNextRouter(output);
                bool next_router_vc_idle = false;
                if (next_router) {
                    int next_input = (output/2)*2+(output+1)%2;
                    next_router_vc_idle = next_router->IdleLocalBuf(next_input, vc, vc, f) > -1;	
                }

                if(distance == 0 || (distance > 0 && IdleLocalBuf(input, vc_start, vc_end, f) > -1 && next_router_vc_idle))
                {
                    if(_next_buf[output]->IsAvailableFor(vc)){
                        if(_next_buf[output]->AvailableFor(vc) >= f->packet_size){
                                return vc;
                        }
                    }
                }
            } else{
                if(_next_buf[output]->UsedBy(vc) == f->pid){
                    if(_next_buf[output]->AvailableFor(vc)){
                        return vc;
                    }
                }
            }
        }
        
        // Thirdly, VCs with room for the whole packet
        for(int vc=vc_start; vc <= vc_end; vc++){
            if(f->head){ 

                if(distance == 0 || (distance > 0 && IdleLocalBuf(input, vc_start, vc_end, f) > -1))
                {
                    if(_next_buf[output]->IsAvailableFor(vc)){
                        if(_next_buf[output]->AvailableFor(vc) >= f->packet_size){
                                return vc;
                        }
                    }
                }
            } else{
                if(_next_buf[output]->UsedBy(vc) == f->pid){
                    if(_next_buf[output]->AvailableFor(vc)){
                        return vc;
                    }
                }
            }
        }

        return -1;
    }

    void SMARTNEBBVCTRouter::ReadFlit(int input, Flit * f) {
#ifdef TRACK_FLOWS
             ++_received_flits[f->cl][input];
#endif

        // Compute hop route
        OutputSet nos;
        _rf(this, f, input, &nos, false);
        f->la_route_set = nos;

        // FIXME: f ID shouldn't be used
        // FIXME: I'm not sure if we have to check if another flit of the packet has been buffered here.
        //bool ready_vc = IdleLocalBuf(input, gBeginVCs[f->cl], gEndVCs[f->cl], f) > -1; // FIXME: Esto es una solucion temporal
        //if (_bypass_path[input] == f->id && _prematurely_stop[input*_vcs+f->vc] != f->pid && ready_vc) {
        bool premat_stop = false;
        for (int vc = 0; vc < _vcs; vc ++) {
            if (_prematurely_stop[input*_vcs+vc] == f->pid) {
                premat_stop = true;
            }
        }
        //bool idle_vc_for_bypass = IdleLocalBuf(input, gBeginVCs[f->cl], gEndVCs[f->cl], f) > -1;
        bool idle_vc_for_bypass = f->head ? _buf[input]->GetState(f->vc) == VC::idle : true;
        if (_bypass_path[input] == f->id && !premat_stop && idle_vc_for_bypass) {
            {
            
                //int output;
                //if (f->head) {
                //// FIXME: We are supposing that the routing is deterministic.
                //    set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
                //    for (auto iter : route) {
                //    //  int vc = FreeDestVC(iter.output_port, iter.vc_start, iter.vc_end);
                //    //  assert(vc > -1);
                //        output = iter.output_port;
                //    }
                //}
                //else{
                //    // Body flit reads output port from input VC
                //    //output = _buf[input]->GetOutputPort(f->vc);
                //	
                //	// XXX: Read from dedicated registers
                //	output = _dest_output[input];
                //}
                int output = _dest_output[input];
            
                if(_out_queue_smart_credits.count(input) == 0) {
                  _out_queue_smart_credits.insert(make_pair(input, Credit::New()));
                }
                _out_queue_smart_credits.find(input)->second->id = f->id;
                _out_queue_smart_credits.find(input)->second->vc.insert(f->vc);

#if defined(PIPELINE_DEBUG)
                if (f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Bypass | Flit " << f->id
                                << " | Input " <<  input << " | vc " << f->vc << " | PID " << f-> pid << std::endl;
                }
#endif 
                
                // NEBB-VCT: Realoaction to an IDLE VC if the packet takes the bypass
                //int idle_vc = IdleLocalBuf(input, gBeginVCs[f->cl], gEndVCs[f->cl], f);
                //assert(idle_vc > -1);
                //f->vc = idle_vc;
                int in_vc = f->vc;

                f->vc = _dest_vc[input];
                //VCManagement(input, f->vc, output, f);
                VCManagement(input, in_vc, output, f);
                //FIXME: esto es una prueba intentando impitar a bluespec
                _bypass_credit[output] = true;

                TransferFlit(input, output, f);
                // Decrement credit count and prepare backwards credit
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch) {
                    //*gWatchOut << "Line: " << __LINE__ << " SMARTRouter::TransferFlit | Cycle: " << GetSimTime()
                    //                    << " Router: " << _id << " input: " << input
                    //                    << " Flit: " << f->id << " pid: " << f->pid
                    //                    << " output: " << output
                    //                    << " dest_vc: " << _dest_vc[input]
                    //                    << std::endl;
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | ST+LT | Flit " << f->id
                                << " | Input " <<  input << " | Output " << output << " | B | PID " << f-> pid << std::endl;
                }
#endif 
                
                _bypass_path[input] = -1;
            }
        } else {
            //BufferWrite(input, f); 
            //if (!f->head) {
            //	for (int vc = 0; vc < _vcs; vc++ ) {
            //		if (_buf[input]->GetActivePID(vc) == f->pid) {
            //			f->vc = vc;
            //		}
            //	}
            //}	
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch) {
                    //*gWatchOut << "Line: " << __LINE__ << " SMARTRouter::TransferFlit | Cycle: " << GetSimTime()
                    //                    << " Router: " << _id << " input: " << input
                    //                    << " Flit: " << f->id << " pid: " << f->pid
                    //                    << " output: " << output
                    //                    << " dest_vc: " << _dest_vc[input]
                    //                    << std::endl;
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | ReadFlit | Flit " << f->id
                                << " | Input " <<  input << " | Output " << -1 << " | L | PID " << f-> pid
                                << " Input VC: " << f->vc
                                << " FreeLocalBuf: " << FreeLocalBuf(input, f->vc, f->vc)
                                << std::endl;
                }
#endif 
            _flits_to_BW[input].push(make_pair(GetSimTime()+1,f));
            _prematurely_stop[input*_vcs+f->vc] = f->tail ? -1 : f->pid;
        }
    }

    void SMARTNEBBVCTRouter::TransferFlit(int input, int output, Flit * f) {
        //SMARTRouter* router = const_cast<SMARTRouter *>(GetNextRouter(output));
        SMARTNEBBVCTRouter* router = (SMARTNEBBVCTRouter*)GetNextRouter(output);
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
        }
        if (f->tail) {
            _output_port_blocked[output] = -1;
        }
    }

    bool SMARTNEBBVCTRouter::AddRequestSAG(SMARTRequest sr) {

        if (sr.distance >= _hpc_max) {
            return true;
        }
        
        if (_smart_dimensions == "oneD" && sr.distance > 0 && (sr.input_port/2 != sr.output_port/2 || sr.output_port >= _outputs - gC)) {
            return true;
        }
        
        _sag_requests.push(sr);

        return false;
    }

    void SMARTNEBBVCTRouter::FilterSAGRequests() {
        
        while ( !_sag_requests.empty()) {
            SMARTRequest sr = _sag_requests.front();
            _sag_requests.pop();

            // NEBB-VCT
            if (sr.f->head && _output_port_blocked[sr.output_port] > -1) {
                continue;
            }
            else if (!sr.f->head && _output_port_blocked[sr.output_port] != sr.f->pid) {
                continue;
            }

            assert(sr.f->head ? _output_port_blocked[sr.output_port] == -1 : _output_port_blocked[sr.output_port] == sr.f->pid );

            int priority;
            priority = _hpc_max - sr.distance;
            if (_smart_priority == "local") {
                priority = sr.distance > 0 ? priority : _hpc_max; // This is not necessary, it's just for completition..
            } else if (_smart_priority == "bypass") {
                priority = sr.distance; // If it's a local flit the priority 0.
            } else{
                std::cerr << "smart_priority=" << _smart_priority << " not implemented. See: " << __FILE__ << ":" <<__LINE__ << std::endl;
                exit(-1);
            }

            int expanded_input = sr.distance == 0 ? sr.input_port * 2 : sr.input_port * 2 + 1;

            if (!_sag_requestors[expanded_input].f || 
                    (_sag_requestors[expanded_input].f && _sag_requestors[expanded_input].distance > sr.distance))
            {
                _sag_requestors[expanded_input] = sr;
            }
        }
    }

    void SMARTNEBBVCTRouter::SwitchAllocationLocal() {

#ifdef TRACK_FLOWS
        bool increase_allocations = false;
#endif

        // Arbitrate among every flit in the front of a VC
        // TODO: change this to hold the VC until the whole packet is sent.
        for (int input = 0; input < _inputs; input++) {
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
            
            Buffer const * const cur_buf = _buf[input];
            
            int dest_vc = -1;
            int output = -1;
            int priority = -1;
            
            int end_vc_iter = _sal_next_vc[input];
            //for (int vc = _sal_next_vc[input]; vc != end_vc_iter; vc = vc + 1 == _vcs ? 0 : vc + 1) {
            do{
                Flit * f = (Flit*)cur_buf->FrontFlit(vc);
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
                        //dest_vc = FreeDestVC(input, output, vc_start, vc_end, f, 0);
                        dest_vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
                        //if (_bypass_credit[output])
                        //{
                        //	dest_vc = 0; //FIXME: delete
                        //	_bypass_credit[output] = false;
                        //}
                        priority = 0;
                    } else { // If it's a body flit, read the VC info.
                        output = cur_buf->GetOutputPort(vc);
                        dest_vc = cur_buf->GetOutputVC(vc);
                        priority = 1; // Priority to body flits so the packet can progress contiguously
                    }

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch && output > -1) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAL 1 | Flit " << f->id
                                << " | Input " <<  input << " | Output " << output << " | PID " << f-> pid
                                << " vc: " << vc << " Active? " << (cur_buf->GetState(vc) == VC::active)
                                << " dest_vc: " << dest_vc
                                << " In use by: " << _next_buf[output]->UsedBy(0)
                                << " Output port blocked by: " << _output_port_blocked[output] 
                                << std::endl;
                }
#endif 

                    // If there is a free destination VC
                    //if (output > -1 && dest_vc > -1 && !_sal_o_winners[output]) {
                    if (dest_vc > -1 && output > -1) {
                        //bool req = _sal_o_winners[output] ? _next_buf[output]->AvailableFor(dest_vc) > 1 : _next_buf[output]->AvailableFor(dest_vc) > 0;
                        bool req = true;

                        // NEBB-VCT
                        //if (f->head && _output_port_blocked[output] > -1) {
                        //	req = false;
                        //} else if (!f->head && _output_port_blocked[output] != f->pid) {
                        //	req = false;
                        //}

                        if (req) {
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch && output > -1) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAL 2 | Flit " << f->id
                                << " | Input " <<  input << " | Output " << output << " | PID " << f-> pid
                                << " vc: " << vc << " Active? " << (cur_buf->GetState(vc) == VC::active)
                                << " dest_vc: " << dest_vc
                                << " In use by: " << _next_buf[output]->UsedBy(0)
                                << std::endl;
                }
#endif 
                        //if (output > -1) {
                            _sw_allocator_local->AddRequest(input, output, vc, priority, priority);

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

            // TODO: Should I create an additional method for the following 5 lines?
            int input = _sw_allocator_local->InputAssigned(output);
            

            // If the input pipeline register hasn't been dequeue go to the next SA-L winner
            //if (input == -1 || _sal_to_sag[input] != NULL) {
            if (input == -1) {
                continue;
            }
            int vc = _sw_allocator_local->ReadRequest(input, output);
            Buffer * cur_buf = _buf[input];
            assert(!cur_buf->Empty(vc));
            Flit * f = cur_buf->FrontFlit(vc);
                
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            //if (f->watch) {
            //        *gWatchOut << "Line: " << __LINE__ << " SMARTRouter::SwitchAllocationLocal | Cycle: " << GetSimTime()
            //                            << " Router: " << _id << " input: " << input
            //                            << " Local Flit: " << f->id << " pid: " << f->pid
            //                            << " VC: " << vc << " preparing SAG requests for output: " << output
            //                            << std::endl;
            //}
            if (f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | SA-L | Flit " << f->id
                            << " | Input " <<  input << " | Output " << output << " | PID " << f-> pid << std::endl;
            }
#endif 
        
            // FIXME: For the moment this only works for single route routing algorithms.
            vector<SMARTRequest> smart_requests = GetFlitRoute(f, input, output);

#ifdef TRACK_FLOWS
            increase_allocations = true;
#endif

            // Dequeue flit from input buffer and store it in the pipeline register
            cur_buf->RemoveFlit(vc); 
            _sal_to_sag[input] = f;
            //_sal_o_winners[output] = true;
        
            // FIXME: delay should be 0 (or 1, I don't know for sure) to make this work
            //_input_credits[input]->Send(c);
            if(_out_queue_credits.count(input) == 0) {
              _out_queue_credits.insert(make_pair(input, Credit::New()));
            }
            _out_queue_credits.find(input)->second->id = f->id;
            _out_queue_credits.find(input)->second->vc.insert(vc);

#if defined(PIPELINE_DEBUG)
                if (f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Local | Flit " << f->id
                                << " | Input " <<  input << " | vc " << vc << " | PID " << f-> pid << std::endl;
                }
#endif 

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

    void SMARTNEBBVCTRouter::SwitchAllocationGlobal() {

        // NEBB-VCT
        FilterSAGRequests();

        // Reset Bypass flags from previous cycle
        _bypass_path.clear();
        _bypass_path.resize(_inputs, -1);

        // Perform SA-G allocation
        // XXX: Request are placed by AddRequestSAG in SA-L
        //_sw_allocator_global->Allocate();
#ifdef TRACK_FLOWS
        bool increase_allocations = false;
#endif

        for (int expanded_input = 0; expanded_input < _inputs*2; expanded_input++) {

            SMARTRouter::SMARTRequest sr = _sag_requestors[expanded_input];
            if (!sr.f) {
                continue;
            }
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (sr.f && sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAG | Flit " << sr.f->id
                            << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                            << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                            << std::endl;
            }
#endif 
            //if (sr.f &&
            //int expanded_input = sr.distance == 0 ? sr.input_port : sr.input_port + _inputs;
            if(sr.output_port < _outputs && FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1) {

                //if (sr.distance > 0 && sr.f->head && IdleLocalBuf(sr.input_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f) == -1){ // XXX: This condition is already done in FreeDestVC (at least in nebb_wh), isn't it?
                //	continue;
                //}

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (sr.f && sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAG 1 | Flit " << sr.f->id
                            << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                            << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                            << std::endl;
            }
#endif 

            // SMART NEBB-VCT
            if (sr.distance == 0 && sr.f->head && _buf[sr.input_port]->GetState(sr.f->vc) == VC::active) {
                continue;
            }

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


                int priority = sr.distance == 0 ? 2 : 0;
                priority = !sr.f->head ? priority*2 : priority;
                _sag_arbiters[sr.output_port]->AddRequest(expanded_input, sr.f->id, priority);
            }
        }

        
        // Setup crossbar, bypass path, bw selector, send flits, send credits...
        for (int output = 0; output < _outputs; output++) {
            //_sal_o_winners[output] = false;
            int expanded_input = _sag_arbiters[output]->Arbitrate();
            _sag_arbiters[output]->Clear();
            //int expanded_input = _sw_allocator_global->InputAssigned(output);  
            if (expanded_input == -1) {
                // There isn't a request for "output"
                continue;
            }
            
            // Destination VC.
            // FIXME: I think this is not necessary anymore
           // int flit_id = _sw_allocator_global->ReadRequest(expanded_input, output);

            SMARTRouter::SMARTRequest sr = _sag_requestors[expanded_input];
            //_sag_requestors[expanded_input] = {NULL, NULL, -1, -1, -1, -1, -1, -1, -1, -1};

            int input = expanded_input / 2;

            assert(sr.f);

            //assert(sr.f->id == flit_id);
            
            // FIXME: I have to store the class of packet in another structure to know the range of VCs the winner can use.
            // Solved?
            int vc;
            if (sr.f->head) {
                vc = FreeDestVC(input, output, sr.vc_start, sr.vc_end, sr.f, sr.distance);
            
            } else {
                vc = FreeDestVC(input, output, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance);
            }
            
            assert(vc > -1);
            //if (vc == -1) {
            //    _bypass_path[input] = -1;
            //    continue;
            //}

            assert(sr.f);

#ifdef TRACK_FLOWS
            increase_allocations = true;
#endif


            //_bypass_path[input] = -1;

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            //if (sr.f->watch) {
            //    *gWatchOut << "Line: " << __LINE__ << " SMARTRouter::SA-G | Cycle: " << GetSimTime()
            //                        << " Router: " << _id << " input: " << input
            //                        << " Flit: " << sr.f->id << " Won SA-G . PID: " << sr.f->pid
            //                        << " output: " << output
            //                        << " Dest VC: " << vc
            //                        << " Distance: " << sr.distance
            //                        << std::endl;
            //}
            if (sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | SA-G | Flit " << sr.f->id
                            << " | Input " <<  input << " | Output " << output << " | PID " << sr.f-> pid << std::endl;
            }
#endif 
            
            if (sr.distance > 0) {
                _dest_vc[input] = vc;
                _dest_output[input] = output;
                _bypass_path[input] = sr.f->id;
            }
            else{
                //Buffer * const cur_buf = _buf[input];
                //int cur_vc = sr.f->vc;
                //Flit * f = cur_buf->FrontFlit(cur_vc);
                Flit * f = _sal_to_sag[input];
                assert (f->id == sr.f->id);
                _sal_to_sag[input] = NULL;
                
                //if (f->watch) {
                // FIXME: I think the following line is wrong
                //f->vc = vc;
                assert(_crossbar_flits[input].second == NULL);
                
                _crossbar_flits[input] = make_pair(output, f);

                //cur_buf->RemoveFlit(cur_vc); 
            
                //if (f->head) {
                //    _sal_next_vc[input] = cur_vc;
                //}
                //if (f->tail) {
                //    _sal_next_vc[input] = cur_vc+1;
                //    if (_sal_next_vc[input] >= _vcs) {
                //        _sal_next_vc[input] = 0;
                //    }
                //}

                int in_vc = f->vc;
                f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
                VCManagement(input, in_vc, output, f);

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

    int SMARTNEBBVCTRouter::IdleLocalBuf(int input, int vc_start, int vc_end, Flit * f) {
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

    //const SMARTNEBBVCTRouter * SMARTNEBBVCTRouter::GetNextRouter(int next_output_port) const
    //{
    //    const FlitChannel * channel = _output_channels[next_output_port];
    //    const Router * router = channel->GetSink();
    //        
    //    return dynamic_cast<const SMARTNEBBVCTRouter *>(router);
    //    //return router;
    //}
} // namespace Booksim
