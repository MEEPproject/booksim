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

#include "booksim.hpp"
#include <vector>
#include <sstream>

#include "ideal.hpp"
#include "misc_utils.hpp" // XXX: is it necessary?
#include "globals.hpp"

namespace Booksim
{

    Ideal::Ideal(const Configuration &config, const string &name) : Network(config, name)
    {
        _ComputeSize(config);
        _Alloc();
        _BuildNet(config);
    }


    // Ideal::~Ideal()
    // {
    //     for (int s = 0; s < _nodes*_nodes; ++s) {
    //         if (_inject[s]) delete _inject[s];
    //         if (_eject[s]) delete _eject[s];
    //     }
    //     for (int d = 0; d < _nodes; ++d) {
    //         if (_inject_cred[d]) delete _inject_cred[d];
    //         if (_eject_cred[d]) delete _eject_cred[d];
    //     }
    // }

    void Ideal::_ComputeSize(const Configuration &config)
    {
        _k = config.GetInt("k");
        _n = config.GetInt("n");

        gK = _k;
        gN = _n;

        _nodes = powi(_k, _n);

        // n stages of k^(n-1) k x k switches
        _size = 0;

        // n-1 sets of wiring between the stages
        _channels = (_n - 1) * _nodes;
    }

    void Ideal::_BuildNet(const Configuration &config)
    {
        std::cout << "Building Ideal Network" << std::endl;
    }

    void Ideal::_Alloc()
    {
        assert((_size != -1) &&
               (_nodes != -1) &&
               (_channels != -1));

        gNodes = _nodes;

        // XXX: These are not used
        _inject.resize(_nodes * _nodes);
        _inject_cred.resize(_nodes);
        //_eject.resize(_nodes * _nodes);
        //_eject_cred.resize(_nodes);

        // XXX: Infinite output queues at the destination nodes to store flits in case of conflicts.
        _output_buffer.resize(_nodes * _nodes);
        _input_buffer_credits.resize(_nodes);

        ostringstream name;
        for (int s = 0; s < _nodes; ++s)
        {
            ostringstream name;
            name.str("");
            name << Name() << "_cchan_ingress_s_" << s;
            _inject_cred[s] = new CreditChannel(this, name.str());
            _inject_cred[s]->SetLatency(1);
            _timed_modules.push_back(_inject_cred[s]);

            for (int d = 0; d < _nodes; ++d)
            {
                int index = d * _nodes + s;
                name.str("");
                name << Name() << "_fchan_ingress_s_" << s << "_d_" << d;
                _inject[index] = new FlitChannel(this, name.str(), _classes);
                _inject[index]->SetSource(NULL, index);
                _inject[index]->SetLatency(1);
                _timed_modules.push_back(_inject[index]);
                // name.str("");
                // name << Name() << "_cchan_ingress_s_" << s << "_d_" << d;
                // _inject_cred[index] = new CreditChannel(this, name.str());
                // _inject_cred[index]->SetLatency(1);
                // _timed_modules.push_back(_inject_cred[index]);
            }
            

        }
    }

    void Ideal::WriteFlit(Flit *f, int source)
    {
        int index = f->dest * _nodes + source;
        assert((index >= 0) && (index < _nodes*_nodes));
        //_inject[index]->Send(f);
        _output_buffer[index].push(f);
        
        // Create credit directly
        Credit * const c = Credit::New();
        c->vc.insert(f->vc);
        c->id = f->id;
        c->packet_size = f->packet_size;
        c->head = f->head;
        c->tail = f->tail;
        _input_buffer_credits[source].push(c);
    }

    Flit *Ideal::ReadFlit(int dest)
    {
        assert((dest >= 0) && (dest < _nodes));
        Flit *f = NULL;
        // TODO: Should I implement some kind of arbitration (Round-Robin for example)?
        for (int i = 0; i < _nodes; ++i)
        {
            int index = dest * _nodes + i;
            //f = _inject[index]->Receive();
            //if (f) break;
            if (!_output_buffer[index].empty()) {
                f = _output_buffer[index].front();
                _output_buffer[index].pop();
                break;
            }
        }
        return f;
    }


    // void Ideal::WriteCredit(Credit *c, int dest)
    // {
    //     int index = dest;
    //     assert((index >= 0) && (index < _nodes));
    //     //_inject[index]->Send(f);
    //     _input_buffer_credits[index].push(c);
    // }

    // Credit *Ideal::ReadCredit(int source)
    // {
    //     assert((source >= 0) && (source < _nodes));
    //     int index = source;
    //     Credit *c = NULL;
    //     if (!_input_buffer_credits[index].empty())
    //     {
    //         c = _input_buffer_credits[index].front();
    //         _input_buffer_credits[index].pop();
    //     }
    //     return c;
    // }


    void Ideal::WriteCredit(Credit *c, int dest)
    {
        assert((dest >= 0) && (dest < _nodes));
        //_inject_cred[dest]->Send(c);
        c->Free(); // These credits are ignored
    }

    Credit *Ideal::ReadCredit(int source)
    {
        assert((source >= 0) && (source < _nodes));
        Credit * c = NULL;
        if (!_input_buffer_credits[source].empty()) {
            c = _input_buffer_credits[source].front();
            _input_buffer_credits[source].pop();
        }
        
        return c;
    }

    void Ideal::RegisterRoutingFunctions()
    {
        gRoutingFunctionMap["ideal_ideal"] = &routing_ideal;
        gRoutingFunctionMap["_ideal"] = &routing_ideal;
    }

    // This routing function is just to bypass errors in the traffic manager
    void routing_ideal( const Router *r, const Flit *f, int in_channel, 
              OutputSet *outputs, bool inject )
    { 
      int vcBegin = gBeginVCs[f->cl];
      int vcEnd = gEndVCs[f->cl];
      assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

      int out_port;

      if(inject) {

        out_port = -1;

      } else {

        out_port = 0;

      }

      outputs->Clear( );

      outputs->AddRange( out_port , vcBegin, vcEnd );
    }
} // namespace Booksim
