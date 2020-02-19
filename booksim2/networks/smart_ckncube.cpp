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

/*kn.cpp
 *
 *Meshs, cube, torus
 *
 */

#include "../booksim.hpp"
#include <cassert>
#include <cmath>
#include <sstream>
#include <vector>
#include <list>

#include "smart_ckncube.hpp"
#include "ckncube.hpp"
#include "../random_utils.hpp"
#include "../misc_utils.hpp"
 //#include "iq_router.hpp"
#include "../routers/bypass_router/smart_router.hpp"

namespace Booksim
{

    SMARTCKNCube::SMARTCKNCube(const Configuration &config, const string & name, bool mesh) :
    Network(config, name)
    {
      _mesh = mesh;

      _ComputeSize(config);
      _Alloc();
      _BuildNet(config);
      string routerType = config.GetStr("router");
      assert(routerType == "smart");
    }

    //void SMARTCKNCube::ReadInputs()
    //{
    //    // Pre-evaluation: SSR signal presetup
    //    for(size_t i = 0; i < _routers.size(); i++){
    //
    //        //TODO: check if the TimedModule is an SMARTRouter.
    //        SMARTRouter * router = dynamic_cast<SMARTRouter *> (_routers[i]);
    //        if(router){
    //           router->PresetupSSR();
    //        }
    //    }
    //
    //    // Arbitration between LAs in the same input (nearest LA has the highest priority)
    //    for(size_t i = 0; i < _routers.size(); i++){
    //
    //        //TODO: check if the TimedModule is an SMARTRouter.
    //        SMARTRouter * router = dynamic_cast<SMARTRouter *> (_routers[i]);
    //        if(router)
    //           router->LookAheadInputArbitration();
    //    }
    //
    //    for(deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
    //            iter != _timed_modules.end();
    //            ++iter) {
    //        (*iter)->ReadInputs();
    //    }
    //}

    //void SMARTCKNCube::WriteOutputs()
    //{
    //    // Pre-evaluation: SSR signal presetup
    //    std::cout << "BREAKPOINT" << std::endl;
    //    for(size_t i = 0; i < _routers.size(); i++){
    //
    //        //TODO: check if the TimedModule is an SMARTRouter.
    //        SMARTRouter * router = dynamic_cast<SMARTRouter *> (_routers[i]);
    //        std::cout << "BREAKPOINT" << std::endl;
    //        if(router){
    //           router->PresetupSSR();
    //        }
    //    }
    //
    //    // Arbitration between LAs in the same input (nearest LA has the highest priority)
    //    for(size_t i = 0; i < _routers.size(); i++){
    //
    //        //TODO: check if the TimedModule is an SMARTRouter.
    //        SMARTRouter * router = dynamic_cast<SMARTRouter *> (_routers[i]);
    //        if(router)
    //           router->LookAheadInputArbitration();
    //    }
    //
    //    for(deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
    //            iter != _timed_modules.end();
    //            ++iter) {
    //        (*iter)->WriteOutputs();
    //    }
    //}



    void SMARTCKNCube::_ComputeSize(const Configuration &config)
    {
      _kVect = config.GetIntArray("k");
      _cVect = config.GetIntArray("c");    //concentration, may be different from k
      _n = config.GetInt("n");

    //  cout << "_kVect.size() " << _kVect.size() << " _cVect.size() " << _cVect.size() << " _n " << _n << endl;
      assert((_kVect.size() == _cVect.size()) && (_n == (int) _kVect.size()));


      _c = 1;
      _size = 1;
      for(int dim=0; dim < _n; dim++){
        _c *= _cVect[dim];
        _size *= _kVect[dim];
      }
      _nodes = _size * _c;
      _channels = 2*_n*_size;

      gKvector = _kVect; gN = _n; gC = _c; gCvector = _cVect;
    }

    void SMARTCKNCube::RegisterRoutingFunctions() {
      gRoutingFunctionMap["dor_smartcmesh"] = &dim_order_cknmesh;
      gRoutingFunctionMap["dim_order_smartcmesh"] = &dim_order_cknmesh;
      gRoutingFunctionMap["dor_smartctorus"] = &dim_order_ckntorus;
      gRoutingFunctionMap["dim_order_smartctorus"] = &dim_order_ckntorus;
    }

