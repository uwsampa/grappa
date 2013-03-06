#include <iostream>
#include <algorithm>
#include <vector>
#include <numeric>
#include <fstream>

using namespace std;
#define PEPS 0.00001
#define NEPS -0.00001

template<typename _ForwardIter, typename T>
static void iota(_ForwardIter __first, _ForwardIter __last, T __val)
{
	while (__first != __last)
		*__first++ = __val++;
}


int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		cout << "Don't forget to pass the input file name" << endl;
	}	
	ifstream input(argv[1]);
	string rhsmatrix = string("restrict_T_") + string(argv[1]);
	string diagonal = string("diag_") + string(argv[1]);
	string nondiagmat = string("offdiag_") + string(argv[1]);

	ofstream rhsout(rhsmatrix.c_str());
	ofstream diagout(diagonal.c_str());
	ofstream matout(nondiagmat.c_str());

	int m, n, nnz;
	input >> m >> n >> nnz;
	vector<int> perm(n);
	iota(perm.begin(), perm.end(), 1);	// 1-based
	random_shuffle(perm.begin(), perm.end());
	int half = m/2;

	if(half *2 < m)	// odd
		rhsout << m << "\t" << (half+1) << "\t" << m << endl;
	else	// even
		rhsout << m << "\t" << half << "\t" << m << endl;
	
	int one = 1;
	for(int i=0; i< half; ++i)
	{
		rhsout << perm[2*i] << "\t" << (i+1) << "\t" << one << endl;
		rhsout << perm[2*i+1] << "\t" << (i+1) << "\t" << one << endl;	
	}	
	if (half *2 < m)	// m was odd
 	{
		rhsout << perm.back() << "\t" << (half+1) << "\t" << one << endl;
	}
	int row, column;
	double value;
	int diagnnz = 0;
	int nondiagnnz = 0;
	int zero = 0;
	diagout << m << "\t" << one << "\t" << zero << endl;
	matout << m << "\t" << n << "\t" << zero << endl;
	while(!input.eof())
	{
		input >> row >> column >> value;
		// keep explicit zeros
		if(row == column)
		{
			diagout << row << "\t" << one << "\t" << value << endl;
			diagnnz++;
		}
		else
		{
			matout << row << "\t" << column << "\t" << value << endl;
			nondiagnnz++;
		}
	}
	cout << "Diagonal nnz: " << diagnnz << endl;
	cout << "Off-diagonal nnz: " << nondiagnnz << endl;
	// will need to reopen this and write first lines
	rhsout.close();
	diagout.close();
	matout.close();

	return 0;
}
