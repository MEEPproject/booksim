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

#include "smart_nebb_wh_fbfcl_router.hpp"

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

    SMARTNEBBWHFBFCLRouter::SMARTNEBBWHFBFCLRouter(Configuration const & config, Module *parent, string
        const & name, int id, int inputs, int outputs) : SMARTRouter( config, parent,
          name, id, inputs, outputs )
    {

    }

    SMARTNEBBWHFBFCLRouter::~SMARTNEBBWHFBFCLRouter() {
      //std::cout << "~SMARTNEBBWHFBFCLRouter not implemented" << std::endl;
      //SMARTRouter::~SMARTRouter();
    }

    // NEBB (FIXME: Should crash)
    int SMARTNEBBWHFBFCLRouter::FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance) {
      for(int vc=vc_start; vc <= vc_end; vc++){
        if(f->head){
          if(f->tail){ // Single-flit packets
            if(_next_buf[output]->IsAvailableFor(vc)){
              if( (f->port_id/2 == output/2 && _next_buf[output]->AvailableFor(vc) > 0) ||
                  (f->port_id/2 != output/2 && _next_buf[output]->AvailableFor(vc) > f->packet_size)){
                return vc;
              }
            }
          } else {
            if(_next_buf[output]->IsAvailableFor(vc)){
              if(_next_buf[output]->AvailableFor(vc) == _next_buf[output]->LimitFor(vc) && FreeLocalBuf(input, vc_start, vc_end)){
                return vc;
              }
            }
          }
        } else{
          if(_next_buf[output]->UsedBy(vc) == f->pid){
            if(_next_buf[output]->AvailableFor(vc)){
              return vc;
            }
          }
        }
      }

      // Second chance only for head flits with distance 0 (body flits doesn't require the buffer empty)
      // This is a local flit moving to the next router to be buffered
      int dest_vc = -1;
      if(distance == 0){
        for(int vc=vc_start; vc <= vc_end; vc++){
          if(_next_buf[output]->IsAvailableFor(vc)){
            if( (f->port_id/2 == output/2 && _next_buf[output]->AvailableFor(vc) >= f->packet_size) ||
                (f->port_id/2 != output/2 && _next_buf[output]->AvailableFor(vc) > f->packet_size)){
              return vc;
            }
          }
        }
      }
      // Another round to check if there is a free VC. (This way we can allow the bypass)
      if(distance == 0){
        for(int vc=vc_start; vc <= vc_end; vc++){
          if(_next_buf[output]->IsAvailableFor(vc)){
            if(_next_buf[output]->AvailableFor(vc) == _next_buf[output]->LimitFor(vc) && _next_buf[output]->LimitFor(vc) > f->packet_size){
              dest_vc = vc;
            }
          }
        }
      }

      return dest_vc;
    }
} // namespace Booksim
