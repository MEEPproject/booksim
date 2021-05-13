// $Id$

/*
 Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
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

/*network.cpp
 *
 *This class is the basis of the entire network, it contains, all the routers
 *channels in the network, and is extended by all the network topologies
 *
 */

#include <cassert>
#include <sstream>

#include "booksim.hpp"
#include "network.hpp"

#include "kncube.hpp"
#include "ckncube.hpp"
#include "smart_ckncube.hpp"
#include "fly.hpp"
//#include "cmesh.hpp"
#include "ckmesh.hpp"
#include "flatfly_onchip.hpp"
#include "qtree.hpp"
#include "tree4.hpp"
#include "fattree.hpp"
#include "anynet.hpp"
#include "dragonfly.hpp"
#include "ideal.hpp"
#include "../NetworkInterface.h"
#include "../routers/bypass_router/smart_router.hpp"

namespace Booksim
{

    //TODO: Optimize the declaration of Lookahead channels only when they are requirer, i.e. when a bypass router is being used and remove injection and ejection channels if they are not going to be used.

    Network::Network(const Configuration &config, const string & name) :
        TimedModule(0, name)
    {
        _size     = -1; 
        _nodes    = -1; 
        _channels = -1;
        _classes  = config.GetInt("classes");
    }

    Network::~Network()
    {
        for (long unsigned r = 0; r < _routers.size(); ++r) {
            if (_routers[r]) delete _routers[r];
        }
        long unsigned s;
        for (s = 0; s < _inject.size(); ++s) {
            if (_inject[s]) delete _inject[s];
        }
        for (s = 0; s < _inject_cred.size(); ++s) {
            if (_inject_cred[s]) delete _inject_cred[s];
        }
        for (s = 0; s < _inject_la.size(); ++s) {
            if (_inject_la[s]) delete _inject_la[s];
        }
        long unsigned d;
        for (d = 0; d < _eject.size(); ++d) {
            if (_eject[d]) delete _eject[d];
        }
        for (d = 0; d < _eject_cred.size(); ++d) {
            if (_eject_cred[d]) delete _eject_cred[d];
        }
        
        for (d = 0; d < _eject_la.size(); ++d) {
            if (_eject_la[d]) delete _eject_la[d];
        }
        long unsigned c;
        for (c = 0; c < _chan.size(); ++c) {
            if (_chan[c]) delete _chan[c];
        }
        for (c = 0; c < _chan_cred.size(); ++c) {
            if (_chan_cred[c]) delete _chan_cred[c];
        }
        for (c = 0; c < _chan_la.size(); ++c) {
            if (_chan_la[c]) delete _chan_la[c];
        }
    }

