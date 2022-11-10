/*
  Multipartites Tables for FloPoCo

  Authors: Franck Meyer, Florent de Dinechin

  This file is part of the FloPoCo project

  Initial software.
  Copyright © INSA-Lyon, INRIA, CNRS, UCBL,
  2008-2015.
  All rights reserved.
  */

#include <iostream>
#include <math.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <cassert>
#include <cmath> //for abs(double)

#include <gmp.h>
#include <gmpxx.h>
#include <mpfr.h>

#include <sollya.h>



#include "../utils.hpp"
#include "FixFunctionByMultipartiteTable.hpp"
#include "Multipartite.hpp"
#include "../BitHeap/BitHeap.hpp"
#include "../Tables/Table.hpp"
#include "../Tables/DiffCompressedTable.hpp"

/*
To replicate the functions used in the 2017 Hsiao paper
-> good match, only the TIV has one MSB constant 0, I don't know why
./flopoco FixFunctionByMultipartiteTable f="1/(x+1)-0.5" signedIn=0 lsbIn=-15 lsbOut=-16

perfect match with the "MP" lines of table 5
./flopoco FixFunctionByMultipartiteTable f="sin(pi/4*x)" signedIn=0 lsbIn=-16 lsbOut=-16
./flopoco FixFunctionByMultipartiteTable f="sin(pi/4*x)" signedIn=0 lsbIn=-24 lsbOut=-24
(et sur le 24 bits on est bien meilleurs que le résultat HMP !)

absolutely no match
./flopoco FixFunctionByMultipartiteTable f="2^x-1"       signedIn=0 lsbIn=-16 lsbOut=-15

>
*/

/*
 TODOs:
 compress TIV only if it saves at least one adder; in general resurrect uncompressed TIV

Bugs when wIn != wOut;

 ./flopoco FixFunctionByMultipartiteTable f="atan(x)/pi" signedIn=0 compressTIV=false lsbIn=-16 lsbOut=-15
./flopoco FixFunctionByMultipartiteTable f="2/(1+x)" signedIn=0 compressTIV=true lsbIn=-10 lsbOut=-12

*/

using namespace std;

namespace flopoco
{
	//----------------------------------------------------------------------------- Constructor/Destructor

