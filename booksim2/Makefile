# $Id $

# Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
# Copyright (c) 2014-2020, Trustees of The University of Cantabria
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#CXX = /opt/intel/bin/icc
LEX = flex
YACC   = bison -y

#DEFINE = -DTRACK_FLOWS=1 -DTRACK_STALLS=1
DEFINE += -DDEADLOCK_ABORT=1
#DEFINE += -DFLIT_DEBUG=1 -DLOOKAHEAD_DEBUG=1 # Incompatible with -DTRACK_*
#DEFINE += -DCREDIT_DEBUG=1
#DEFINE += -DARCH_DEBUG=1
#DEFINE += -DPIPELINE_DEBUG=1
#DEFINE += -DDEBUG_FLATFLY=1
#DEFINE += -DTOPOLOGY_DEBUG=1
#DEFINE += -DPACKET_TRACE=1

INCPATH = -I. -Iarbiters -Iallocators -Irouters -Inetworks -Ipower
CPPFLAGS += -Wall $(INCPATH) $(DEFINE)
# @FIXME: remove -fpermissive 
CPPFLAGS += -O3 -std=c++11
#CPPFLAGS += -std=c++11 -fno-inline-functions -O0
#CPPFLAGS += -g
# //BSMOD: Enable this option if dynamic library compilation is needed
#CPPFLAGS += -fPIC
LFLAGS +=

PROG := booksim
LIBDYN := libbooksim.so
LIBSTA := libbooksim.a

# simulator source files
CPP_SRCS = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard */*/*.cpp) $(wildcard */*/*/*.cpp)
CPP_HDRS = $(wildcard *.hpp) $(wildcard */*.hpp) $(wildcard */*/*.hpp) $(wildcard */*/*/*.hpp)
CPP_DEPS = $(CPP_SRCS:.cpp=.d)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

LEX_SRCS = lex.yy.c
LEX_OBJS = lex.yy.o

YACC_SRCS = y.tab.c
YACC_HDRS = y.tab.h
YACC_OBJS = y.tab.o

NETRACE_SRCS = netrace/netrace.c
NETRACE_HDRS = netrace/netrace.h
NETRACE_OBJS = netrace/netrace.o

OBJS :=  $(CPP_OBJS) $(LEX_OBJS) $(YACC_OBJS) $(NETRACE_OBJS)
	 
MAIN_OBJS1 = $(subst booksim_wrapper.o, ,$(OBJS))
MAIN_OBJS = $(subst test.o, ,$(MAIN_OBJS1))

.PHONY: clean

all: $(PROG)

$(PROG): $(OBJS)
	 $(CXX) $(LFLAGS) $(MAIN_OBJS) -o $@

$(LEX_SRCS): config.l
	$(LEX) $<

$(YACC_SRCS) $(YACC_HDRS): config.y
	$(YACC) -d $<

$(LEX_OBJS): $(LEX_SRCS) $(YACC_HDRS)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(YACC_OBJS): $(YACC_SRCS)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(NETRACE_OBJS): $(NETRACE_SRCS) $(NETRACE_HDRS)
	$(CC) $(CPPFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -MMD -c $< -o $@

lib: $(OBJS)
	echo $(subst main.o, ,$(OBJS))
	$(CXX) -shared $(LFLAGS) $(subst main.o, ,$(OBJS)) -o libbooksim.so

lib_static: $(OBJS)
	echo $(subst main.o, ,$(OBJS))
	ar rvs libbooksim.a $(subst main.o, ,$(OBJS))

#TODO: pipeline_debug
#TODO: track flags

clean:
	rm -f $(YACC_SRCS) $(YACC_HDRS)
	rm -f $(LEX_SRCS)
	rm -f $(CPP_DEPS)
	rm -f $(OBJS)
	rm -f $(PROG)
	rm -f $(LIBDYN)
	rm -f $(LIBSTA)

distclean: clean
	rm -f *~ */*~ */*/*~
	rm -f *.o */*.o */*/*.o
	rm -f *.d */*.d */*/*.d

-include $(CPP_DEPS)
