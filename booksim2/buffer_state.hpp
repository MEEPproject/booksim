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

#ifndef _BUFFER_STATE_HPP_
#define _BUFFER_STATE_HPP_

#include <vector>
#include <queue>
#include <string>

#include "module.hpp"
#include "flit.hpp"
#include "credit.hpp"
#include "config_utils.hpp"

namespace Booksim
{

    class BufferState : public Module {
      
      class BufferPolicy : public Module {
      protected:
        BufferState const * const _buffer_state;
      public:
        BufferPolicy(Configuration const & config, BufferState * parent, 
             const string & name);
        BufferPolicy(Configuration const & config, BufferState * parent, 
             const string & name, const string & buffer_policy );
        virtual void SetMinLatency(int min_latency) {}
        virtual void TakeBuffer(int vc = 0);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual void FreeSlotFor(int vc = 0);
        //(I) Added a Flit argument, used by some BufferPolicies
        virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const = 0;
        //(I) Added a Flit argument, used by some BufferPolicies
        virtual int AvailableFor(int vc = 0) const = 0;
        virtual int LimitFor(int vc = 0) const = 0;
        //(I) CheckBubbleRestriction is used by bubble and flit bubble flow control. Returns TRUE if the
        // buffer success this restriction and FALSE if not
        virtual bool CheckBubbleRestriction(int vc_in, int vc_out, int inport, int outport, Flit const * const f) const;


        static BufferPolicy * New(Configuration const & config, 
                      BufferState * parent, const string & name);
        static BufferPolicy * New(Configuration const & config, 
                      BufferState * parent, const string & name, const string & buffer_policy);

        virtual void Display( ostream & os = cout ) const {}
      };
      
      class PrivateBufferPolicy : public BufferPolicy {
      protected:
        int _vc_buf_size;
      public:
        PrivateBufferPolicy(Configuration const & config, BufferState * parent, 
                const string & name);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const;
        virtual int AvailableFor(int vc = 0) const;
        virtual int LimitFor(int vc = 0) const;
      };
      
      // Used in SMART routers to implement dest node buffers larger
      // than internal buffers
      class DestPrivateBufferPolicy : public PrivateBufferPolicy {
      //protected:
      //  int _vc_buf_size;
      public:
        DestPrivateBufferPolicy(Configuration const & config, BufferState * parent, 
                const string & name);
      //  virtual void SendingFlit(Flit const * const f, bool vct = false);
      //  virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const;
      //  virtual int AvailableFor(int vc = 0) const;
      //  virtual int LimitFor(int vc = 0) const;
      };
      
      //(I) Virtual Cut-Through buffer policy (Based on Private)
      class VCTBufferPolicy : public BufferPolicy {
      protected:
        int _bubble_size; // Maximum of the packet sizes
        int _vc_buf_size;
      public:
        VCTBufferPolicy(Configuration const & config, BufferState * parent, 
                const string & name);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const;
        virtual int AvailableFor(int vc = 0) const;
        virtual int LimitFor(int vc = 0) const;
        virtual bool CheckBubbleRestriction(int vc_in, int vc_out, int inport, int outport, Flit const * const f) const;
      };
      
      //(I) FBFC-L buffer policy (Based on Private)
      class FBFCLBufferPolicy : public BufferPolicy {
      protected:
        int _bubble_size; // Maximum of the packet sizes
        int _vc_buf_size;
      public:
        FBFCLBufferPolicy(Configuration const & config, BufferState * parent, 
                const string & name);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const;
        virtual int AvailableFor(int vc = 0) const;
        virtual int LimitFor(int vc = 0) const;
        virtual bool CheckBubbleRestriction(int vc_in, int vc_out, int inport, int outport, Flit const * const f) const;
      };
      
      class SharedBufferPolicy : public BufferPolicy {
      protected:
        int _buf_size;
        vector<int> _private_buf_vc_map;
        vector<int> _private_buf_size;
        vector<int> _private_buf_occupancy;
        int _shared_buf_size;
        int _shared_buf_occupancy;
        vector<int> _reserved_slots;
        void ProcessFreeSlot(int vc = 0);
        bool _watch;
      public:
        SharedBufferPolicy(Configuration const & config, BufferState * parent, 
                   const string & name);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual void FreeSlotFor(int vc = 0);
        virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const;
        virtual int AvailableFor(int vc = 0) const;
        virtual int LimitFor(int vc = 0) const;
        virtual void Display( ostream & os = cout ) const;
      };

      class LimitedSharedBufferPolicy : public SharedBufferPolicy {
      protected:
        int _vcs;
        int _active_vcs;
        int _max_held_slots;
      public:
        LimitedSharedBufferPolicy(Configuration const & config, 
                      BufferState * parent,
                      const string & name);
        virtual void TakeBuffer(int vc = 0);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const;
        virtual int AvailableFor(int vc = 0) const;
        virtual int LimitFor(int vc = 0) const;
      };
        