	/**
	 @brief The FixFunctionByMultipartiteTable constructor
	 @param[string] functionName_		a string representing the function, input range should be [0,1) or [-1,1)
	 @param[int]    lsbIn_		input LSB weight
	 @param[int]    lsbOut_		output LSB weight
	 @param[int]	nbTOi_	number of tables which will be created
	 @param[bool]	signedIn_	true if the input range is [-1,1)
	 */
	FixFunctionByMultipartiteTable::FixFunctionByMultipartiteTable(OperatorPtr parentOp, Target *target, string functionName_, int nbTOi_, bool signedIn_, int lsbIn_, int lsbOut_, bool correctRounding_, bool optimiseForFPGA_):
		Operator(parentOp, target), nbTOi(nbTOi_), correctRounding(correctRounding_), optimiseForFPGA(optimiseForFPGA_)
{
		compressTIV = target->tableCompression();
		srcFileName="FixFunctionByMultipartiteTable";

		ostringstream name;
		name << "FixFunctionByMultipartiteTable_" << getNewUId();
		setNameWithFreqAndUID(name.str());
		f = new FixFunction(functionName_, signedIn_, lsbIn_, lsbOut_);
		if(f->signedOut) {
			THROWERROR("Sorry there is a known bug if the function output can get negative. If this is important to you we will get it fixed of course")
		}
		epsilonT = 1.0 / (1<<(f->wOut+1));


		setCopyrightString("Franck Meyer, Florent de Dinechin (2015-2020)");
		addHeaderComment("-- Evaluator for " +  f->getDescription() + "\n");
		REPORT(DETAILED, "Entering: FixFunctionByMultipartiteTable \"" << functionName_ << "\" " << nbTOi_ << " " << signedIn_ << " " << lsbIn_  << " " << lsbOut_ << " ");
		int wX=-lsbIn_;
		addInput("X", wX);
		int outputSize =-lsbIn_; //f->wOut; testing this for synthesis bc we don't care // TODO finalRounding would impact this line
		addOutput("Y" ,outputSize , 2);
		useNumericStd();

		/* // Exploration 
		int sizeMax = f->wOut<<f->wIn; // size of a plain table
		topTen=vector<Multipartite*>(ten);
		for (int i=0; i<ten; i++){
			topTen[i] = new Multipartite(this, f, f->wIn, f->wOut, target);
			topTen[i]-> totalSize =	sizeMax;
		}


		// Outer loop on guardBitSlack;
		guardBitsSlack =-1; // first  try the exploration with one guard bit less than the safe value
		Multipartite* bestMP;
		bool successWithguardBitsSlack = false;
		int rank;
		while (guardBitsSlack<=0 && !successWithguardBitsSlack) {

			bool decompositionFound;
			if(nbTOi==0) {
				// The following is not as clean as it should be because the code was first written with nbTOi a global variable...
				nbTOi=1;
				decompositionFound=true;
				while (decompositionFound) {
					REPORT(INFO, "Exploring nbTO=" << nbTOi);
					buildOneTableError();
					buildGammaiMin();
					decompositionFound = enumerateDec();
					if(!decompositionFound)
						REPORT(INFO, "No decomposition found for nbTOi=" << nbTOi << ", stopping search.");
					nbTOi ++;
				}
				if(topTen[0]->totalSize == sizeMax) {
					THROWERROR("\nSorry, the multipartite method doesn't seem to work for this function. \nTry another of the FixFunction* operators. At least FixFunctionByTable should work...");
				}
			}
			else	{ // nbTOi was given
				// build the required tables of errors
				buildOneTableError();
				buildGammaiMin();
				decompositionFound = enumerateDec();
				if(!decompositionFound)
					THROWERROR("No decomposition found for nbTOi=" << nbTOi << ", aborting");
			}

			// Parameter space exploration complete. Now checking the results
			rank = 0 ;
			bool tryAgain = true;
			while (rank < ten && tryAgain) {
				// time to report
				bestMP = topTen[rank];
				if(bestMP->totalSize==sizeMax) { // This is one of the dummy mpts
					tryAgain=false;
				}
				else {
					REPORT(INFO, "Now running exhaustive test on candidate #" << rank << " :" << endl
								 << tab << bestMP->descriptionString() << endl
								 << tab<< bestMP->descriptionStringLaTeX()  );
					bestMP->mkTables();
					if (bestMP->exhaustiveTest()) {
						REPORT(INFO, "... passed, now building the operator");
						tryAgain = false;
					}
					else {
						REPORT(INFO, "... failed, trying next candidate");
						rank++;
					}
				}
			}

			if(rank==ten || bestMP->totalSize==sizeMax) {
				REPORT(INFO, "It seems we have to use the safe value of g... starting again");
				for (int i=0; i<ten; i++){
					topTen[i]-> totalSize =	sizeMax;
				}
				guardBitsSlack ++;
				//REPORT(0,"guardBitsSlack now " << guardBitsSlack);
				nbTOi=nbTOi_;
			}
			else {
				successWithguardBitsSlack=true;
			}
		}

		if(guardBitsSlack>0)
			THROWERROR("Unable to reach faithful rounding with this value of NbTOi");
		// Exploration complete. Now building the operator

		bestMP = topTen[rank];

		REPORT(DEBUG,"Full table dump:" <<endl << bestMP->fullTableDump()); */

		// Ask julia everything
		if(f->signedIn)
			THROWERROR("Sorry idk how to do that, flemme");
		
		int msb_in = -1;
		int lsb_in = f->lsbIn;
		int msb_out = -1;
		int lsb_out = f->lsbOut;

		string exec;
		string flopoco_dir;
		string julia_dir;
		flopoco_dir = ""; // Add the path to flopoco here
		julia_dir = ""; // It does not work for me without the hardcoded path but maybe you can figure it out
		exec = julia_dir + "julia " + flopoco_dir + "src/FixFunctions/table_multipartite_flopoco.jl --f \"" + functionName_ + "\" --msbIn " + to_string(msb_in) + " --lsbIn " + to_string(lsb_in) + " --msbOut " + to_string(msb_out) + " --lsbOut " + to_string(lsb_out);
		if (correctRounding == 1)
			exec += " --correct";
		if (optimiseForFPGA == 1)
			exec += " --FPGA";

		cout << exec << endl;
		system(exec.c_str());

		// Get julia's results
		ifstream myfile (flopoco_dir + "src/FixFunctions/tables.data", ios::in);
		int m, alpha, betai, gammai, lsb_TIV, lsb_TOi, msb_TIV, msb_TOi, useXori;
		std::vector<int> beta, gamma, lsb_TO, msb_TO, useXor;
		myfile >> m;
		cout << m;
		myfile >> alpha;
		for (int i=0; i < m; i++) {
			myfile >> betai;
			beta.push_back(betai);
		}
		for (int i=0; i < m; i++) {
			myfile >> gammai;
			gamma.push_back(gammai);
		}
		for (int i=0; i < m; i++) {
			myfile >> useXori;
			useXor.push_back(useXori);
		}
		myfile >> msb_TIV;
		myfile >> lsb_TIV;
		for (int i=0; i < m; i++) {
			myfile >> msb_TOi;
			msb_TO.push_back(msb_TOi);
		}
		for (int i=0; i < m; i++) {
			myfile >> lsb_TOi;
			lsb_TO.push_back(lsb_TOi);
		}
		int lsb_min = min(lsb_TIV, *min_element(lsb_TO.begin(), lsb_TO.end()));
		// print the right stuff for synthesis
		ostringstream latex_report;
		latex_report << "Latex table line : " << endl << "$";
		latex_report << msb_TIV - lsb_TIV +1 << "\\cdot 2^{" << alpha << "}";
		for (int t=m-1; t >= 0; t--) {
			if (useXor[t] == 0)
				latex_report << " + " << msb_TO[t] - lsb_TO[t] + 1 << "\\cdot 2^{" << beta[t] + gamma[t] - useXor[t] << "}";
			else
				latex_report << " + \\underline{" << msb_TO[t] - lsb_TO[t] + 1 << "\\cdot 2^{" << beta[t] + gamma[t] - useXor[t] << "}}";
		}
		latex_report << "$\\\\" << endl;
		REPORT(INFO, latex_report.str());
		// Read the table entries
		int TIVi, TOi;
		std::vector<int> TIV;
		std::vector<int> TO[m];
		//cout << "Entries of TIV : " << (1 << alpha) << endl;
		for (int i = 0; i < (1<< alpha); i++) {
			myfile >> TIVi;
			TIV.push_back(TIVi);
		}
		for (int t = 0; t < m; t++) {
			//cout << "Entries of TO" << t << " : " << (1 << alpha) << endl;
			for (int i = 0; i < (1 <<(beta[t] + gamma[t] - useXor[t])); i++) {
				myfile >> TOi;
				(TO[t]).push_back(TOi);
			}
		}

		vector<mpz_class> mpzTIV;
		for (auto i : TIV) {
			mpzTIV.push_back(mpz_class((long) i));
		}
		//if(!target->tableCompression()) { // uncompressed TIV
		// TIV compression is seen as an extra TO 
			vhdl << tab << declare("inTIV", alpha) << " <= X" << range(f->wIn-1, f->wIn-alpha) << ";" << endl;

			Table::newUniqueInstance(this, "inTIV", "outTIV",
															 mpzTIV, "TIV", alpha, msb_TIV - lsb_TIV +1);
				vhdl << endl;
		/*}else
			{ // Hsiao-compressed TIV
				REPORT (INFO, "TIV compression report:" << endl << bestMP->dcTIV.report()); 
				vhdl << tab << declare("inSSTIV", bestMP->dcTIV.subsamplingIndexSize) << " <= X" << range(f->wIn-1, f->wIn-bestMP->dcTIV.subsamplingIndexSize) << ";" << endl;
				vector<mpz_class> mpzssTIV;
				for (auto i : bestMP->ssTIV)
					mpzssTIV.push_back(mpz_class((long) i));
				Table::newUniqueInstance(this, "inSSTIV", "outSSTIV",
																 mpzssTIV, "SSTIV", bestMP->dcTIV.subsamplingIndexSize, bestMP->dcTIV.subsamplingWordSize );
				vhdl << endl;

				vhdl << tab << declare("inDiffTIV", bestMP->alpha) << " <= X" << range(f->wIn-1, f->wIn-bestMP->alpha) << ";" << endl;
				vector<mpz_class> mpzDiffTIV;
				//cerr << " Starting enum"  << endl;
				for (auto i : bestMP->diffTIV) {
					mpzDiffTIV.push_back(mpz_class((long) i));
					//cerr  << " " <<mpz_class((long) i) << endl;
				}
				//cerr << "bestMP->dcTIV.diffWordSize=" <<bestMP->dcTIV.diffWordSize << endl;
				Table::newUniqueInstance(this, "inDiffTIV", "outDiffTIV",
																 mpzDiffTIV, "DiffTIV", bestMP->alpha, bestMP->dcTIV.diffWordSize );
				// TODO need to sign-extend for 1/(1+x), but it makes an error for sin(x)
				//  getSignalByName("outDiffTIV")->setIsSigned(); // so that it is sign-extended in the bit heap
				// No need to sign-extend it, it is already taken care of in the construction of the table.
			}*/

		int p = 0;
		int inputSize = msb_in - lsb_in +1;
		for(unsigned int i = 0; i < m; ++i) {
			string ai = join("a", i);
			string bi = join("b", i);
			string inTOi = join("inTO", i);
			string outTOi = join("outTO", i);
			string deltai = join("delta", i);
			string nameTOi = join("TO", i);
			string signi = join("sign", i);

			p += beta[i];
			//if (gamma[i] != 0)
			//	vhdl << tab << declare(ai, gamma[i]) << " <= X" << range(inputSize - 1, inputSize - gamma[i]) << ";" << endl;
			vhdl << tab << declare(bi, beta[i]) << " <= X" << range(p - 1, p - beta[i]) << ";" << endl;
			vhdl << tab << declare(signi) << " <= not(" << bi << of( beta[i] - 1) << ");" << endl;
			vhdl << tab << declare(inTOi, gamma[i]+beta[i]-useXor[i]) << " <= ";
			if (gamma[i] == 0) {
			       	vhdl << "";
			} else {
				vhdl << "X" << range(inputSize - 1, inputSize - gamma[i]);
			}
			if (useXor[i]) {
				if (beta[i] != 1)
					vhdl << ((gamma[i] == 0) ? "" : " &") <<  " ((" << bi << range(beta[i]-1-useXor[i], 0) << ")" << " xor " << rangeAssign(beta[i]-2,0, signi) << ")";
				vhdl << ";" << endl;
			} else {
				vhdl << ((gamma[i] == 0) ? "" : " &") <<  " ((" << bi << range(beta[i]-1-useXor[i], 0) << "));" << endl;
			}
			vector<mpz_class> mpzTOi;
			for (long i : TO[i]) {
				mpzTOi.push_back(mpz_class((long) i));
			}
			int outputSizeTOi = msb_TO[i] - lsb_TO[i] + 1;
			Table::newUniqueInstance(this, inTOi, outTOi, mpzTOi, nameTOi, gamma[i]+beta[i]-useXor[i], outputSizeTOi);
			//string trueSign = (bestMP->negativeTOi[i] ? "(not "+signi+")" : signi); // our tables are only positive
			string trueSign = (useXor[i]) ? signi : "\"0\"";
			vhdl << tab << declare(deltai, outputSizeTOi+1) << " <= " << trueSign << " & (" <<  outTOi;
		      	if (useXor[i])
				vhdl << " xor " << rangeAssign(outputSizeTOi-1,0, trueSign);
			vhdl << ");" << endl;
			getSignalByName(deltai)->setIsSigned(); // so that it is sign-extended in the bit heap
		}

		// Throwing everything into a bit heap

		BitHeap *bh = new BitHeap(this, msb_out - lsb_min +1); // TODO this is using an adder tree

		//if(!target->tableCompression()) { // uncompressed TIV
			bh->addSignal("outTIV", lsb_TIV - lsb_min); // TODO check que c'est la bonne formule
		/*}else
			{ // Hsiao-compressed TIV
				bh->addSignal("outSSTIV", bestMP->dcTIV.subsamplingShift()); // shifted because its LSB bits were shaved in the Hsiao compression
				bh->addSignal("outDiffTIV");
			}*/

		for(unsigned int i = 0; i < m; ++i)		{
			bh->addSignal(join("delta", i), lsb_TO[i] - lsb_min );
		}
		bh->startCompression();

		vhdl << tab << "Y <= " << /* "sum" */bh->getSumName() << range( msb_out - lsb_min, lsb_out-lsb_min) << ";" << endl;
	}


