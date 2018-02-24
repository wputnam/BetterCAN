/*This file is part of BetterCAN.
 Copyright 2018 William Putnam, University of Aizu
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.*/

#ifndef _TCBMODHPP
#define _TCBMODHPP

#include "TCB.hpp"
#include <atomic>
#include <chrono>

struct hackInfo{
	char to, from;
	std::string lastAL;
	std::unique_ptr<Timer> hack_timer, resend;
	hackInfo(char t, char f, std::string las) :
		to(t),
		from(f),
		lastAL(las),
		hack_timer(std::make_unique<Timer>(4)),
		resend(std::make_unique<Timer>(0))
		{ hack_timer->reset(); }
};

struct toAndFrom{
	char to, from;
	bool transmitting;
	toAndFrom(char t, char f) :
		to(t),
		from(f),
		transmitting(false)
		{}
};

class TCBmodule{
private:
	//SSMonly vector is for SSM's connections to MCU1
	std::vector<std::shared_ptr<TCB>> TCBentries, SSMonly;
	std::vector<hackInfo> hacks;
	std::vector<toAndFrom> sessionConns;
	unsigned char CANID;
	std::string helppath = "/tmp/help", infilepipepath, outfilepipepath;
	pid_t ibPid, MCUinPid, awarePid, stackPid;
	bool SSMtoMCU2 = false, isSSM = false;
	std::atomic_bool killAllConn, doneKilling;
	byte key[CryptoPP::AES::MAX_KEYLENGTH];
	
public:
	TCBmodule(char c);
	~TCBmodule(){}
	void startInputLoop(std::string s);
	void startMCUin(std::string s);
	void startAware();
	void startStack();
	void stopInputLoop(){ kill(ibPid, SIGKILL); }
	void stopMCUin(){ kill(MCUinPid, SIGKILL); }
	void stopAware(){ kill(awarePid, SIGKILL); }
	void stopStack(){ kill(stackPid, SIGKILL); }
	bool checkConn(char c);
	void acceptInput();
	int entryCheck(char c);
	int entryCheckSSM(char c);
	std::string checksum(std::string in);
	bool checkChecksum(std::string in);
	void giveOutput(int index);
	void msgFromAL(char destID,std::string msg);
	int findMatchingSessConn(char t, char f);
	int findMatchingAck(int a);
	int findMatchingAckSSM(char c);
	void endAllConnections();
	void endConnectionLoop();
	void discoveredHack();
	void checkHacks();
	void retransmissions();
	void retransmissionsSSM(int s);
};

#endif
