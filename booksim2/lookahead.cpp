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

/*lookahead.cpp
 *
 *lookahead struct is a control signals that carries the information to
 * allocate router resources in advance in bypass routers.
 *
 */

#include "booksim.hpp"
#include "lookahead.hpp"

namespace Booksim
{

    ostream& operator<<( ostream& os, const Lookahead& l )
    {
        /*
           os << "  Flit ID: " << f.id << " (" << &f << ")" 
           << " Packet ID: " << f.pid
           << " Class: " << f.cl 
           << " Head: " << f.head
           << " Tail: " << f.tail << endl;
           os << "  Source: " << f.src << "  Dest: " << f.dest << " Intm: "<<f.intm<<endl;
           os << "  Creation time: " << f.ctime << " Injection time: " << f.itime << " Arrival time: " << f.atime << " Phase: "<<f.ph<< endl;
           os << "  VC: " << f.vc << endl;
           */
        return os;
    }

    Lookahead::Lookahead(Flit * f)
    {
        id = f->id;
        pid = f->pid;
        vc = f->vc;
        src = f->src;
        dest = f->dest;
        pri = f->pri;
        head = f->head;
        tail = f->tail;
        cl = f->cl;
        ph = f->ph;
        la_route_set = OutputSet(f->la_route_set);
        packet_size = f->packet_size;
        watch = f->watch;
        ssr_hops = 0;
    }

    Lookahead::Lookahead(Lookahead * la)
    {
        id = la->id;
        pid = la->pid;
        vc = la->vc;
        src = la->src;
        dest = la->dest;
        pri = la->pri;
        head = la->head;
        tail = la->tail;
        cl = la->cl;
        ph = la->ph;
        la_route_set = OutputSet(la->la_route_set);
        packet_size = la->packet_size;
        watch = la->watch;
        ssr_hops = 0;
    }

    // Deprecated???
    void Lookahead::ConvertLookaheadToFlit(Flit * f)
    {
        f->pid = pid;
        f->vc = vc;
        f->src = src;
        f->dest = dest;
        f->pri = pri;
        f->head = head;
        f->tail = tail;
        f->cl = cl;
        f->ph = ph;
        f->la_route_set = la_route_set;
        f->packet_size = packet_size;
        f->watch = watch;
    }

    // Deprecated???
    void Lookahead::SetLookahead(Flit * f)
    {
        id = f->id;
        pid = f->pid;
        vc = f->vc;
        src = f->src;
        dest = f->dest;
        pri = f->pri;
        head = f->head;
        tail = f->tail;
        cl = f->cl;
        ph = f->ph;
        la_route_set = f->la_route_set;
        packet_size = f->packet_size;
        watch = f->watch;
    }

    void Lookahead::CloneLookahead(Lookahead * la)
    {
        id = la->id;
        pid = la->pid;
        vc = la->vc;
        src = la->src;
        dest = la->dest;
        pri = la->pri;
        head = la->head;
        tail = la->tail;
        cl = la->cl;
        ph = la->ph;
        la_route_set = la->la_route_set;
        packet_size = la->packet_size;
        watch = la->watch;
        ssr_hops = 0;
    }
} // namespace Booksim