	FixFunctionByMultipartiteTable::~FixFunctionByMultipartiteTable() {
		delete f;
		for (int i=0; i<ten; i++)
			delete topTen[i];
	}


	//------------------------------------------------------------------------------------- Public methods

	void FixFunctionByMultipartiteTable::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		tc = new TestCase(this);
		tc->addInput("X", 0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", 1);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addInput("X", (mpz_class(1) << f->wIn) - 1);
		emulate(tc);
		tcl->add(tc);

	}


	void FixFunctionByMultipartiteTable::emulate(TestCase* tc)
	{
		f->emulate(tc, correctRounding);
	}

	//------------------------------------------------------------------------------------ Private classes

	// enumerating the alphas is much simpler
	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenum(int alpha, int m)
	{
		vector<vector<int>> res;
		vector<vector<int>> res1;
		int ptr;

		if (m==1)
		{
			res = vector<vector<int>>(alpha, vector<int>(1));
			for(int i = 0; i < alpha; i++)
			{
				res[i][0] = i+1;
			}
		}
		else
		{
			res1 = alphaenum(alpha, m-1);
			res = vector<vector<int>>(alpha*res1.size());
			ptr = 0;

			for(int i = 0; i < alpha; i++)
			{
				for(unsigned int j = 0; j < res1.size(); j++)
				{
					res[ptr] = vector<int>(m);
					res[ptr][0] = i+1;
					for(int k = 1; k < m; k++)
					{
						res[ptr][k] = res1[j][k-1];
					}
					ptr++;
				}
			}
		}
		return res;
	}


