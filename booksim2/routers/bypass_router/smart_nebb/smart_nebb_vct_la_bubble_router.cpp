// $Id$

/*
 Copyright (c) 2014-2020, Trustees of The University of Cantabria
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.  Redistributions in binary
 form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with
 the distribution.

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

#include "smart_nebb_vct_la_bubble_router.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cassert>
#include <limits>
#include <climits>
#include <algorithm>

#include "../../../globals.hpp"
#include "../../../random_utils.hpp"
#include "../../../vc.hpp"
#include "../../../routefunc.hpp"
#include "../../../outputset.hpp"
#include "../../../buffer.hpp"
#include "../../../buffer_state.hpp"
#include "../../../arbiters/roundrobin_arb.hpp"
#include "../../../allocators/allocator.hpp"

namespace Booksim
{

    SMARTNEBBVCTLABubbleRouter::SMARTNEBBVCTLABubbleRouter(Configuration const & config, Module *parent, string
            const & name, int id, int inputs, int outputs) : SMARTNEBBVCTLARouter( config, parent,
                name, id, inputs, outputs )
    {
        string packet_size_str = config.GetStr("packet_size");
        if(packet_size_str.empty()) {
            _bubble_size = config.GetInt("packet_size");
        } else {
            const vector<int > packet_size = config.GetIntArray("packet_size");
            _bubble_size = *max_element(packet_size.begin(), packet_size.end());
        }
        
        // Check minimum buffer size requirement
        for (int output=0; output < _outputs; output ++) {
            for (int vc=0; vc < _vcs; vc++) {
                assert(_next_buf[output]->LimitFor(vc) >= 2*_bubble_size);
            }
        }
    }

    SMARTNEBBVCTLABubbleRouter::~SMARTNEBBVCTLABubbleRouter() {
    }

    int SMARTNEBBVCTLABubbleRouter::FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance) {
        
        BufferState * dest_buf = _next_buf[output];
        
        // Firstly, empty VCs
        if(f->head){
            for(int vc=vc_start; vc <= vc_end; vc++){
                if(dest_buf->IsAvailableFor(vc) &&
                   dest_buf->AvailableFor(vc) == dest_buf->LimitFor(vc) &&
                   (distance == 0 ||
                   (distance > 0 && FreeLocalBuf(input, vc_start, vc_end)))
                ){
                    return vc;
                }
            }
        }

        // Bubble has to be evaluated on dimension changes
        bool dimension_change = false;
        //if ((output < _outputs -gC) && input/2 != output/2) {
        if (input/2 != output/2 && output < _outputs - gC) {
            dimension_change = true;
        }

        // Secondly, VCs with room for the whole packet and the next router has an idle VC
        for(int vc=vc_start; vc <= vc_end; vc++){
            if(f->head){ 
                if(dest_buf->IsAvailableFor(vc)){
                    // Free slots
                    int fs = dest_buf->AvailableFor(vc);
                    // Check vc availability
                    bool vc_available = dimension_change ?
                                        fs >= f->packet_size + _bubble_size :
                                        fs >= f->packet_size;
                    if(vc_available){
                        return vc;
                    }
                }
            } else{
                if(dest_buf->UsedBy(vc) == f->pid){
                    return vc;
                }
            }
        }
        return -1;
    }
} // namespace Booksim
