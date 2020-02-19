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

// ----------------------------------------------------------------------
//
// CKMesh: Network with <Int> Terminal Nodes arranged in a concentrated
//        mesh topology
//
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
//  $Author: jbalfour $
//  $Date: 2007/06/28 17:24:35 $
//  $Id$
//  Modified 11/6/2007 by Ted Jiang
//  Now handeling n = most power of 2: 16, 64, 256, 1024
// ----------------------------------------------------------------------
#include "../booksim.hpp"
#include <vector>
#include <sstream>
#include <cassert>
#include "../random_utils.hpp"
#include "../misc_utils.hpp"
#include "ckmesh.hpp"

namespace Booksim
{

    int CKMesh::_cX = 0 ;
    int CKMesh::_cY = 0 ;
    int CKMesh::_memo_NodeShiftX = 0 ;
    int CKMesh::_memo_NodeShiftY = 0 ;
    int CKMesh::_memo_PortShiftY = 0 ;

    CKMesh::CKMesh( const Configuration& config, const string & name ) 
      : Network(config, name) 
    {
      _ComputeSize( config );
      _Alloc();
      _BuildNet(config);
    }

    void CKMesh::RegisterRoutingFunctions() {
      gRoutingFunctionMap["dor_ckmesh"] = &dor_ckmesh;
    //  gRoutingFunctionMap["xy_yx_ckmesh"] = &xy_yx_ckmesh;
    }

    void CKMesh::_ComputeSize( const Configuration &config ) {

      int k = config.GetInt( "k" );
      int n = config.GetInt( "n" );
      assert(n <= 2); // broken for n > 2
      int c = config.GetInt( "c" );
      assert(c == 4 || c == 1); // broken for other cases

      ostringstream router_name;
      //how many routers in the x or y direction
      _xcount = config.GetInt("x");
      _ycount = config.GetInt("y");
      assert(_xcount == _ycount); // broken for asymmetric topologies
      //configuration of hohw many clients in X and Y per router
      _xrouter = config.GetInt("xr");
      _yrouter = config.GetInt("yr");
      assert(_xrouter == _yrouter); // broken for asymmetric concentration

      gK = _k = k ;
      gN = _n = n ;
      gC = _c = c ;

      assert(c == _xrouter*_yrouter);
      
      _nodes    = _c * powi( _k, _n); // Number of nodes in network
      _size     = powi( _k, _n);      // Number of routers in network
      _channels = 4 * _n * _size;     // Number of channels in network

      _cX = _c < _n ? 1 : _c / _n ;   // Concentration in X Dimension 
      _cY = _c / _cX ;  // Concentration in Y Dimension

      _memo_NodeShiftX = _cX >> 1 ;
      _memo_NodeShiftY = log_two(gK * _cX) + ( _cY >> 1 ) ;
      _memo_PortShiftY = log_two(gK * _cX)  ;

    }