	// with a table stating where to start the enumeration
	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenum(int alpha, int m, vector<int> gammaimin)
	{
		//test if it is possible with this alpha
		for (int i=0; i<m; i++)
			if (gammaimin[i]>alpha)
				return vector<vector<int>>(0);// else return an empty enumeration
		//else
		return alphaenumrec(alpha, m, gammaimin);

	}


	vector<vector<int>> FixFunctionByMultipartiteTable::alphaenumrec(int alpha, int m, vector<int> gammaimin)
	{
		vector<vector<int>> res;
		vector<vector<int>> res1;
		int ptr;
		if (m==1)
		{
			res = vector<vector<int>>(alpha-gammaimin[0]+1, vector<int>(1));
			for(int i = gammaimin[0]; i <= alpha; i++)
			{
				res[i-gammaimin[0]][0] = i;
			}
		}
		else
		{
			res1 = alphaenumrec(alpha, m-1, gammaimin);
			res = vector<vector<int>>((alpha-gammaimin[m-1]+1)*res1.size());
			ptr = 0;
			for(int i = gammaimin[m-1]; i <= alpha; i++)
			{
				for(unsigned int j = 0; j < res1.size(); j++)
				{
					res[ptr] = vector<int>(m);
					res[ptr][0] = i;
					for(int k = 1; k < m; k++)
						res[ptr][k] = res1[j][k-1];
					ptr++;
				}
			}
		}
		return res;
	}


