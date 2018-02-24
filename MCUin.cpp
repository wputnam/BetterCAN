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

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>

using namespace std;

int main(int argc, char** argv) {

	struct stat buf;
	string latestVal, currentVal;
	
	int CANID = strtol(argv[1], NULL, 10);
	string datafilepipepath = "/tmp/TCBtoAL"+to_string(CANID);

	cout << "MCUIN FOR CAN ID " << CANID << endl << endl;
	cout << "CURRENT VALUE: N/A";

	for (;;){
		if (stat(datafilepipepath.c_str(), &buf) != -1){
			ifstream in(datafilepipepath.c_str(), ios::in);
			latestVal.clear();
			in >> latestVal;
			if (latestVal.compare(currentVal) != 0){
				cout << endl;		//to prove that the value has changed
				currentVal.clear();
				currentVal = latestVal;
			}
			remove(datafilepipepath.c_str());
		}
		cout << "\rCURRENT VALUE: " << currentVal;	//keep same values on one line
	}
}
