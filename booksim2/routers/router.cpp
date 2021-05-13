// $Id$

/*
 Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
 Copyright (c) 2014-2020, Trustees of The Leland Stanford Junior University
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

/*router.cpp
 *
 *The base class of either iq router or event router
 *contains a list of channels and other router configuration variables
 *
 *The older version of the simulator uses an array of flits and credit to 
 *simulate the channels. Newer version ueses flitchannel and credit channel
 *which can better model channel delay
 *
 *The older version of the simulator also uses vc_router and chaos router
 *which are replaced by iq rotuer and event router in the present form
 */

#include "booksim.hpp"
#include <iostream>
#include <cassert>
#include "router.hpp"

//////////////////Sub router types//////////////////////
#include "iq_router.hpp"
#include "vct_router.hpp"
#include "fbfcl_router.hpp"
#include "event_router.hpp"
#include "chaos_router.hpp"
#include "bypass_router/bypass_no_arb_router.hpp" // Bypass router (Baseline without arb)
#include "bypass_router/bypass_no_arb_fbfcl_router.hpp" // Bypass router (Torus-FBFCL; Baseline without arb)
#include "bypass_router/bypass_arb_router.hpp" // Bypass router (Baseline with LA arb)
#include "bypass_router/bypass_arb_fbfcl_router.hpp" // NEBB-VCT (Torus-FBFCL; Baseline with LA arb)
#include "bypass_router/bypass_vct_router.hpp" // NEBB-VCT
#include "bypass_router/bypass_vct_bubble_router.hpp" // NEBB-VCT (Torus-bubble)
#include "bypass_router/hybrid_router.hpp" // NEBB-Hybrid
#include "bypass_router/smart_router.hpp" // SMART
#include "bypass_router/smart_nebb/smart_not_empty_realocation_router.hpp" //???
#include "bypass_router/smart_nebb/smart_nebb_wh_router.hpp" // SMART (NEBB-WH)
#include "bypass_router/smart_nebb/smart_nebb_vct_router.hpp" // SMART (NEBB-VCT)
#include "bypass_router/smart_nebb/smart_nebb_vct_opt_router.hpp" // SMART++
#include "bypass_router/smart_nebb/smart_nebb_vct_opt_bubble_router.hpp"//Torus
#include "bypass_router/smart_nebb/smart_la_router.hpp" // S-SMART
#include "bypass_router/smart_nebb/smart_nebb_vct_la_router.hpp" // S-SMART++
#include "bypass_router/smart_nebb/smart_nebb_vct_la_bubble_router.hpp" // S-SMART++ Torus
#include "bypass_router/smart_nebb/smart_nebb_wh_fbfcl_router.hpp"
#include "bypass_router/hybrid_fbfcl_router.hpp"
///////////////////////////////////////////////////////

namespace Booksim
{

    int const Router::STALL_BUFFER_BUSY = -2;
    int const Router::STALL_BUFFER_CONFLICT = -3;
    int const Router::STALL_BUFFER_FULL = -4;
    int const Router::STALL_BUFFER_RESERVED = -5;
    int const Router::STALL_CROSSBAR_CONFLICT = -6;

    Router::Router( const Configuration& config,
            Module *parent, const string & name, int id,
            int inputs, int outputs ) :
        TimedModule( parent, name ), _id( id ), _inputs( inputs ), _outputs( outputs ),
        _partial_internal_cycles(0.0)
    {
        _crossbar_delay   = ( config.GetInt( "st_prepare_delay" ) + 
                config.GetInt( "st_final_delay" ) );
        _credit_delay     = config.GetInt( "credit_delay" );
        _input_speedup    = config.GetInt( "input_speedup" );
        _output_speedup   = config.GetInt( "output_speedup" );
        _internal_speedup = config.GetFloat( "internal_speedup" );
        _classes          = config.GetInt( "classes" );

#ifdef TRACK_FLOWS
        _received_flits.resize(_classes, vector<int>(_inputs, 0));
        _stored_flits.resize(_classes);
        _sent_flits.resize(_classes, vector<int>(_outputs, 0));
        _active_packets.resize(_classes);
        _outstanding_credits.resize(_classes, vector<int>(_outputs, 0));
#endif

#ifdef TRACK_STALLS
        _switch_arbiter_input_stalls.resize(_classes, 0);
        _buffer_busy_stalls.resize(_classes, 0);
        _buffer_conflict_stalls.resize(_classes, 0);
        _buffer_full_stalls.resize(_classes, 0);
        _buffer_reserved_stalls.resize(_classes, 0);
        _crossbar_conflict_stalls.resize(_classes, 0);
        _output_blocked_stalls.resize(_classes,0);

        _la_buffer_busy.resize(_classes, 0);
        _la_buffer_conflict.resize(_classes, 0);
        _la_buffer_full.resize(_classes, 0);
        _la_buffer_reserved.resize(_classes, 0);
        _la_crossbar_conflict.resize(_classes, 0);
        _la_sa_winners_killed.resize(_classes, 0);
        _la_output_blocked.resize(_classes,0);
#endif

    }

    void Router::AddInputChannel( FlitChannel *channel, CreditChannel *backchannel )
    {
        _input_channels.push_back( channel );
        _input_credits.push_back( backchannel );
        channel->SetSink( this, _input_channels.size() - 1 ) ;
    }

    void Router::AddOutputChannel( FlitChannel *channel, CreditChannel *backchannel )
    {
        _output_channels.push_back( channel );
        _output_credits.push_back( backchannel );
        _channel_faults.push_back( false );
        channel->SetSource( this, _output_channels.size() - 1 ) ;
    }