	vector<vector<int>> FixFunctionByMultipartiteTable::betaenum (int sum, int size)
	{
		int ptr;
		vector<int> t;
		int totalsize(0);
		vector<vector<int>> res;
		vector<vector<vector<int>>> enums;

		if (size == 1)
		{
			t = vector<int>(size);
			t[0] = sum;
			res = vector<vector<int>>(1);
			res[0] = t;
		}
		else if (2*size>sum)
		{
			res=vector<vector<int>>(0);
		}
		else if (2*size == sum)
		{
			t = vector<int>(size);
			for (int i = 0; i < size; i++)
				t[i] = 2;

			res = vector<vector<int>>(1);
			res[0] = t;
		}
		else
		{ // 2*size < sum
			enums = vector<vector<vector<int>>>(sum-2*size+1);//to flatten at the end

			for (int i = 2; i <= sum-2*(size-1); i++)
			{
				enums[i-2] = betaenum(sum-i, size-1);
				totalsize += enums[i-2].size();
			}
			// now we know the size to alloc
			res = vector<vector<int>>(totalsize);
			// now flatten all these smaller enums
			ptr = 0;
			for (unsigned int i = 0; i < enums.size(); i++)
			{
				for(unsigned int j = 0; j < enums[i].size(); j++)
				{
					res[ptr] = vector<int>(size);
					res[ptr][0] = i+2;
					for(int k = 1; k < size; k++)
					{
						res[ptr][k] = enums[i][j][k-1];
					}
					ptr++;
				}
			}

		}
		return res;
	}


	/** See the article p5, right. */
	void FixFunctionByMultipartiteTable::buildGammaiMin()
	{
		int wi = f->wIn;
		int gammai;
		gammaiMin.clear();
		gammaiMin = vector<vector<int>>(wi, vector<int>(wi));
		for(int pi = 0; pi < wi; pi++)
		{
			for(int betai = 1; betai < (wi-pi-1); betai++)
			{
				gammai=1;
				while ( (gammai < wi) && (oneTableError[pi][betai][gammai] > epsilonT) )
					gammai++;

				gammaiMin[pi][betai] = gammai;
			}
		}
	}


