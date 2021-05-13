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

#include "smart_router.hpp"

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
#include "../../arbiters/arbiter.hpp"

namespace Booksim
{


    SMARTRouter::SMARTRouter(Configuration const & config, Module *parent, string
            const & name, int id, int inputs, int outputs) : Router( config, parent,
            name, id, inputs, outputs )
    {
        // General initializations
        _vcs = config.GetInt("num_vcs");
        _crossbar_flits.resize(_inputs);
        _bypass_path.resize(_inputs, -1);
        //FIXME: Replace this with _cur_buf state info when using multi-flit packets
        _dest_vc.resize(_inputs);
        _dest_output.resize(_inputs);
        _smart_dimensions = config.GetStr("smart_dimensions");

        _prematurely_stop.resize(_inputs*_vcs,-1);
        
        // Routing
        string const rf = config.GetStr("routing_function") + "_" + config.GetStr("topology");
        map<string, tRoutingFunction>::const_iterator rf_iter = gRoutingFunctionMap.find(rf);
        if (rf_iter == gRoutingFunctionMap.end()) {
            Error("Invalid routing function: " + rf);
        }
        _rf = rf_iter->second;

        // SMART initialization
        _hpc_max = config.GetInt("smart_max_hops");

        // Buffer initialization
        //_buf.resize(_inputs*2);
        _buf.resize(_inputs);
        //for (int i = 0; i < _inputs*2; ++i) {
        for (int i = 0; i < _inputs; ++i) {
            ostringstream module_name;
            module_name << "buf_" << i;
            _buf[i] = new Buffer(config, _outputs, this, module_name.str( ) );
            module_name.str("");
        }

        // Allocate next VCs' buffer state
        _next_buf.resize(_outputs);
        /*
        for (int j = 0; j < _outputs; ++j) {
            ostringstream module_name;
            module_name << "next_vc_o" << j;
            _next_buf[j] = new BufferState( config, this, module_name.str( ) );
            module_name.str("");
        }
        */
        for (int j = 0; j < _outputs - gC; ++j) {
            ostringstream module_name;
            module_name << "next_vc_o" << j;
            _next_buf[j] = new BufferState( config, this, module_name.str( ) );
            module_name.str("");
        }
        for (int j = _outputs - gC; j < _outputs; ++j) {
            ostringstream module_name;
            module_name << "next_vc_o" << j;
            _next_buf[j] = new BufferState( config, this, module_name.str( ), "destination" );
            module_name.str("");
        }

        // Allocators initilization FIXME: vc allocator not used for the moment
        string vc_alloc_type = config.GetStr("vc_allocator");
        _vc_allocator = Allocator::NewAllocator(this, "vc_allocator", vc_alloc_type, _vcs*_inputs,
                    _vcs*_outputs);
        
        // Switch Allocation Local: buffered flits
        string sw_alloc_type = config.GetStr( "sw_allocator" );
        _sw_allocator_local = Allocator::NewAllocator(this, "sw_allocator", sw_alloc_type, _inputs,
                    _outputs);

        _sal_next_vc.resize(_inputs,0);
        _sal_next_vc_counter.resize(_inputs,10); // 5 tries before move to next vc

        _sal_to_sag.resize(_inputs, NULL); // Pipeline registers to store winners of SA-L
        _sal_o_winners.resize(_inputs, false);
        _bypass_credit.resize(_inputs, false);

        // Switch Allocation Global: among SMART Requests
        //_sw_allocator_global = Allocator::NewAllocator(this, "sw_allocator",
        //        sw_alloc_type, _inputs*2, _outputs); // 2*inputs: SSRs and local flits
        _sag_arbiters.resize(_outputs);
        for(int output=0; output < _outputs; output++)
        {
            ostringstream module_name;
            module_name << "SA-G_" << output;
            _sag_arbiters[output] = Arbiter::NewArbiter(this, module_name.str() , "matrix", _inputs*2);
        }
        
        // Hold SMART Requests that were place in SA-G
        _sag_requestors.resize(_inputs*2);

        // Smart priority
        _smart_priority = config.GetStr("smart_priority");

        // Smart bypass destination
        _smart_dest_bypass = config.GetInt("smart_dest_bypass");

        // Flits to buffer write
        _flits_to_BW.resize(_inputs);

#ifdef TRACK_FLOWS
        for (int c = 0; c < _classes; ++c) {
            _stored_flits[c].resize(_inputs, 0);
            _active_packets[c].resize(_inputs, 0);
        }
        _sal_allocations = 0;
        _sag_allocations = 0;
#endif
      
        _credit_buffer.resize(_inputs); 
        _smart_credit_buffer.resize(_inputs); 

        //_destination_credit.resize(_outputs, -1);
        _destination_queue_credits.resize(_outputs);

        // @FIXME: this flag is intended to reduce the number of instructions
        // executed when the router doesn't have flits or SSRs to process.
        // However, it is not working correctly (at least in SMART++) so it is
        // dissabled (set to 1 instead of 0). This flag counts the flits in 
        // the router, hence 0 means that there are not flits.
        //_active = 0;
        _active = 1;
    }

