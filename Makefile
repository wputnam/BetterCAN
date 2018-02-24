#This file is part of BetterCAN.
#Copyright 2018 William Putnam, University of Aizu
#
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

CXX = g++
CFLAGS = --std=c++11 -O2 -g -pthread
LIBS = -lcryptopp -lpthread
SUBDIRS = fuzzTest injectionTest mitmTest

ifeq ($(shell uname),Linux)
all: aware ib keyGen MCUin ob stack tcbm

aware: Aware.o SSMaware.o
		$(CXX) $(CFLAGS) -o aware Aware.o SSMaware.o
ib: inputbuffer.o
		$(CXX) $(CFLAGS) -o ib inputbuffer.o
keyGen: keyGen.o
		$(CXX) $(CFLAGS) -o keyGen keyGen.o $(LIBS)
MCUin: MCUin.o
		$(CXX) $(CFLAGS) -o MCUin MCUin.o
ob: outputbuffer.o
		$(CXX) $(CFLAGS) -o ob outputbuffer.o
stack: SSMstack.o
		$(CXX) $(CFLAGS) -o stack SSMstack.o
tcbm: MCUout.o TCBmodule.o TCB.o Timer.o
		$(CXX) $(CFLAGS) -o tcbm MCUout.o TCBmodule.o TCB.o Timer.o $(LIBS)

Aware.o: Aware.cpp Aware.hpp
		$(CXX) -c Aware.cpp
inputbuffer.o: inputbuffer.cpp
		$(CXX) -c inputbuffer.cpp
keyGen.o: keyGen.cpp
		$(CXX) -c keyGen.cpp
MCUin.o: MCUin.cpp
		$(CXX) -c MCUin.cpp
MCUout.o: MCUout.cpp TCBmodule.hpp TCB.hpp Timer.hpp
		$(CXX) -c MCUout.cpp
outputbuffer.o: outputbuffer.cpp
		$(CXX) -c outputbuffer.cpp
SSMaware.o: SSMaware.cpp Aware.hpp
		$(CXX) -c SSMaware.cpp
SSMstack.o: SSMstack.cpp SSMstack.hpp
		$(CXX) -c SSMstack.cpp
TCB.o: TCB.cpp TCB.hpp Timer.hpp
		$(CXX) -c TCB.cpp
TCBmodule.o: TCBmodule.cpp TCBmodule.hpp TCB.hpp Timer.hpp
		$(CXX) -c TCBmodule.cpp
Timer.o: Timer.cpp Timer.hpp
		$(CXX) -c Timer.cpp

else
all:
		@echo "ERR: need Linux to compile this project."
endif

.PHONY: clean

clean:
		rm -f *.o aware ib keyGen MCUin ob stack tcbm /tmp/ibToTCB* /tmp/TCBToOb* /tmp/TCBtoAL* /tmp/newawareentry34 /tmp/newstackentry34 /tmp/makeawareentry34 /tmp/delaware34