	/**
	 * @brief buildOneTableError : Builds the error for every beta_i and gamma_i, to evaluate precision
	 */
	void FixFunctionByMultipartiteTable::buildOneTableError()
	{
		int wi = f->wIn;
		int gammai, betai, pi;
		oneTableError.clear();
		oneTableError = vector<vector<vector<double>>>(wi, vector<vector<double>>(wi, vector<double>(wi))); // there will be holes

		for(pi=0; pi<wi; pi++) {
			for(betai=1; betai<wi-pi-1; betai++) {
				for(gammai=1; gammai<= wi-pi-betai; gammai++) {
					oneTableError[pi][betai][gammai] =	errorForOneTable(pi, betai,gammai);
				}
			}
		}
	}



#if 0 //REMOVE ME

	void	 FixFunctionByMultipartiteTable::buildDiffTIVCache(){
		for (auto g=0; g<2+intlog2(f->wIn); g++) {
			vector<DifferentialCompression*> v;
			for (auto alpha=0; alpha<f->wIn; alpha++) {
				// build the TIV
				vector<mpz_class> mptiv;
				for (auto i=0; i<(1<<alpha); i++) {
					mptiv.push_back(mpz_class(TIVFunction(i)));
				}
				// then compress it and store it
				auto d=DifferentialCompression::find_differential_compression(mptiv, alpha, f->wOut + g);
				v.push_back(d);
			}
			DCTIV.push_back(v);
		}
	}

#endif

	/**
	 * @brief enumerateDec : This function enumerates all the decompositions and returns the smallest one
	 * @return The smallest Multipartite decomposition.
	 * @throw "It seems we could not find a decomposition" if there isn't any decomposition with an acceptable error
	 */
#if 0
	bool FixFunctionByMultipartiteTable::enumerateDec()
	{
		Multipartite *mpt;
		int beta, p;
		int n = f->wIn;
		int alphamin = 2;
		int alphamax = (2*n)/3;
		vector<vector<int>> betaEnum;
		vector<vector<int>> alphaEnum;
		vector<int> gammaimin;
		vector<int> gammai;
		vector<int> betai;

		int sizeMax = f->wOut <<f->wIn; // size of a plain table

		bool decompositionFound=false;

		for (int alpha = alphamin; alpha <= alphamax; alpha++)		{
			beta = n-alpha;
			betaEnum = betaenum(beta, nbTOi);
			for(unsigned int e = 0; e < betaEnum.size(); e++) {
				betai = betaEnum[e];

				gammaimin = vector<int>(nbTOi);
				p = 0;
				for(int i = nbTOi-1; i >= 0; i--) {
					gammaimin[i] = gammaiMin[p][betai[i]];
					p += betai[i];
				}

				alphaEnum = alphaenum(alpha,nbTOi,gammaimin);
				for(unsigned int ae = 0; ae < alphaEnum.size(); ae++)		{
					gammai = alphaEnum[ae];
					mpt = new Multipartite(f, nbTOi,
																 alpha, beta,
																 gammai, betai, this);
					if(mpt->mathError < epsilonT){
						decompositionFound=true;
						mpt->buildGuardBitsAndSizes();
						insertInTopTen(mpt);
					}
					else
						mpt->totalSize = sizeMax;
				}
			}
			// exit this loop as soon as 2^alpha > best totalSize
			if( (f->wOut << (alpha+1)) > topTen[0]->totalSize)
				alpha =  alphamax+1 ; // exit
		}
		return decompositionFound;
	}

#else // only betai=2 or 3 is useful
	bool FixFunctionByMultipartiteTable::enumerateDec()
	{
		Multipartite *mpt;
		int beta, p;
		int n = f->wIn;
		int alphamin = n/3;
		int alphamax = (2*n)/3;
		vector<vector<int>> betaEnum;
		vector<vector<int>> alphaEnum;
		vector<int> gammaimin;
		vector<int> gammai;
		vector<int> betai;

		int sizeMax = f->wOut <<f->wIn; // size of a plain table

		bool decompositionFound=false;

		for (int alpha = alphamin; alpha <= alphamax; alpha++)		{
			beta = n-alpha;
			betaEnum = betaenum(beta, nbTOi);
			for(unsigned int e = 0; e < betaEnum.size(); e++) {
				betai = betaEnum[e];

				gammaimin = vector<int>(nbTOi);
				p = 0;
				for(int i = nbTOi-1; i >= 0; i--) {
					gammaimin[i] = gammaiMin[p][betai[i]];
					p += betai[i];
				}

				alphaEnum = alphaenum(alpha,nbTOi,gammaimin);
				for(unsigned int ae = 0; ae < alphaEnum.size(); ae++)		{
					gammai = alphaEnum[ae];
					mpt = new Multipartite(f, nbTOi,
																 alpha, beta,
																 gammai, betai, this, getTarget());
					if(mpt->mathError < epsilonT){
						decompositionFound=true;
						mpt->buildGuardBitsAndSizes();
						insertInTopTen(mpt);
					}
					else
						mpt->totalSize = sizeMax;
				}
			}
			// exit this loop as soon as 2^alpha > best totalSize
			if( (f->wOut << (alpha+1)) > topTen[0]->totalSize)
				alpha =  alphamax+1 ; // exit
		}
		return decompositionFound;
	}

#endif



