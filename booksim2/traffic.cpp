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
#include <sstream>
#include <algorithm>
#include "random_utils.hpp"
#include "traffic.hpp"

namespace Booksim
{

    TrafficPattern::TrafficPattern(int nodes)
    : _nodes(nodes)
    {
        if(nodes <= 0) {
            cout << "Error: Traffic patterns require at least one node." << endl;
            exit(-1);
        }
    }

    void TrafficPattern::reset()
    {

    }

    TrafficPattern * TrafficPattern::New(string const & pattern, int nodes, 
            Configuration const * const config)
    {
        string pattern_name;
        string param_str;
        size_t left = pattern.find_first_of('(');
        if(left == string::npos) {
            pattern_name = pattern;
        } else {
            pattern_name = pattern.substr(0, left);
            size_t right = pattern.find_last_of(')');
            if(right == string::npos) {
                param_str = pattern.substr(left+1);
            } else {
                param_str = pattern.substr(left+1, right-left-1);
            }
        }
        std::cout << "pattern_name " << pattern_name << " param_str " << param_str << std::endl;
        vector<string> params = tokenize_str(param_str);

        TrafficPattern * result = NULL;
        if(pattern_name == "bitcomp") {
            result = new BitCompTrafficPattern(nodes);
        } else if(pattern_name == "transpose") {
            result = new TransposeTrafficPattern(nodes);
        } else if(pattern_name == "bitrev") {
            result = new BitRevTrafficPattern(nodes);
        } else if(pattern_name == "shuffle") {
            result = new ShuffleTrafficPattern(nodes);
        } else if(pattern_name == "randperm") {
            int perm_seed = -1;
            if(params.empty()) {
                if(config) {
                    if(config->GetStr("perm_seed") == "time") {
                        perm_seed = int(time(NULL));
                        cout << "SEED: perm_seed=" << perm_seed << endl;
                    } else {
                        perm_seed = config->GetInt("perm_seed");
                    }
                } else {
                    cout << "Error: Missing parameter for random permutation traffic pattern: " << pattern << endl;
                    exit(-1);
                }
            } else {
                perm_seed = atoi(params[0].c_str());
            }
            result = new RandomPermutationTrafficPattern(nodes, perm_seed);
        //BSMOD: Add AcmeScalarMemoryTrafficPattern
        } else if(pattern_name == "acmescalarmemory") {
            string mem_loc_cfg = config->GetStr( "acme_mem_location" );
            assert(mem_loc_cfg == "left" || mem_loc_cfg == "right" || mem_loc_cfg == "both");
            assert(config->GetStr( "injection_process" ) == "acmememorytraffic");
            vector<int> kVect;
            vector<int> cVect;
            int n;
            kVect = config->GetIntArray( "k" );
            cVect = config->GetIntArray( "c" );
            n = config->GetInt("n");
            assert(n == 2); // Implemented for D-2 mesh
            if(kVect.empty()) { // Fix kVect for square mesh network
                kVect.push_back(config->GetInt( "k" ));
                kVect.push_back(config->GetInt( "k" ));
            }
            if(cVect.empty())
                cVect.push_back(config->GetInt( "c" ));
            assert((int) kVect.size() == n);
            assert(cVect[0] == 1); // Traffic pattern has been implemented only for non-concentrated mesh topo
            // Check classes and reply configuration
            int classes = config->GetInt( "classes" );
            assert(classes % 2 == 0);
            vector<int> replies = config->GetIntArray( "reply_class" );
            assert(!replies.empty());
            result = new AcmeScalarMemoryTrafficPattern(nodes, kVect, mem_loc_cfg);
        //BSMOD: Add AcmeVectorMemoryTrafficPattern
        } else if(pattern_name == "acmevectormemory") {
            string mem_loc_cfg = config->GetStr( "acme_mem_location" );
            AcmeVectorMemoryTrafficPattern::MEM_LOCATION mem_tiles_location;
            if(mem_loc_cfg == "left"){
                mem_tiles_location = AcmeVectorMemoryTrafficPattern::MEM_LOCATION::LEFT;
            }else if(mem_loc_cfg == "right"){
                mem_tiles_location = AcmeVectorMemoryTrafficPattern::MEM_LOCATION::RIGHT;
            }else if(mem_loc_cfg == "both"){
                mem_tiles_location = AcmeVectorMemoryTrafficPattern::MEM_LOCATION::BOTH;
            }else{
                cout << "Error: Invalid value in acme_mem_location parameter, options:\n" 
                     << "\t-left\n" 
                     << "\t-right\n"
                     << "\t-both"
                     << endl;
                exit(-1);
            }
            // This traffic patterns is only valid with acmememorytraffic injection process
            assert(config->GetStr( "injection_process" ) == "acmememorytraffic");
            // Network
            vector<int> kVect;
            vector<int> cVect;
            int n;
            kVect = config->GetIntArray( "k" );
            cVect = config->GetIntArray( "c" );
            n = config->GetInt("n");
            assert(n == 2); // Implemented for D-2 mesh
            if(kVect.empty()) { // Fix kVect for square mesh network
                kVect.push_back(config->GetInt( "k" ));
                kVect.push_back(config->GetInt( "k" ));
            }
            if(cVect.empty())
                cVect.push_back(config->GetInt( "c" ));
            assert((int) kVect.size() == n);
            assert(cVect[0] == 1); // Traffic pattern has been implemented only for non-concentrated mesh topo
            // Check classes and reply configuration
            int classes = config->GetInt( "classes" );
            assert(classes % 2 == 0);
            vector<int> replies = config->GetIntArray( "reply_class" );
            assert(!replies.empty());
            // Pattern option
            if(params.size() < 1) {
                cout << "Error: Missing parameter for AcmeVectorMemory traffic pattern, options:\n" 
                     << "\t-OWN: VAS tile communicates only with its own mcpu.\n" 
                     << "\t-SAMECOL: VAS tile communicates with its own mcpu and with another random mem tile in the same column.\n"
                     << "\t-RANDOM: VAS tile communicates with its own mcpu and with another random mem tile in the system.\n"
                     << "\t-OPPCOL: VAS tile communicates with its own mcpu and with another random mem tile in the opposite column.\n"
                     << "\t-FARTHEST: VAS tile communicates with its own mcpu and with the farest mem tile."
                     << endl;
                exit(-1);
            }
            string option = params[0].c_str();
            AcmeVectorMemoryTrafficPattern::MCPU_OPTION mcpu_option;
            if(option == "OWN" || option == "own") {
                mcpu_option = AcmeVectorMemoryTrafficPattern::MCPU_OPTION::OWN;
            }else if(option == "SAMECOL" || option == "samecol"){
                mcpu_option = AcmeVectorMemoryTrafficPattern::MCPU_OPTION::SAME_COL;
            }else if(option == "RANDOM" || option == "random"){
                assert(mem_loc_cfg == "both"); // Only supported for both columns option
                mcpu_option = AcmeVectorMemoryTrafficPattern::MCPU_OPTION::RANDOM;
            }else if(option == "OPPCOL" || option == "oppcol"){
                assert(mem_loc_cfg == "both"); // Only supported for both columns option
                mcpu_option = AcmeVectorMemoryTrafficPattern::MCPU_OPTION::OPP_COL;
            }else if(option == "FARTHEST" || option == "farthest"){
                mcpu_option = AcmeVectorMemoryTrafficPattern::MCPU_OPTION::FARTHEST;
            }else{
                cout << "Error: Invalid option in AcmeVectorMemory traffic pattern, options:\n" 
                     << "\t-OWN: VAS tile communicates only with its own mcpu.\n" 
                     << "\t-SAMECOL: VAS tile communicates with its own mcpu and with another random mem tile in the same column.\n"
                     << "\t-RANDOM: VAS tile communicates with its own mcpu and with another random mem tile in the system.\n"
                     << "\t-OPPCOL: VAS tile communicates with its own mcpu and with another random mem tile in the opposite column.\n"
                     << "\t-FARTHEST: VAS tile communicates with its own mcpu and with the farest mem tile."
                     << endl;
                exit(-1);
            }
            result = new AcmeVectorMemoryTrafficPattern(nodes, kVect, mem_tiles_location, mcpu_option);
        } else if(pattern_name == "uniform") {
            result = new UniformRandomTrafficPattern(nodes);
        } else if(pattern_name == "background") {
            vector<int> excludes = tokenize_int(params[0]);
            result = new UniformBackgroundTrafficPattern(nodes, excludes);
        } else if(pattern_name == "diagonal") {
            result = new DiagonalTrafficPattern(nodes);
        } else if(pattern_name == "asymmetric") {
            result = new AsymmetricTrafficPattern(nodes);
        } else if(pattern_name == "taper64") {
            result = new Taper64TrafficPattern(nodes);
        } else if(pattern_name == "bad_dragon") {
            bool missing_params = false;
            int k = -1;
            if(params.size() < 1) {
                if(config) {
                    k = config->GetInt("k");
                } else {
                    missing_params = true;
                }
            } else {
                k = atoi(params[0].c_str());
            }
            int n = -1;
            if(params.size() < 2) {
                if(config) {
                    n = config->GetInt("n");
                } else {
                    missing_params = true;
                }
            } else {
                n = atoi(params[1].c_str());
            }
            if(missing_params) {
                cout << "Error: Missing parameters for dragonfly bad permutation traffic pattern: " << pattern << endl;
                exit(-1);
            }
            result = new BadPermDFlyTrafficPattern(nodes, k, n);
        } else if((pattern_name == "tornado") || (pattern_name == "neighbor") ||
                (pattern_name == "badperm_yarc")) {
            bool missing_params = false;
            int k = -1;
            //BSMOD: Fix neighbor and tornado traffics for square cmesh implemented as CNKCube
            vector<int> kVect;
            if(params.size() < 1) {
                if(config) {
                    kVect = config->GetIntArray( "k" );
                    if(kVect.size() > 1) {
                        k = kVect[0];
                        // Only valid for square meshes
                        for(int i=1; i < (int) kVect.size(); i++)
                            assert(kVect[i] == kVect[i-1]);
                    } else
                        k = config->GetInt("k");
                } else {
                    missing_params = true;
                }
            } else {
                k = atoi(params[0].c_str());
            }
            int n = -1;
            if(params.size() < 2) {
                if(config) {
                    n = config->GetInt("n");
                } else {
                    missing_params = true;
                }
            } else {
                n = atoi(params[1].c_str());
            }
            int xr = -1;
            if(params.size() < 3) {
                if(config) {
                    xr = config->GetInt("xr");
                } else {
                    missing_params = true;
                }
            } else {
                xr = atoi(params[2].c_str());
            }
            if(missing_params) {
                cout << "Error: Missing parameters for digit permutation traffic pattern: " << pattern << endl;
                exit(-1);
            }
            if(pattern_name == "tornado") {
                result = new TornadoTrafficPattern(nodes, k, n, xr);
            } else if(pattern_name == "neighbor") {
                result = new NeighborTrafficPattern(nodes, k, n, xr);
            } else if(pattern_name == "badperm_yarc") {
                result = new BadPermYarcTrafficPattern(nodes, k, n, xr);
            }
        } else if(pattern_name == "hotspot") {
            if(params.empty()) {
                params.push_back("-1");
            } 
            vector<int> hotspots = tokenize_int(params[0]);
            for(size_t i = 0; i < hotspots.size(); ++i) {
                if(hotspots[i] < 0) {
                    hotspots[i] = RandomInt(nodes - 1);
                }
            }
            vector<int> rates;
            if(params.size() >= 2) {
                rates = tokenize_int(params[1]);
                rates.resize(hotspots.size(), rates.back());
            } else {
                rates.resize(hotspots.size(), 1);
            }
            result = new HotSpotTrafficPattern(nodes, hotspots, rates);
        } else if(pattern_name == "hotspot_uniform") {
            if(params.empty()) {
                params.push_back("-1");
            } 
            vector<int> hotspots = tokenize_int(params[0]);
            for(size_t i = 0; i < hotspots.size(); ++i) {
                if(hotspots[i] < 0) {
                    hotspots[i] = RandomInt(nodes - 1);
                }
            }
            vector<int> rates;
            if(params.size() >= 2) {
                rates = tokenize_int(params[1]);
                rates.resize(hotspots.size(), rates.back());
            } else {
                rates.resize(hotspots.size(), 1);
            }
            result = new HotSpotUniformTrafficPattern(nodes, hotspots, rates);
        } else if(pattern_name == "one_to_many") {
            result = new OneToManyTrafficPattern(nodes);
        } else if(pattern_name == "debug"){
            result = new DebugTrafficPattern(nodes);
        } else {
            cout << "Error: Unknown traffic pattern: " << pattern << endl;
            exit(-1);
        }
        return result;
    }