    SMARTRouter::~SMARTRouter() {
        //std::cout << "~SMARTRouter not implemented" << std::endl;
        delete _vc_allocator;
        delete _sw_allocator_local;
        
        for(int output=0; output < _outputs; output++)
        {
            delete _sag_arbiters[output];
            delete _next_buf[output];
        }
        
        for (int i = 0; i < _inputs; ++i) {
            delete _buf[i];
        }
        
    }

    void SMARTRouter::ReadInputs() {
		
		//@TODO: Split the following loops in functions

        for (int input = 0; input < _inputs; ++input) { 

            // Read input channel
            Flit * const f = _input_channels[input]->Receive();
            
            // Proceed if there is a flit
            if (f) {
                 ++_active;
#ifdef TRACK_FLOWS
                 ++_received_flits[f->cl][input];
#endif

                // FIXME: Only injection channels should enter here.

                //Perform Buffer Write
                BufferWrite(input, f);
            }
            //_bypass_path[input] = false;
        }

        for (int input = 0; input < _inputs; input++) {
            if(!_flits_to_BW[input].empty()) {
                pair<long, Flit *> elem = _flits_to_BW[input].front();
                if (elem.second && elem.first == GetSimTime()) {
                    BufferWrite(input, elem.second);
                    _flits_to_BW[input].pop();
                }
            }
        }

        for (int output = 0; output < _outputs; output++) {
            if (!_destination_queue_credits[output].empty()) { 
                pair<long, Credit *> elem = _destination_queue_credits[output].front();
                if (elem.first >= GetSimTime()){
                    continue;
                }

                Credit * const c = elem.second;

                if (c) {
                     // FIXME: Esto es un fix muy guarro (mirar en VCManagement)
                     //if (output < _outputs - gC)
                     {
                         _next_buf[output]->ProcessCredit(c);
#if defined(PIPELINE_DEBUG)
                        *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Reception | Flit " << c->id
                                    << " | Output " <<  output << " Packet size: " << c->packet_size << std::endl;
#endif 
                     }
                     c->Free();
                }

                _destination_queue_credits[output].pop();
            }
        }

        for (int output = 0; output < _outputs; output++) {
            Credit * const c = _output_credits[output]->Receive();

            if (c) {
                 // FIXME: Esto es un fix muy guarro (mirar en VCManagement)
                 if (output < _outputs - gC)
                 {
                     _next_buf[output]->ProcessCredit(c);
#if defined(PIPELINE_DEBUG)
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Reception | Flit " << c->id
                                << " | Output " <<  output << " Packet size: " << c->packet_size <<  std::endl;
#endif 
                 }
                 c->Free();
            }

            // FIXME: Chapuza para emular a bluespec
            //if (_destination_credit[output] > -1)
            //{
            //    Credit * c = Credit::New();
            //    c->vc.insert(_destination_credit[output]);
            //    _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
            //    _destination_credit[output] = -1;
            //}

#ifdef PIPELINE_DEBUG
            *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit availability | Output " 
                        << output << " | " << _next_buf[output]->Print() << std::endl;
#endif

        }
        // ST first
        //if(_active > 0){
            SwitchTraversal();
        //}
    }

    void SMARTRouter::WriteOutputs() {
        //if(_active > 0) {
            SwitchAllocationLocal();
        //}
        _OutputQueuing();
        _SendCredits();
    }