	/** 5th equation implementation */
	double FixFunctionByMultipartiteTable::epsilon(int ci_, int gammai, int betai, int pi)
	{
		int wi = f->wIn; // for the notations of the paper
		double ci = (double)ci_; // for the notations of the paper

		double xleft = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * intpow2(-gammai) * ci;
		double xright = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * (intpow2(-gammai) * (ci+1) - intpow2(-wi+pi+betai));
		double delta = (f->signedIn ? 2 : 1) * intpow2(pi-wi) * (intpow2(betai) - 1);

		double eps = 0.25 * (f->eval(xleft + delta)
							 - f->eval(xleft)
							 - f->eval(xright+delta)
							 + f->eval(xright)
							 );
		return eps;
	}


	double FixFunctionByMultipartiteTable::epsilon2ndOrder(int Ai, int gammai, int betai, int pi)
	{
		int wi = f->wIn;
		double xleft = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * intpow2(-gammai)  * ((double)Ai);
		double delta = (f->signedIn ? 2 : 1) * ( intpow2(pi-wi) * (intpow2(betai) - 1));
		double epsilon = 0.5 * (f->eval(xleft + (delta-1)/2)
								- (delta-1)/2 * (f->eval(xleft + delta)
												 - f->eval(xleft)));
		return epsilon;
	}


	/** eq 11 */
	double FixFunctionByMultipartiteTable::errorForOneTable(int pi, int betai, int gammai)
	{
		double eps1, eps2, eps;

		// Beware when gammai+betai+pi = inputSize,
		// we then have to compute a second order term for the error
		if(gammai+betai+pi==f->wIn)
		{
			eps1 = abs(epsilon2ndOrder(0, gammai, betai, pi));
			eps2 = abs(epsilon2ndOrder( (int)intpow2(gammai) -1, gammai, betai, pi));
			if (eps1 > eps2)
				eps = eps1;
			else
				eps = eps2;
		}
		else
		{
			eps1 = abs(epsilon(0, gammai, betai, pi));
			eps2 = abs(epsilon( (int)intpow2(gammai) -1, gammai, betai, pi));

			if (eps1 > eps2)
				eps = eps1;
			else
				eps = eps2;
		}
		return eps;
	}


	void  FixFunctionByMultipartiteTable::insertInTopTen(Multipartite* mp) {
		REPORT(DEBUG, "Entering  insertInTopTen");
		int rank=ten-1;
		Multipartite* current = topTen[rank];
		while(rank >= 0 &&  // mp strictly smaller than current
						(mp->totalSize < current->totalSize   ||    (mp->totalSize == current->totalSize &&  mp->m < current->m))) {
			rank--;
			if(rank>=0) current = topTen[rank];
		}

		if(rank<ten-1) { // this mp belongs to the top ten,
			rank ++; // the last rank for which mp was strictly smaller than topTen[rank]
			REPORT(INFO, "The following is now top #" << rank <<" : " << mp->descriptionString());
			delete(topTen[ten-1]);
			// shift the bottom
			for (int i=ten-1; i>rank; i--)
				topTen[i] = topTen[i-1];
			topTen[rank] = mp;
			//debug
			for (rank=0; rank<ten; rank++)
				REPORT(DETAILED, "top "<< rank << " size is " << topTen[rank]->totalSize);
		}
	}


