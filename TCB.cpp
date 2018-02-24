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

#include "TCB.hpp"

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

//Name: diagInfo
//Description: Pipe out packet data.
//Output: Concatnated strings for transport layer
//-------------------------------------------------------------------------------------------------
string TCB::diagInfo(){
	stringstream ss, ss2;
	char c = 0;
	
	ss <<  setw(4) << setfill('0') << hex << seq << setw(4) << setfill('0') << ackn;
	
	if (didSYNACK && !establishing && !isEstab) c = 0b00010000;
	else if (!didSYNACK && establishing && !isEstab) c = 0b10010000;
	else if (didSYNACK && isEstab && !sending) c = 0b11000000;
	else if (isEstab && isVerif && !sending && !killConn && !killed) c = 0b10000000;
	else if (isEstab && isVerif && sending && !killConn) c = 0b01000000;
	else if (hardReset) c = 0b00100000;
	else if (killConn && !killed) c = 0b00001000;
	else if (!killConn && killed)c = 0b10001000;
	else if (killConn && killed) c = 0b10000000;
	
	ss2 << '0' << c << "00";
	return ss.str() + ss2.str();
}
//-------------------------------------------------------------------------------------------------
//Name: relation
//Description: Are we MCU1, MCU2, or SSM?
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::relation(int MCU){
	ifstream ifs("my.key", ios::in);
	char readIn[32];
	string tmp;

	if (MCU == 0) MCU1 = true;
	else if (MCU == 1) MCU2 = true;
	
	cerr << "NOT: Initialized TCBentry for ID " << canID;
	if (MCU1 && !MCU2) cerr << " oriented as MCU1\n";
	else if (!MCU1 && MCU2) cerr << " oriented as MCU2\n";
	else if (!MCU1 && !MCU2) cerr << " oriented as SSM\n";
	
	if (ifs.is_open()){
		ifs.read(readIn, AES::MAX_KEYLENGTH);
		for (int i = 0; i < AES::MAX_KEYLENGTH; ++i) key[i] = (byte) readIn[i];
		ifs.close();
	}
	else{
		cerr << "ERR: can't open AES key file in stack application\n";
		exit(1);
	}
}
//-------------------------------------------------------------------------------------------------
//Name: updateStats
//Description: Increment the seq and ack numbers for the TCB entry.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::updateStats(unsigned int ds, unsigned int da){
	seq += ds;
	ackn += da;
}
//-------------------------------------------------------------------------------------------------
//Name: checkStats
//Description: Change TCB entry's connection flags before running
//					the switch.
//Output: 0 if all packet variables match TCB entry, or error code if one does not
//-------------------------------------------------------------------------------------------------
int TCB::checkStats(unsigned int inSeq, unsigned int inAckn, char c){
	if (c != canID) return 1;
	else if (seq > 0 && ((inAckn+currentInput.size()) != inSeq)) return 2;
	else if (ackn != 0 && inAckn != seq) return 3;
	return 0;
}
//-------------------------------------------------------------------------------------------------
//Name: flagStats
//Description: Check the current status of the flags in the TCB entry.
//Output: flag status code
//-------------------------------------------------------------------------------------------------
int TCB::flagStats(bool f[5]){
	//cerr << "FLAGSTATS: " << f[0] << f[1] << f[2] << f[3] << f[4] << endl;
	if (equal(f, f+4, SYNonly)) return 0;
	if (equal(f, f+4, SYNACK)) return 1;
	if (equal(f, f+4, ACKonly)) return 2;
	if (equal(f, f+4, RSTonly)) return 3;
	if (equal(f, f+4, FINACK)) return 4;
	if (equal(f, f+4, PSHonly)) return 5;
	if (equal(f, f+4, PSHACK)) return 6;
	if (equal(f, f+4, FINonly)) return 7;
	if (f[2]) return 8;
	if (f[1]) return 9;
	
	return -1;		//unknown
}
//-------------------------------------------------------------------------------------------------
//Name: prepareSwitch
//Description: Do input checking here for switch.
//Output: N/A
//-------------------------------------------------------------------------------------------------
bool TCB::prepareSwitch(int s, int a, bool f[5], string input){
	int tmp;
	
	if (input.size() > 0) currentInput = input;
	tmp = checkStats(s, a, canID);
	if (tmp > 0){
		cerr << "ERR: incoming packet for " << canID << " rejected ";
		if (tmp == 1) cerr << "because source CAN ID is wrong\n";
		else if (tmp > 1) cerr << "due to ERR CODE 3\n";
		return false;
	}
	else {
		ackn = s;
		seq = s;
		TCBswitch(f);
		return true;
	}
}
//-------------------------------------------------------------------------------------------------
//Name: switch
//Description: Do input checking here for switch.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::TCBswitch(bool f[5]){
	switch(thisConnState){
		case CLOSED: { caseClosed(f); break; }
		case SYN_SENT: { caseSynSent(f); break; }
		case SYN_RCVD: { caseSynRcvd(f); break; }
		case ESTABLISHED: { caseEstablished(f); break; }
		case FIN_WAIT: { caseFinWait(f); break; }
		case TIME_WAIT: { caseTimeWait(f); break; }
		default:{ break; }
	}
	cerr << "TCS OF " << (int)canID << ": " << thisConnState << endl;
}
//-------------------------------------------------------------------------------------------------
//Name: caseClosed
//Description: thisConnState = CLOSED
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::caseClosed(bool f[5]){
	if (flagStats(f) == 0 && !makeConn){
		thisConnState = SYN_RCVD;
		establishing = true;
		isVerif = true;
	}
	else if (flagStats(f) == 0 && makeConn){
		thisConnState = SYN_SENT;
		makeConn = false;
		didSYNACK = true;
	}
	else{
		hardReset = true;
	}
}
//-------------------------------------------------------------------------------------------------
//Name: caseSynSent
//Description: thisConnState = SYN_SENT
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::caseSynSent(bool f[5]){
	if (retrans->isExpired()){
		thisConnState = CLOSED;
	}
	else if (flagStats(f) == 0){
		thisConnState = SYN_RCVD;
	}
	else if (flagStats(f) == 1){
		thisConnState = ESTABLISHED;
		isEstab = true;
		isVerif = true;
	}
}
//-------------------------------------------------------------------------------------------------
//Name: caseSynRcvd
//Description: thisConnState = SYN_RCVD
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::caseSynRcvd(bool f[5]){
	if (flagStats(f) == 6 || flagStats(f) == 2){
		thisConnState = ESTABLISHED;
		
		if (MCU2){
			establishing = false;
			isEstab = true;
			isVerif = true;
		}
		else if (!MCU1 && !MCU2 && establishing){
			isEstab = true;
			isVerif = true;
		}
	}
	else if (retrans->isExpired()){
		cerr << "expired\n";
	}
	else if (flagStats(f) == 3){
		thisConnState = CLOSED;
		isEstab = false;
		isVerif = false;
	}
}
//-------------------------------------------------------------------------------------------------
//Name: caseEstablished
//Description: thisConnState = ESTABLISHED
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::caseEstablished(bool f[5]){
	//for FIN (dest)
	if (flagStats(f) == 7){
		thisConnState = FIN_WAIT;
		if (!killConn) killed = true;
	}
	//for SSM's RST
	else if (hardReset){
		killed = true;
		thisConnState = TIME_WAIT;
		time_wait->reset();
	}
	//for RST
	else if (flagStats(f) == 3){
		thisConnState = TIME_WAIT;
		time_wait->reset();	
	}
	//for FIN (sender)
	else if (killConn) thisConnState = FIN_WAIT;
}
//-------------------------------------------------------------------------------------------------
//Name: caseFinWait
//Description: thisConnState = FIN_WAIT
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::caseFinWait(bool f[5]){
	if (flagStats(f) == 2){
		thisConnState = TIME_WAIT;
		killed = true;
		time_wait->reset();
	}
}
//-------------------------------------------------------------------------------------------------
//Name: caseTimeWait
//Description: thisConnState = TIME_WAIT
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::caseTimeWait(bool f[5]){
	if (time_wait->isExpired() || flagStats(f) == 2) thisConnState = CLOSED;
}
//-------------------------------------------------------------------------------------------------
//Name: makeOutput
//Description: Make our output to give to TCBmodule
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::makeOutput(char src, string outfilepipepath, string al){
	ofstream ofs(outfilepipepath.c_str(), ios::out | ios::binary);
	stringstream ss;
	string tmp, SLenc, check1, check2;
	
	currentOutput.clear();
	ss << diagInfo();
	
	//for MCU1 starting connection to SSM ***OR*** SSM starting connection to MCU2
	if ((MCU1 && !MCU2 && didSYNACK && !isEstab) ||
		(!MCU1 && !MCU2 && didSYNACK && !isEstab)){
			try{
				ECB_Mode< AES >::Encryption e;
				e.SetKey(key, sizeof(key));
				StringSource("mastersthesis", true, new StreamTransformationFilter(e, new StringSink(SLenc)));
				ss << 'B' << SLenc;
			}
			catch(const CryptoPP::Exception& e){
				cerr << "ERR: MAKEOUTPUT ENCRYPTION GONE WRONG\n" << e.what() << "\nEXITING\n";
				exit(1);
			}
		}
	//for SSM acking MCU1 ***OR *** MCU2 acking SSM (the latter is init only)
	else if ((!MCU1 && !MCU2 && establishing && !isEstab) ||
		(!MCU1 && MCU2 && establishing)){
			try{
				ECB_Mode< AES >::Encryption e;
				e.SetKey(key, sizeof(key));
				StringSource("2", true, new StreamTransformationFilter(e, new StringSink(SLenc)));	//(char) 50
				ss << 'A' << SLenc;
			}
			catch(const CryptoPP::Exception& e){
				cerr << "ERR: MAKEOUTPUT ENCRYPTION GONE WRONG\n" << e.what() << "\nEXITING\n";
				exit(1);
			}
		}
	//for MCU1 sending data to SSM
	else if (MCU1 && !MCU2 && didSYNACK && isEstab && isVerif && !killed){
			try{
				ECB_Mode< AES >::Encryption e;
				e.SetKey(key, sizeof(key));
				StringSource(string(1,canID), true, new StreamTransformationFilter(e, new StringSink(SLenc)));
				ss << 'D' << SLenc;
			}
			catch(const CryptoPP::Exception& e){
				cerr << "ERR: MAKEOUTPUT ENCRYPTION GONE WRONG\n" << e.what() << "\nEXITING\n";
				exit(1);
			}
		}
	//for MCU2 acking w/ latestFVenc
	else if (!MCU1 && MCU2 && lastValue != -1 && isEstab && isVerif && !killed){
		ss << 'C' << latestFVenc;
	}
	//for SSM sending data to MCU2
	else if (!MCU1 && !MCU2 && isEstab && isVerif && sending){
		if (lastValueEnc.empty()){
			try{
				ECB_Mode< AES >::Encryption e;
				e.SetKey(key, sizeof(key));
				StringSource("0", true, new StreamTransformationFilter(e, new StringSink(SLenc)));
				ss << 'E' << SLenc;
				lastValueEnc.assign(al);
			}
			catch(const CryptoPP::Exception& e){
				cerr << "ERR: MAKEOUTPUT ENCRYPTION GONE WRONG\n" << e.what() << "\nEXITING\n";
				exit(1);
			}
		}
		else{
			ss << "E" << lastValueEnc;
			lastValueEnc.assign(al);
		}
	}
	
	//finally, application layer data
	ss << al;
	
	currentOutput = ss.str();
	currentOutput[8] = src;	//source CAN ID
	
	if (!hardReset) updateStats(currentOutput.size(), 0);
	else updateStats(currentOutput.size(), seq-ackn);
	
	ss.str(string(""));
	ss.clear();
	ss << diagInfo();
	tmp = ss.str().substr(0,4);
	currentOutput.replace(0, 4, tmp);
	tmp.clear();
	if (hardReset){
		tmp = ss.str().substr(4,4);
		currentOutput.replace(4, 4, tmp);
		tmp.clear();
	}
	
	tmp = checksum(currentOutput);
	check1 = tmp.substr(0,8);
	check2 = tmp.substr(8,8);
	currentOutput[10] = static_cast<char>(std::stoi(check1, nullptr, 2));
	currentOutput[11] = static_cast<char>(std::stoi(check2, nullptr, 2));
	
	cerr << "OUTPUT OF " << canID << ": " << currentOutput << endl;
	ofs << currentOutput;
	ofs.flush();
	
	retrans->reset();
	keepalive->reset();
}
//-------------------------------------------------------------------------------------------------
//Name: checksum
//Description: Performs the calculation of the checksum on the packet.
//Output: the checksum in binary (string) form
//-------------------------------------------------------------------------------------------------
string TCB::checksum(string in){
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
//Name: MCU1toSSM_SL
//Description: Handles session layer between MCU1 and SSM.
//Output: see return values
//-------------------------------------------------------------------------------------------------
char TCB::MCU1toSSM_SL(string sl, string pay){
	string decoded;
	
	if (sl.size() == 0) return 30;
	
	//decrypt and then run the switch
	try{
		ECB_Mode< AES >::Decryption d;
		d.SetKey(key, sizeof(key));
		StringSource s(sl.substr(1,sl.size()-1), true, new StreamTransformationFilter(d, new StringSink(decoded)));
	}
	catch(const CryptoPP::Exception& e){
		cerr << "ERR: MCU1TOSSM_SL DECRYPTION GONE WRONG\n" << e.what() << "\nREJECTING (ERR CODE 4)\n";
		return 25;
	}
	if (sl[0] == 'A'){
		if (decoded.compare("2") == 0){ 						//because this is a proof of concept
			isVerif = true;
			return 50;
		}
		else if (decoded.compare("1") == 0){
			cerr << "ERR: Connection rejected by SSM - password incorrect?\n";
		}
	}
	else if (sl[0] == 'B'){//cerr << sl << endl;
		if (decoded.compare("mastersthesis") == 0){
			isVerif = true;
			return 50;
		}
	}
	else if (sl[0] == 'D'){
		if (pay.size() > 0){
			ofstream ofs("/tmp/newstackentry"+string(SSMID), ios::out);
			//send lastvalue to the stack, need four entries: time, to (MCU2), from (MCU1), val
			ofs << time(NULL) << " " << (int)decoded[0] << " " << (int)canID << " " << pay;
		}
		return decoded[0];
	}
	return 25;			//rejected, or not applicable
}
//-------------------------------------------------------------------------------------------------
//Name: SSMtoMCU2_SL
//Description: Handles session layer between SSM and MCU2.
//Output: see return values
//-------------------------------------------------------------------------------------------------
char TCB::SSMtoMCU2_SL(string sl){
	string decoded;
	
	if (sl.size() == 0) return 30;
	
	try{
		ECB_Mode< AES >::Decryption d;
		d.SetKey(key, sizeof(key));
		StringSource s(sl.substr(1,sl.size()-1), true, new StreamTransformationFilter(d, new StringSink(decoded)));
	}
	catch(const CryptoPP::Exception& e){
		cerr << "ERR: SSMTOMCU2_SL ENCRYPTION GONE WRONG\n" << e.what();
		if (MCU2 && lastPackGarb){			//for in case decryption of previous packet AL was a failure
			decoded.clear();
			decoded = to_string(latestFV);
			//lastPackGarb = false;
			cerr << "\nBecause last packet AL layer had garbage, use the same FV as last time\n";
		}
		else{
			cerr  << "\nREJECTING (ERR CODE 4)\n";
			return 25;
		}
	}
	if (sl[0] == 'A'){
		if (decoded.compare("2") == 0){ 						//because this is a proof of concept
			isVerif = true;
			return 50;
		}
		else if (decoded.compare("1") == 0){
			cerr << "ERR: Connection rejected by SSM - password incorrect?\n";
		}
	}
	else if (sl[0] == 'B'){//cerr << sl << endl;
		//password check here (plain text for now, sorry)
		if (decoded.compare("mastersthesis") == 0){
			isVerif = true;
			return 50;
		}
	}
	else if (sl[0] == 'C'){
		latestFV = stof(decoded, NULL);
		return 30;
	}
	else if (sl[0] == 'E'){
		lastValue = stol(decoded, NULL, 10);
		return 60;
	}
	return 25;			//rejected, or not applicable
}
//-------------------------------------------------------------------------------------------------
//Name: MCU2dest
//Description: Look in current input, decrypt destination of MCU1's
//					message, and return it.
//Output: CAN ID of destination
//-------------------------------------------------------------------------------------------------
char TCB::MCU2dest(){
	string theID, msgenc = currentInput.substr(13, 16);
	try{ //do decrypting here
		ECB_Mode< AES >::Decryption d;
		d.SetKey(key, sizeof(key));
		StringSource s(msgenc, true, new StreamTransformationFilter(d, new StringSink(theID)));
	}
	catch(const CryptoPP::Exception& e){
		cerr << "ERR: MCU2DEST DECRYPTION GONE WRONG\n" << e.what() << "\nNOT PASSING ALONG MSG (ERR CODE 4)\n";
		return 25;
	}
	return theID[0];
}
//-------------------------------------------------------------------------------------------------
//Name: SLtoAL
//Description: Send data to AL and get factor value.
//Output: N/A
//-------------------------------------------------------------------------------------------------
void TCB::SLtoAL(char CANID){
	//cerr << "ENTERING SLTOAL\n";
	string path = ALpipepath + to_string(int(CANID)), msgenc = currentInput.substr(29, currentInput.size()-29), msg, lfv(15, '\0');
	ofstream ofs(path.c_str(), ios::out | ios::binary);
	int resultNow = 1;
	
	try{ //do decrypting here
		ECB_Mode< AES >::Decryption d;
		d.SetKey(key, sizeof(key));
		StringSource s(msgenc, true, new StreamTransformationFilter(d, new StringSink(msg)));
		resultNow = stoi(msg, NULL, 10);
	}
	catch(const CryptoPP::Exception& e){
		cerr << "ERR: SLTOAL DECRYPTION GONE WRONG\n" << e.what() << "\nNOT PASSING TO APPLICATION LAYER (ERR CODE 4)\n";
		latestFV = 1;
		resultNow = lastValue;
		lastPackGarb = true;
		return;
	}
	if (lastValue < 1){
		lastValue = resultNow;
		latestFV = 1;
	}
	else if (lastPackGarb){
		lastPackGarb = false;
		latestFV = 1;
	}
	else latestFV = resultNow / lastValue;
	ofs << msg;		//off to AL with ye

	//encrypt latestFV
	try{
		snprintf(&lfv[0], lfv.size(), "%.2f", latestFV);
		cerr << "lfv: " << lfv << endl;
		latestFVenc.clear();
		ECB_Mode< AES >::Encryption e;
		e.SetKey(key, sizeof(key));
		StringSource(lfv, true, new StreamTransformationFilter(e, new StringSink(latestFVenc)));
	}
	catch(const CryptoPP::Exception& e){
		cerr << "ERR: SLTOAL ENCRYPTION GONE WRONG\n" << e.what() << "\nEXITING\n";
		exit(1);
	}
}