      class DynamicLimitedSharedBufferPolicy : public LimitedSharedBufferPolicy {
      public:
        DynamicLimitedSharedBufferPolicy(Configuration const & config, 
                         BufferState * parent,
                         const string & name);
        virtual void TakeBuffer(int vc = 0);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
      };
      
      class ShiftingDynamicLimitedSharedBufferPolicy : public DynamicLimitedSharedBufferPolicy {
      public:
        ShiftingDynamicLimitedSharedBufferPolicy(Configuration const & config, 
                             BufferState * parent,
                             const string & name);
        virtual void TakeBuffer(int vc = 0);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
      };
      
      class FeedbackSharedBufferPolicy : public SharedBufferPolicy {
      protected:
        int _ComputeRTT(int vc, int last_rtt) const;
        int _ComputeLimit(int rtt) const;
        int _ComputeMaxSlots(int vc) const;
        int _vcs;
        vector<int> _occupancy_limit;
        vector<long> _round_trip_time;
        //BSMOD: Change time to long long
        vector<queue<long long> > _flit_sent_time;
        int _min_latency;
        int _total_mapped_size;
        int _aging_scale;
        int _offset;
      public:
        FeedbackSharedBufferPolicy(Configuration const & config, 
                       BufferState * parent, const string & name);
        virtual void SetMinLatency(int min_latency);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual void FreeSlotFor(int vc = 0);
        virtual bool IsFullFor(int vc = 0, Flit const * const f = NULL) const;
        virtual int AvailableFor(int vc = 0) const;
        virtual int LimitFor(int vc = 0) const;
      };
      
      class SimpleFeedbackSharedBufferPolicy : public FeedbackSharedBufferPolicy {
      protected:
        vector<int> _pending_credits;
      public:
        SimpleFeedbackSharedBufferPolicy(Configuration const & config, 
                         BufferState * parent, const string & name);
        virtual void SendingFlit(Flit const * const f, bool vct = false);
        virtual void FreeSlotFor(int vc = 0);
      };
      
      bool _wait_for_tail_credit;
      int  _size;
      int  _occupancy;
      vector<int> _vc_occupancy;
      int  _vcs;
      
      BufferPolicy * _buffer_policy;
      
      //BSMOD: Change flit and packet id to long
      vector<long> _in_use_by;
      vector<bool> _tail_sent;
      vector<long> _last_id;
      vector<long> _last_pid;

#ifdef TRACK_BUFFERS
      int _classes;
      vector<queue<int> > _outstanding_classes;
      vector<int> _class_occupancy;
#endif

    public:

      BufferState( const Configuration& config, 
               Module *parent, const string& name );
      
      BufferState( const Configuration& config, 
               Module *parent, const string& name, const string& buffer_policy );

      ~BufferState();

      inline void SetMinLatency(int min_latency) {
        _buffer_policy->SetMinLatency(min_latency);
      }

      void ProcessCredit( Credit const * const c, bool vct = false);
      void SendingFlit(Flit const * const f, bool vct = false);

      void TakeBuffer( int vc = 0, long tag = 0 );

      inline bool IsFull() const {
        assert(_occupancy <= _size);
        return (_occupancy == _size);
      }
      inline bool IsFullFor(int vc = 0, Flit const * const f = NULL) const {
        return _buffer_policy->IsFullFor(vc, f);
      }
      inline int AvailableFor(int vc = 0) const {
        return _buffer_policy->AvailableFor(vc);
      }
      inline int LimitFor( int vc = 0 ) const {
        return _buffer_policy->LimitFor(vc);
      }

      inline bool CheckBubbleRestriction(int vc_in, int vc_out, int inport, int outport, Flit const * const f) const{
        return _buffer_policy->CheckBubbleRestriction(vc_in, vc_out, inport, outport, f); 
      }

      inline bool IsEmptyFor(int vc = 0) const {
        assert((vc >= 0) && (vc < _vcs));
        return (_vc_occupancy[vc] == 0);
      }
      inline bool IsAvailableFor( int vc = 0 ) const {
        assert( ( vc >= 0 ) && ( vc < _vcs ) );
        return _in_use_by[vc] < 0;
      }
      //BSMOD: Change flit and packet id to long
      inline long UsedBy(int vc = 0) const {
        assert( ( vc >= 0 ) && ( vc < _vcs ) );
        return _in_use_by[vc];
      }

      inline int Occupancy() const {
        return _occupancy;
      }

      inline int OccupancyFor( int vc = 0 ) const {
        assert((vc >= 0) && (vc < _vcs));
        return _vc_occupancy[vc];
      }

      int GetAvailVCMinOccupancy(int vc_start, int vc_end) const;

      inline string Print() const {
        string str;

        for (int i = 0; i < _vcs; i++) {
            str.append(std::to_string(_vc_occupancy[i]));
            str.append(" ");
        }

        return str;
      }
      
#ifdef TRACK_BUFFERS
      inline int OccupancyForClass(int c) const {
        assert((c >= 0) && (c < _classes));
        return _class_occupancy[c];
      }
#endif

      void Display( ostream & os = cout ) const;
    };
} // namespace Booksim

#endif 