    void SMARTCKNCube::_BuildNet(const Configuration &config)
    {
      int left_router_id;
      int right_router_id;

      int right_input;
      int left_input;

      int right_output;
      int left_output;

      ostringstream router_name;

      //latency type, noc or conventional network
      bool use_noc_latency;
      use_noc_latency = (config.GetInt("use_noc_latency")==1);


      for (int router_id = 0; router_id < _size; ++router_id) {

        router_name << "router";

        vector<int> router_index = RouterIndex(router_id);
        
        for (int dim = 0; dim < _n; ++dim)
          router_name << "_" << router_index[dim];

        _routers[router_id] = Router::NewRouter(config, this, router_name.str(),
                                            router_id, 2*_n + _c, 2*_n + _c);
        _timed_modules.push_back(_routers[router_id]);

        router_name.str("");


        for (int dim = 0; dim < _n; ++dim){


          //find the neighbor
          left_router_id  = _LeftRouter(router_id, dim);
          right_router_id = _RightRouter(router_id, dim);

          //
          // Current (N)ode
          // (L)eft router_id
          // (R)ight router_id
          //
          //   L--->N<---R
          //   L<---N--->R
          //

          
          int latency = 1; 
          if(use_noc_latency){
              //If there are different concentrations per dimension we use the maximum
              //TODO: change the maximum per the corresponding latency. Not to much complicate knowing the concentration per dimension.
              //int mesh_latency = int(pow(_c-1,1.0/_n))+1;
              int mesh_latency = _cVect[dim];

              // torus channel is longer due to folding
              latency = _mesh ? mesh_latency : 2*mesh_latency ;
          }else{
              latency = 1; 
          }
          

          //get the input channel number
          right_input = _LeftChannel(right_router_id, dim);
          left_input  = _RightChannel(left_router_id, dim);

          //cout << "router_id: " << router_id << " dim: " << dim << " left_router " << left_router_id << " right_router " << right_router_id << endl;
          //add the input channel
          _routers[router_id]->AddInputChannel(_chan[right_input], _chan_cred[right_input], _chan_la[right_input]);
          _routers[router_id]->AddInputChannel(_chan[left_input], _chan_cred[left_input], _chan_la[left_input]);

          //set input channel latency
          //This condition should be removed it is most compact up, and this is redundant.
          //if(use_noc_latency){
            _chan[right_input]->SetLatency(latency);
            _chan[left_input]->SetLatency(latency);
            _chan_cred[right_input]->SetLatency(latency);
            _chan_cred[left_input]->SetLatency(latency);
            _chan_la[right_input]->SetLatency(latency);
            _chan_la[left_input]->SetLatency(latency);
          //} else {
          //  _chan[left_input]->SetLatency(1);
          //  _chan_cred[right_input]->SetLatency(1);
          //  _chan_cred[left_input]->SetLatency(1);
          //  _chan[right_input]->SetLatency(1);
          //  _chan_la[right_input]->SetLatency(1);
          //  _chan_la[left_input]->SetLatency(1);
          //}
          //get the output channel number
          right_output = _RightChannel(router_id, dim);
          left_output  = _LeftChannel(router_id, dim);

          //add the output channel
          _routers[router_id]->AddOutputChannel(_chan[right_output], _chan_cred[right_output], _chan_la[right_output]);
          _routers[router_id]->AddOutputChannel(_chan[left_output], _chan_cred[left_output], _chan_la[left_output]);

          //set output channel latency
          //if(use_noc_latency){
            _chan[right_output]->SetLatency(latency);
            _chan[left_output]->SetLatency(latency);
            _chan_cred[right_output]->SetLatency(latency);
            _chan_cred[left_output]->SetLatency(latency);
            _chan_la[right_output]->SetLatency(latency);
            _chan_la[left_output]->SetLatency(latency);
          //} else {
          //  _chan[right_output]->SetLatency(1);
          //  _chan[left_output]->SetLatency(1);
          //  _chan_cred[right_output]->SetLatency(1);
          //  _chan_cred[left_output]->SetLatency(1);
          //  _chan_la[right_output]->SetLatency(1);
          //  _chan_la[left_output]->SetLatency(1);
          //}
        }
      }

      for (int node = 0; node < _nodes; ++node) {
        int router_id = NodeInRouter(node);
        //cout << "node: " << node << " inject size " << _inject_la.size() << " router_id: " << router_id << endl;
        _routers[router_id]->AddInputChannel(_inject[node], _inject_cred[node], _inject_la[node]);
        _routers[router_id]->AddOutputChannel(_eject[node], _eject_cred[node], _eject_la[node]);
        // TODO: Check if we are using or not bypass routers
        _inject[node]->SetLatency(1);
        _eject[node]->SetLatency(1);
      }
    }

