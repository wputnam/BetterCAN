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

#include "SSMstack.hpp"

using namespace std;

int main(int argc, char** argv){

	char hackFrom;
	struct stat buf;
	vector<entry> stack;
	string tmp, datafilepipepath = "/tmp/newstackentry"+string(SSMID),
		errfilepipepath = "/tmp/hack"+string(SSMID), help = "/tmp/help",
		forSSM = "/tmp/forSSM"+string(SSMID);
	stringstream ss;
	int hackTime;
	
	cerr << "SSM stack application\n\n";
	
	for(;;){
		//read in new stack entry from file
		if (stat(datafilepipepath.c_str(), &buf) != -1){
			ifstream in(datafilepipepath.c_str(), ios::in | ios::binary);
			string token;
			
			for (string line; getline(in, line);){
				tmp.append(line);
				if (in.peek() != ' ' && in.eof()) break;
				else tmp.append("\n");
			}
			ss << tmp;
			tmp.clear();
			vector<string> tokens;
			while (getline(ss, token, ' ')){
				tokens.push_back(token);
			}
			if (!tokens.empty()){
				entry e;
				e.time = (time_t) strtol(tokens[0].c_str(), NULL, 10);
				e.to = (char) strtol(tokens[1].c_str(), NULL, 10);
				e.from = (char) strtol(tokens[2].c_str(), NULL, 10);
				if (tokens[3].size() < 16 && tokens.size() > 4){			//in case a space was a string char, which was treated as a delimiter
					for (int i = 4; i < tokens.size(); ++i){
						tokens[3].append(" ");
						tokens[3].append(tokens[i]);
					}
				}
				e.data = tokens[3];
				stack.push_back(e);
				cerr << "NOT: Stack has latest " << stack.size() << " entries\n";
				remove(datafilepipepath.c_str());
			}
		}
		
		//delete oldest stack size entry when its size is greater than N
		if (stack.size() > N){
			stack.erase(stack.begin());
			cerr << "NOT: Removed oldest data entry from stack\n";
		}
		
		tmp.clear();
		ss.str(string(""));
		ss.clear();
		
		//check for notification of hack, and if there is one, read it in
		if (stat(errfilepipepath.c_str(), &buf) != -1){
			cerr << "here\n";
			ifstream in(errfilepipepath.c_str(), ios::in);
			string token;
			
			getline(in, tmp);
			ss << tmp;
			vector<string> tokens;
			while (getline(ss, token, ' ')) tokens.push_back(token);
			if (!tokens.empty()){
				hackFrom = tokens[0][0];
				hackTime = strtol(tokens[1].c_str(), NULL, 10);
				for (int i = stack.size()-1; i > -1; --i){
					cerr << i << " " << stack[i].time << " " << hackTime << endl;
					if (stack[i].time > hackTime){
						if (hackFrom == stack[i].from){
							cerr << "NOT: Hacked data entry found, removing\n";
							stack.erase(stack.begin()+i);
						}
					}
					else if (hackFrom == stack[i].from){
						ofstream ofs(help.c_str(), ios::out);
						ofs << stack[i].to << stack[i].from << " " << stack[i].data;
						ofs.close();
						break;
					}
				}
				remove(errfilepipepath.c_str());
			}
		}
		tmp.clear();
		ss.str(string(""));
		ss.clear();
	}
}