    // With lookahead lines for bypass
    void Router::AddInputChannel( FlitChannel *channel, CreditChannel *backchannel, LookaheadChannel *lookahead_signals )
    {
        _input_channels.push_back( channel );
        _input_credits.push_back( backchannel );
        _input_lookahead.push_back( lookahead_signals );
        channel->SetSink( this, _input_channels.size() - 1 ) ;
    }

    void Router::AddOutputChannel( FlitChannel *channel, CreditChannel *backchannel, LookaheadChannel *lookahead_signals )
    {
        _output_channels.push_back( channel );
        _output_credits.push_back( backchannel );
        _output_lookahead.push_back( lookahead_signals );
        _channel_faults.push_back( false );
        channel->SetSource( this, _output_channels.size() - 1 ) ;
    }

    void Router::Evaluate( )
    {
        _partial_internal_cycles += _internal_speedup;
        while( _partial_internal_cycles >= 1.0 ) {
            _InternalStep( );
            _partial_internal_cycles -= 1.0;
        }
    }

    void Router::OutChannelFault( int c, bool fault )
    {
        assert( ( c >= 0 ) && ( (size_t)c < _channel_faults.size( ) ) );

        _channel_faults[c] = fault;
    }

    bool Router::IsFaultyOutput( int c ) const
    {
        assert( ( c >= 0 ) && ( (size_t)c < _channel_faults.size( ) ) );

        return _channel_faults[c];
    }

    /*Router constructor*/
    Router *Router::NewRouter( const Configuration& config,
            Module *parent, const string & name, int id,
            int inputs, int outputs )
    {
        const string type = config.GetStr( "router" );
        Router *r = NULL;
        if ( type == "iq" ) {
            r = new IQRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "event" ) {
            r = new EventRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "chaos" ) {
            r = new ChaosRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "vct" ) {
            //@TODO: refactor VCTRouter to reuse functions of IQRouter
            r = new VCTRouter( config, parent, name, id, inputs, outputs ); //(I) Virtual Cut-Through
        } else if ( type == "fbfcl" ) {
            //@TODO: refactor FBFCLRouter to reuse functions of IQRouter
            r = new FBFCLRouter( config, parent, name, id, inputs, outputs ); //(I) Flit Bubble Flow Control Localized
        } else if ( type == "bypass_no_arb" ) {
            // L. Peh 2007: A 4.6Tbits/s 3.2GHz... @TODO: comlete the citation
            r = new BypassNoArbRouter( config, parent, name, id, inputs, outputs ); 
        // The following bypass routers are described in:
        // I. Perez, E. Vallejo, and R. Beivide. Efficient router bypass via hybrid
        // flow control. In 2018 11th International Workshop on Network on Chip
        // Architectures (NoCArc), pages 1–6, Oct 2018
        // @TODO: replace names or describe
        } else if ( type == "bypass_no_arb_fbfcl" ) {
            r = new BypassNoArbFBFCLRouter( config, parent, name, id, inputs, outputs ); 
        } else if ( type == "bypass_arb" ) {
            r = new BypassArbRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "bypass_arb_fbfcl" ) {
            r = new BypassArbFBFCLRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "bypass_vct" ) {
            r = new BypassVCTRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "bypass_vct_bubble" ) {
            r = new BypassVCTBubbleRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "hybrid" ) {
            r = new HybridRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "hybrid_fbfcl" ) {
            r = new HybridFBFCLRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "hybrid_simplified" ) {
            r = new BypassArbRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "hybrid_simplified_fbfcl" ) {
            r = new BypassArbFBFCLRouter( config, parent, name, id, inputs, outputs );
        } else if ( type == "smart" ) {
          const string smart_type = config.GetStr("smart_type");
          // SMART original paper:
        
          if(smart_type == "classic"){
            r = new SMARTRouter( config, parent, name, id, inputs, outputs );
          // The following SMART routers are described in:
          // I. Pérez, E. Vallejo, and R. Beivide. SMART++: Reducing cost
          // and improving efficiency of multi-hop bypass in NoC routers. In
          // International Symposium on Networks-on-Chip (NOCS), pages 5:1–5:8,
          // 2019.
          } else if(smart_type == "not_empty_realocation"){
            r = new SMARTNotEmptyRealocationRouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "nebb_wh"){
            r = new SMARTNEBBWHRouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "nebb_wh_fbfcl"){
            r = new SMARTNEBBWHFBFCLRouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "nebb_vct"){ // Doesn't have credit optimization (@TODO: check if there are more changes)
            r = new SMARTNEBBVCTRouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "nebb_vct_opt"){ // SMART++
            r = new SMARTNEBBVCTOPTRouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "nebb_vct_opt_bubble"){
            r = new SMARTNEBBVCTOPTBubbleRouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "s-smart"){ // S-SMART++ 
            r = new SMARTLARouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "nebb_vct_la"){ // S-SMART++ 
            r = new SMARTNEBBVCTLARouter(config, parent, name, id, inputs, outputs);
          } else if(smart_type == "nebb_vct_la_bubble"){ // S-SMART++ torus
            r = new SMARTNEBBVCTLABubbleRouter(config, parent, name, id, inputs, outputs);
          }
        } else {
            cerr << "Unknown router type: " << type << endl;
        }
        /*For additional router, add another else if statement*/
        /*Original booksim specifies the router using "flow_control"
         *we now simply call these types. 
         */

        return r;
    }
} // namespace Booksim
