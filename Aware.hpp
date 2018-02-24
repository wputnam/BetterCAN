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

#ifndef __SSMAWAREHPP
#define __SSMAWAREHPP

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define N 5	//this is a
#define SSMID "34"

class Aware{
private:
	char toID, fromID;
	float data1[N], data2[N], data3[N];
	std::time_t start1, start2, start3;	//when was data set "created?"
	std::string errfilepipepath = "/tmp/hack" + std::string(SSMID);

public:
	Aware(char t, char f);
	~Aware(){ std::cerr << "end of connection entry for " << toID << " from " << fromID << std::endl; }
	void calcAvgs();
	void decide(float avg1, float avg2, float avg3);
	void getSets(float newFactor);
	bool sameToID(char c) const{ return c == toID; }
	bool sameFromID(char c) const{ return c == fromID; }
};

#endif
