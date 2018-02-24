#/*This file is part of BetterCAN.
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

include "TCBmodule.hpp"

using namespace std;
using CryptoPP::AES;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::ECB_Mode;
using CryptoPP::Exception;
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::StreamTransformationFilter;

//Name: [TCBmodule constructor]
//Description: initialize TCB module for the MCU/SSM
//Output: N/A
//-------------------------------------------------------------------------------------------------
TCBmodule::TCBmodule(char c){
	int tmpCANID, sysStat;
	stringstream ss;
	ifstream ifs("my.key", ios::in);
	char readIn[32];
	
	//get CANID into hex, then initialize our filepipepath
	CANID = c;
    	if (c == 34){
    		isSSM = true;
    		//hack_timer = make_unique<Timer>(4);
    		thread thAware(&TCBmodule::startAware, this);
    		thAware.detach();
    		thread thStack(&TCBmodule::startStack, this);
    		thStack.detach();
    	}
	infilepipepath = "/tmp/ibToTCB" + to_string(CANID);
	outfilepipepath = "/tmp/TCBToOb" + to_string(CANID);
	
	killAllConn = false;
	doneKilling = false;
	
	//load inputbuffer.cpp
	ss << "./ib " << hex << CANID << " &";
	thread th(&TCBmodule::startInputLoop, this, to_string(CANID));
	th.detach();
	
	//get thread going for input buffer
	thread th2(&TCBmodule::acceptInput, this);
	th2.detach();
	
	//get thread going for MCUin
	if (!isSSM){
		thread th3(&TCBmodule::startMCUin, this, to_string(CANID));
		th3.detach();
	}
	
	if (ifs.is_open()){
		ifs.read(readIn, AES::MAX_KEYLENGTH);
		for (int i = 0; i < AES::MAX_KEYLENGTH; ++i) key[i] = (byte) readIn[i];
		ifs.close();
	}
	else{
		cerr << "ERR: can't open AES key file in TCBmodule\n";
		exit(1);
	}
}
//-------------------------------------------------------------------------------------------------
//Name: startInputLoop
//Description: function to run the ib executable
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::startInputLoop(string s){
	int status;
	ibPid = fork();
	
	switch(ibPid){
		case -1: { perror("fork"); exit(1); }
		case 0: {
			execl("./ib", s.c_str(), (const char*) NULL);
			perror("execl");
		}
		default: {
			while (waitpid(ibPid, &status, 0) == -1);
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0){
				cerr << "ERR: input buffer threading problems\n";
				exit(1);
			}
			break;
		}
	}
}
//-------------------------------------------------------------------------------------------------
//Name: startMCUin
//Description: function to run the MCUin executable
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::startMCUin(string s){
	int status;
	MCUinPid = fork();
	
	switch(MCUinPid){
		case -1: { perror("fork"); exit(1); }
		case 0: {
			execl("/usr/bin/xterm", "xterm", "-hold", "-e", "./MCUin", s.c_str(), (const char*) NULL);
			perror("execl");
		}
		default: {
			while (waitpid(MCUinPid, &status, 0) == -1);
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0){
				cerr << "ERR: MCUin threading problems\n";
				exit(1);
			}
			break;
		}
	}
}
//-------------------------------------------------------------------------------------------------
//Name: startAware
//Description: function to run the aware executable
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::startAware(){
	int status;
	awarePid = fork();
	
	switch(awarePid){
		case -1: { perror("fork"); exit(1); }
		case 0: {
			execl("/usr/bin/xterm", "xterm", "-hold", "-e", "./aware", (const char*) NULL);
			perror("execl");
		}
		default: {
			while (waitpid(awarePid, &status, 0) == -1);
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0){
				cerr << "ERR: aware threading problems\n";
				exit(1);
			}
			break;
		}
	}
}
//-------------------------------------------------------------------------------------------------
//Name: startStack
//Description: function to run the stack executable
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::startStack(){
	int status;
	stackPid = fork();
	
	switch(stackPid){
		case -1: { perror("fork"); exit(1); }
		case 0: {
			execl("/usr/bin/xterm", "xterm", "-hold", "-e", "./stack", (const char*) NULL);
			perror("execl");
		}
		default: {
			while (waitpid(stackPid, &status, 0) == -1);
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0){
				cerr << "ERR: aware threading problems\n";
				exit(1);
			}
			break;
		}
	}
}
//-------------------------------------------------------------------------------------------------
//Name: checkConn
//Description: check to see if we have a connection with the specified device
//					If not, we have to make one. If we are SSM, this connection
//					is for MCU2.
//Output: N/A
//-------------------------------------------------------------------------------------------------
bool TCBmodule::checkConn(char c){
	if (isSSM && SSMonly.empty()){
		cerr << "ERR: SSM cannot set AL-issued connections\n";
		return false;
	}

	else if (c == CANID){
		cerr << "ERR: Cannot establish connection to self\n";
		return false;
	}

	for (int i = 0; i < TCBentries.size(); ++i){
		if (TCBentries[i]->sameID(c)){
			cerr << "CONNECTION EXISTS AT " << i << endl;
			return true;
		}
	}
	
	//entry does not exist, let's get one going!
	//STEP 1
	int entryN = TCBentries.size();
	bool b[5] = {false, false, false, true, false};
	TCBentries.push_back(make_shared<TCB>(0, 0, c, 0));
	TCBentries[entryN]->makeConn = true;
	TCBentries[entryN]->prepareSwitch(0, 0, b, "");
	TCBentries[entryN]->makeOutput(CANID, outfilepipepath, "");
	TCBentries[entryN]->needAck = true;
	giveOutput(-1);
	
	return true;
}
//-------------------------------------------------------------------------------------------------
//Name: acceptInput
//Description: loop constantly for anything read in by ib. Parse input
//					based on whether message is for SSM or self.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::acceptInput(){
	struct stat buf;
	string input;
	int entryN, toSSM2, sessConnEnt;
	unsigned int s, a;
	char c, chksm[2];
	bool newStart[5] = {false, false, false, true, false}, flags[5] = {false, false, false, false, false};
	
	for(;;){
		if (stat(infilepipepath.c_str(), &buf) != -1){
			ifstream ifs(infilepipepath.c_str(), ios::in | ios::binary);
			for (string line; getline(ifs, line);){
				input.append(line);
				if (ifs.peek() && ifs.eof()) break;
				else input.append("\n");
			}
			cerr << "//-------------------------------------------------------\n";

			chksm[0] = input[10];
			chksm[1] = input[11];
			try{
				cerr << "INPUT: " << input.substr(0,8) << " " << (int)input[8] << " " << bitset<8>(input[9]) << " " << (int) input[10] << " " << (int)input[11] << " " <<input.substr(12, input.size()-12) << endl;
			} catch (...){
				cerr << "Oops! Got a little ahead of myself there.\n";
				ifs.seekg(0, ios::end);
				if (ifs.tellg() < 1){							//in case there's a file, but it's empty
					input.clear();
					remove(infilepipepath.c_str());			//clear out buffer
				}
				continue;
			}
			
			//if our checksums match, it's a good packet
			if (checkChecksum(input) && input.size() > 11){
				try{
					//extract other values from the packet
					s = stoi(input.substr(0,4), nullptr, 16);
					a = stoi(input.substr(4,4), nullptr, 16);
					c = input[8];
					bitset<8> connFlags(input[9]);
					flags[0] = connFlags[7];
					flags[1] = connFlags[6];
					flags[2] = connFlags[5];
					flags[3] = connFlags[4];
					flags[4] = connFlags[3];
				
					if (isSSM){
						entryN = entryCheckSSM(c);
						if (entryN != -1){
							if (SSMonly[entryN]->thisConnState == 4 && connFlags.to_ulong() == 128){	//MCU1's last ACK
								SSMonly[entryN]->prepareSwitch(s, a, flags, input);
								input.clear();
								remove(infilepipepath.c_str());			//clear out buffer
								continue;
							}
							//STEP 4
							c = SSMonly[entryN]->MCU1toSSM_SL(input.substr(12, 17), input.substr(29,16));
							if (SSMonly[entryN]->thisConnState == 4 && SSMonly[toSSM2]->flagStats(flags) == 2){
								//for getting ACK from FIN-ACK (going into TIME_WAIT)
								SSMonly[entryN]->prepareSwitch(s, a, flags, input);
								input.clear();
								remove(infilepipepath.c_str());			//clear out buffer
								continue;
							}
							if (c != 25){
								SSMonly[entryN]->needAck = true;	//for after MCU2 init
								//find the new entry, or make one
								toSSM2 = entryCheck(c);
								if (toSSM2 == -1){
									//notify aware alg of new connection
									ofstream ofs("/tmp/makeawareentry" + string(SSMID), ios::out);
									toSSM2 = TCBentries.size();
									TCBentries.push_back(make_shared<TCB>(0, 0, c, 2));
									sessionConns.push_back(toAndFrom(c, SSMonly[entryN]->getCanID()));
									//make aware session here
									ofs << c << SSMonly[entryN]->getCanID();
									ofs.close();
									TCBentries[toSSM2]->makeConn = true;
									TCBentries[toSSM2]->prepareSwitch(0, 0, newStart, input);
									TCBentries[toSSM2]->makeOutput(CANID, outfilepipepath, "");
									TCBentries[toSSM2]->needAck = true;
									SSMtoMCU2 = true;
									giveOutput(toSSM2);
									//now update the MCU1/SSM connection
									if (connFlags.to_ulong() == 192){	//if PSHACK
										SSMonly[entryN]->currentInput.clear();
										SSMonly[entryN]->currentInput = input;
										SSMonly[entryN]->needAck = true;
										SSMonly[entryN]->prepareSwitch(s, a, flags, input);
									}
								}
								else if (TCBentries[toSSM2]->thisConnState == 3 && SSMonly[entryN]->thisConnState < 3){
									ofstream ofs("/tmp/makeawareentry" + string(SSMID), ios::out);
									SSMonly[entryN]->currentInput.clear();
									SSMonly[entryN]->currentInput = input;
									SSMonly[entryN]->prepareSwitch(s, a, flags, input);
									SSMonly[entryN]->makeOutput(CANID, outfilepipepath, "");
									giveOutput(entryN);
									sessionConns.push_back(toAndFrom(c, SSMonly[entryN]->getCanID()));
									ofs << c << SSMonly[entryN]->getCanID();
									ofs.close();
								}
								else{	//MCU1 to SSM data
									toSSM2 = entryCheck(c);
									if (!SSMonly[entryN]->prepareSwitch(s, a, flags, input) && isSSM){
										retransmissionsSSM(s);
										input.clear();
										remove(infilepipepath.c_str());			//clear out buffer
										continue;
									}
									SSMonly[entryN]->needAck = true;
									if (TCBentries[toSSM2]->flagStats(flags) == 7){
										TCBentries[toSSM2]->killConn = true;
										TCBentries[toSSM2]->prepareSwitch(TCBentries[toSSM2]->seq, TCBentries[toSSM2]->ackn, flags, "");
									}
									else TCBentries[toSSM2]->sending = true;
									TCBentries[toSSM2]->makeOutput(CANID, outfilepipepath, input.substr(29, input.size()-29));
									TCBentries[toSSM2]->needAck = true;
									SSMtoMCU2 = true;
									giveOutput(toSSM2);
									sessConnEnt = findMatchingSessConn(TCBentries[toSSM2]->getCanID(), SSMonly[entryN]->getCanID());
									sessionConns[sessConnEnt].transmitting = true;
								}
							}
						}
						else if (entryCheck(c) != -1){
							//STEP 6
							entryN = entryCheck(c);
							char q = TCBentries[entryN]->SSMtoMCU2_SL(input.substr(12, 17));
							if (q == 50){
								TCBentries[entryN]->prepareSwitch(s, a, flags, input);
								TCBentries[entryN]->makeOutput(CANID, outfilepipepath, "");
								TCBentries[entryN]->needAck = true;
								SSMtoMCU2 = true;
								giveOutput(entryN);
								TCBentries[entryN]->needAck = false;
							}
							//STEP 8
							if (q == 30 && connFlags.to_ulong() == 128){	//if ACK
								toSSM2 = entryN;
								entryN = findMatchingAckSSM(c);
								sessConnEnt = findMatchingSessConn(TCBentries[toSSM2]->getCanID(), SSMonly[entryN]->getCanID());
								if (sessionConns[sessConnEnt].transmitting){
									ofstream ofs("/tmp/newawareentry"+string(SSMID), ios::out);
									ofs << TCBentries[toSSM2]->getCanID() << SSMonly[entryN]->getCanID() << " " << TCBentries[toSSM2]->latestFV;//send factor to the awareness
									ofs.close();
									sessionConns[sessConnEnt].transmitting = false;
								}
								if (entryN != -1){
									SSMonly[entryN]->makeOutput(CANID, outfilepipepath, "");
									giveOutput(entryN);
								}
								//update TCB entry for SSM to MCU2 conn
								TCBentries[toSSM2]->needAck = false;
								TCBentries[toSSM2]->currentInput.clear();
								TCBentries[toSSM2]->currentInput = input;
								TCBentries[toSSM2]->prepareSwitch(s, a, flags, input);
							}
							if (q == 30 && connFlags.to_ulong() == 136){	//if FIN-ACK
								toSSM2 = entryN;
								entryN = findMatchingAckSSM(c);
								if (entryN != -1){
									cerr << "FIN-ACK\n";						//SSMonly[entryN]->killed
									SSMonly[entryN]->makeOutput(CANID, outfilepipepath, "");
									giveOutput(entryN);
								}
								//update TCB entry for SSM to MCU2 conn
								TCBentries[toSSM2]->needAck = false;
								TCBentries[toSSM2]->currentInput.clear();
								TCBentries[toSSM2]->currentInput = input;
								TCBentries[toSSM2]->prepareSwitch(s, a, flags, input);
								//***WARNING!*** This sleep function is needed!
								//ob needs enough time to deliver MCU1's message first!
								this_thread::sleep_for(chrono::microseconds(40000));
								//if you remove this sleep line, MCU1 gets whatever MCU2 gets as well
								//This delay can be removed when this code is weaned off file pipes
								TCBentries[toSSM2]->makeOutput(CANID, outfilepipepath, "");
								SSMtoMCU2 = true;
								giveOutput(toSSM2);
							}
						}
						else{
							//STEP 2
							entryN = SSMonly.size();
							SSMonly.push_back(make_shared<TCB>(input.size(), 0, c, 2));
							if (SSMonly[entryN]->MCU1toSSM_SL(input.substr(12, input.size()-12), "") == 50){
								SSMonly[entryN]->prepareSwitch(s, a, flags, input);
								SSMonly[entryN]->makeOutput(CANID, outfilepipepath, "");
								SSMonly[entryN]->needAck = true;
								giveOutput(entryN);
								SSMonly[entryN]->needAck = false;
							}
						}
					}
					else{	//NOT SSM
						//STEP 3 (MCU1), STEP 5 (MCU2)
						//we are assuming that the newest entry on the stack is for the waiting connection
						entryN = entryCheck(c);
						if (entryN == -1){
							if (c == 34){ //for MCU1!
								entryN = findMatchingAck(a);
								if (connFlags.to_ulong() == 32){				//RST flag
									for (int i = 0; i < TCBentries.size(); ++i){
										if (TCBentries[i]->seq == a){			//go by ACK number for now
											TCBentries[i]->prepareSwitch(s, a, flags, input);
										}
									}
								}
								else if (TCBentries.size() == 0 || entryN == -1){	//for MCU2
									entryN = TCBentries.size();
									TCBentries.push_back(make_shared<TCB>(input.size(), 0, c, 1));
									cerr << "TCB pointer: " << TCBentries[entryN] << endl;
									if (TCBentries[entryN]->SSMtoMCU2_SL(input.substr(12, input.size()-12)) == 50){
										if (TCBentries[entryN]->isMCU1()) TCBentries[entryN]->makeConn = true;
										TCBentries[entryN]->prepareSwitch(s, a, flags, input);
										TCBentries[entryN]->makeOutput(CANID, outfilepipepath, "");
										TCBentries[entryN]->needAck = true;
										giveOutput(-1);
									}
								}
								//after step 8, stop the initializing chain
								else if (TCBentries[entryN]->flagStats(flags) == 2){
									if (TCBentries[entryN]->prepareSwitch(s, a, flags, input)){
										input.clear();
										remove(infilepipepath.c_str());
										continue;
									}
									else cerr << "dun goofed\n";
								}
								else if(TCBentries[entryN]->prepareSwitch(s, a, flags, input)){
									TCBentries[entryN]->makeOutput(CANID, outfilepipepath, "");
									TCBentries[entryN]->needAck = true;
									giveOutput(entryN);
								}
							}
							else{
								cerr << "SOMETHING ODD\n";
							}
						}
						else{//STEP 7 (MCU2), MCU2 also receives PSH data here
							if (TCBentries[entryN]->needAck && TCBentries[entryN]->prepareSwitch(s, a, flags, input)){
								char q = TCBentries[entryN]->SSMtoMCU2_SL(input.substr(12, 17));
								if (TCBentries[entryN]->isMCU2() && TCBentries[entryN]->thisConnState == 3 && q == 60) TCBentries[entryN]->SLtoAL(CANID);
								else if (q == 25){
									input.clear();
									remove(infilepipepath.c_str());
									continue;
								}
								if (TCBentries[entryN]->isMCU2() && TCBentries[entryN]->thisConnState == 5 && connFlags.to_ulong() == 128){	//MCU2 getting ACK after FIN-ACK
									input.clear();
									remove(infilepipepath.c_str());
									continue;
								}
								else {
									TCBentries[entryN]->makeOutput(CANID, outfilepipepath, "");
									TCBentries[entryN]->needAck = true;
									giveOutput(-1);
								}
							}
						}
					}
				} catch(std::out_of_range& e){
					cerr << "ERR CODE 1\n";		//for the ugly chars, testing eval
				}
				catch(exception& ex){
					cerr << "GENERAL ERROR\n" << ex.what() << endl;			//for other
				}
			}
			else{
				cerr << "ERR CODE 2\n";
			}
			input.clear();
			remove(infilepipepath.c_str());			//clear out buffer
		}
		//logic here for any necessary retransmissions
		retransmissions();
		//logic here for erasing entries when time_wait expires
		for (int i = 0; i < SSMonly.size(); ++i){
			if (SSMonly[i]->time_wait->isExpired() && SSMonly[i]->thisConnState == 5){
				cerr << "NOT: Connection " << SSMonly[i]->getCanID() << " in SSMonly vector removed\n";
				SSMonly.erase(SSMonly.begin()+i);
			}
		}
		for (int i = 0; i < TCBentries.size(); ++i){
			if (TCBentries[i]->time_wait->isExpired() && TCBentries[i]->thisConnState == 5){
				cerr << "NOT: Connection " << TCBentries[i]->getCanID() << " in TCBentries vector removed\n";
				TCBentries.erase(TCBentries.begin()+i);
			}
		}
		//logic here for if a hack has been discovered
		if (isSSM && (stat(helppath.c_str(), &buf) != -1)) discoveredHack();
		//logic here to check on the hacks
		if (isSSM) checkHacks();
		//logic here for if it's time to end the program
		if (killAllConn) endConnectionLoop();
	}
}
//-------------------------------------------------------------------------------------------------
//Name: retransmissions
//Description: MCU1 only. If no ACK was received, send data again.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::retransmissions(){
	for (int i = 0; i < TCBentries.size(); ++i){
		if (TCBentries[i]->isMCU1() && TCBentries[i]->needAck && TCBentries[i]->retrans->isExpired()){
			ofstream ofs(outfilepipepath.c_str(), ios::out | ios::binary);
			ofs << TCBentries[i]->currentOutput;
			ofs.flush();
			giveOutput(-1);
			TCBentries[i]->retrans->reset();
			cerr << "NOT: had to resend packet to " << TCBentries[i]->getCanID() << " b/c timer expired\n";
			//***WARNING!*** This sleep function is needed!
			//ob needs enough time to deliver MCU1's message first!
			this_thread::sleep_for(chrono::microseconds(40000));
			//if you remove this sleep line, and there are multiple expired timers, not all messages may be sent
			//This delay can be removed when this code is weaned off file pipes
		}
	}
}
//-------------------------------------------------------------------------------------------------
//Name: retransmissionsSSM
//Description: SSM only. If MCU1 never got that previous ACK, it will show in the retransmission input.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::retransmissionsSSM(int s){
	for (int i = 0; i < SSMonly.size(); ++i){
		if (SSMonly[i]->ackn == s){
			ofstream ofs(outfilepipepath.c_str(), ios::out | ios::binary);
			ofs << SSMonly[i]->currentOutput;
			ofs.flush();
			giveOutput(i);
			SSMonly[i]->retrans->reset();
			cerr << "NOT: had to resend ACK packet to " << TCBentries[i]->getCanID() << " b/c timer expired\n";
			//***WARNING!*** This sleep function is needed!
			//ob needs enough time to deliver MCU1's message first!
			this_thread::sleep_for(chrono::microseconds(40000));
			//if you remove this sleep line, and there are multiple expired timers, not all messages may be sent
			//This delay can be removed when this code is weaned off file pipes
			break;
		}
	}
}
//-------------------------------------------------------------------------------------------------
//Name: findMatchingSessConn
//Description: SSM only. Check for matching session connection.
//Output: Array index of connection, or -1 if it doesn't exist
//-------------------------------------------------------------------------------------------------
int TCBmodule::findMatchingSessConn(char t, char f){
	for (int i = 0; i < sessionConns.size(); ++i){
		if (sessionConns[i].to == t && sessionConns[i].from == f) return i;
	}
	cerr << "WARN: returning -1 from findMatchingSessConn(), either new entry or this could be bad!\n";
	return -1;
}
//-------------------------------------------------------------------------------------------------
//Name: findMatchingAck
//Description: MCU1 only. (?) Check existing connections to match up
//					entry from SSM to entry for MCU2.
//Output: Array index of connection, or -1 if it doesn't exist
//-------------------------------------------------------------------------------------------------
int TCBmodule::findMatchingAck(int a){
	int entryN = -1;
	for (int i = 0; i < TCBentries.size(); ++i){
		if (TCBentries[i]->seq == a && TCBentries[i]->needAck){
			entryN = i;
			TCBentries[entryN]->needAck = false;
			break;
		}
	}
	if (entryN == -1) cerr << "WARN: returning -1 from findMatchingAck(), either new entry or this could be bad!\n";
	return entryN;
}
//-------------------------------------------------------------------------------------------------
//Name: findMatchingAckSSM
//Description: SSM only. Check existing connections to match up
//					entry from SSM to entry for MCU1.
//Output: Array index of connection, or -1 if it doesn't exist
//-------------------------------------------------------------------------------------------------
int TCBmodule::findMatchingAckSSM(char c){
	int entryN = -1;
	for (int i = 0; i < SSMonly.size(); ++i){
		if (SSMonly[i]->needAck && SSMonly[i]->MCU2dest() == c){
			entryN = i;
			SSMonly[entryN]->needAck = false;
			break;
		}
	}
	if (entryN == -1) cerr << "WARN: returning -1 from findMatchingAckSSM(), either new entry or this could be bad!\n";
	return entryN;
}
//-------------------------------------------------------------------------------------------------
//Name: entryCheck
//Description: Check to see if we have a TCB connection entry in our TCBentries
//					vector.
//Output: Array index of connection, or -1 if it doesn't exist
//-------------------------------------------------------------------------------------------------
int TCBmodule::entryCheck(char c){
	int entryN = -1;
	for (int i = 0; i < TCBentries.size(); ++i){
		if (TCBentries[i]->sameID(c)){
			entryN = i;
			break;
		}
	}
	return entryN;
}
//-------------------------------------------------------------------------------------------------
//Name: entryCheckSSM
//Description: Same as entryCheck, but for the SSM and SSMonly vector.
//Output: Array index of connection, or -1 if it doesn't exist.
//-------------------------------------------------------------------------------------------------
int TCBmodule::entryCheckSSM(char c){
	int entryN = -1;
	for (int i = 0; i < SSMonly.size(); ++i){
		if (SSMonly[i]->sameID(c)){
			entryN = i;
			break;
		}
	}
	return entryN;
}
//-------------------------------------------------------------------------------------------------
//Name: giveOutput
//Description: Send our packets out to ob. System command depends on
//					whether or not we are SSM, and if we are, whether or not
//					we are sending to MCU1 or MCU2.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::giveOutput(int index){
	stringstream ss;
	int sysStat;
	
	//MCU1/2 to SSM
	if (!isSSM && index == -1) ss << "./ob " << dec <<  34 << " " << (int)CANID << " &";
	//SSM to MCU1
	else if (isSSM && !SSMtoMCU2) ss << "./ob " << dec <<  (unsigned) SSMonly[index]->getCanID() << " " << 34 << " &";
	//SSM to MCU2
	else if (isSSM && SSMtoMCU2){
		//cerr << "whoops\n";
		ss << "./ob " << dec <<  (unsigned) TCBentries[index]->getCanID() << " " << 34 << " &";
		SSMtoMCU2 = false;
	}
	else ss << "./ob " << dec <<  34 << " " << (int)CANID << " &";
	
	sysStat = system(ss.str().c_str());
	if (sysStat < 0 && sysStat >= 127){
		cerr << "ERR: unable to run command " << ss.str() << ", errno " << errno << endl;
	}
}
//-------------------------------------------------------------------------------------------------
//Name: msgFromAL
//Description: Our message from MCUout to be sent. (application layer)
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::msgFromAL(char destID, string msg){
	int entryN = entryCheck(destID);
	string msgenc;
	
	try{
		ECB_Mode< AES >::Encryption e;
		e.SetKey(key, sizeof(key));
		StringSource(msg, true, new StreamTransformationFilter(e, new StringSink(msgenc)));
		TCBentries[entryN]->sending = true;
		TCBentries[entryN]->makeOutput(CANID, outfilepipepath, msgenc);
		TCBentries[entryN]->needAck = true;
		giveOutput(-1);
	}
	catch(const CryptoPP::Exception& e){
		cerr << "ERR: MSGFROMAL ENCRYPTION GONE WRONG\n" << e.what() << "\nNO PACKET SENT\n";
	}
}
//-------------------------------------------------------------------------------------------------
//Name: checkChecksum
//Description: Check to see if the checksum for the incoming packet
//					is valid. (deprecated for now)
//Output: true if valid checksum, false otherwise
//-------------------------------------------------------------------------------------------------
bool TCBmodule::checkChecksum(string in){
	string check1, check2, tmp = checksum(in);
	check1 = tmp.substr(0,8);
	check2 = tmp.substr(8,8);
	
	return (static_cast<char>(std::stoi(check1, nullptr, 2)) == in[10]) && (static_cast<char>(std::stoi(check2, nullptr, 2)) == in[11]);
}
//-------------------------------------------------------------------------------------------------
//Name: checksum
//Description: Performs the calculation of the checksum on the packet.
//Output: the checksum in binary (string) form
//-------------------------------------------------------------------------------------------------
string TCBmodule::checksum(string in){
	string tmp, check1, check2;
	unsigned int total, t = 0, q = 0;
	vector<int> num;
	
	//clear checksum in frame
	in[10] = (char) 0;
	in[11] = (char) 0;
	
	for (int j = 0; j < in.length(); ++j){
		tmp.clear();
		for (int i = 7; i >= 0; --i) tmp += ((in[j] & (1 << i))? '1' : '0');
		++j;
    		if (j+1 == in.length()){
    			tmp.append("00000000");
    			break;
    		}
    		for (int i = 7; i >= 0; --i) tmp += ((in[j] & (1 << i))? '1' : '0');
    		q = bitset<16>(tmp).to_ulong();
    		num.push_back(q);
 	}
  
  	for (int i = 0; i < num.size(); ++i) t += num[i];
  
  	while (t>>16) t = (t & 0xffff) + (t >> 16);
  
 	t = 0xffff - t;
 	
 	bitset<16> bits (t);
	tmp.clear();
	return bits.to_string();
}
//-------------------------------------------------------------------------------------------------
//Name: endAllConnections
//Description: When disconnecting, first gracefully terminate all connections.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::endAllConnections(){
	for(;;){
		killAllConn = true;
		if (doneKilling) return;	//done with ending all connections
	}
}
//-------------------------------------------------------------------------------------------------
//Name: endConnectionLoop
//Description: Going through our TCBentries vector, gracefully terminate each connection.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::endConnectionLoop(){
	struct stat buf;
	string input;
	int entryN, toSSM2, tmp;
	unsigned int s, a;
	char c, chksm[2];
	bool newStart[5] = {false, false, false, true, false}, flags[5] = {false, false, false, false, false};
	
	bool f[5] = {false, false, false, false, true};
	for (int i = 0; i < TCBentries.size(); ++i){
		if (TCBentries[i]->thisConnState == 3){
			TCBentries[i]->killConn = true;
			TCBentries[i]->TCBswitch(f);
			TCBentries[i]->makeOutput(CANID, outfilepipepath, "");
			TCBentries[i]->needAck = true;
			giveOutput(-1);
			for (;;){
				if (stat(infilepipepath.c_str(), &buf) != -1){
					ifstream ifs(infilepipepath.c_str(), ifstream::in | ios::binary);
					ifs >> input;
					cerr << "//-------------------------------------------------------\n";
					cerr << "END INPUT: " << input.substr(0,8) << " " << (int)input[8] << " " << bitset<8>(input[9]) << endl;
							
					//if our checksums match, it's a good packet
					chksm[0] = input[10];
					chksm[1] = input[11];
					if (checkChecksum(input)){
						//extract other values from the packet
						s = stoi(input.substr(0,4), nullptr, 16);
						a = stoi(input.substr(4,4), nullptr, 16);
						c = input[8];
						bitset<8> connFlags(input[9]);
						f[0] = connFlags[7];
						f[1] = connFlags[6];
						f[2] = connFlags[5];
						f[3] = connFlags[4];
						f[4] = connFlags[3];
						if (connFlags.to_ulong() == 136 && TCBentries[i]->prepareSwitch(s, a, f, input)){
							TCBentries[i]->makeOutput(CANID, outfilepipepath, "");
							giveOutput(-1);
							usleep(50);				//give ob a chance to work!
							break;						//done with this entry
						}
					}
					retransmissions();
					input.clear();
					remove(infilepipepath.c_str());			//clear out buffer
				}
			}
		}
	}
	input.clear();
	remove(infilepipepath.c_str());			//clear out buffer
	doneKilling = true;
	return;
}
//-------------------------------------------------------------------------------------------------
//Name: discoveredHack
//Description: If the stack notifies us of a hack, do something.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::discoveredHack(){
	ifstream ifs(helppath.c_str(), ios::in | ios::binary);
	char culprit, victim;
	string input, lastData;
	cerr << "~~~HACK DETECTED~~~\n";
			
	for (string line; getline(ifs, line);){
		input.append(line);
		if (ifs.peek() && ifs.eof()) break;
		else input.append("\n");
	}
	victim = input[0];
	culprit = input[1];
	lastData = input.substr(3, 16);
	//do RST here
	for (int i = 0; i < SSMonly.size(); ++i){
		if (SSMonly[i]->getCanID() == culprit){
			bool b[5] = {false, false, true, false, false};
			SSMonly[i]->hardReset = true;
			SSMonly[i]->TCBswitch(b);
			SSMonly[i]->makeOutput(CANID, outfilepipepath, "");
			giveOutput(i);
			hacks.push_back(hackInfo(victim, culprit, lastData));
			for (int j = 0; j < sessionConns.size(); ++j){
				if (sessionConns[i].from == culprit){
					ofstream ofs("/tmp/delaware"+string(SSMID), ios::out);
					ofs << sessionConns[i].to << sessionConns[i].from;
					sessionConns.erase(sessionConns.begin()+i);
				}
			}
		}
	}
	remove(helppath.c_str());
}
//-------------------------------------------------------------------------------------------------
//Name: checkHacks
//Description: Check on status of [un]resolved hacks.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCBmodule::checkHacks(){
	for (int i = 0; i < hacks.size(); ++i){
		if (hacks[i].hack_timer->isExpired() && hacks[i].resend->isExpired()){	//do we need archived data?
			bool b[5] = {false, true, false, false, false};
			int toSSM2 = entryCheck(hacks[i].to);
			TCBentries[toSSM2]->TCBswitch(b);
			TCBentries[toSSM2]->makeOutput(CANID, outfilepipepath, hacks[i].lastAL);
			SSMtoMCU2 = true;
			giveOutput(toSSM2);
			//***WARNING!*** This sleep function is needed!
			//ob needs enough time to deliver MCU1's message first!
			this_thread::sleep_for(chrono::microseconds(40000));
			//if you remove this sleep line, MCU1 will not get the RST message
			//This delay can be removed when this code is weaned off file pipes
			hacks[i].resend->reset();
		}
		else{
			for (int j = 0; j < SSMonly.size(); ++j){
				for (int k = 0; k < hacks.size(); ++k){
					if (SSMonly[j]->getCanID() == hacks[k].from && SSMonly[j]->thisConnState != 5){
						cerr << "hack threat gone\n";
						hacks.erase(hacks.begin()+k);
						break;
					}
				}
			}
		}
	}
}