    int TrafficPattern::chainDestination(int source, int destination)
    {
        return -1;
    }

        PermutationTrafficPattern::PermutationTrafficPattern(int nodes)
    : TrafficPattern(nodes)
    {

    }

        BitPermutationTrafficPattern::BitPermutationTrafficPattern(int nodes)
    : PermutationTrafficPattern(nodes)
    {
        if((nodes & -nodes) != nodes) {
            cout << "Error: Bit permutation traffic patterns require the number of "
                << "nodes to be a power of two." << endl;
            exit(-1);
        }
    }

        BitCompTrafficPattern::BitCompTrafficPattern(int nodes)
    : BitPermutationTrafficPattern(nodes)
    {

    }

    int BitCompTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        int const mask = _nodes - 1;
        return ~source & mask;
    }

        TransposeTrafficPattern::TransposeTrafficPattern(int nodes)
    : BitPermutationTrafficPattern(nodes), _shift(0)
    {
        while(nodes >>= 1) {
            ++_shift;
        }
        if(_shift % 2) {
            cout << "Error: Transpose traffic pattern requires the number of nodes to "
                << "be an even power of two." << endl;
            exit(-1);
        }
        _shift >>= 1;
    }

    int TransposeTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        int const mask_lo = (1 << _shift) - 1;
        int const mask_hi = mask_lo << _shift;
        return (((source >> _shift) & mask_lo) | ((source << _shift) & mask_hi));
    }

        BitRevTrafficPattern::BitRevTrafficPattern(int nodes)
    : BitPermutationTrafficPattern(nodes)
    {

    }

    int BitRevTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        int result = 0;
        for(int n = _nodes; n > 1; n >>= 1) {
            result = (result << 1) | (source % 2);
            source >>= 1;
        }
        return result;
    }

        ShuffleTrafficPattern::ShuffleTrafficPattern(int nodes)
    : BitPermutationTrafficPattern(nodes)
    {

    }

    int ShuffleTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        int const shifted = source << 1;
        return ((shifted & (_nodes - 1)) | bool(shifted & _nodes));
    }

    DigitPermutationTrafficPattern::DigitPermutationTrafficPattern(int nodes, int k,
            int n, int xr)
    : PermutationTrafficPattern(nodes), _k(k), _n(n), _xr(xr)
    {

    }

        TornadoTrafficPattern::TornadoTrafficPattern(int nodes, int k, int n, int xr)
    : DigitPermutationTrafficPattern(nodes, k, n, xr)
    {

    }

    int TornadoTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        int offset = 1;
        int result = 0;

        for(int n = 0; n < _n; ++n) {
            result += offset *
                (((source / offset) % (_xr * _k) + ((_xr * _k + 1) / 2 - 1)) % (_xr * _k));
            offset *= (_xr * _k);
        }
        return result;
    }

        NeighborTrafficPattern::NeighborTrafficPattern(int nodes, int k, int n, int xr)
    : DigitPermutationTrafficPattern(nodes, k, n, xr)
    {

    }

    int NeighborTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        int offset = 1;
        int result = 0;
        for(int n = 0; n < _n; ++n) {
            result += offset *
                (((source / offset) % (_xr * _k) + 1) % (_xr * _k));
            offset *= (_xr * _k);
        }
        return result;
    }

    RandomPermutationTrafficPattern::RandomPermutationTrafficPattern(int nodes, 
            int seed)
    : TrafficPattern(nodes)
    {
        _dest.resize(nodes);
        randomize(seed);
    }

    void RandomPermutationTrafficPattern::randomize(int seed)
    {
        vector<long> save_x;
        vector<double> save_u;
        SaveRandomState(save_x, save_u);
        RandomSeed(seed);

        _dest.assign(_nodes, -1);

        for(int i = 0; i < _nodes; ++i) {
            int ind = RandomInt(_nodes - 1 - i);

            int j = 0;
            int cnt = 0;
            while((cnt < ind) || (_dest[j] != -1)) {
                if(_dest[j] == -1) {
                    ++cnt;
                }
                ++j;
                assert(j < _nodes);
            }

            _dest[j] = i;
        }

        RestoreRandomState(save_x, save_u); 
    }

    int RandomPermutationTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        assert((_dest[source] >= 0) && (_dest[source] < _nodes));
        return _dest[source];
    }

    //BSMOD: Add AcmeVectorMemoryTrafficPattern
    AcmeVectorMemoryTrafficPattern::AcmeVectorMemoryTrafficPattern(int nodes, vector<int> kVect, MEM_LOCATION mem_tiles_location, MCPU_OPTION mcpu_dest)
    : TrafficPattern(nodes),
      _mem_location(mem_tiles_location),
      _mcpu_dest_option(mcpu_dest),
      _kVect(kVect)
    {
        if(_mem_location == BOTH && kVect[0] % 2 != 0)
            cout << "WARNING: The size of the dimension is odd, so we assign one node more to right mcpus per row." << endl;
        if(kVect[1] % 2 != 0)
            cout << "WARNING: The size of the dimension is odd, so we assign one row more to the lowest mcpu index on each column." << endl;
        _mcpu_for_vas_tile.resize(nodes, -1);
        for(int cnode=0; cnode < _nodes; cnode++) {
            if(!isMemTile(cnode)) { // Node is a VAS tile
                switch(_mem_location){
                    case LEFT:
                        _mcpu_for_vas_tile[cnode] = (cnode / kVect[0]) * kVect[0];
                        break;
                    case RIGHT:
                        _mcpu_for_vas_tile[cnode] = ((cnode / kVect[0])+1) * kVect[0] - 1;
                        break;
                    case BOTH:
                        if((cnode % kVect[0]) < (kVect[0]/2)){
                            _mcpu_for_vas_tile[cnode] = (cnode / kVect[0]) * kVect[0];
                        }else{
                            _mcpu_for_vas_tile[cnode] = ((cnode / kVect[0])+1) * kVect[0] - 1;
                        }
                        break;
                    default:
                        assert(false);
                }
            } else {
                _both_mem_tiles.push_back(cnode);
                if (isLeftMemTile(cnode)) {
                    _left_mem_tiles.push_back(cnode);
                } else {
                    _right_mem_tiles.push_back(cnode);
                }
            }
        }

        /*
        cout << "MCPU for each tile (-1 is MCPU): ";
        for(std::vector<int>::iterator it=_mcpu_for_vas_tile.begin(); it != _mcpu_for_vas_tile.end(); it++)
            cout << to_string(*it) << ",";
        cout << endl;
        
        cout << "Left mem tiles: ";
        for(std::vector<int>::iterator it=_left_mem_tiles.begin(); it != _left_mem_tiles.end(); it++)
            cout << to_string(*it) << ",";
        cout << endl;
        
        cout << "Right mem tiles: ";
        for(std::vector<int>::iterator it=_right_mem_tiles.begin(); it != _right_mem_tiles.end(); it++)
            cout << to_string(*it) << ",";
        cout << endl;

        cout << "Both mem tiles: ";
        for(std::vector<int>::iterator it=_both_mem_tiles.begin(); it != _both_mem_tiles.end(); it++)
            cout << to_string(*it) << ",";
        cout << endl;
        */
    }

    int AcmeVectorMemoryTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        assert(_mcpu_for_vas_tile[source] >= 0);
        
        int result = _mcpu_for_vas_tile[source];

        /*
        cout << "Line " << __LINE__ 
             << " Src: " << to_string(source)
             << " AcmeVectorMemoryTrafficPattern::dest: " << to_string(result) << endl;
        */
        return result;
    }

    int AcmeVectorMemoryTrafficPattern::chainDestination(int source, int destination)
    {
        int chainDest = -1;
        assert(isMemTile(destination));

        switch(_mcpu_dest_option) {
            case OWN:
                assert(false); // Chain is not possible in OWN traffic option
                break;
            case SAME_COL:
                do {
                    if(isLeftMemTile(destination))
                        chainDest = _left_mem_tiles[rand() % _left_mem_tiles.size()];
                    else
                        chainDest = _right_mem_tiles[rand() % _right_mem_tiles.size()];
                } while(chainDest == destination);
                break;
            case RANDOM:
                do {
                    chainDest = _both_mem_tiles[rand() % _both_mem_tiles.size()];
                } while(chainDest == destination);
                break;
            case OPP_COL:
                if(isLeftMemTile(destination))
                    chainDest = _right_mem_tiles[rand() % _right_mem_tiles.size()];
                else
                    chainDest = _left_mem_tiles[rand() % _left_mem_tiles.size()];
                break;
            case FARTHEST:
                switch(_mem_location) {
                    case LEFT:
                        if(destination < _left_mem_tiles[_left_mem_tiles.size()/2]) {
                            chainDest = _left_mem_tiles[_left_mem_tiles.size()-1];
                        } else {
                            chainDest = _left_mem_tiles[0];
                        }
                        break;
                    case RIGHT:
                        if(destination < _right_mem_tiles[_right_mem_tiles.size()/2]) {
                            chainDest = _right_mem_tiles[_right_mem_tiles.size()-1];
                        } else {
                            chainDest = _right_mem_tiles[0];
                        }
                        break;
                    case BOTH:
                        if(isLeftMemTile(destination)) {
                            if(destination < _left_mem_tiles[_left_mem_tiles.size()/2]) {
                                chainDest = _right_mem_tiles[_left_mem_tiles.size()-1];
                            } else {
                                chainDest = _right_mem_tiles[0];
                            }
                        } else {
                            if(destination < _right_mem_tiles[_right_mem_tiles.size()/2]) {
                                chainDest = _left_mem_tiles[_right_mem_tiles.size()-1];
                            } else {
                                chainDest = _left_mem_tiles[0];
                            }
                        }
                }
                break;
            default:
                assert(false);
        }

        //cout << "source: " << source << " ownmcpu: " << destination << " chainmcpu: " << chainDest << endl;

        return chainDest;
    }

    bool AcmeVectorMemoryTrafficPattern::isLeftMemTile(int node)
    {
        bool result = false;
        if(_mem_location != RIGHT && node % _kVect[0] == 0)
            result = true;
        return result;
    }

    bool AcmeVectorMemoryTrafficPattern::isRightMemTile(int node)
    {
        bool result = false;
        if(_mem_location != LEFT && node % _kVect[0] == (_kVect[0] - 1))
            result = true;
        return result;
    }

    bool AcmeVectorMemoryTrafficPattern::isMemTile(int node)
    {
        return (isLeftMemTile(node)) || (isRightMemTile(node));
    }

        RandomTrafficPattern::RandomTrafficPattern(int nodes)
    : TrafficPattern(nodes)
    {

    }

    //BSMOD: Add AcmeScalarMemoryTrafficPattern
    AcmeScalarMemoryTrafficPattern::AcmeScalarMemoryTrafficPattern(int nodes, vector<int> kVect, string mem_tiles_location)
    : RandomTrafficPattern(nodes)
    {
        for(int cnode=0; cnode < _nodes; cnode++) {
            if((mem_tiles_location != "right" && cnode % kVect[0] == 0) ||              // Left MEM tiles
               (mem_tiles_location != "left" && cnode % kVect[0] == (kVect[0] - 1))) {  // Right MEM tiles
               _mem_tiles.push_back(cnode);
            } // Node is a VAS tile
        }
        _mem = _mem_tiles.size();

        //cout << "MEM Tiles: ";
        //for(std::vector<int>::iterator it=_mem_tiles.begin(); it != _mem_tiles.end(); it++)
        //    cout << to_string(*it) << ",";
        //cout << endl;
    }

    int AcmeScalarMemoryTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        assert(std::find(_mem_tiles.begin(), _mem_tiles.end(), source) == _mem_tiles.end());
        
        int result = _mem_tiles[RandomInt(_mem - 1)];

        //cout << "Line " << __LINE__ 
        //     << " Src: " << to_string(source)
        //     << " AcmeScalarMemoryTrafficPattern::dest: " << to_string(result) << endl;

        return result;
    }

        UniformRandomTrafficPattern::UniformRandomTrafficPattern(int nodes)
    : RandomTrafficPattern(nodes)
    {

    }

    int UniformRandomTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        //int result = RandomInt(_nodes - 1);
        //if (source == 0) {
        //    std::cout << "random value: " << result << std::endl; 
        //}
        return RandomInt(_nodes - 1);
    }

        UniformBackgroundTrafficPattern::UniformBackgroundTrafficPattern(int nodes, vector<int> excluded_nodes)
    : RandomTrafficPattern(nodes)
    {
        for(size_t i = 0; i < excluded_nodes.size(); ++i) {
            int const node = excluded_nodes[i];
            assert((node >= 0) && (node < _nodes));
            _excluded.insert(node);
        }
    }

    int UniformBackgroundTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        int result;

        do {
            result = RandomInt(_nodes - 1);
        } while(_excluded.count(result) > 0);

        return result;
    }

        DiagonalTrafficPattern::DiagonalTrafficPattern(int nodes)
    : RandomTrafficPattern(nodes)
    {

    }

    int DiagonalTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        return ((RandomInt(2) == 0) ? ((source + 1) % _nodes) : source);
    }

        AsymmetricTrafficPattern::AsymmetricTrafficPattern(int nodes)
    : RandomTrafficPattern(nodes)
    {

    }

    int AsymmetricTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        int const half = _nodes / 2;
        return (source % half) + (RandomInt(1) ? half : 0);
    }

        Taper64TrafficPattern::Taper64TrafficPattern(int nodes)
    : RandomTrafficPattern(nodes)
    {
        if(nodes != 64) {
            cout << "Error: Tthe Taper64 traffic pattern requires the number of nodes "
                << "to be exactly 64." << endl;
            exit(-1);
        }
    }

    int Taper64TrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        if(RandomInt(1)) {
            return ((64 + source + 8 * (RandomInt(2) - 1) + (RandomInt(2) - 1)) % 64);
        } else {
            return RandomInt(_nodes - 1);
        }
    }

        BadPermDFlyTrafficPattern::BadPermDFlyTrafficPattern(int nodes, int k, int n)
    : DigitPermutationTrafficPattern(nodes, k, n, 1)
    {

    }

    int BadPermDFlyTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        int const grp_size_routers = 2 * _k;
        int const grp_size_nodes = grp_size_routers * _k;

        return ((RandomInt(grp_size_nodes - 1) + ((source / grp_size_nodes) + 1) * grp_size_nodes) % _nodes);
    }

    BadPermYarcTrafficPattern::BadPermYarcTrafficPattern(int nodes, int k, int n, 
            int xr)
    : DigitPermutationTrafficPattern(nodes, k, n, xr)
    {

    }

    int BadPermYarcTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));
        int const row = source / (_xr * _k);
        return RandomInt((_xr * _k) - 1) * (_xr * _k) + row;
    }

    HotSpotTrafficPattern::HotSpotTrafficPattern(int nodes, vector<int> hotspots, 
            vector<int> rates)
    : TrafficPattern(nodes), _hotspots(hotspots), _rates(rates), _max_val(-1)
    {
        assert(!_hotspots.empty());
        size_t const size = _hotspots.size();
        _rates.resize(size, _rates.empty() ? 1 : _rates.back());
        for(size_t i = 0; i < size; ++i) {
            int const hotspot = _hotspots[i];
            assert((hotspot >= 0) && (hotspot < _nodes));
            int const rate = _rates[i];
            assert(rate >= 0);
            _max_val += rate;
        }
    }

    int HotSpotTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        if(_hotspots.size() == 1) {
            return _hotspots[0];
        }

        int pct = RandomInt(_max_val);

        for(size_t i = 0; i < (_hotspots.size() - 1); ++i) {
            int const limit = _rates[i];
            if(limit > pct) {
                return _hotspots[i];
            } else {
                pct -= limit;
            }
        }
        assert(_rates.back() > pct);
        return _hotspots.back();
    }
    
    HotSpotUniformTrafficPattern::HotSpotUniformTrafficPattern(int nodes, vector<int> hotspots, 
            vector<int> rates)
    : TrafficPattern(nodes), _hotspots(hotspots), _rates(rates), _max_val(-1)
    {
        assert(!_hotspots.empty());
        size_t const size = _hotspots.size();
        _rates.resize(size, _rates.empty() ? 1 : _rates.back());
        for(size_t i = 0; i < size; ++i) {
            int const hotspot = _hotspots[i];
            assert((hotspot >= 0) && (hotspot < _nodes));
            int const rate = _rates[i];
            assert(rate >= 0);
            _max_val += rate;
        }
    }

    int HotSpotUniformTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        if(_hotspots.size() == 1) {
            return _hotspots[0];
        }

        int pct = RandomInt(100);

        int limit = 0;
        for(size_t i = 0; i < _hotspots.size(); ++i) {
            limit += _rates[i];
            if(limit > pct) {
                return _hotspots[i];
            }
        }
        return RandomInt(_nodes-1);
    }

    OneToManyTrafficPattern::OneToManyTrafficPattern(int nodes)
    : TrafficPattern(nodes)
    {
        // FIXME: Parametrize this
        _source = 0;
    }

    int OneToManyTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        if(source != _source) {
            return -1;
        }
        else {
            return RandomInt(_nodes-1);
        }
    }

    DebugTrafficPattern::DebugTrafficPattern(int nodes)
    : TrafficPattern(nodes)
    {
        // FIXME: Parametrize this
        //_source = 0;
    }

    int DebugTrafficPattern::dest(int source)
    {
        assert((source >= 0) && (source < _nodes));

        //return _nodes-1;
        //if(source != 2 && source != 3) {
        if(source == 0) {
            return -1;
        }
        else {
            return 0;
            //return RandomInt(_nodes-1);
            //return 0;
        }
    }
} // namespace Booksim
