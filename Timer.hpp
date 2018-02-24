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

#include <ctime>
#include <chrono>
#include <iostream>

class Timer{
typedef std::chrono::high_resolution_clock hrc;
typedef std::chrono::nanoseconds nanosecs;
typedef std::chrono::seconds secs;

private:
	hrc::time_point start;
	int type;
	
public:
	Timer(int t);
	~Timer(){}
	void reset(){ start = hrc::now(); }
	bool isExpired();
};
