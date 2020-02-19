/* 
Copyright (c) 2014, Mario Badr
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef NETWORK_INTERFACE_H_
#define NETWORK_INTERFACE_H_

#include "netstream/socketstream.h"
#include "netstream/messages.h"
#include "booksim.hpp"
#include <queue>

namespace Booksim
{

    //FIXME: Move the following constants to a better place
    const static int REQUEST = 1;
    const static int WRITE = 0;
    const static int READ = 1;
    const static int PUTC = 2;
    const static int PUTD = 3;
    const static int INV = 4;

    const static int RESPONSE = 2;
    const static int ACK = 0;
    const static int WB_ACK = 1;
    const static int DATA = 2;
    const static int UNBLOCK = 5;

    const static int CONTROL_SIZE = 8;
    const static int DATA_SIZE = 72;

    class NetworkInterface {
    public:
        int Init(char * in_socket_path);
        int Step(queue<InjectReqMsg> * Injection, queue<EjectResMsg> * Ejection);
    //	void ReqStepMsg(); // is it spare?
    private:
        SocketStream* m_channel;
    };
} // namespace Booksim
#endif