    Network * Network::New(const Configuration & config, const string & name)
    {
        const string topo = config.GetStr("topology");
        Network * n = NULL;

        /*Init socket server to SynFull*/
        string sim_type = config.GetStr("sim_type");
        bool synfull = sim_type == "synfull" ? true : false;
        if (synfull) {
            ni = new NetworkInterface();
            cout << "Waiting for Traffic Generator client\n";
            ni->Init((char *) config.GetStr("synfull_socket").c_str());
        }

        if (topo == "torus") {
            KNCube::RegisterRoutingFunctions() ;
            n = new KNCube(config, name, false);
        } else if (topo == "mesh") {
            KNCube::RegisterRoutingFunctions() ;
            n = new KNCube(config, name, true);
        } else if (topo == "ctorus") {
            CKNCube::RegisterRoutingFunctions() ;
            n = new CKNCube(config, name, false);
        } else if (topo == "cmesh") {
            CKNCube::RegisterRoutingFunctions() ;
            n = new CKNCube(config, name, true);
        } else if (topo == "smartctorus") {
            SMARTCKNCube::RegisterRoutingFunctions() ;
            n = new CKNCube(config, name, false);
        } else if (topo == "smartcmesh") {
            SMARTCKNCube::RegisterRoutingFunctions() ;
            n = new CKNCube(config, name, true);
            // } else if (topo == "cmesh") {
            //   CMesh::RegisterRoutingFunctions() ;
            //   n = new CMesh(config, name);
        } else if (topo == "ckmesh") {
            CKMesh::RegisterRoutingFunctions() ;
            n = new CKMesh(config, name);
        } else if (topo == "fly") {
            KNFly::RegisterRoutingFunctions() ;
            n = new KNFly(config, name);
        } else if (topo == "qtree") {
            QTree::RegisterRoutingFunctions() ;
            n = new QTree(config, name);
        } else if (topo == "tree4") {
            Tree4::RegisterRoutingFunctions() ;
            n = new Tree4(config, name);
        } else if (topo == "fattree") {
            FatTree::RegisterRoutingFunctions() ;
            n = new FatTree(config, name);
        } else if (topo == "flatfly") {
            FlatFlyOnChip::RegisterRoutingFunctions() ;
            n = new FlatFlyOnChip(config, name);
        } else if (topo == "anynet"){
            AnyNet::RegisterRoutingFunctions() ;
            n = new AnyNet(config, name);
        } else if (topo == "dragonflynew"){
            DragonFlyNew::RegisterRoutingFunctions() ;
            n = new DragonFlyNew(config, name);
        } else if (topo == "ideal"){
            Ideal::RegisterRoutingFunctions() ;
            n = new Ideal(config, name);
        } else {
            cerr << "Unknown topology: " << topo << endl;
        }

        /*legacy code that insert random faults in the networks
         *not sure how to use this
         */
        if (n && (config.GetInt("link_failures") > 0)) {
            n->InsertRandomFaults(config);
        }

        return n;
    }

    void Network::_Alloc()
    {
        assert((_size != -1) && 
                (_nodes != -1) && 
                (_channels != -1));

        _routers.resize(_size);
        gNodes = _nodes;

        /*booksim used arrays of flits as the channels which makes have capacity of
         *one. To simulate channel latency, flitchannel class has been added
         *which are fifos with depth = channel latency and each cycle the channel
         *shifts by one
         *credit channels are the necessary counter part
         */
        _inject.resize(_nodes);
        _inject_cred.resize(_nodes);
        _inject_la.resize(_nodes);
        for (int s = 0; s < _nodes; ++s) {
            ostringstream name;
            name << Name() << "_fchan_ingress" << s;
            _inject[s] = new FlitChannel(this, name.str(), _classes);
            _inject[s]->SetSource(NULL, s);
            _timed_modules.push_back(_inject[s]);
            name.str("");
            name << Name() << "_cchan_ingress" << s;
            _inject_cred[s] = new CreditChannel(this, name.str());
            _timed_modules.push_back(_inject_cred[s]);
            // NOTE: We don't do injection lookahead, but maybe in the future...
            name.str("");
            name << Name() << "_lachan_ingress" << s;
            _inject_la[s] = new LookaheadChannel(this, name.str());
            _timed_modules.push_back(_inject_la[s]);
        }
        _eject.resize(_nodes);
        _eject_cred.resize(_nodes);
        _eject_la.resize(_nodes);
        for (int d = 0; d < _nodes; ++d) {
            ostringstream name;
            name << Name() << "_fchan_egress" << d;
            _eject[d] = new FlitChannel(this, name.str(), _classes);
            _eject[d]->SetSink(NULL, d);
            _timed_modules.push_back(_eject[d]);
            name.str("");
            name << Name() << "_cchan_egress" << d;
            _eject_cred[d] = new CreditChannel(this, name.str());
            _timed_modules.push_back(_eject_cred[d]);
            // NOTE: We don't need ejection lookahead, but maybe in the future...
            name.str("");
            name << Name() << "_lachan_egress" << d;
            _eject_la[d] = new LookaheadChannel(this, name.str());
            _timed_modules.push_back(_eject_la[d]);
        }
        _chan.resize(_channels);
        _chan_cred.resize(_channels);
        _chan_la.resize(_channels);
        for (int c = 0; c < _channels; ++c) {
            ostringstream name;
            name << Name() << "_fchan_" << c;
            _chan[c] = new FlitChannel(this, name.str(), _classes);
            _timed_modules.push_back(_chan[c]);
            name.str("");
            name << Name() << "_cchan_" << c;
            _chan_cred[c] = new CreditChannel(this, name.str());
            _timed_modules.push_back(_chan_cred[c]);
            name.str("");
            name << Name() << "_lachan_" << c;
            _chan_la[c] = new LookaheadChannel(this, name.str());
            _timed_modules.push_back(_chan_la[c]);
        }
    }