    void SMARTRouter::_InternalStep() {
        
        // The order of the stages is very important (see Network class):
        // First SwitchTraversal to clean _crossbar_flits (see ReadFlits)
        // Second SA-G (this method: _InternalStep)
        // Third SA-L (see WriteOutputs)
        SwitchAllocationGlobal();
        // FIXME: This doesn't work (at least on SMART++: check _sag_requests)
        //if(_active > 0){
        //    SwitchAllocationGlobal();
        //}
    }
            
    void SMARTRouter::BufferWrite(int input, Flit * f)
    {
        // Init flit hpc
        f->hpc.push_back(0);
        // Set router id and port id
        f->router_id = _id;
        f->port_id = input;

        int in_vc = f->vc;

        //_prematurely_stop[input*_vcs+in_vc] = f->tail ? -1 : f->pid;
        
        
        Buffer * const cur_buf = _buf[input];

#ifdef TRACK_FLOWS
        ++_stored_flits[f->cl][input];
        if (f->head)
            ++_active_packets[f->cl][input];
#endif

        cur_buf->AddFlit(in_vc, f);

        // For SA-L when a body flit is buffered when the head (and others) take the bypass
        if (!f->head && cur_buf->FrontFlit(in_vc) == f) {
            _sal_next_vc[input] = in_vc;
        }

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
        if (f->watch) {
            int front_id = cur_buf->FrontFlit(in_vc) ? cur_buf->FrontFlit(in_vc)->id : -1;
            *gWatchOut  << GetSimTime() << " | " << FullName() << " | BW | Flit " << f->id
                        << " | Input " <<  input << " | in vc " << in_vc << " | PID " << f-> pid
                        << " | Front flit " << front_id
                        << " | VC Buffer occupancy " << cur_buf->GetOccupancy(in_vc)
                        << " | Total buffer occupancy " << cur_buf->GetOccupancy()
                        << std::endl;
            }
#endif 
    }