    int SMARTCKNCube::_LeftChannel(int router, int dim)
    {
      // The base channel for a node is 2*_n*node
      int base = 2*_n*router;
      // The offset for a left channel is 2*dim + 1
      int off  = 2*dim + 1;
      
      return (base + off);
    }

    int SMARTCKNCube::_RightChannel(int router, int dim)
    {
      // The base channel for a node is 2*_n*node
      int base = 2*_n*router;
      // The offset for a right channel is 2*dim
      int off  = 2*dim;
      return (base + off);
    }

    int SMARTCKNCube::_LeftRouter(int router, int dim)
    {
      vector<int> router_index = RouterIndex(router);
      vector<int> left_router_index = router_index;
      // if at the left edge of the dimension, wraparound
      if(router_index[dim] == 0){
        left_router_index[dim] = gKvector[dim] - 1;
      //FIXME: IF k == 1 this is broken. Solve this here is easy but I should check if this also affects to the network build function.
      } else {
        --left_router_index[dim];
      }

      return IndexToRouter(left_router_index);
    }

    int SMARTCKNCube::_RightRouter(int router, int dim)
    {
      vector<int> router_index = RouterIndex(router);
      vector<int> right_router_index = router_index;
      // if at the left edge of the dimension, wraparound
      if(router_index[dim] == gKvector[dim]-1){
        right_router_index[dim] = 0;
      } else {
        ++right_router_index[dim];
      }

      return IndexToRouter(right_router_index);
    }

    int SMARTCKNCube::GetN() const
    {
      return _n;
    }

    int SMARTCKNCube::GetK() const
    {
      return _k;
    }

    /*legacy, not sure how this fits into the own scheme of things*/
    void SMARTCKNCube::InsertRandomFaults(const Configuration &config)
    {
      int num_fails = config.GetInt("link_failures");

      if (_size && num_fails) {
        vector<long> save_x;
        vector<double> save_u;
        SaveRandomState(save_x, save_u);
        int fail_seed;
        if (config.GetStr("fail_seed") == "time") {
          fail_seed = int(time(NULL));
          cout << "SEED: fail_seed=" << fail_seed << endl;
        } else {
          fail_seed = config.GetInt("fail_seed");
        }
        RandomSeed(fail_seed);

        vector<bool> fail_nodes(_size);

        for (int i = 0; i < _size; ++i) {
          int node = i;

          // edge test
          bool edge = false;
          for (int n = 0; n < _n; ++n) {
            if (((node % _k) == 0) ||
                 ((node % _k) == _k - 1)) {
              edge = true;
            }
            node /= _k;
          }

          if (edge) {
            fail_nodes[i] = true;
          } else {
            fail_nodes[i] = false;
          }
        }

        for (int i = 0; i < num_fails; ++i) {
          int j = RandomInt(_size - 1);
          bool available = false;
          int node=0, chan=0;
          int t;

          for (t = 0; (t < _size) && (!available); ++t) {
            node = (j + t) % _size;

            if (!fail_nodes[node]) {
              // check neighbors
              int c = RandomInt(2*_n - 1);

              for (int n = 0; (n < 2*_n) && (!available); ++n) {
                chan = (n + c) % 2*_n;

                if (chan % 1) {
                  available = fail_nodes[_LeftRouter(node, chan/2)];
                } else {
                  available = fail_nodes[_RightRouter(node, chan/2)];
                }
              }
            }

            if (!available) {
              cout << "skipping " << node << endl;
            }
          }

          if (t == _size) {
            Error("Could not find another possible fault channel");
          }


          OutChannelFault(node, chan);
          fail_nodes[node] = true;

          for (int n = 0; (n < _n) && available ; ++n) {
            fail_nodes[_LeftRouter(node, n)]  = true;
            fail_nodes[_RightRouter(node, n)] = true;
          }

          cout << "failure at node " << node << ", channel "
               << chan << endl;
        }

        RestoreRandomState(save_x, save_u);
      }
    }

    double SMARTCKNCube::Capacity() const
    {
      return (double)_k / (_mesh ? 8.0 : 4.0);
    }

    int SMARTCKNCube::NodeInRouter(int node)
    {
      vector<int> node_index = NodeIndex(node);
      vector<int> router_index;
      //cout << "Node " << node << " Node Index " << node_index[0] << "_" << node_index[1] << endl;
      for(int dim=0; dim < gN; dim++){
        router_index.push_back(node_index[dim] / gCvector[dim]);
      }
      return IndexToRouter(router_index);
    }

    vector<int> SMARTCKNCube::RouterIndex(int router)
    {
      int routerID = router;
      vector<int> index;
      for(int dim=0; dim < gN; dim++){
        index.push_back(routerID % gKvector[dim]);
        routerID /= gKvector[dim];
      }
      return index;
    }

