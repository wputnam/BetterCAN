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

#include <linux/can.h>
#include <linux/can/raw.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

int main(int argc, char** argv) {

	const char  *intrf = "vcan0";
	int listeningID = strtol(argv[argc-1], NULL, 10);
	int rc, sockfd, opt, enable = 1;
	struct sockaddr_can canaddr;
	struct ifreq ifr;
	string filepipepath = "/tmp/ibToTCB" + to_string(listeningID);
	sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (sockfd == -1){
		cerr << "Can't open socket for listeningID " << listeningID << endl;
		return 1;
	}
	
	rc = setsockopt(sockfd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable, sizeof(enable));
	if (rc == -1){
		cerr << "Can't set socket options for listeningID " << listeningID << endl;
		return 1;
	}

	std::strncpy(ifr.ifr_name, intrf, IFNAMSIZ);
	if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1){
		cerr << "Can't interact with network interface for listeningID " << listeningID << dec << ", errno " << errno << endl;
		return 1;
	}
	
	canaddr.can_family = AF_CAN;
	canaddr.can_ifindex = ifr.ifr_ifindex;
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	rc = bind(sockfd, (struct sockaddr *)&canaddr, sizeof(canaddr));
	if (rc == -1){
		cerr << "Can't bind socket for listeningID "<< listeningID << endl;
		return 1;
	}

	//loop for a new frame, collect it, and put payload in pipe for TCBmodule
	for(;;){
		struct canfd_frame fr;
		
		int bytes = read(sockfd, &fr, CANFD_MTU);
		if (bytes > 8 && fr.can_id == listeningID){
			//cout << bytes << endl;
			ofstream ofs;
			ostringstream oss("");
			int fd;
			struct stat buf;
			
  			for (int i = 0; i < fr.len; ++i) oss << fr.data[i];
  			//cerr << "got packet: " << oss.str() << endl;
			
   			if (stat(filepipepath.c_str(), &buf) == -1) ofs.open(filepipepath.c_str(), ios::out | ios::binary);
    			ofs << oss.str();
    			ofs.close();
    			//cerr << "got packet: " << oss.str() << endl;
    			
    			//cerr << oss.str().size() << endl;
    			oss.str(string(""));
    			oss.flush();
		}
		fr.can_id = 0;
		fr.len = 0;
		for (int i = 0; i < 64; ++i) fr.data[i] = '\0';
	}

	return 0;

}
