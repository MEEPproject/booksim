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

// TODO: Move this class outside smart_nebb directory
#include "smart_la_router.hpp"

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

    SMARTLARouter::SMARTLARouter(Configuration const & config, 
            Module *parent, string const & name, int id, int inputs,
            int outputs) : SMARTRouter( config, parent, name, id, inputs,
                outputs )
    {
        // NEBB-VCT
        _output_port_blocked.resize(_outputs, -1);

        _body_flit_output.resize(_inputs*_vcs, -1);
        _body_flit_output_local.resize(_inputs*_vcs, -1);

        _packet_stop.resize(_inputs, false);
        _bypass_blocked.resize(_inputs, false);
        _spec_bypass_path.resize(_inputs, -1);
        _spec_dest_vc.resize(_inputs, -1);
        _spec_dest_output.resize(_inputs, -1);
        _spec_bypass_blocked.resize(_inputs, false);

        //_destination_credit_ps.resize(_outputs);

        _spec_smart_credit_buffer.resize(_inputs); 
    }

    SMARTLARouter::~SMARTLARouter() {
    }

    //TODO: refactor to use SMARTRouter function
    void SMARTLARouter::ReadInputs() {

        for (int input = 0; input < _inputs; ++input) { 

            // Read input channel
            Flit * const f = _input_channels[input]->Receive();

            // Proceed if there is a flit
            if (f) {

#ifdef TRACK_FLOWS
             ++_received_flits[f->cl][input];
#endif

                //Perform Buffer Write
                BufferWrite(input, f);
            }
        }

        for (int input = 0; input < _inputs; input++) {
            if(!_flits_to_BW[input].empty()) {
                pair<long, Flit *> elem = _flits_to_BW[input].front();
                // In SuperSMART++ packets we must check if the bypass is
                // enabled for the flit in the input pipeline register
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
				if (elem.second && elem.second->watch) {
					*gWatchOut  << GetSimTime() << " | " << FullName() << " | Pre Spec-Bypass | Flit " << elem.second->id
								<< " | Input " << input << " _spec_bypass_path: " << _spec_bypass_path[input]
                                << " | Processing time " << elem.first
								<< std::endl;
				}
#endif 
                if (elem.second && elem.first == GetSimTime()) {
                    // Speculative bypass
                    Flit * f = elem.second;

                    _flits_to_BW[input].pop();

                    if (f->head) {
                        _packet_stop[input] = true;
                    }

                    if (f->tail) {
                        _packet_stop[input] = false;
                    }

                    bool premat_stop = false;
                    for (int vc = 0; vc < _vcs; vc ++) {
                        if (_prematurely_stop[input*_vcs+vc] == f->pid) {
                            premat_stop = true;
                        }
                    }

                    // NEBB-VCT with VCT credits
                    bool local_flit_between_sal_sag = false;
                    bool idle_vc_for_bypass = f->head ? _buf[input]->GetState(f->vc) == VC::idle : true;

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
					if (f->watch) {
						*gWatchOut  << GetSimTime() << " | " << FullName() << " | Pre Spec-Bypass | Flit " << f->id
									<< " | Input " << input << " _spec_bypass_path: " << _spec_bypass_path[input]
									<< " | !premat_stop " << !premat_stop << " idle_vc_for_bypass " << idle_vc_for_bypass
									<< " !local_flit_between_sal_sag " << !local_flit_between_sal_sag
									<< std::endl;
					}
#endif 

                    //if (_bypass_path[input] == f->pid && !premat_stop && idle_vc_for_bypass && !local_flit_between_sal_sag) {
                    if (_spec_bypass_path[input] == f->pid && !premat_stop && idle_vc_for_bypass && !local_flit_between_sal_sag) {

                        int output = _spec_dest_output[input];
                        int in_vc = f->vc;
                        f->vc = _spec_dest_vc[input];

                        // Credit generation
                        if (f->head) {
                            if(_out_queue_spec_smart_credits.count(input) == 0) {
                              _out_queue_spec_smart_credits.insert(make_pair(input, Credit::New()));
                            }
                            _out_queue_spec_smart_credits.find(input)->second->id = f->id;
                            _out_queue_spec_smart_credits.find(input)->second->vc.insert(in_vc);
                            _out_queue_spec_smart_credits.find(input)->second->packet_size = f->packet_size;


#if defined(PIPELINE_DEBUG)
                            if (f->watch) {
                                *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Spec-Bypass | Flit " << f->id
                                            << " | Input " <<  input << " | vc " << in_vc << " | PID " << f-> pid << std::endl;
                            }
#endif 
                        }

                        // Dest VC taking
                        if (f->head) {
                            _next_buf[output]->TakeBuffer(f->vc, f->pid);
                        }

                        _next_buf[output]->SendingFlit(f, true); // Activate VCT flag
                        if (f->head)
                        {
                            if (output >= _outputs - gC)
                            {
                                //assert(_destination_credit_ps[output].first == -1);
                                //_destination_credit_ps[output].push(make_pair(f->vc, f->packet_size));
                                Credit * c = Credit::New();
                                c->id = f->id;
                                c->packet_size = f->packet_size;
                                c->vc.insert(f->vc);
                                _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
                            }
                        }

                        //FIXME: esto es una prueba intentando impitar a bluespec
                        _bypass_credit[output] = true;

                        TransferFlit(input, output, f, in_vc);
                        // Decrement credit count and prepare backwards credit
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                        if (f->watch) {
                            *gWatchOut  << GetSimTime() << " | " << FullName() << " | ST+LT (Spec-Bypass) | Flit " << f->id
                                        << " | Input " <<  input << " | Output " << output << " | B | PID " << f-> pid
                                        << " | in VC " << in_vc << " | dest VC " << f->vc << std::endl;
                        }
#endif 
                        if (f->head) {
                            _spec_bypass_blocked[input] = true;
                        }

                        if (f->tail) {// Bypass blocked for the whole packet
                            _spec_bypass_path[input] = -1;
                            _spec_bypass_blocked[input] = false;
                        }	


                    } else {
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                        if (f->watch) {
                            int tmp_pid = -1;
                            Flit * tmp_f = _buf[input]->FrontFlit(f->vc);
                            if (tmp_f) tmp_pid = tmp_f->pid;
                            *gWatchOut  << GetSimTime() << " | " << FullName() << " | ReadFlit (ReadInputs - latched) | Flit " << f->id
                                        << " | Input " <<  input << " | Output " << -1 << " | L | PID " << f-> pid
                                        << " Input VC: " << f->vc
                                        << " FreeLocalBuf: " << FreeLocalBuf(input, f->vc, f->vc)
                                        << " _bypass_path: " << _bypass_path[input]
                                        << " !premat_stop: " << !premat_stop
                                        << " idle_vc_for_bypass: " << idle_vc_for_bypass
                                        << " In use by: " << tmp_pid
                                        << " !local_flit_between_sal_sag: " << !local_flit_between_sal_sag
                                        << " to BW"
                                        << std::endl;
                        }
#endif 
                        BufferWrite(input, elem.second);
                        _prematurely_stop[input*_vcs+f->vc] = f->tail ? -1 : f->pid;
                    }
                    // FIXME: uncomment this
                    //if (f->head) {
                    //    _packet_stop[input] = true;
                    //}

                    //if (f->tail) {
                    //    _packet_stop[input] = false;
                    //}
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
                         _next_buf[output]->ProcessCredit(c, true);
#if defined(PIPELINE_DEBUG)
                        *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Reception | Flit " << c->id
                                    << " | Output " <<  output << std::endl;
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
                     _next_buf[output]->ProcessCredit(c, true);
#if defined(PIPELINE_DEBUG)
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Reception | Flit " << c->id
                                << " | Output " <<  output << std::endl;
#endif 
                 }
                 c->Free();
            }

            // FIXME: Chapuza para emular a bluespec
            //if (_destination_credit_ps[output].first > -1)
            //if (!_destination_credit_ps[output].empty())
            //{
            //    pair<int, int> elem = _destination_credit_ps[output].front();
            //    _destination_credit_ps[output].pop();
            //    Credit * c = Credit::New();
            //    c->vc.insert(elem.first);
            //    c->packet_size = elem.second;
            //    _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
            //    //_destination_credit_ps[output] = make_pair(-1,-1);
            //}

#ifdef PIPELINE_DEBUG
            *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit availability | Output " 
                        << output << " | " << _next_buf[output]->Print() << std::endl;
#endif

        }
        SwitchTraversal();

        //Filter SSRs before SA-G to model propagation-based SSRs.
        FilterSSRequests();

    }

    // Spec-SMART credits
    void SMARTLARouter::_OutputQueuing( )
    {
        SMARTRouter::_OutputQueuing( );

        for(map<int, Credit *>::const_iterator iter = _out_queue_spec_smart_credits.begin();
                iter != _out_queue_spec_smart_credits.end();
                ++iter)
        {
            int const input = iter->first;
            assert((input >= 0) && (input < _inputs));

            Credit * const c = iter->second;
            assert(c);
            assert(!c->vc.empty());
            _spec_smart_credit_buffer[input].push(c);
        }
        _out_queue_spec_smart_credits.clear();
    }

    void SMARTLARouter::_SendCredits( )
    {
        for ( int input = 0; input < _inputs; ++input ) {
            if ( !_credit_buffer[input].empty( ) ) {
                Credit * const c = _credit_buffer[input].front( );
                assert(c);
                _credit_buffer[input].pop( );
                _input_credits[input]->Send( c );
                _input_credits[input]->ReadInputs();
                _input_credits[input]->WriteOutputs();
            } else if (!_smart_credit_buffer[input].empty())
            {
                Credit * const c = _smart_credit_buffer[input].front( );
                assert(c);
                _smart_credit_buffer[input].pop( );
                _input_credits[input]->Send( c );
                _input_credits[input]->ReadInputs();
                _input_credits[input]->WriteOutputs();

            } else if (!_spec_smart_credit_buffer[input].empty())
            {
                Credit * const c = _spec_smart_credit_buffer[input].front( );
                assert(c);
                _spec_smart_credit_buffer[input].pop( );
                _input_credits[input]->Send( c );
                _input_credits[input]->ReadInputs();
                _input_credits[input]->WriteOutputs();

            }
        }
    }


    int SMARTLARouter::FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance) {

        for (int vc=vc_start; vc <= vc_end; vc++) {
            if (f->head) {
                if (_next_buf[output]->IsAvailableFor (vc)) {
                    if (_next_buf[output]->AvailableFor (vc) == _next_buf[output]->LimitFor (vc)) {
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


    void SMARTLARouter::ReadFlit(int input, Flit * f) {
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

        // NEBB-VCT with VCT credits
        bool local_flit_between_sal_sag = false;
        bool idle_vc_for_bypass = f->head ? _buf[input]->GetState(f->vc) == VC::idle : true;

        bool dim_change = false;
        if(_smart_dimensions == "oneD"){
            if (_bypass_path[input] == f->pid) {
                int dim_output = _dest_output[input]/2;
                int dim_input = input/2;
                dim_change = dim_output != dim_input;
            }
        }

        bool body_flit_bypass_blocked = !f->head ? _bypass_blocked[input] : true;

        if (!dim_change && _bypass_path[input] == f->pid && !premat_stop && idle_vc_for_bypass && !local_flit_between_sal_sag && !_packet_stop[input] && body_flit_bypass_blocked) {
        //if (!dim_change && _bypass_path[input] == f->pid && !premat_stop && idle_vc_for_bypass && !local_flit_between_sal_sag && body_flit_bypass_blocked) {

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
                        *gWatchOut  << GetSimTime() << " | " << FullName() << " | Credit Bypass | Flit " << f->id
                            << " | Input " <<  input << " | vc " << in_vc << " | PID " << f-> pid
                            << std::endl;
                    }
#endif 
                }

                if (f->head) {
                    _next_buf[output]->TakeBuffer(f->vc, f->pid);
                }

                _next_buf[output]->SendingFlit(f, true); // Activate VCT flag
                if (f->head)
                {
                    if (output >= _outputs - gC)
                    {
                        //assert(_destination_credit_ps[output].first == -1);
                        //_destination_credit_ps[output].push(make_pair(f->vc, f->packet_size));
                        Credit * c = Credit::New();
                        c->id = f->id;
                        c->packet_size = f->packet_size;
                        c->vc.insert(f->vc);
                        _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
                    }
                }

                //FIXME: esto es una prueba intentando impitar a bluespec
                _bypass_credit[output] = true;

                TransferFlit(input, output, f, in_vc);
                // Decrement credit count and prepare backwards credit
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | ST+LT (Bypass) | Flit " << f->id
                                << " | Input " <<  input << " | Output " << output << " | B | PID " << f-> pid
                                << std::endl;
                }
#endif 

                if (f->head) {
                    _bypass_blocked[input] = true;
                }

                if (f->tail) {
                    _bypass_path[input] = -1;
                    _bypass_blocked[input] = false;
                }

        } else {
#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (f->watch) {
                    int tmp_pid = -1;
                    Flit * tmp_f = _buf[input]->FrontFlit(f->vc);
                    if (tmp_f) tmp_pid = tmp_f->pid;
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | ReadFlit | Flit " << f->id
                                << " | Input " <<  input << " | Output " << -1 << " | L | PID " << f-> pid
                                << " Input VC: " << f->vc
                                << " FreeLocalBuf: " << FreeLocalBuf(input, f->vc, f->vc)
                                << " _bypass_path: " << _bypass_path[input]
                                << " !premat_stop: " << !premat_stop
                                << " idle_vc_for_bypass: " << idle_vc_for_bypass
                                << " In use by: " << tmp_pid
                                << " !local_flit_between_sal_sag: " << !local_flit_between_sal_sag
                                << " !_packet_stop[input]: " << !_packet_stop[input]
                                << " !dim_change: " << !dim_change
                                << " to pipe register"
                                << std::endl;
                }
#endif 
            _flits_to_BW[input].push(make_pair(GetSimTime()+1,f));
            //_prematurely_stop[input*_vcs+f->vc] = f->tail ? -1 : f->pid;
            //_packet_stop[input] = true;
            // XXX: Esto es una prueba. Quizás hay que añadir una variable para bloquar el bypass si llega la cabeza de un paquete
        }
    }

    void SMARTLARouter::TransferFlit(int input, int output, Flit * f, int cur_vc) {

        SMARTLARouter* router = (SMARTLARouter*)GetNextRouter(output);
        int in_channel = GetOutputFlitChannel(output)->GetSinkPort();
        //int cur_vc = f->vc;

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
            //assert(_body_flit_output[input] == -1);
            _body_flit_output[input*_vcs+cur_vc] = output;
#ifdef PIPELINE_DEBUG
            *gWatchOut << GetSimTime() << " | " << FullName() << " | TransferFlit - Body flit output lock | Flit " << f->id << " PID: " << f->pid << " Extended Input: " << input*_vcs+cur_vc << " | Output " << output << std::endl;
#endif 
        }
        if (f->tail) {
            _output_port_blocked[output] = -1;
            _body_flit_output[input*_vcs+cur_vc] = -1;
#ifdef PIPELINE_DEBUG
            *gWatchOut << GetSimTime() << " | " << FullName() << " | TransferFlit - Body flit output free | Flit " << f->id << " PID: " << f->pid << " Extended Input: " << input*_vcs+cur_vc << " | Output " << output << std::endl;
#endif 
        }
    }

    bool SMARTLARouter::AddRequestSAG(SMARTRequest sr) {

        bool finish = false;
        if (sr.distance > _hpc_max) {
            return true;
        }
        else if (sr.distance == _hpc_max){
            sr.last_ssr = true;
            sr.dim_change = false;
            finish = true;
        }

        if (_smart_dimensions == "oneD") {
            if (sr.distance > 0 && (
                        sr.input_port/2 != sr.output_port/2 ||
                        sr.output_port >= _outputs - gC)
               ) {
                sr.last_ssr = true;
                sr.dim_change = true;
                finish = true;
            }
        }

#ifdef PIPELINE_DEBUG
        *gWatchOut << GetSimTime() << " | " << FullName() << " | SSR | Flit " << sr.f->id << " | Spec: " << sr.spec << " Distance: " << sr.distance << " sr.input_port " << sr.input_port << " sr.output_port " << sr.output_port << " last_ssr: " << sr.last_ssr << " sr.dim_change " << sr.dim_change << std::endl;
#endif 

        _ssr_requests.push_back(sr);
        return finish;
    }

    // TODO: in this case defining _ssr_requests as a Map is much simplier
    void SMARTLARouter::KillSAGRequests(int fid) {
        for (unsigned int i=0; i < _filtered_ssr_requests.size(); i++) {
            SMARTRequest sr = _filtered_ssr_requests[i];

            if (sr.f->id == fid) {
                _ssr_requests.erase(_filtered_ssr_requests.begin()+i);

#ifdef PIPELINE_DEBUG
                *gWatchOut << GetSimTime() << " | " << FullName() << " | SSR (Kill) | Flit " << sr.f->id << " | Spec: " << sr.spec << " Distance: " << sr.distance << " sr.input_port " << sr.input_port << " sr.output_port " << sr.output_port << " last_ssr: " << sr.last_ssr << " sr.dim_change " << sr.dim_change << std::endl;
#endif 

                // Kill SSR in the next router
                //Router
                SMARTLARouter * next_router = (SMARTLARouter *) GetNextRouter(sr.output_port);
                if (next_router) {
                    next_router->KillSAGRequests(fid);
                }
                break; // Shouldn't be more than one SSR per flit.
            }
        }
    }

    void SMARTLARouter::FilterSSRequests() {

        vector<queue<SMARTRequest>> last_SSRs; // Used to generate Spec-SSRs
        last_SSRs.resize(_outputs);
        queue<SMARTRequest> next_cycle_SSRs; // Used to reenque SSRs for next cycle
        vector<SMARTRequest> output_requests;
        output_requests.resize(_outputs);

        while ( !_ssr_requests.empty()) {
            SMARTRequest sr = _ssr_requests.back();
            _ssr_requests.pop_back();

            int input = sr.input_port;
            int output = sr.output_port;

#ifdef PIPELINE_DEBUG
            *gWatchOut << GetSimTime() << " | " << FullName() << " | SSR (Eval) | Flit " << sr.f->id << " | Spec: " << sr.spec << " Distance: " << sr.distance << " sr.input_port " << sr.input_port << " sr.output_port " << sr.output_port << " last_ssr: " << sr.last_ssr << " sr.dim_change " << sr.dim_change << " _output_port_blocked " << _output_port_blocked[output] << " _bypass_blocked " << _bypass_blocked[input] << " _spec_bypass_blocked[input] " << _spec_bypass_blocked[input] << std::endl;
#endif 

            if (sr.sag_cycle > GetSimTime()) {
                next_cycle_SSRs.push(sr);
                continue;
            }

            // NEBB-VCT
            if (_output_port_blocked[output] > -1) {
                KillSAGRequests(sr.f->id);
                continue;
            }

            if (_bypass_blocked[input]) {
                KillSAGRequests(sr.f->id);
                continue;
            }

            if (_spec_bypass_blocked[input]) {
                KillSAGRequests(sr.f->id);
                continue;
            }

            assert(_smart_priority == "local");

            if (sr.last_ssr) { // Prepare Spec SSR for next cycle
                last_SSRs[output].push(sr);
            } else {
                if (!output_requests[output].f) {
                    output_requests[output] = sr;
                } else {
                    SMARTRequest current_sr = output_requests[output];
                    if (current_sr.spec == sr.spec && current_sr.distance > sr.distance) {
                        output_requests[output] = sr;
                    } else if (!sr.spec && current_sr.spec) {
                        output_requests[output] = sr;
                    }
                }
            }
#ifdef PIPELINE_DEBUG
            *gWatchOut << GetSimTime() << " | " << FullName() << " | SSR (Eval1) | Flit " << sr.f->id << " | Spec: " << sr.spec << " Distance: " << sr.distance << " sr.input_port " << sr.input_port << " sr.output_port " << sr.output_port << " last_ssr: " << sr.last_ssr << " sr.dim_change " << sr.dim_change << std::endl;
#endif 
        }

        for (int output=0; output < _outputs; output++) {
            if (output_requests[output].f) {
                _filtered_ssr_requests.push_back(output_requests[output]);
#ifdef PIPELINE_DEBUG
            SMARTRequest sr = output_requests[output];
            *gWatchOut << GetSimTime() << " | " << FullName() << " | SSR (Eval2) | Flit " << sr.f->id << " | Spec: " << sr.spec << " Distance: " << sr.distance << " sr.input_port " << sr.input_port << " sr.output_port " << sr.output_port << " last_ssr: " << sr.last_ssr << " sr.dim_change " << sr.dim_change << std::endl;
#endif 
            }
        }

        // Re-enqueue next cycle (speculative) SSRs 
        while (!next_cycle_SSRs.empty()) {
            SMARTRequest sr = next_cycle_SSRs.front();
            next_cycle_SSRs.pop();
            assert(sr.sag_cycle == GetSimTime() + 1);

            _ssr_requests.push_back(sr);
        }

        // Speculative SSRs for the next cycle
        for (int output = 0; output < _outputs; output++) {
            SMARTRequest sr;
            bool write_sr = false;

            if (last_SSRs[output].size() == 1) {
                write_sr = true;
                sr = last_SSRs[output].front();
                last_SSRs[output].pop();
            }
            else if (last_SSRs[output].size() > 1) {
                while (!last_SSRs[output].empty()) {
                    SMARTRequest current_sr = last_SSRs[output].front();
                    last_SSRs[output].pop();
                    if (!write_sr) {
                        sr = current_sr;
                    } else {
                        if (!current_sr.spec && sr.spec) {
                            sr = current_sr;
                        } else if (current_sr.spec == sr.spec
                                   && current_sr.distance < sr.distance) {
                            sr = current_sr;
                        }
                    }
                    write_sr = true;

                }
            }
            if (write_sr) {
                int distance = sr.distance;

                vector<SMARTRequest> smart_requests = GetFlitRoute(sr.f,
                                                         sr.input_port,
                                                         sr.output_port);
                // Place SMART Request to Switch Allocation Global of router X
                for (auto request : smart_requests) {
                    request.spec = true;
                    request.sag_cycle = GetSimTime() + 1;
                    request.agregated_dist = request.distance + distance;
                    if (request.router->AddRequestSAG(request)) {
                        break; // Maximum HPC or dimension change. Don't send more SSRs.
                    }
                }
            }
        }
    }

    void SMARTLARouter::PrepareSAGRequests() {

        while ( !_filtered_ssr_requests.empty()) {
            SMARTRequest sr = _filtered_ssr_requests.back();
            _filtered_ssr_requests.pop_back();

#ifdef PIPELINE_DEBUG
            *gWatchOut << GetSimTime() << " | " << FullName() << " | SSR (Eval2) | Flit " << sr.f->id << " | Spec: " << sr.spec << " Distance: " << sr.distance << " sr.input_port " << sr.input_port << " sr.output_port " << sr.output_port << " last_ssr: " << sr.last_ssr << " sr.dim_change " << sr.dim_change << std::endl;
#endif 

            // Even: local flit; Odd: SSR
            int expanded_input = sr.distance == 0 ? sr.input_port * 2 : sr.input_port * 2 + 1;

            if (!_sag_requestors[expanded_input].f) {
                _sag_requestors[expanded_input] = sr;
            } else if (_sag_requestors[expanded_input].spec && !sr.spec) {
                _sag_requestors[expanded_input] = sr;
            } else if (_sag_requestors[expanded_input].spec == sr.spec && _sag_requestors[expanded_input].agregated_dist > sr.agregated_dist) {
                _sag_requestors[expanded_input] = sr;
            }

#ifdef PIPELINE_DEBUG
            *gWatchOut << GetSimTime() << " | " << FullName() << " | SSR (Eval3) | Flit " << sr.f->id << " | Spec: " << sr.spec << " Distance: " << sr.distance << " sr.input_port " << sr.input_port << " sr.output_port " << sr.output_port << " last_ssr: " << sr.last_ssr << " sr.dim_change " << sr.dim_change << " expanded_input " << expanded_input << " aggregated distance: " << sr.agregated_dist << std::endl;
#endif 
        }

    }

    void SMARTLARouter::SwitchAllocationLocal() {

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
                if (of->head) {
                    vector<SMARTRequest> smart_requests = GetFlitRoute(of, input, o_output);
                    // Place SMART Request to Switch Allocation Global of router X
                    for (auto request : smart_requests) {
                        if (request.router->AddRequestSAG(request)) {
                            break; // Maximum HPC or dimension change. Don't send more SSRs.
                        }
                    }
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
                                    << " dest_vc: " << dest_vc
                                    << " In use by: " << _next_buf[output]->UsedBy(0)
                                    << " Avail credits: " << _next_buf[output]->AvailableFor(0)
                                    << " Output port blocked by: " << _output_port_blocked[output] 
                                    << std::endl;
                    }
#endif 

                    // If there is a free destination VC
                    if (dest_vc > -1 && output > -1) {
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
                        _sw_allocator_local->AddRequest(input, output, vc, priority, priority);

                        // Finish the do-while loop
                        break;
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
        for (int output = 0; output < _outputs; output++) {

            int input = _sw_allocator_local->InputAssigned(output);

            if (input == -1) {
                continue;
            }
            int vc = _sw_allocator_local->ReadRequest(input, output);
            Buffer * cur_buf = _buf[input];
            assert(!cur_buf->Empty(vc));
            Flit * f = cur_buf->FrontFlit(vc);

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
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

            // FIXME: delay should be 0 (or 1, I don't know for sure) to make this work
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

            // Place SMART Request to Switch Allocation Global of router X
            if (f->head) {
                for (auto request : smart_requests) {
                    request.sag_cycle = GetSimTime() + 1;
                    if (request.router->AddRequestSAG(request)) {
                        break; // Maximum HPC or dimension change. Don't send more SSRs.
                    }
                }
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

    void SMARTLARouter::SwitchAllocationGlobal() {

        PrepareSAGRequests();

        // Reset Bypass flags from previous cycle
        for (int input = 0; input < _inputs; input++) {
            if (!_bypass_blocked[input]) {
                _bypass_path[input] = -1;
            }
            if (!_spec_bypass_blocked[input]) {
                _spec_bypass_path[input] = -1;
            }
        }

        // Perform SA-G allocation

#ifdef TRACK_FLOWS
        bool increase_allocations = false;
#endif
        for (int expanded_input = 0; expanded_input < _inputs*2; expanded_input++) {

            SMARTRouter::SMARTRequest sr = _sag_requestors[expanded_input];

            if (!sr.f) {
                continue;
            }

            if (sr.last_ssr) {
                continue;
            }
            
            // Duplicated from FilterSSRequests because ST+LT can be done after 
            // the filtering
            if (_output_port_blocked[sr.output_port] > -1) {
                continue;
            }

            assert(sr.f->head);

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (sr.f && sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SA-G 1 | Flit " << sr.f->id
                            << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                            << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                            << std::endl;
                if (_sag_requestors[2*(expanded_input/2)].f) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SA-G 1.5 | Flit " << sr.f->id
                            << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                            << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                            << " Local flit: " << _sag_requestors[2*(expanded_input/2)].f->id
                            << std::endl;

                }
            }
#endif 
            //if (sr.output_port < _outputs && FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1) {
            if (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1) {

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (sr.f && sr.f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SA-G 2 | Flit " << sr.f->id
                                << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                                << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                                << std::endl;
                }
#endif 

                // SMART NEBB-VCT
                if (sr.distance == 0 && IdleLocalBuf(sr.input_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f) == -1) {
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



                int priority = sr.distance == 0 ? 4 : 2;
                if (sr.spec) priority = 0; // Speculative SSRs
                priority = !sr.f->head ? priority*2 : priority;

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
                if (sr.f && sr.f->watch) {
                    *gWatchOut  << GetSimTime() << " | " << FullName() << " | Starting SA-G 3 | Flit " << sr.f->id
                                << " | Input " <<  sr.input_port << " | Output " << sr.output_port << " | PID " << sr.f->pid
                                << " Free VC: " << (FreeDestVC(sr.input_port, sr.output_port, gBeginVCs[sr.f->cl], gEndVCs[sr.f->cl], sr.f, sr.distance) > -1)
                                << " priority: " << priority
                                << std::endl;
                }
#endif 

                _sag_arbiters[sr.output_port]->AddRequest(expanded_input, sr.f->id, priority);
            }
        }


        // Setup crossbar, bypass path, bw selector, send flits, send credits...
        for (int output = 0; output < _outputs; output++) {
            //_sal_o_winners[output] = false;
            int expanded_input = _sag_arbiters[output]->Arbitrate();
            _sag_arbiters[output]->Clear();

            if (expanded_input == -1) {
                // There isn't a request for "output"
                continue;
            }

            SMARTRouter::SMARTRequest sr = _sag_requestors[expanded_input];

            int input = expanded_input / 2;

            assert(sr.f);

            int vc;
            // TODO: check if the else part is enough.
            vc = FreeDestVC(input, output, sr.vc_start, sr.vc_end, sr.f, sr.distance);

            assert(vc > -1);

            assert(sr.f);

#ifdef TRACK_FLOWS
            increase_allocations = true;
#endif

#if defined(FLIT_DEBUG) || defined(PIPELINE_DEBUG)
            if (sr.f->watch) {
                *gWatchOut  << GetSimTime() << " | " << FullName() << " | SA-G | Flit " << sr.f->id
                            << " | Input " <<  input << " | Output " << output << " | PID " << sr.f-> pid << " | Spec: " << sr.spec
                            << " | SR distance: " << sr.distance << std::endl;
            }
#endif 

            if (sr.distance > 0 || sr.spec) {
                // FIXME: I don't like this here.
                // TODO: The last ssr of the speculative SSRs is winning SA-G. It mustn't put the request, instead it has to generate another speculative SSR.
                if (sr.spec && (sr.distance == 0 || sr.last_ssr)) {
                    //_packet_stop[input] = true;
                    _spec_bypass_path[input] = sr.f->pid;
                    _spec_dest_vc[input] = vc;
                    _spec_dest_output[input] = output;
                }
                else{
                    _bypass_path[input] = sr.f->pid;
                    _dest_vc[input] = vc;
                    _dest_output[input] = output;
                }

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

                // FIXME: delay should be 0 (or 1, I don't know for sure) to make this work
                if (f->packet_size > 1) {
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
                if (!f->tail) {
                    _output_port_blocked[output] = f->pid;
                    _body_flit_output_local[input*_vcs+in_vc] = output;
#ifdef PIPELINE_DEBUG
                    *gWatchOut << GetSimTime() << " | " << FullName() << " | SA-G - Body flit output lock | Flit " << f->id << " PID: " << f->pid << " Extended Input: " << input*_vcs+in_vc << " | Output " << output << std::endl;
#endif 
                }
            }
        }

        for (int expanded_input = 0; expanded_input < _inputs*2; expanded_input++) {
            _sag_requestors[expanded_input] = {NULL, NULL, -1, -1, -1, -1, -1, -1, -1, -1};
        }

        // Body flits in sal_to_sag register advance (VCT)
        for (int input = 0; input < _inputs; input++) {
            //assert(_sal_to_sag[input] == NULL);
            if (_sal_to_sag[input] != NULL) {
                Flit * f = _sal_to_sag[input];

                if (f->head) { // Only body flits
                    continue;
                }

                int in_vc = f->vc;

                int output = _body_flit_output_local[input*_vcs+in_vc];
                f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
                VCManagement(input, in_vc, output, f);

                if (f->tail) {// tail flit, unblock SA
                    _output_port_blocked[output] = -1;		
                    _body_flit_output[input*_vcs+in_vc] = -1;
                }

                _crossbar_flits[input] = make_pair(output, f);

                _sal_to_sag[input] = NULL;
            }
        }

#ifdef TRACK_FLOWS
        if (increase_allocations) {
            _sag_allocations++;
        }
#endif
    }

    int SMARTLARouter::IdleLocalBuf(int input, int vc_start, int vc_end, Flit * f) {
     for (int vc=vc_start; vc <= vc_end; vc++) {

        bool flit_in_sag_reg = true;

        if (f->head && _buf[input]->GetState(vc) == VC::idle && flit_in_sag_reg) {
            return vc;
        } else if (!f->head && _buf[input]->GetState(vc) == VC::active && f->pid == _buf[input]->GetActivePID(vc)) {
            return vc;
        }
     } 
     return -1;
    }

    void SMARTLARouter::VCManagement(int input, int in_vc, int output, Flit * f) {

        if (f->head) {
            // FIXME: Read first route
            set<OutputSet::sSetElement> const route = f->la_route_set.GetSet();
            int output = route.begin()->output_port;

            f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
            // Set input VC output for body flits
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
            _next_buf[output]->TakeBuffer(f->vc, f->pid);
        }else{
            f->vc = FreeDestVC(input, output, gBeginVCs[f->cl], gEndVCs[f->cl], f, 0);
            if (f->tail) {
                _buf[input]->SetOutput(in_vc, -1, -1);
                _buf[input]->SetState(in_vc, VC::idle);
                _buf[input]->SetActivePID(in_vc, -1);
            }
        }

        // Decrement credit count
        // FIXME: Esto es un fix guarro para evitar la latencia extra que introduce el LT al mandar el flit al nodo de destino.
        _next_buf[output]->SendingFlit(f, true);
        if (f->head)
        {
            if (output >= _outputs - gC)
            {
                //assert(_destination_credit_ps[output].first == -1);
                //_destination_credit_ps[output].push(make_pair(f->vc, f->packet_size));
                Credit * c = Credit::New();
                c->id = f->id;
                c->packet_size = f->packet_size;
                c->vc.insert(f->vc);
                _destination_queue_credits[output].push(make_pair(GetSimTime()+1, c));
            }
        }
    }
} // namespace Booksim
