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

#ifndef _TCBHPP
#define _TCBHPP

#include <bitset>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cryptopp/osrng.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

#include "Timer.hpp"

#define CLOSED 0
#define SYN_SENT 1
#define SYN_RCVD 2
#define ESTABLISHED 3
#define FIN_WAIT 4
#define TIME_WAIT 5

#define SSMID "34"

class TCB{
private:
	bool MCU1 = false, MCU2 = false;
	bool didSYNACK = false, isEstab = false, establishing = false, isVerif = false, killing = false, killed = false, lastPackGarb = false;
	float lastValue = -1;
	unsigned char canID;	//for our destination!
	std::string ALpipepath, latestFVenc, lastValueEnc;
	byte key[CryptoPP::AES::MAX_KEYLENGTH];
	
	const bool SYNonly[5] = {false, false, false, true, false};
	const bool SYNACK[5] = {true, false, false, true, false};
	const bool ACKonly[5] = {true, false, false, false, false};
	const bool RSTonly[5] = {false, false, true, false, false};
	const bool FINACK[5] = {true, false, false, false, true};
	const bool PSHonly[5] = {false, true, false, false, false};
	const bool PSHACK[5] = {true, true, false, false, false};
	const bool FINonly[5] = {false, false, false, false, true};

public:
	unsigned int seq = 0, ackn = 0;
	int thisConnState;
	float latestFV = 0;
	std::unique_ptr<Timer> retrans, keepalive, time_wait;
	bool makeConn = false, isAuth = false, needAck = false, sending = false, hardReset = false, killConn = false;
	std::string currentInput, currentOutput;
	TCB(int s, int a, char c, int MCU) :
		seq(s),
		ackn(a),
		canID(c),
		retrans(std::make_unique<Timer>(0)),
		keepalive(std::make_unique<Timer>(1)),
		time_wait(std::make_unique<Timer>(2)),
		thisConnState(0),
		ALpipepath("/tmp/TCBtoAL")
		{ relation(MCU); }
	~TCB(){std::cerr << "NOT: " << canID << " dying!\n";}
	
	bool sameID(char c){ return canID == c; }
	bool isMCU1() const { return MCU1; }
	bool isMCU2() const { return MCU2; }
	std::string diagInfo();
	char getCanID() const { return canID; }
	void relation(int MCU);
	int flagStats(bool f[5]);
	bool prepareSwitch(int s, int a, bool f[5], std::string input);
	void updateStats(unsigned int ds, unsigned int da);
	int checkStats(unsigned int inSeq, unsigned int inAckn, char c);
	void makeOutput(char src, std::string outfilepipepath, std::string al);
	std::string checksum(std::string in);
	char MCU1toSSM_SL(std::string sl, std::string pay);
	char SSMtoMCU2_SL(std::string sl);
	char MCU2dest();
	void SLtoAL(char CANID);
	
	void TCBswitch(bool f[5]);
	void caseClosed(bool f[5]);
	void caseSynSent(bool f[5]);
	void caseSynRcvd(bool f[5]);
	void caseEstablished(bool f[5]);
	void caseFinWait(bool f[5]);
	void caseTimeWait(bool f[5]);
	
};

#endif
