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

#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_

#include <vector>

#include "vc.hpp"
#include "flit.hpp"
#include "outputset.hpp"
#include "routefunc.hpp"
#include "config_utils.hpp"

namespace Booksim
{

    class Buffer : public Module {
      
      // NUmber of flits in the buffer
      int _occupancy;
      // Buffer size
      int _size;

      // Virtual channels of the buffer
      vector<VC*> _vc;

#ifdef TRACK_BUFFERS
      vector<int> _class_occupancy;
#endif

    public:
      
      Buffer( const Configuration& config, int outputs,
          Module *parent, const string& name );
      ~Buffer();

      void AddFlit( int vc, Flit *f );

      inline Flit *RemoveFlit( int vc )
      {
        --_occupancy;
#ifdef TRACK_BUFFERS
        int cl = _vc[vc]->FrontFlit()->cl;
        assert(_class_occupancy[cl] > 0);
        --_class_occupancy[cl];
#endif
        return _vc[vc]->RemoveFlit( );
      }
      
      // Takes the head flit of the virtual channel
      inline Flit *FrontFlit( int vc ) const
      {
        return _vc[vc]->FrontFlit( );
      }
      
      // Is the virtual channel empty?
      inline bool Empty( int vc ) const
      {
        return _vc[vc]->Empty( );
      }

      // Is the buffer full
      inline bool Full( ) const
      {
        return _occupancy >= _size;
      }

      // Gets the state of the virtual channel
      inline VC::eVCState GetState( int vc ) const
      {
        return _vc[vc]->GetState( );
      }

      // Sets the state of the virtual channel 
      inline void SetState( int vc, VC::eVCState s )
      {
        _vc[vc]->SetState(s);
      }

      // Gets route of the front flit in the virtual channel
      inline const OutputSet *GetRouteSet( int vc ) const
      {
        return _vc[vc]->GetRouteSet( );
      }

      // Sets route to the front flit in the virtual channel
      inline void SetRouteSet( int vc, OutputSet * output_set )
      {
    //    set<OutputSet::sSetElement> const route = output_set->GetSet();
    //    for(auto route_iter : route) {
    //        *gWatchOut << "Name: " << FullName() << " vc " << vc << " Next Output Port: " << route_iter.output_port << " ptr: " << output_set << std::endl;
    //    }
        _vc[vc]->SetRouteSet(output_set);
      }

      // Sets output (output port + output vc) of the virtual channel
      inline void SetOutput( int vc, int out_port, int out_vc )
      {
        _vc[vc]->SetOutput(out_port, out_vc);
      }

      // Gets output port
      inline int GetOutputPort( int vc ) const
      {
        return _vc[vc]->GetOutputPort( );
      }

      // Gets output vc
      inline int GetOutputVC( int vc ) const
      {
        return _vc[vc]->GetOutputVC( );
      }

      // Gets priority
      inline int GetPriority( int vc ) const
      {
        return _vc[vc]->GetPriority( );
      }
      
      inline void UpdatePriority( int vc ) const
      {
        _vc[vc]->UpdatePriority( );
      }
      
      inline void SetPriority( int vc, int pri ) const
      {
        _vc[vc]->SetPriority(pri);
      }
      
      //BSMOD: Change flit and packet id to long
      inline long GetLastPid( int vc ) const
      {
        return _vc[vc]->GetLastPid( );
      }

      // Computes route?
      inline void Route( int vc, tRoutingFunction rf, const Router* router, const Flit* f, int in_channel )
      {
        _vc[vc]->Route(rf, router, f, in_channel);
      }

      // ==== Debug functions ====

      inline void SetWatch( int vc, bool watch = true )
      {
        _vc[vc]->SetWatch(watch);
      }

      inline bool IsWatched( int vc ) const
      {
        return _vc[vc]->IsWatched( );
      }

      inline int GetOccupancy( ) const
      {
        return _occupancy;
      }

      inline int GetOccupancy( int vc ) const
      {
        return _vc[vc]->GetOccupancy( );
      }

      //BSMOD: Change flit and packet id to long
      inline long GetActivePID(int vc) const
      {
        return _vc[vc]->GetActivePID();
      }
      
      //BSMOD: Change flit and packet id to long
      inline void SetActivePID(int vc, long pid)
      {
        _vc[vc]->SetActivePID(pid);
      }
      
      // XXX: Only used by bypass routers.
      inline void ReleaseVC(int vc)
      {
        _vc[vc]->ReleaseVC();
      }
      
      //BSMOD: Change flit and packet id to long
      inline long GetExpectedPID(int vc) const
      {
        return _vc[vc]->GetExpectedPID();
      }

#ifdef TRACK_BUFFERS
      inline int GetOccupancyForClass(int c) const
      {
        return _class_occupancy[c];
      }
#endif

      void Display( ostream & os = cout ) const;
    };
} // namespace Booksim

#endif 
