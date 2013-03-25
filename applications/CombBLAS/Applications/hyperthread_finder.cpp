#include <map>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

int main(int argv, char * argc[])
{
	// map (proc id, core id) to physical id
	map< pair<int, int>, int> htmap;
	pair< map< pair<int,int> ,int>::iterator, bool > ret;

	ifstream fin(argc[1]);
	int i=0;
	while(!fin.eof())
	{
		// processor       : 75
		// physical id     : 3
		// core id         : 2
		int proc, pid, cid; 
		string line;
		getline(fin, line);
		size_t loc1 = line.find(":");
		stringstream linestream1(line.substr(loc1+1, string::npos));
		linestream1 >> proc;

		getline(fin, line);
		size_t loc2 = line.find(":");
		stringstream linestream2(line.substr(loc2+1, string::npos));
		linestream2 >> pid;

		getline(fin, line);
		size_t loc3 = line.find(":");
		stringstream linestream3(line.substr(loc3+1, string::npos));
		linestream3 >> cid;
		
		ret = htmap.insert(map< pair<int,int>, int>::value_type(make_pair(pid,cid), proc)); 
		if (ret.second)
		{
			cout << "rank " << i++ << "=localhost slot=" << pid <<":" << cid<< endl;
		}
	}
}
