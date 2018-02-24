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

#include "Timer.hpp"

using namespace std;

Timer::Timer(int t){
	type = t;
	if (type == 0){}						//timer for retrans
	else if (type == 1){}				//timer for keepalive
	else if (type == 2){}				//timer for time_wait
	else if (type == 3){}				//error counter (SSM)
	else if (type == 4){}				//hack timer
}

bool Timer::isExpired(){
	if (type == 0 && chrono::duration_cast<secs>(hrc::now() - start).count() > 1) return true;
	else if (type == 1 && chrono::duration_cast<secs>(hrc::now() - start).count() > 10) return true;
	else if (type == 2 && chrono::duration_cast<secs>(hrc::now() - start).count() > 5) return true;
	else if (type == 3 && chrono::duration_cast<secs>(hrc::now() - start).count() > 5) return true;
	else if (type == 4 && chrono::duration_cast<secs>(hrc::now() - start).count() > 10) return true;
	else return false;
}
