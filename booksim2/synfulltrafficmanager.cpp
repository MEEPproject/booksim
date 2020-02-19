// $Id$

/*
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

#include <sstream>

#include "synfulltrafficmanager.hpp"
#include "NetworkInterface.h"
#include "random_utils.hpp"

namespace Booksim
{

    SynFullTrafficManager::SynFullTrafficManager( const Configuration &config, 
                            const vector<Network *> & net )
      : TrafficManager(config, net), _overall_runtime(0)
    {
      _sample_period = config.GetInt( "sample_period" );
      _max_samples    = config.GetInt( "max_samples" );
      _warmup_periods = config.GetInt( "warmup_periods" );
      _channel_width = config.GetInt( "channel_width" );
      _max_length = 0;


    }

    SynFullTrafficManager::~SynFullTrafficManager( )
    {
        cout << "Destroying... nothing" << endl;
    }

    void SynFullTrafficManager::_Inject( )
    {

      InjectReqMsg InjectionPacket;

      while(!_injection_queue_messages.empty()){
        InjectionPacket = _injection_queue_messages.front();
        _injection_queue_messages.pop();
        if(InjectionPacket.packetSize%_channel_width > 0) {
            InjectionPacket.packetSize = InjectionPacket.packetSize*8/_channel_width +1;
        } else {
            InjectionPacket.packetSize = InjectionPacket.packetSize*8/_channel_width;
        }
        int pid = _GeneratePacket(InjectionPacket.source, InjectionPacket.dest, InjectionPacket.packetSize, InjectionPacket.cl, _time);
        assert(pid == InjectionPacket.id);
      }
    }

    void SynFullTrafficManager::_RetirePacket( Flit * head, Flit * tail )
    {
      EjectResMsg EjectionPacket;

      EjectionPacket.id = head->pid;
      EjectionPacket.remainingRequests = _ejection_queue_messages.size() + 1;
      EjectionPacket.source = head->src;
      EjectionPacket.dest = head->dest;
      EjectionPacket.packetSize = head->packet_size;
      EjectionPacket.cl = head->cl;

      _ejection_queue_messages.push(EjectionPacket);
      TrafficManager::_RetirePacket(head,tail);
    }

    void SynFullTrafficManager::_ResetSim( )
    {
      TrafficManager::_ResetSim( );
    }

    bool SynFullTrafficManager::_SingleSim( )
    {

      _sim_state = running;
      
      cout << "Beginning measurements..." << endl;
     

      while(ni->Step(&_injection_queue_messages, &_ejection_queue_messages) == 0) {
       _Step();
        
        if((_time % _sample_period) == 0) {
          UpdateStats();
          DisplayStats();
        }
      }
      cout << "Completed measurements after " << _time << " cycles." << endl;

      _sim_state = draining;
      _drain_time = _time;

      return 1;
    }

    // FIXME: Is this necessary?
    bool SynFullTrafficManager::_Completed( )
    {
      return true;
    }

    void SynFullTrafficManager::_UpdateOverallStats()
    {
      TrafficManager::_UpdateOverallStats();
      _overall_runtime += (_drain_time - _reset_time);
    }
      
    string SynFullTrafficManager::_OverallStatsHeaderCSV() const
    {
      ostringstream os;
      os << TrafficManager::_OverallStatsHeaderCSV()
         << ',' << "runtime";
      return os.str();
    }

    string SynFullTrafficManager::_OverallClassStatsCSV(int c) const
    {
      ostringstream os;
      os << TrafficManager::_OverallClassStatsCSV(c)
         << ',' << (double)_overall_runtime / (double)_total_sims;
      return os.str();
    }

    void SynFullTrafficManager::_DisplayClassStats(int c, ostream & os) const
    {
      TrafficManager::_DisplayClassStats(c, os);
    }

    void SynFullTrafficManager::_DisplayOverallClassStats(int c, ostream & os) const
    {
      TrafficManager::_DisplayOverallClassStats(c, os);
      os << "Overall workload runtime = " << (double)_overall_runtime / (double)_total_sims
         << " (" << _total_sims << " samples)" << endl;
    }
} // namespace Booksim
