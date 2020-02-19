/* 
Copyright (c) 2014, Mario Badr
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * socketstream.cpp
 *
 *  Created on: Dec 16, 2009
 *      Author: sam
 * 
 * Modified by Wenbo Dai, University of Toronto, Aug 12, 2013
 * use Unix Socket, instead of Internet Socket
 */

#include "socketstream.h"

#include <cstdio>
#include <cerrno>

namespace Booksim
{

    //int SocketStream::listen(char * in_socket_path)
    int SocketStream::listen(char * in_socket_path)
    {
        //const char *socket_path = "./socket";
        //socket_path = "./socket";
        //socket_path = in_socket_path;
        // Create a socket
        if ( (so = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            cout << "Error creating socket." << endl;
            return -1;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

        // Bind it to the listening port
        unlink(socket_path);
        if (bind(so, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
             cout << "Error binding socket." << endl;
             return -1;
        }
        // Listen for connections
        if (::listen(so, NS_MAX_PENDING) != 0) {
             cout << "Error listening on socket." << endl;
             return -1;
        }
        bIsAlive = true;

#ifdef NS_DEBUG
        cout << "Listening on socket" << endl;
#endif

        return 0;
    }

    // accept a new connection
    SocketStream* SocketStream::accept()
    {
        struct sockaddr_un clientaddr;
        socklen_t clientaddrlen = sizeof clientaddr;
        int clientsock = ::accept(so, (struct sockaddr*)&clientaddr, &clientaddrlen);
        if ( clientsock < 0 ){
            cout << "Error accepting a connection";
            return NULL;
        }
        // prevent small packets from getting stuck in OS queues
        //int on = 1;
        //setsockopt (so, SOL_TCP, TCP_NODELAY, &on, sizeof (on));

        return new SocketStream(clientsock, (struct sockaddr*)&clientaddr, clientaddrlen, socket_path);
    }

    int SocketStream::connect()
    {
        //const char *socket_path = "./socket";
        //socket_path = "./socket";
        // Create a socket.
        if ( (so = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ){
            cout << "Error creating socket." << endl;
            return -1;
        }
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

        // Connect to the server.
        if ( ::connect(so, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            cout << "Connection failed." << endl;
            return -1;
        }

        // prevent small packets from getting stuck in OS queues
        //int on = 1;
        //setsockopt (so, SOL_TCP, TCP_NODELAY, &on, sizeof (on));

        bIsAlive = true;

#ifdef NS_DEBUG
        cout << "Connected to host" << endl;
#endif

        return 0;
    }

    // read from the socket
    int SocketStream::get(void *data, int number)
    {

        int remaining = number;
        int received = 0;
        char *dataRemaining = (char*) data;

        errno = 0;
        while (remaining > 0 && (errno == 0 || errno == EINTR))
        {
            received = recv(so, dataRemaining, remaining, 0); // MSG_WAITALL
            if (received > 0)
            {
                dataRemaining += received;
                remaining -= received;
            }
        }

        return number - remaining;
    }

    // write to socket
    int SocketStream::put(const void *data, int number)
    {
        // MSG_NOSIGNAL prevents SIGPIPE signal from being generated on failed send
        return send(so, data, number, MSG_NOSIGNAL);
    }
} // namespace Booksim