    vector<int> SMARTCKNCube::NodeIndex(int node)
    {
      int nodeID = node;
      vector<int> index;
      for(int dim=0; dim < gN; dim++){
        index.push_back(nodeID % (gKvector[dim]*gCvector[dim]));
        nodeID /= gKvector[dim]*gCvector[dim];
      }
      return index;
    }

    int SMARTCKNCube::IndexToRouter(vector<int> router_index)
    {
      int router_id=router_index[0];
      for(int dim=1; dim < gN; dim++){
        router_id += powi(gKvector[dim-1],dim)*router_index[dim];
      }
      //cout << "Router index " << router_index[0] << "_" << router_index[1]<< endl;
      return router_id;
    }

    int SMARTCKNCube::ConcentrationOffset(int node)
    {
      vector<int> node_index = NodeIndex(node);
      int offset = 0;
      for(int dim=0; dim < gN; dim++){
        offset += powi(gCvector[dim],dim)*(node_index[dim] % gCvector[dim]);
      }
      return offset;
    }

    //int dor_next_cknmesh(int cur, int dest, bool descending)
    //{
    //
    //  int router_dest = SMARTCKNCube::NodeInRouter(dest);
    //  //cout << "Current router " << cur << " flit destination " << dest << " Router destination " << router_dest << " Node Offset " << SMARTCKNCube::ConcentrationOffset(dest) << endl;
    //  if (cur == router_dest) {
    //    return 2*gN+SMARTCKNCube::ConcentrationOffset(dest);  // Eject
    //  }
    //
    //  int dim_left=0;
    //  vector<int> router_cur_index = SMARTCKNCube::RouterIndex(cur);
    //  vector<int> router_dest_index = SMARTCKNCube::RouterIndex(router_dest);
    //  if(descending){
    //    for(int dim=gN-1;dim < gN; dim--){
    //      if(router_cur_index[dim] != router_dest_index[dim]){
    //        dim_left = dim;
    //        break;
    //      }
    //    }
    //  }else{
    //    for(int dim=0;dim < gN; dim++){
    //      if(router_cur_index[dim] != router_dest_index[dim]){
    //        dim_left = dim;
    //        break;
    //      }
    //    }
    //  }
    //
    //  //cout << "Dim_left " << dim_left << endl;
    //  if (router_cur_index[dim_left] < router_dest_index[dim_left]) {
    //    return 2*dim_left;     // Right
    //  } else {
    //    return 2*dim_left + 1; // Left
    //  }
    //}
    //
    ////void dor_next_ckntorus(int cur, int dest, int in_port,
    ////        int *out_port, int *partition,
    ////        bool balance = false)
    //int dor_next_ckntorus(int cur, int dest)
    //{
    //    //FIXME: dateline is not supported
    //    int dim_left = 0;
    //    int dir = 0;
    //    int dist2 = 0;
    //
    //    int router_dest = SMARTCKNCube::NodeInRouter(dest);
    //
    //    // Eject if this is destination router
    //    if(cur == router_dest) {
    //        return 2*gN+SMARTCKNCube::ConcentrationOffset(dest);
    //    }
    //
    //    vector<int> router_cur_index = SMARTCKNCube::RouterIndex(cur);
    //    vector<int> router_dest_index = SMARTCKNCube::RouterIndex(router_dest);
    //
    //    // TODO: add posibility of choosing dimmension order
    //    // if(descending){
    //    //for(int dim=gN-1;dim < gN; dim--){
    //    //    if(router_cur_index[dim] != router_dest_index[dim]){
    //    //        dim_left = dim;
    //    //        break;
    //    //    }
    //    //}
    //    //}else{
    //    for(int dim=0;dim < gN; dim++){
    //        if(router_cur_index[dim] != router_dest_index[dim]){
    //            dim_left = dim;
    //            break;
    //        }
    //    }
    //    //}
    //
    //    // calc distance (it depends in src and destination positions)
    //    int positive_distance = 0;
    //    int negative_distance = 0;
    //    if(router_cur_index[dim_left] < router_dest_index[dim_left]) {
    //        positive_distance = router_dest_index[dim_left] - router_cur_index[dim_left];
    //        negative_distance = gKvector[dim_left] - router_dest_index[dim_left] + router_cur_index[dim_left];
    //    } else {
    //        assert(router_cur_index[dim_left] > router_dest_index[dim_left]);
    //        positive_distance = gKvector[dim_left] - router_cur_index[dim_left] + router_dest_index[dim_left];
    //        negative_distance = router_cur_index[dim_left] - router_dest_index[dim_left];
    //    }
    //
    //    // take minimum distance direction
    //    if(positive_distance < negative_distance) {
    //        return 2*dim_left;
    //    } else if(negative_distance < positive_distance) {
    //        return 2*dim_left + 1;
    //    } else {
    //        // ramdom port
    //        return 2*dim_left + RandomInt(1);
    //    }
    //
    //
    //    // TODO: Port this code
    //    /*
    //    if (dim_left < gN) {
    //
    //        if ((in_port/2) != dim_left) {
    //            // Turning into a new dimension
    //
    //            cur %= gK; dest %= gK;
    //            dist2 = gK - 2 * ((dest - cur + gK) % gK);
    //
    //            if ((dist2 > 0) || 
    //                    ((dist2 == 0) && (RandomInt(1)))) {
    //                *out_port = 2*dim_left;     // Right
    //                dir = 0;
    //            } else {
    //                *out_port = 2*dim_left + 1; // Left
    //                dir = 1;
    //            }
    //
    //            if (partition) {
    //                if (balance) {
    //                    // Cray's "Partition" allocation
    //                    // Two datelines: one between k-1 and 0 which forces VC 1
    //                    //                another between ((k-1)/2) and ((k-1)/2 + 1) which 
    //                    //                forces VC 0 otherwise any VC can be used
    //
    //                    if (((dir == 0) && (cur > dest)) ||
    //                            ((dir == 1) && (cur < dest))) {
    //                        *partition = 1;
    //                    } else if (((dir == 0) && (cur <= (gK-1)/2) && (dest >  (gK-1)/2)) ||
    //                            ((dir == 1) && (cur >  (gK-1)/2) && (dest <= (gK-1)/2))) {
    //                        *partition = 0;
    //                    } else {
    //                        *partition = RandomInt(1); // use either VC set
    //                    }
    //                } else {
    //                    // Deterministic, fixed dateline between nodes k-1 and 0
    //
    //                    if (((dir == 0) && (cur > dest)) ||
    //                            ((dir == 1) && (dest < cur))) {
    //                        *partition = 1;
    //                    } else {
    //                        *partition = 0;
    //                    }
    //                }
    //            }
    //        } else {
    //            // Inverting the least significant bit keeps
    //            // the packet moving in the same direction
    //            *out_port = in_port ^ 0x1;
    //        }    
    //
    //    } else {
    //        *out_port = 2*gN;  // Eject
    //    }
    //    */
    //}
    //
    //void dim_order_cknmesh(const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject)
    //{
    //  int out_port = inject ? -1 : dor_next_cknmesh(r->GetID(), f->dest, false);
    //  //if (!inject) cout << "Flit ID: " << f->id << " | Current router " << r->GetID()  << " | Flit destination " <<  f->dest << " | Out port " << out_port << endl;
    //
    //  int vcBegin = gBeginVCs[f->cl];
    //  int vcEnd = gEndVCs[f->cl];
    //  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));
    //
    //  if (!inject && f->watch) {
    //    *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
    //               << "Adding VC range ["
    //               << vcBegin << ","
    //               << vcEnd << "]"
    //               << " at output port " << out_port
    //               << " for flit " << f->id
    //               << " (input port " << in_channel
    //               << ", destination " << f->dest << ")"
    //               << "." << endl;
    //  }
    //
    //  outputs->Clear();
    //
    //  outputs->AddRange(out_port, vcBegin, vcEnd);
    //}
    //
    //void dim_order_ckntorus(const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject)
    //{
    //  int out_port = inject ? -1 : dor_next_ckntorus(r->GetID(), f->dest);
    //  //if (!inject) cout << "Flit ID: " << f->id << " | Current router " << r->GetID()  << " | Flit destination " <<  f->dest << " | Out port " << out_port << endl;
    //
    //  int vcBegin = gBeginVCs[f->cl];
    //  int vcEnd = gEndVCs[f->cl];
    //  assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));
    //
    //  if (!inject && f->watch) {
    //    *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
    //               << "Adding VC range ["
    //               << vcBegin << ","
    //               << vcEnd << "]"
    //               << " at output port " << out_port
    //               << " for flit " << f->id
    //               << " (input port " << in_channel
    //               << ", destination " << f->dest << ")"
    //               << "." << endl;
    //  }
    //
    //  outputs->Clear();
    //
    //  outputs->AddRange(out_port, vcBegin, vcEnd);
    //}
} // namespace Booksim
