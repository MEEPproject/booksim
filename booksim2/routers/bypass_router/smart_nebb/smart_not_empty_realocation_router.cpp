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

Author: Ivan Perez - University of Cantabria
*/

#include "smart_not_empty_realocation_router.hpp"

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

    SMARTNotEmptyRealocationRouter::SMARTNotEmptyRealocationRouter(Configuration const & config, Module *parent, string
            const & name, int id, int inputs, int outputs) : SMARTRouter( config, parent,
                name, id, inputs, outputs )
    {

    }

    SMARTNotEmptyRealocationRouter::~SMARTNotEmptyRealocationRouter() {
    }


    int SMARTNotEmptyRealocationRouter::FreeDestVC(int input, int output, int vc_start, int vc_end, Flit * f, int distance) {
        // The input VC doesn't have to be empty
        for (int vc=vc_start; vc <= vc_end; vc++) {
            if (f->head) {
                if (_next_buf[output]->IsAvailableFor(vc)) {
                    if (_next_buf[output]->AvailableFor(vc) == _next_buf[output]->LimitFor(vc)) {
                        return vc;
                    }
                }
            } else{
                if (_next_buf[output]->UsedBy(vc) == f->pid) {
                    if (_next_buf[output]->AvailableFor (vc)) {
                        return vc;
                    }
                }
            }
        }
            
        for (int vc=vc_start; vc <= vc_end; vc++) {
            if (f->head && (distance == 0 || (distance > 0 && FreeLocalBuf(input, vc_start, vc_end)))) {
                if (_next_buf[output]->IsAvailableFor(vc)) {
                    if (_next_buf[output]->AvailableFor(vc) >= f->packet_size) {
                        return vc;
                    }
                }
            }
        }

        return -1;
    }
} // namespace Booksim