    void CKMesh::_BuildNet( const Configuration& config ) {

      if(gTrace){
        cout<<"Setup Finished Router"<<endl;
      }

      //latency type, noc or conventional network
      bool use_noc_latency;
      use_noc_latency = (config.GetInt("use_noc_latency")==1);
      
      ostringstream name;
      // The following vector is used to check that every
      //  processor in the system is connected to the network
      vector<bool> channel_vector(_nodes, false) ;
      
      //
      // Routers and Channel
      //
      for (int node = 0; node < _size; ++node) {

        // Router index derived from mesh index
        const int y_index = node / _k ;
        const int x_index = node % _k ;

        const int degree_in  = 4 *_n + _c ;
        const int degree_out = 4 *_n + _c ;

        name.str("");
        name << "router_" << y_index << '_' << x_index;
        _routers[node] = Router::NewRouter( config, 
                        this, 
                        name.str(), 
                        node,
                        degree_in,
                        degree_out);
        _timed_modules.push_back(_routers[node]);

        //
        // Port Numbering: as best as I can determine, the order in
        //  which the input and output channels are added to the
        //  router determines the associated port number that must be
        //  used by the router. Output port number increases with 
        //  each new channel
        //

        //
        // Processing node channels
        //
        for (int y = 0; y < _cY ; y++) {
          for (int x = 0; x < _cX ; x++) {
        int link = (_k * _cX) * (_cY * y_index + y) + (_cX * x_index + x) ;
        assert( link >= 0 ) ;
        assert( link < _nodes ) ;
        assert( channel_vector[ link ] == false ) ;
        channel_vector[link] = true ;
        // Ingress Ports
        _routers[node]->AddInputChannel(_inject[link], _inject_cred[link]);
        // Egress Ports
        _routers[node]->AddOutputChannel(_eject[link], _eject_cred[link]);
        //injeciton ejection latency is 1
        _inject[link]->SetLatency( 1 );
        _eject[link]->SetLatency( 1 );
            if(gTrace) {
              cout<<"Inj: out port "<<link<<" in port "<<link<<" node "<<node<<" latency "<<_inject[link]->GetLatency()<<endl;
            }
          }
        }

        //
        // router to router channels
        //
        const int offset = powi( _k, _n ) ;
        const int jump[8][2] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {-1,-1}, {-1,1}, {1,-1} };
        
        //the channel number of the input output channels.
        for(int port=0; port < 8; port++) {
          const int dst_port = (port & (~1)) | (~port & 1); // Switch the last bit of the port
          int out = _k * y_index + x_index + port * offset ;
          int in  = _k * ((y_index+jump[port][1])) + ((x_index+jump[port][0])) + dst_port * offset ;
          int latency = jump[port][0] != 0 ? _cY : _cX;
          
          // Express Channels on boundary nodes
          if (x_index + jump[port][0] < 0 || x_index + jump[port][0] >= _k) {
            // Router on left or right edges of mesh. Connect the port that would
            // be unused to same port of another router on the same edge of the
            // mesh.
            in = _k * ((y_index + _k/2)%_k) + x_index + port * offset ;
            latency = _cY*_k/2;
            // continue; 
          }
          if (y_index + jump[port][1] < 0 || y_index + jump[port][1] >= _k) {
            // Router on left or right edges of mesh. Connect the port that would
            // be unused to same port of another router on the same edge of the
            // mesh.
            in = _k * y_index + ((x_index + _k/2)%_k) + port * offset ;
            latency = _cX*_k/2;
            // continue; 
          }
          if(!use_noc_latency) {
             latency = 1;
          }

          _chan[out]->SetLatency( latency );
          _chan_cred[out]->SetLatency( latency );
          _routers[node]->AddOutputChannel( _chan[out], _chan_cred[out] );
          _routers[node]->AddInputChannel( _chan[in], _chan_cred[in] );
          
          if(gTrace) {
            cout<<"Link "<<port<<": out port "<<out<<" in port "<<in<<" node "<<name.str()<<" latency "<<_chan[out]->GetLatency()<<endl;
          }
        }
      }    

      // Check that all processors were connected to the network
      for ( int i = 0 ; i < _nodes ; i++ ) 
        assert( channel_vector[i] == true ) ;
      