    void SMARTRouter::SwitchTraversal() {
        for (int input = 0; input < _inputs; input++) {
            pair<int, Flit *> iter = _crossbar_flits[input];
            int output = iter.first;
            Flit * f = iter.second;
         
            if (f) {
                TransferFlit(input, output, f);
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | ST+LT (Local) | Flit " << f->id
                                << " | Input " <<  input << " | Output " << output << " | L | PID " << f-> pid
                                << " VC: " << f->vc << std::endl;
                }
#endif 
            }
        }
        _crossbar_flits.clear();
        _crossbar_flits.resize(_inputs);
        --_active;
    }

    // This function performs ST+LT. Called by SwithTraversal
    //  and ReadFlit (Bypass)	
    void SMARTRouter::TransferFlit(int input, int output, Flit * f) {
        SMARTRouter * router = (SMARTRouter *)(GetNextRouter(output));
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
    }

    void SMARTRouter::ReadFlit(int input, Flit * f) {
#ifdef TRACK_FLOWS
             ++_received_flits[f->cl][input];
#endif
        ++_active;

        // Compute hop route
        OutputSet nos;
        _rf(this, f, input, &nos, false);
        f->la_route_set = nos;

        bool premat_stop = false;
        for (int vc = 0; vc < _vcs; vc ++) {
            if (_prematurely_stop[input*_vcs+vc] == f->pid) {
                premat_stop = true;
            }
        }
        bool multiflit_bypass = f->packet_size > 1 && f->head ? FreeLocalBuf(input, f->vc, f->vc) : true;
        if (_bypass_path[input] == f->id && !premat_stop && multiflit_bypass) {
            {
            
                int output = _dest_output[input];
                int in_vc = f->vc;
                f->vc = _dest_vc[input];
            
                if(_out_queue_smart_credits.count(input) == 0) {
                  _out_queue_smart_credits.insert(make_pair(input, Credit::New()));
                }
                _out_queue_smart_credits.find(input)->second->id = f->id;
                _out_queue_smart_credits.find(input)->second->vc.insert(in_vc);

#if defined(PIPELINE_DEBUG)
                if (f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Bypass | Flit " << f->id
                                << " | Input " <<  input << " | vc " << f->vc << " | PID " << f-> pid << std::endl;
                }
#endif 
                
                //VCManagement(input, f->vc, output, f);

                if (f->packet_size > 1) {
                    VCManagement(input, in_vc, output, f);
                } else {
                    assert(f->packet_size == 1);
                    
                    _next_buf[output]->TakeBuffer(f->vc, f->pid);
                    _next_buf[output]->SendingFlit(f);
                    //FIXME: chapuza para imitar a bluespec
                    if (output >= _outputs - gC)
                    {
                        //_destination_credit[output] = f->vc;
                        Credit * c = Credit::New();
                        c->id = f->id;
                        c->vc.insert(f->vc);
                        _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
                    }
                }

                //FIXME: esto es una prueba intentando impitar a bluespec
                //_bypass_credit[output] = true;

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


    void SMARTRouter::SwitchAllocationLocal() {

#ifdef TRACK_FLOWS
        bool increase_allocations = false;
#endif

        // Arbitrate among every flit in the front of a VC
        // FIXME: for each input port choose a random VC. It has to have a flit.
        // TODO: change this to hold the VC until the whole packet is sent.
        for (int input = 0; input < _inputs; input++) {
            //int vc = RandomInt(_vcs-1);
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
                Flit const * const f = cur_buf->FrontFlit(vc);
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

                        dest_vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], (Flit*) f, 0);

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
                                << std::endl;
                }
#endif 

                    // If there is a free destination VC
                    //if (output > -1 && dest_vc > -1 && !_sal_o_winners[output]) {
                    if (dest_vc > -1 && output > -1) {
                        //bool req = _sal_o_winners[output] ? _next_buf[output]->AvailableFor(dest_vc) > 1 : _next_buf[output]->AvailableFor(dest_vc) > 0;
                        bool req = true;

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

            Buffer * const cur_buf = _buf[input];
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

    void SMARTRouter::EvaluateFlitNextRouter()
    {
        //std::cout << "SMARTRouter::EvaluateFlitNextRouter() not implemented" << std::endl;
    }

    void SMARTRouter::SMARTSetupRequest()
    {
        //std::cout << "SMARTRouter::SMARTSetupRequest() not implemented" << std::endl;
    }

    void SMARTRouter::SwitchAllocationGlobal() {

        // Reset Bypass flags from previous cycle
        _bypass_path.clear();
        _bypass_path.resize(_inputs, -1);

        // Perform SA-G allocation
        // XXX: Request are placed by AddRequestSAG in SA-L

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

                if (sr.distance > 0 && sr.f->head && IdleLocalBuf(sr.input_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f) == -1){ // XXX: This condition is already done in FreeDestVC (at least in nebb_wh), isn't it?
                    continue;
                }

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (sr.f && sr.f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SAG 1 | Flit " << sr.f->id
                        << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                        << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                        << std::endl;
                }
#endif 

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

            int vc;
            if (sr.f->head) {
                vc = FreeDestVC(input, output, sr.vc_start, sr.vc_end, sr.f, sr.distance);

            } else {
                vc = FreeDestVC(input, output, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance);
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
                _bypass_path[input] = sr.f->id;
            }
            else{
                Flit * f = _sal_to_sag[input];
                assert (f->id == sr.f->id);
                _sal_to_sag[input] = NULL;

                assert(_crossbar_flits[input].second == NULL);
                _crossbar_flits[input] = make_pair(output, f);

                int in_vc = f->vc;
                f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
                VCManagement(input, in_vc, output, f);
            }
        }

        for (int expanded_input = 0; expanded_input < _inputs*2; expanded_input++) {
            _sag_requestors[expanded_input] = {NULL, NULL, -1, -1, -1, -1, -1, -1, -1, -1};
        }

#ifdef TRACK_FLOWS
        if (increase_allocations) {
            _sag_allocations++;
        }
#endif
    }

    // Returns true to exit the calling loop
    bool SMARTRouter::AddRequestSAG(SMARTRequest sr) {

        if (sr.distance >= _hpc_max) {
            //std::cout << "Exceeded HPC maximum distance" << std::endl;
            return true;
        }

        if (_smart_dimensions == "oneD" && sr.distance > 0 && (sr.input_port/2 != sr.output_port/2 || sr.output_port >= _outputs - gC)) {
            return true;
        }
        // else if (_smart_dimensions == "n" && sr.distance > 0) // includes destination optimization: do nothing

        int expanded_input = sr.distance == 0 ? sr.input_port * 2 : sr.input_port * 2 + 1;

        // Implementation of local priority
        if (!_sag_requestors[expanded_input].f || 
                (_sag_requestors[expanded_input].f && _sag_requestors[expanded_input].distance > sr.distance))
        {
            _sag_requestors[expanded_input] = sr;
        }

        return false;
    }



    int SMARTRouter::FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance) {
        // TODO: to support dateline, which limits the range of VCs available I call to the routing funtion.
        //       Therefore, vc_start and vc_end are ignored.
        OutputSet nos;
        _rf(this, f, input, &nos, false);
        set<OutputSet::sSetElement> const route = nos.GetSet();
        // FIXME: pick last route's output port. With adaptive algorithms this
        // doesn't work.
        int r_vc_start = -1;
        int r_vc_end   = -1;
        for (auto iter : route) {
            r_vc_start = iter.vc_start;
            r_vc_end   = iter.vc_end;
        }

        for (int vc=r_vc_start; vc <= r_vc_end; vc++) {
            if (f->head) {
                if (_next_buf[output]->IsAvailableFor (vc)) {
                    if (_next_buf[output]->AvailableFor (vc) == _next_buf[output]->LimitFor(vc)) {
                        return vc;
                    }
                }
            } else{
                if (_next_buf[output]->UsedBy(vc) == f->pid) {
                    if (_next_buf[output]->AvailableFor (vc)) {
                        return vc;
                    }
                }
            }
        }

        return -1;
    }

    bool SMARTRouter::FreeLocalBuf(int input, int vc_start, int vc_end) {
     for (int vc=vc_start; vc <= vc_end; vc++) {
        // FIXME: Apparently checking if the VC is idle is not necessary but, might it be?
        if (_buf[input]->Empty(vc) && _buf[input]->GetState(vc) == VC::idle && _flits_to_BW[input].empty()) {
            return true;
        }
     } 
     return false;
    }

    int SMARTRouter::IdleLocalBuf(int input, int vc_start, int vc_end, Flit * f) {
     for (int vc=vc_start; vc <= vc_end; vc++) {
        // FIXME: Apparently checking if the VC is idle is not necessary but, might it be?
        //if ( f->packet_size == 1 && f->head && _buf[input]->GetState(vc) == VC::idle) {
        if ( f->packet_size == 1) { // Single-flit packets
            return vc;
        } else if (!f->head && _buf[input]->GetState(vc) == VC::active && f->pid == _buf[input]->GetActivePID(vc)) {
            return vc;
        } else if (f->head && f->packet_size > 1 && FreeLocalBuf(input, vc, vc)) {
            return vc;
        }
     } 
     return -1;
    }

    // Returns [(output_port, (vc_min, vc_max)), (output_port),
    // (vc_min, vc_max), <infor hop2>, ... ]
    vector<SMARTRouter::SMARTRequest> SMARTRouter::GetFlitRoute(Flit * f,
                                            int input_port, int output_port) {
        // Clone flit to obtain whole route from this hop.  I use the Lookahead to
        // clone the flit.
        assert(f);

        Lookahead * la = new Lookahead(f);

        vector<SMARTRequest> route_path;

        //This hop also counts (distance 0)
        set<OutputSet::sSetElement> const local_route = f->la_route_set.GetSet();

        int vc_start = gBeginVCs[f->cl];
        int vc_end = gEndVCs[f->cl];
        assert(vc_start != -1 && vc_end != -1);
        SMARTRequest sr_initial = {this, f, input_port, output_port,
                                   input_port, f->vc, output_port, vc_start,
                                   vc_end, 0};
        route_path.push_back(sr_initial);

        // Iterate over every hop in the route.
        // FIXME: I'm supposing that in all the
        // topologies the nodes are connected to the last port
        int next_output_port = output_port;
        //const SMARTRouter * router = this;
        SMARTRouter * router = this;
        int distance = 0;
        int hops = 0;
        while (next_output_port < _outputs-gC) {
            la->la_route_set.Clear();
            // Load output channel and get next router
            // FIXME: here we have to request the output channel of the router in hop X.
            router = router->GetNextRouter(next_output_port);
            const FlitChannel * channel = router->GetOutputFlitChannel(next_output_port);

            // If the output terminal of the channel is a router:
            if (router) {
                int in_channel = channel->GetSinkPort();
                OutputSet nos;
                // Computes next hope route
                la->vc = gBeginVCs[f->cl];
                _rf(router, la, in_channel, &nos, false);
                la->la_route_set = nos;
                set<OutputSet::sSetElement> const route = nos.GetSet();

                // Iterate through all the posible routes (output ports and destinations VCs)
                distance++;
                for (auto iter : route) {
                    next_output_port = iter.output_port;
                }
                int next_vc_start = gBeginVCs[f->cl];
                int next_vc_end = gEndVCs[f->cl];
                assert(next_vc_start > -1 && next_vc_end > -1);

                SMARTRequest sr = {router, f, input_port, output_port, 
                                   in_channel, f->vc,
                                   next_output_port, next_vc_start,
                                   next_vc_end, distance, -1};
                route_path.push_back(sr);
                hops++;
            }
            // Add total hops to every sr:
            for (int i = 0; i < (int)route_path.size(); i++) {
                route_path[i].hops = hops;
            }
        }

        la->Free();

        // Return list of output ports that conform the route.
        return route_path;
    }

    // TODO: Is there a better solution for these two fuctions?.
    SMARTRouter * SMARTRouter::GetNextRouter(int next_output_port)
    {
        const FlitChannel * channel = _output_channels[next_output_port];
        const Router * router = channel->GetSink();
            
        return (SMARTRouter *)(router);
        //return router;
    }

    const FlitChannel * SMARTRouter::GetOutputFlitChannel(int next_output_port) const
    {
        const FlitChannel * channel = _output_channels[next_output_port];
        return channel;
    }

    void SMARTRouter::VCManagement(int input, int in_vc, int output, Flit * f) {

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
                                << " dest_vc: " << f->vc
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
        {
            _next_buf[output]->SendingFlit(f);
            if (output >= _outputs - gC)
            {
                //_destination_credit[output] = f->vc;
                Credit * c = Credit::New();
                c->id = f->id;
                c->vc.insert(f->vc);
                _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
            }
        }
    }

    void SMARTRouter::_OutputQueuing( )
    {
      for(map<int, Credit *>::const_iterator iter = _out_queue_credits.begin();
          iter != _out_queue_credits.end();
          ++iter) {

        int const input = iter->first;
        assert((input >= 0) && (input < _inputs));

        Credit * const c = iter->second;
        assert(c);
        assert(!c->vc.empty());

        _credit_buffer[input].push(c);
      }
      _out_queue_credits.clear();
      
      for(map<int, Credit *>::const_iterator iter = _out_queue_smart_credits.begin();
          iter != _out_queue_smart_credits.end();
          ++iter) {

        int const input = iter->first;
        assert((input >= 0) && (input < _inputs));

        Credit * const c = iter->second;
        assert(c);
        assert(!c->vc.empty());

        _smart_credit_buffer[input].push(c);
      }
      _out_queue_smart_credits.clear();
    }

    void SMARTRouter::_SendCredits( )
    {
      for ( int input = 0; input < _inputs; ++input ) {
        if ( !_credit_buffer[input].empty( ) ) {
          Credit * const c = _credit_buffer[input].front( );
          assert(c);
#if defined(PIPELINE_DEBUG)
          *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Send (Local) | Flit " << c->id
              << " | Input " <<  input << " Packet size: " << c->packet_size << std::endl;
#endif 
          _credit_buffer[input].pop( );
          _input_credits[input]->Send( c );
          _input_credits[input]->ReadInputs();
          _input_credits[input]->WriteOutputs();
        } else if (!_smart_credit_buffer[input].empty())
        {
          Credit * const c = _smart_credit_buffer[input].front( );
#if defined(PIPELINE_DEBUG)
          *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Send (SMART) | Flit " << c->id
              << " | Input " <<  input << " Packet size: " << c->packet_size << std::endl;
#endif 
          assert(c);
          _smart_credit_buffer[input].pop( );
          _input_credits[input]->Send( c );
          _input_credits[input]->ReadInputs();
          _input_credits[input]->WriteOutputs();
        }
      }
    }


    //-----------------------------------------------------------
    // misc.
    // ----------------------------------------------------------
    void SMARTRouter::Display( ostream & os ) const
    {
                for (int input = 0; input < _inputs; ++input) {
                    bool empty = true;
                    for (int vc = 0; vc < _vcs; ++vc) {
                        if (!_buf[input]->Empty(vc)) {
                            empty = false;
                        }
                    }
                    if (!empty) {
                        _buf[input]->Display( os );
                    }
                }
    }
} // namespace Booksim
