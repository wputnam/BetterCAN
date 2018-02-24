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

#include "TCBmodule.hpp"

using namespace std;

int main (int argc, char** argv){

	string msg;
	char destID, srcID;
	
	if (argc != 2){
		cerr << "Usage: ./tcbm [source CAN ID]";
		exit(1);
	}
	srcID = argv[1][0];
	TCBmodule TCBm(srcID);
	
	if (srcID == 34){
		cout << "OPERATING IN SSM MODE\nTYPE \"^q\" (no quotes) TO QUIT\n";
		for(;;){
			cin >> msg;
			if (msg.compare("^q") == 0) break;
		}
		TCBm.stopAware();
		TCBm.stopStack();
		TCBm.stopInputLoop();	//ib is never killed unless this is here
	}
	
	else{
		while (msg.compare("^q") != 0){
			for(;;){
				if (msg.compare("^q") == 0) break;
				cout << "Enter destination CAN ID in ASCII form.\n";
				cin >> destID;
				if (TCBm.checkConn(destID)){
					cout << "Enter message to send. ('^q' to quit)\n";
					cin >> msg;
					if (msg.compare("^q") == 0) break;
				
					//send off to TCBmodule
					TCBm.msgFromAL(destID, msg);
				}
			}
		}
	}
	//close all open connections
	
	TCBm.endAllConnections();
	
	TCBm.stopInputLoop();
	TCBm.stopMCUin();
	
	return 0;
}
