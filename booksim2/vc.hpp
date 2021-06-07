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

#ifndef _VC_HPP_
#define _VC_HPP_

#include <deque>

#include "flit.hpp"
#include "outputset.hpp"
#include "routefunc.hpp"
#include "config_utils.hpp"

namespace Booksim
{

    class VC : public Module {
        public:
            // VC States. FIXME: Do we have to change this states if we change the router architecture?
            enum eVCState { state_min = 0, idle = state_min, routing, sa_input = routing, vc_alloc, sa_output = vc_alloc, active, 
                state_max = active };
            //
            struct state_info_t {
                int cycles;
            };
            //
            static const char * const VCSTATE[];

        private:

            // BUffer FIFO
            deque<Flit *> _buffer;

            // VC State
            eVCState _state;

            // Output port + VC range
            OutputSet *_route_set;
            // ???
            int _out_port, _out_vc;
            //BSMOD: Change flit and packet id to long
            long _active_pid;

            // Types of priority. FIXME: Here we should add the new priority type for RSU priorities
            enum ePrioType { local_age_based, queue_length_based, hop_count_based, none, other };

            // Prioriyt type
            ePrioType _pri_type;

            // Flit priority???
            int _pri;

            // ???
            int _priority_donation;

            // flit watched?
            bool _watched;

            // Packet id expected. This is used to check if the flits of a packet are written in sequence into the buffer.
            // If the last flit was a tail flit, _expected_pid is -1, and the check is not done.
            //BSMOD: Change flit and packet id to long
            long _expected_pid;

            //BSMOD: Change flit and packet id to long
            // ID of last flit
            long _last_id;
            // Packet ID of last packet
            long _last_pid;

            // Is lookahead routing activated?
            bool _lookahead_routing;

        public:

            VC( const Configuration& config, int outputs,
                    Module *parent, const string& name );
            ~VC();

            // Write flit in the buffer (FIFO)
            void AddFlit( Flit *f );
            // Read flit from the head of the FIFO
            inline Flit *FrontFlit( ) const
            {
                return _buffer.empty() ? NULL : _buffer.front();
            }

            // Read flit from the HEAD of the buffer and remove it.
            Flit *RemoveFlit( );

            // IS the buffer empty? 
            inline bool Empty( ) const
            {
                return _buffer.empty( );
            }

            // Get state if the VC
            inline VC::eVCState GetState( ) const
            {
                return _state;
            }

            // Set the state of the VC
            void SetState( eVCState s );

            // Get route set (output port + VC range) of the leading flit
            const OutputSet *GetRouteSet( ) const;
            // Set route set of the leading flit
            void SetRouteSet( OutputSet * output_set );

            // Set output (output port and output vc) of the leading flit
            void SetOutput( int port, int vc );

            // Get output port of the leading flit
            inline int GetOutputPort( ) const
            {
                return _out_port;
            }

            // Get output VC of the leading flit
            inline int GetOutputVC( ) const
            {
                return _out_vc;
            }

            // ???
            void UpdatePriority();

            void SetPriority(int pri);

            // Get priority of the leading flit
            inline int GetPriority( ) const
            {
                return _pri;
            }
            // ???
            void Route( tRoutingFunction rf, const Router* router, const Flit* f, int in_channel );

            // Get number of flits stored in the buffer
            inline int GetOccupancy() const
            {
                return (int)_buffer.size();
            }

            //BSMOD: Change flit and packet id to long
            inline long GetLastPid( ) const
            {
                return _last_pid;
            }

            //BSMOD: Change flit and packet id to long
            inline void SetActivePID(long pid)
            {
              _active_pid = pid;
            }
            //BSMOD: Change flit and packet id to long
            inline long GetActivePID() const
            {
                return _active_pid;
            }
            
            //BSMOD: Change flit and packet id to long
            inline long GetExpectedPID() const
            {
                return _expected_pid;
            }
            
            inline void ReleaseVC()
            {
              _expected_pid = -1;
            }

            // ==== Debug functions ====

            void SetWatch( bool watch = true );
            bool IsWatched( ) const;
            void Display( ostream & os = cout ) const;

    };
} // namespace Booksim

#endif 
