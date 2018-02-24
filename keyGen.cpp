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

#include <cryptopp/osrng.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>

using namespace std;
using CryptoPP::AES;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::ECB_Mode;
using CryptoPP::Exception;
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;
using CryptoPP::StringSink;
using CryptoPP::StringSource;

int main(int arcg, char** argv){
	AutoSeededRandomPool prng;
	byte key[AES::MAX_KEYLENGTH];
	string keyStr;
	ofstream ofs("my.key", ios::out);
	
	prng.GenerateBlock(key, sizeof(key));
	
	StringSource(key, sizeof(key), true, new HexEncoder(new StringSink(keyStr)));
	
	cout << "new AES-256 key: " << keyStr << endl;
	
	if (ofs.is_open()){
		ofs << key;
		ofs.close();
	}
	else cerr << "ERR: key could not be written to file\n";
	
	return 0;
}