    void Network::ReadInputs()
    {
        for(deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
                iter != _timed_modules.end();
                ++iter) {
            (*iter)->ReadInputs();
        }
        
    }

    void Network::Evaluate()
    {
        for(deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
                iter != _timed_modules.end();
                ++iter) {
            (*iter)->Evaluate();
        }
    }

    void Network::WriteOutputs()
    {
        for(deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
                iter != _timed_modules.end();
                ++iter) {
            (*iter)->WriteOutputs();
        }
    }

    void Network::WriteFlit(Flit *f, int source)
    {
        assert((source >= 0) && (source < _nodes));
        _inject[source]->Send(f);
    }

    Flit *Network::ReadFlit(int dest)
    {
        assert((dest >= 0) && (dest < _nodes));
        return _eject[dest]->Receive();
    }

    void Network::WriteCredit(Credit *c, int dest)
    {
        assert((dest >= 0) && (dest < _nodes));
        _eject_cred[dest]->Send(c);
    }

    Credit *Network::ReadCredit(int source)
    {
        assert((source >= 0) && (source < _nodes));
        return _inject_cred[source]->Receive();
    }

    void Network::WriteLookahead(Lookahead *la, int source)
    {
        assert((source >= 0) && (source < _nodes));
        _inject_la[source]->Send(la);
    }

    // FIXME: I believe this method is not going to be used
    Lookahead *Network::ReadLookahead(int dest)
    {
        assert((dest >= 0) && (dest < _nodes));
        return _eject_la[dest]->Receive();
    }

    void Network::InsertRandomFaults(const Configuration &config)
    {
        Error("InsertRandomFaults not implemented for this topology!");
    }

    void Network::OutChannelFault(int r, int c, bool fault)
    {
        assert((r >= 0) && (r < _size));
        _routers[r]->OutChannelFault(c, fault);
    }

    double Network::Capacity() const
    {
        return 1.0;
    }

    /* this function can be heavily modified to display any information
     * neceesary of the network, by default, call display on each router
     * and display the channel utilization rate
     */
    void Network::Display(ostream & os) const
    {
        for (int r = 0; r < _size; ++r) {
            _routers[r]->Display(os);
        }
    }

    void Network::DumpChannelMap(ostream & os, string const & prefix) const
    {
        os << prefix << "source_router,source_port,dest_router,dest_port" << endl;
        for(int c = 0; c < _nodes; ++c)
            os << prefix
                << "-1," 
                << _inject[c]->GetSourcePort() << ',' 
                << _inject[c]->GetSink()->GetID() << ',' 
                << _inject[c]->GetSinkPort() << endl;
        for(int c = 0; c < _channels; ++c)
            os << prefix
                << _chan[c]->GetSource()->GetID() << ',' 
                << _chan[c]->GetSourcePort() << ',' 
                << _chan[c]->GetSink()->GetID() << ',' 
                << _chan[c]->GetSinkPort() << endl;
        for(int c = 0; c < _nodes; ++c)
            os << prefix
                << _eject[c]->GetSource()->GetID() << ',' 
                << _eject[c]->GetSourcePort() << ',' 
                << "-1," 
                << _eject[c]->GetSinkPort() << endl;
    }

    void Network::DumpNodeMap(ostream & os, string const & prefix) const
    {
        os << prefix << "source_router,dest_router" << endl;
        for(int s = 0; s < _nodes; ++s)
            os << prefix
                << _eject[s]->GetSource()->GetID() << ','
                << _inject[s]->GetSink()->GetID() << endl;
    }
} // namespace Booksim
