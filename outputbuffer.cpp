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

int main (int argc, char** argv){

	const char  *intrf = "vcan0";
	string msg;
	struct canfd_frame frame;
	stringstream ss;
	int destID = strtol(argv[1], NULL, 10);
	int srcID = strtol(argv[2], NULL, 10);
	ifstream ifs;
	int rc, sockfd, opt, bytes, enable = 1;
	struct sockaddr_can canaddr;
	struct ifreq ifr;
	string fifo = "/tmp/TCBToOb" + to_string(srcID), out;
	const char *filepipepath = fifo.c_str();

	sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (sockfd == -1){
		cerr << "Can't open socket, errno: " << errno << endl;
		return 1;
	}

	rc = setsockopt(sockfd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable, sizeof(enable));
	if (rc == -1){
		cerr << "Can't set socket options\n";
		return 1;
	}

	std::strncpy(ifr.ifr_name, intrf, IFNAMSIZ);
	if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1){
		cerr << "Can't interact with network interface, errno " << errno << endl;
		return 1;
	}

	canaddr.can_family = AF_CAN;
	canaddr.can_ifindex = ifr.ifr_ifindex;
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	rc = bind(sockfd, (struct sockaddr *)&canaddr, sizeof(canaddr));

	if (rc == -1){
		cerr << "Can't bind socket\n";
		return 1;
	}

	//get packet from pipe
	ifs.open(filepipepath, ios::in | ios::binary);
	if (ifs.is_open()){
		for (string line; getline(ifs, line);){	//watch out for whitespaces!
			out.append(line);
			if (ifs.peek() && ifs.eof()) break;
			else out.append("\n");
		}
		frame.can_id = destID;
		frame.len = out.size();
		for (int i = 0; i < 64; ++i) frame.data[i] = (int)out[i];
		bytes = write(sockfd, &frame, sizeof(struct canfd_frame));
		
		//destroy file pipe
		ifs.close();
		remove(filepipepath);
	}

	return 0;
}