	TestList FixFunctionByMultipartiteTable::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;

		if(index==-1)
		{ // The unit tests
			vector<string> function;
			vector<bool> signedIn;
			vector<int> msbOut;
			vector<bool> scaleOutput;			// multiply output by (1-2^lsbOut) to prevent it  reaching 2^(msbOut+1) due to faithful rounding
			function.push_back("2^x-1");			// input in [0,1) output in [0, 1) : need scaleOutput
			signedIn.push_back(false);
			msbOut.push_back(0);
			scaleOutput.push_back(true);

			function.push_back("1/(x+1)");  // input in [0,1) output in [0.5,1] but we don't want scaleOutput
			signedIn.push_back(false);
			msbOut.push_back(0);
			scaleOutput.push_back(false);
			msbOut.push_back(0);

			function.push_back("sin(pi/4*x)");
			signedIn.push_back(false);
			msbOut.push_back(-1);
			scaleOutput.push_back(false);

			function.push_back("sin(pi/2*x)");
			signedIn.push_back(false);
			msbOut.push_back(-1);
			scaleOutput.push_back(true);

#if 0
			// It seems we don't manage yet the case when the function may get negative
			// but the code has been elegantly fixed: it now THROWERRORs
			function.push_back("log(0.5+x)");
			signedIn.push_back(false);
			msbOut.push_back(-1);
			scaleOutput.push_back(true);
#endif

			for (int lsbIn=-8; lsbIn >= -16; lsbIn--) {
				for (size_t i =0; i<function.size(); i++) {
					paramList.clear();
					string f = function[i];
					int lsbOut = lsbIn + msbOut[i]; // to have inputSize=outputSize
					if(scaleOutput[i]) {
						f = "(1-1b"+to_string(lsbOut) + ")*(" + f + ")";
					}
					f="\"" + f + "\"";
					paramList.push_back(make_pair("f", f));
					paramList.push_back(make_pair("signedIn", to_string(signedIn[i]) ) );
					paramList.push_back(make_pair("lsbIn", to_string(lsbIn)));
					paramList.push_back(make_pair("lsbOut", to_string(lsbOut)));
					if(lsbIn>-14)
						paramList.push_back(make_pair("TestBench n=","-2"));
					testStateList.push_back(paramList);
				}
			}
		}
		else
		{
				// finite number of random test computed out of index
		}

		return testStateList;
	}



	OperatorPtr FixFunctionByMultipartiteTable::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		string f;
		bool signedIn, correctRounding, optimiseForFPGA;
		int lsbIn, lsbOut, nbTO;
		UserInterface::parseString(args, "f", &f);
		UserInterface::parsePositiveInt(args, "nbTO", &nbTO);
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		UserInterface::parseBoolean(args, "signedIn", &signedIn);
		UserInterface::parseBoolean(args, "correctRounding", &correctRounding);
		UserInterface::parseBoolean(args, "optimiseForFPGA", &optimiseForFPGA);
		return new FixFunctionByMultipartiteTable(parentOp, target, f, nbTO, signedIn, lsbIn, lsbOut, correctRounding, optimiseForFPGA);
	}

	void FixFunctionByMultipartiteTable::registerFactory(){
		UserInterface::add("FixFunctionByMultipartiteTable", // name
											 "A function evaluator using the multipartite method.",
											 "FunctionApproximation", // category
											 "",
						   "f(string): function to be evaluated between double-quotes, for instance \"exp(x*x)\";\
signedIn(bool): if true the function input range is [-1,1), if false it is [0,1);\
correctRounding(bool): use correct rounding;\
optimiseForFPGA(bool): optimise for FPGA;\
lsbIn(int): weight of input LSB, for instance -8 for an 8-bit input;\
lsbOut(int): weight of output LSB;\
nbTO(int)=0: number of Tables of Offsets, between 1 (bipartite) to 4 or 5 for large input sizes -- 0: let the tool choose",

											 "This operator uses the multipartite table method as introduced in <a href=\"http://perso.citi-lab.fr/fdedinec/recherche/publis/2005-TC-Multipartite.pdf\">this article</a>, with the improvement described in <a href=\"http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=6998028&tag=1\">this article</a>. ",
											 FixFunctionByMultipartiteTable::parseArguments,
											 FixFunctionByMultipartiteTable::unitTest
											 ) ;

	}
}

