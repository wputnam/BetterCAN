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

int main(int argc, char** argv){

	struct stat buf;
	string datafilepipepath = "/tmp/newawareentry"+string(SSMID), makeentry = "/tmp/makeawareentry"+string(SSMID), deleteentry = "/tmp/delaware"+string(SSMID);
	vector<shared_ptr<Aware>> dataSources;
	
	cerr << "SSM awareness algorithm\n\n";
	
	for(;;){
		if (stat(makeentry.c_str(), &buf) != -1){
			ifstream in(makeentry.c_str(), ios::in);
			string tmp;
			
			getline(in, tmp);
			dataSources.push_back(make_shared<Aware>(tmp[0], tmp[1]));
			remove(makeentry.c_str());
		}
		if (stat(datafilepipepath.c_str(), &buf) != -1){
			ifstream in(datafilepipepath.c_str(), ios::in);
			stringstream ss;
			vector<string> tokens;
			string tmp, token;
			char destCANID, srcCANID;
			int entryN = -1;
			float factor;
			
			getline(in, tmp);
			ss << tmp;
			while (getline(ss, token, ' ')){
				tokens.push_back(token);
			}
			if (!tokens.empty()){			//if tokens is empty, program will hang on the next line!
				destCANID = tokens[0][0];
				srcCANID = tokens[0][1];
				factor = stof(tokens[1].c_str(), NULL);
				for (int i = 0; i < dataSources.size(); ++i){
					if (dataSources[i]->sameToID(destCANID) && dataSources[i]->sameFromID(srcCANID)){
						entryN = i;
						break;
					}
				}
				dataSources[entryN]->getSets(factor);
				remove(datafilepipepath.c_str());
			}
		}
		if (stat(deleteentry.c_str(), &buf) != -1){
			ifstream in(deleteentry.c_str(), ios::in);
			string tmp;
			
			getline(in, tmp);
			for (int i = 0; i < dataSources.size(); ++i){
				if (dataSources[i]->sameToID(tmp[0]) && dataSources[i]->sameFromID(tmp[1])){
					cerr << "removing terminated connection\n";
					dataSources.erase(dataSources.begin()+i);
					break;
				}
			}
			remove(deleteentry.c_str());
		}
	}

}