      if(gTrace){
        cout<<"Setup Finished Link"<<endl;
      }
    }


    // ----------------------------------------------------------------------
    //
    //  Routing Helper Functions
    //
    // ----------------------------------------------------------------------

    int CKMesh::NodeToRouter( int address ) {

      int y  = (address /  (_cX*gK))/_cY ;
      int x  = (address %  (_cX*gK))/_cY ;
      int router = y*gK + x ;
      
      return router ;
    }

    int CKMesh::NodeToPort( int address ) {
      
      const int maskX  = _cX - 1 ;
      const int maskY  = _cY - 1 ;

      int x = address & maskX ;
      int y = (int)(address/(2*gK)) & maskY ;

      return (gC / 2) * y + x;
    }

    // ----------------------------------------------------------------------
    //
    //  Routing Functions
    //
    // ----------------------------------------------------------------------

    void dor_ckmesh( const Router *r, const Flit *f, int in_channel, 
            OutputSet *outputs, bool inject )
    {
      // ( Traffic Class , Routing Order ) -> Virtual Channel Range
      int vcBegin = gBeginVCs[f->cl];
      int vcEnd = gEndVCs[f->cl];
      assert(((f->vc >= vcBegin) && (f->vc <= vcEnd)) || (inject && (f->vc < 0)));

      int out_port;

      if(inject) {

        out_port = -1;

      } else {

        // Current Router
        int cur_router = r->GetID();

        // Destination Router
        int dest_router = CKMesh::NodeToRouter( f->dest ) ;  
      
        if (dest_router == cur_router) {

          // Forward to processing element
          out_port = CKMesh::NodeToPort( f->dest ) ;

        } else {

          // Forward to neighbouring router
          out_port = ckmesh_next( cur_router, dest_router, f->watch);
          if ( f->watch ) { 
            *gWatchOut << GetSimTime() << " | "
    //                   << "node" << dest << " | "
                       << "Routing flit " << f->id 
                       << " (packet " << f->pid
                       << ", current = " << cur_router%gK << "," << cur_router/gK
                       << ", dest = " << dest_router%gK << "," << dest_router/gK
                       << ", port = " << out_port-gC
                       << ")." << endl;
          }
        }
      }

      outputs->Clear();

      outputs->AddRange( out_port, vcBegin, vcEnd);
    }

    //============================================================
    //
    //=====
    int ckmesh_next( int cur, int dest, int watch ) {
      
      const int cur_y = cur/gK ;
      const int cur_x = cur%gK ;
      const int rr_y = dest/gK - cur/gK ;
      const int rr_x = dest%gK - cur%gK ;
      const int b4 = cur_x < gK-1 && cur_y < gK-1;
      const int b5 = cur_x > 0    && cur_y > 0   ;
      const int b6 = cur_x > 0    && cur_y < gK-1;
      const int b7 = cur_x < gK-1 && cur_y > 0   ;

      if(rr_x * rr_y > 0) {// Same sign
         const int far = abs(rr_x - rr_y) >= 2;
         if(abs(rr_x) > abs(rr_y)) {
            if(watch) cout << "1" << endl;
            if( rr_x > rr_y ) return gC + 0;
            if( rr_x < rr_y ) return gC + 1;
            // if( 0 ) return gC + 2;
            // if( 0 ) return gC + 3;
            if( b4 && rr_y > 0 ) return gC + 4;
            if( b5 && rr_y < 0 ) return gC + 5;
            if( b6 && rr_x < rr_y && far ) return gC + 6;
            if( b7 && rr_x > rr_y && far ) return gC + 7;
         }
         else if(abs(rr_x) > abs(rr_y)) {
            if(watch) cout << "2" << endl;
            // if( 0 ) return gC + 0;
            // if( 0 ) return gC + 1;
            if( rr_y > rr_x ) return gC + 2;
            if( rr_y < rr_x ) return gC + 3;
            if( b4 && rr_x > 0 ) return gC + 4;
            if( b5 && rr_x < 0 ) return gC + 5;
            if( b6 && rr_x > rr_y && far ) return gC + 6;
            if( b7 && rr_x < rr_y && far ) return gC + 7;
         }
         else {
            if(watch) cout << "3" << endl;
            // if( 0 ) return gC + 0;
            // if( 0 ) return gC + 1;
            // if( 0 ) return gC + 2;
            // if( 0 ) return gC + 3;
            if( b4 && rr_x > 0 ) return gC + 4;
            if( b5 && rr_x < 0 ) return gC + 5;
            // if( b6 && 0 ) return gC + 6;
            // if( b7 && 0 ) return gC + 7;
         }
      }
      else if(rr_x * rr_y < 0) {// Different sign
         const int far = abs(rr_x + rr_y) >= 2;
         if(abs(rr_x) > abs(rr_y)) {
            if(watch) cout << "4" << endl;
            if( rr_x + rr_y > 0 ) return gC + 0;
            if( rr_x + rr_y < 0 ) return gC + 1;
            // if( 0 ) return gC + 2;
            // if( 0 ) return gC + 3;
            if( b4 && rr_x > rr_y && far ) return gC + 4;
            if( b5 && rr_x < rr_y && far ) return gC + 5;
            if( b6 && rr_y > 0 ) return gC + 6;
            if( b7 && rr_y < 0 ) return gC + 7;
         }
         else if(abs(rr_x) > abs(rr_y)) {
            if(watch) cout << "5" << endl;
            // if( 0 ) return gC + 0;
            // if( 0 ) return gC + 1;
            if( rr_x + rr_y > 0 ) return gC + 2;
            if( rr_x + rr_y < 0 ) return gC + 3;
            if( b4 && rr_x < rr_y && far ) return gC + 4;
            if( b5 && rr_x > rr_y && far ) return gC + 5;
            if( b6 && rr_x < 0 ) return gC + 6;
            if( b7 && rr_x > 0 ) return gC + 7;
         }
         else {
            if(watch) cout << "6" << endl;
            // if( 0 ) return gC + 0;
            // if( 0 ) return gC + 1;
            // if( 0 ) return gC + 2;
            // if( 0 ) return gC + 3;
            // if( b4 && 0 ) return gC + 4;
            // if( b5 && 0 ) return gC + 5;
            if( b6 && rr_y > 0 ) return gC + 6;
            if( b7 && rr_y < 0 ) return gC + 7;
         }
      }
      else { // One component is zero
            if(watch) cout << "7" << endl;
        if( rr_x > 0 ) return gC + 0;
        if( rr_x < 0 ) return gC + 1;
        if( rr_y > 0 ) return gC + 2;
        if( rr_y < 0 ) return gC + 3;
        if( b4 && (  rr_x >= 2 ||  rr_y >= 2 ) ) return gC + 4;
        if( b5 && ( -rr_x >= 2 || -rr_y >= 2 ) ) return gC + 5;
        if( b6 && ( -rr_x >= 2 ||  rr_y >= 2 ) ) return gC + 6;
        if( b7 && (  rr_x >= 2 || -rr_y >= 2 ) ) return gC + 7;
      }
      assert(false);
      return -1;
    }
} // namespace Booksim
