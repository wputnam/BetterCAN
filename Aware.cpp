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

#include "Aware.hpp"

using namespace std;

Aware::Aware(char t, char f){
	toID = t;
	fromID = f;
	cerr << "to and from: " << int(toID) << " " << int(fromID) << endl;
	for (int i = 0; i < N; ++i){
		data1[i] = -1;
		data2[i] = -1;
		data3[i] = -1;
	}
}

void Aware::getSets(float newFactor){
	bool done = false;
	
	if (data1[0] == -1) start1 = time(NULL);
	for (int i = 0; i < N; ++i){
		if (data1[i] == -1){
			data1[i] = newFactor;
			cout << "data1 " << i << endl;
			done = true;
			break;
		}
	}
	
	if (data2[0] == -1 && !done) start2 = time(NULL);
	if (!done){
		for (int j = 0; j < N; ++j){
			if (data2[j] == -1){
				data2[j] = newFactor;
				cout << "data2 " << j << endl;
				done = true;
				break;
			}
		}
	}
	
	if (data3[0] == -1 && !done) start3 = time(NULL);
	if (!done){
		for (int k = 0; k < N; ++k){
			if (data3[k] == -1){
				data3[k] = newFactor;
				cout << "data3 " << k << endl;
				done = true;
				if (k == N-1){
					calcAvgs();
					memcpy(data1, data2, N*sizeof(float));
					memcpy(data2, data3, N*sizeof(float));
					for (int q = 0; q < N; ++q) data3[q] = -1;
					start1 = start2;
					start2 = start3;
					start3 = 0;
				}
				return;
			}
		}
	}
}

void Aware::calcAvgs(){
	cerr << "CALCAVGS\n";
	float d1avg = 0, d2avg = 0, d3avg = 0;
	
	for (int j = 0; j < N; ++j){
		d1avg += data1[j];
		d2avg += data2[j];
		d3avg += data3[j];
	}
	
	d1avg /= N;
	d2avg /= N;
	d3avg /= N;
	
	decide(d1avg, d2avg, d3avg);
}

void Aware::decide(float avg1, float avg2, float avg3){
	cerr << avg1 << " " << avg2 << " " << avg3 << endl;
	cerr << "verdict: ";
	
	if (avg1 == avg2 && avg1 == avg3){
		//no change in values
		cerr << "constant\n";
	}
	
	else if (abs(avg1-avg2) > 0.1 || abs(avg2-avg3) > 0.1){
		//hack
		cerr << "HACK\n";
		ofstream ofs("/tmp/hack"+string(SSMID), ios::out);
		ofs << fromID << " " << start2;
	}
	
	else if (avg1 != 1 && avg2 != 1 && avg3 != 1){
		//standard increase/decrease
		cerr << "gradual change\n";
	}
	
	else if (avg1 == 1 && (abs(avg1-avg2) < 0.1 || abs(avg2-avg3) < 0.1) && avg3 == 1){
		//blip
		cerr << "blip\n";
	}
	else {
		//transitionary behavior between data sets
		cerr << "N/A\n";
	}
}
