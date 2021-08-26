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

#include <iostream>
#include <vector>
#include <cassert>
#include <limits>
#include "random_utils.hpp"
#include "injection.hpp"

namespace Booksim
{

    using namespace std;

    InjectionProcess::InjectionProcess(int nodes, double rate)
      : _nodes(nodes), _rate(rate)
    {
      if(nodes <= 0) {
        cout << "Error: Number of nodes must be greater than zero." << endl;
        exit(-1);
      }
      if((rate < 0.0) || (rate > 1.0)) {
        cout << "Error: Injection process must have load between 0.0 and 1.0."
         << endl;
        exit(-1);
      }
    }

    void InjectionProcess::reset()
    {

    }

    InjectionProcess * InjectionProcess::New(string const & inject, int nodes, 
                         double load, 
                         Configuration const * const config)
    {
      string process_name;
      string param_str = "";
      size_t left = inject.find_first_of('(');
      if(left == string::npos) {
        process_name = inject;
      } else {
        process_name = inject.substr(0, left);
        size_t right = inject.find_last_of(')');
        if(right == string::npos) {
          param_str = inject.substr(left+1);
        } else {
          param_str = inject.substr(left+1, right-left-1);
        }
      }
      vector<string> params = tokenize_str(param_str);

      InjectionProcess * result = NULL;
      if(process_name == "bernoulli") {
        result = new BernoulliInjectionProcess(nodes, load);
      //BSMOD: Add AcmeMemoryTrafficInjectionProcess
      } else if(process_name == "acmememorytraffic") {
        string mem_loc_cfg = config->GetStr( "acme_mem_location" );
        assert(mem_loc_cfg == "left" || mem_loc_cfg == "right" || mem_loc_cfg == "both");
        vector<int> kVect;
        int n;
        kVect = config->GetIntArray( "k" );
        n = config->GetInt("n");
        assert(n == 2); // Implemented for D-2 mesh
        if(kVect.empty()) { // Fix kVect for square mesh network
          kVect.push_back(config->GetInt( "k" ));
          kVect.push_back(config->GetInt( "k" ));
        }
        assert((int) kVect.size() == n);
        result = new AcmeMemoryTrafficInjectionProcess(nodes, load, kVect, mem_loc_cfg);
      } else if(process_name == "on_off") {
        bool missing_params = false;
        double alpha = numeric_limits<double>::quiet_NaN();
        if(params.size() < 1) {
          if(config) {
        alpha = config->GetFloat("burst_alpha");
          } else {
        missing_params = true;
          }
        } else {
          alpha = atof(params[0].c_str());
        }
        double beta = numeric_limits<double>::quiet_NaN();
        if(params.size() < 2) {
          if(config) {
        beta = config->GetFloat("burst_beta");
          } else {
        missing_params = true;
          }
        } else {
          beta = atof(params[1].c_str());
        }
        double r1 = numeric_limits<double>::quiet_NaN();
        if(params.size() < 3) {
          r1 = config ? config->GetFloat("burst_r1") : -1.0;
        } else {
          r1 = atof(params[2].c_str());
        }
        if(missing_params) {
          cout << "Missing parameters for injection process: " << inject << endl;
          exit(-1);
        }
        if((alpha < 0.0 && beta < 0.0) || 
           (alpha < 0.0 && r1 < 0.0) || 
           (beta < 0.0 && r1 < 0.0) || 
           (alpha >= 0.0 && beta >= 0.0 && r1 >= 0.0)) {
          cout << "Invalid parameters for injection process: " << inject << endl;
          exit(-1);
        }
        vector<int> initial(nodes);
        if(params.size() > 3) {
          initial = tokenize_int(params[2]);
          initial.resize(nodes, initial.back());
        } else {
          for(int n = 0; n < nodes; ++n) {
        initial[n] = RandomInt(1);
          }
        }
        result = new OnOffInjectionProcess(nodes, load, alpha, beta, r1, initial);
      } else {
        cout << "Invalid injection process: " << inject << endl;
        exit(-1);
      }
      return result;
    }

    //=============================================================

    BernoulliInjectionProcess::BernoulliInjectionProcess(int nodes, double rate)
      : InjectionProcess(nodes, rate)
    {

    }

    bool BernoulliInjectionProcess::test(int source)
    {
      assert((source >= 0) && (source < _nodes));
      return (RandomFloat() < _rate);
    }

    //BSMOD: Add AcmeVasMemTilesTrafficPattern
    AcmeMemoryTrafficInjectionProcess::AcmeMemoryTrafficInjectionProcess(int nodes, double rate, vector<int> kVect, string mem_tiles_location)
      : BernoulliInjectionProcess(nodes, rate)
    {
      for(int cnode=0; cnode < _nodes; cnode++)
        if((mem_tiles_location != "right" && cnode % kVect[0] == 0) ||              // Left MEM tiles
           (mem_tiles_location != "left" && cnode % kVect[0] == (kVect[0] - 1)))    // Right MEM tiles
            _mem_tiles.insert(cnode);

      //cout << "MEM Tiles: ";
      //for(auto it=_mem_tiles.begin(); it != _mem_tiles.end(); it++)
      //    cout << to_string(*it) << ",";
      //cout << endl;
    }

    bool AcmeMemoryTrafficInjectionProcess::test(int source)
    {
      assert((source >= 0) && (source < _nodes));
      bool result = false;

      unordered_set<int>::const_iterator find_mem = _mem_tiles.find(source);
      if(find_mem == _mem_tiles.end()) { // SRC is a VAS tile - inject!
        result = RandomFloat() < _rate;
      } // SRC is a MEM tile - no inject because MEM tiles only answers to VAS tiles

      //cout << "LINE " << __LINE__
      //     << " SRC: " << to_string(source)
      //     << " injects: " << to_string(result) << endl;

      return result;
    }

    //=============================================================

    OnOffInjectionProcess::OnOffInjectionProcess(int nodes, double rate, 
                             double alpha, double beta, 
                             double r1, vector<int> initial)
      : InjectionProcess(nodes, rate), 
        _alpha(alpha), _beta(beta), _r1(r1), _initial(initial)
    {
      assert(alpha <= 1.0);
      assert(beta <= 1.0);
      assert(r1 <= 1.0);
      if(alpha < 0.0) {
        assert(beta >= 0.0);
        assert(r1 >= 0.0);
        _alpha = beta * rate / (r1 - rate);
      } else if(beta < 0.0) {
        assert(alpha >= 0.0);
        assert(r1 >= 0.0);
        _beta = alpha * (r1 - rate) / rate;
      } else {
        assert(r1 < 0.0);
        _r1 = rate * (alpha + beta) / alpha;
      }
      reset();
    }

    void OnOffInjectionProcess::reset()
    {
      _state = _initial;
    }

    bool OnOffInjectionProcess::test(int source)
    {
      assert((source >= 0) && (source < _nodes));

      // advance state
      _state[source] = 
        _state[source] ? (RandomFloat() >= _beta) : (RandomFloat() < _alpha);

      // generate packet
      return _state[source] && (RandomFloat() < _r1);
    }
} // namespace Booksim
