/*
  Multipartites Tables for FloPoCo

  Authors: Franck Meyer, Florent de Dinechin

  This file is part of the FloPoCo project

  Initial software.
  Copyright © INSA-Lyon, INRIA, CNRS, UCBL,
  2008-2020.
  All rights reserved.
*/

#include "Multipartite.hpp"
#include "FixFunctionByMultipartiteTable.hpp"
#include "FixFunction.hpp"

#include "../utils.hpp"
#include "../BitHeap/BitHeap.hpp"
//#include "QuineMcCluskey.h"

#include <map>
#include <sstream>
#include <vector>
#include <cmath> //for abs(double)

using namespace std;
using namespace flopoco;

namespace flopoco
{
	//--------------------------------------------------------------------------------------- Constructors
	Multipartite::Multipartite(FixFunctionByMultipartiteTable* mpt_, FixFunction *f_, int inputSize_, int outputSize_, Target* target):
		f(f_), inputSize(inputSize_), outputSize(outputSize_),  mpt(mpt_), _target(target)
	{
		inputRange = 1<<inputSize_;
		epsilonT = 1.0 / (1<<(outputSize+1));
	}

	Multipartite::Multipartite(FixFunction *f_, int m_, int alpha_, int beta_, vector<int> gammai_, vector<int> betai_, FixFunctionByMultipartiteTable *mpt_, Target* target):
		f(f_), m(m_), alpha(alpha_),  beta(beta_), gammai(gammai_), betai(betai_), mpt(mpt_), _target(target)
	{
		inputRange = 1 << f->wIn;
		inputSize = f->wIn;
		outputSize = f->wOut;
		pi = vector<int>(m);
		pi[0] = 0;
		for (int i = 1; i < m; i++)
			pi[i] = pi[i-1] + betai[i-1];

		epsilonT = 1 / (1<<(outputSize+1));

		// compute approximation error out of the precomputed tables of MPT
		// poor encapsulation: mpt will test it it is low enough
		mathError=0;
		for (int i=0; i<m; i++)
			{
				mathError += mpt->oneTableError[pi[i]][betai[i]][gammai[i]];
			}
	}



	//------------------------------------------------------------------------------------ Private methods






	void Multipartite::computeTOiSize(int i)
	{
		double r1, r2, r;
		double delta = deltai(i);
		r1 = abs( delta * si(i,0) );
		r2 = abs( delta * si(i,  (1<<gammai[i]) - 1));
		r = max(r1,r2);
		// This r is a float, the following line scales it to the output
		outputSizeTOi[i]= (int)floor( outputSize + guardBits + log2(r));

		// the size computation takes into account that we exploit the symmetry by using the xor trick
		sizeTOi[i] = (1<< (gammai[i]+betai[i]-1)) * outputSizeTOi[i];
	}


	/** Just as in the article */
	double Multipartite::deltai(int i)
	{
		return mui(i, (1<<betai[i])-1) - mui(i, 0);
	}


	/** Just as in the article  */
	double Multipartite::mui(int i, int Bi)
	{
		int wi = inputSize;
		return  (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) *  intpow2(-wi+pi[i]) * Bi;
	}


	/** Just as in the article */
	double Multipartite::si(int i, int Ai)
	{
		int wi = inputSize;
		double xleft = (f->signedIn ? -1 : 0)
			+ (f->signedIn ? 2 : 1) * intpow2(-gammai[i])  * ((double)Ai);
		double xright= (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * ((intpow2(-gammai[i]) * ((double)Ai+1)) - intpow2(-wi+pi[i]+betai[i]));
		double delta = deltai(i);
		double si =  (f->eval(xleft + delta)
									- f->eval(xleft)
									+ f->eval(xright+delta)
									- f->eval(xright) )    / (2*delta);
		return si;
	}


	int64_t min(int64_t x, int64_t y) { // ?? it doesn't seem to exist
		return (x>y? y:x);
	}

	int64_t max(int64_t x, int64_t y) { // ?? it doesn't seem to exist
		return (x>y? x:y);
	}

#if 0
	// This version works just as well but is less generic. Vae victis
	void Multipartite::computeTIVCompressionParameters() {
		string srcFileName = mpt->getSrcFileName(); // for REPORT to work
		REPORT(FULL, "Entering computeTIVCompressionParameters: alpha=" << alpha << "  uncompressed size=" << sizeTIV);
		// OLD
		int64_t bestS,bestSizeTIV;
		// compression using only positive numbers
		bestS = 0;
		bestSizeTIV=sizeTIV;
		for(int s = 1; s < alpha-1; s++) {
			// Under convexity hypothesis, the maximum slack is either left or right of the interval
			// (maybe we assume here convexity of the first derivative, too)
			int64_t  yLL, yLR, yRL, yRR, deltaLeft, deltaRight, deltaMax, deltaBits, slack, saved_LSBs_in_SSTIV, tempRho, tempSizeSSTIV, tempSizeDiffTIV, tempCompressedSize;
			int i = 0; // compute extremal values on the left of the interval
			yLL	 = TIVFunction( i<<s );
			yLR = TIVFunction( ((i+1)<<s)-1 );
			deltaLeft = abs(yLL-yLR);
			i = (1<<(alpha - s))-1; // compute extremal values on the right of the interval
			yRL	 = TIVFunction( i<<s );
			yRR = TIVFunction( ((i+1)<<s)-1 );

			deltaRight = abs(yRL-yRR);
			deltaMax = max(deltaLeft,deltaRight);   // for instance s=1 and we find deltaMax=21
			deltaBits = intlog2(deltaMax);          // 21 fits on 5 bits. The deltaTIV will have 5 output bits.
			REPORT(FULL, "trying s=" << s << " deltaleft deltaright = "<< deltaLeft << " " << deltaRight << "  \tdeltaBits="<<deltaBits);
			// Now how many bits can we shave from the SSTIV?
			slack = (1<<deltaBits)-1 - deltaMax; // so the deltaTIV may represent values between 0 and 31, therefore we have a slack of 31-21=10
			saved_LSBs_in_SSTIV = intlog2(slack)-1; // for instance if slack=10 we may save 3 bits, because it will offset the TIV at most by 7 which is smaller than 10
			// This is the number of bits we are sure we can shave, but this is only a worst-case analysis:  we could be more lucky
			// However it will save very little: TODO if somebody wants to try

			tempRho = alpha - s;
			tempSizeSSTIV = (outputSize+guardBits-saved_LSBs_in_SSTIV)<<tempRho;
			tempSizeDiffTIV = deltaBits<<alpha;
			tempCompressedSize = tempSizeSSTIV + tempSizeDiffTIV;
			REPORT(DETAILED, "computeTIVCompressionParameters, unsigned: alpha=" << alpha << "  s=" << s << " compressedSize=" << tempCompressedSize
						 << " =" << outputSize+guardBits-saved_LSBs_in_SSTIV << ".2^" << tempRho
						 << " + " << deltaBits << ".2^" << alpha
						 << "  ( slack=" << slack <<"  saved_LSBs_in_SSTIV=" << saved_LSBs_in_SSTIV <<" )");

			if (tempCompressedSize < bestSizeTIV) {
				bestSizeTIV = tempCompressedSize;
				bestS = s;
				// tentatively set the attributes of the Multipartite class
				rho = alpha - s;
				sizeSSTIV = tempSizeSSTIV;
				sizeDiffTIV = tempSizeDiffTIV;
				sizeTIV = tempCompressedSize;
				totalSize = sizeSSTIV + sizeDiffTIV;
				for (int i=0; i<m; i++)		{
					totalSize += sizeTOi[i];
				}

				nbZeroLSBsInSSTIV = saved_LSBs_in_SSTIV;
				dcTIV.subsamplingWordSize = outputSize+guardBits-nbZeroLSBsInSSTIV;
				outputSizeDiffTIV = deltaBits;
			}
		}
		REPORT(FULL, "Old computeTIVCompressionParameters: alpha=" << alpha << "  ssTIVIn=" << rho  << "  dcTIV.subsamplingWordSize=" << dcTIV.subsamplingWordSize << "  outputSizeDiffTIV=" <<  outputSizeDiffTIV);
		REPORT(FULL, "    total TIV size before=" << ((outputSize+guardBits)<<alpha) << "    compressed=" <<  sizeSSTIV + sizeDiffTIV);

	}

#else
	void Multipartite::computeTIVCompressionParameters() {
		string srcFileName = mpt->getSrcFileName(); // for REPORT to work
		REPORT(FULL, "Entering computeTIVCompressionParameters: alpha=" << alpha << "  uncompressed size=" << sizeTIV);


		// NEW : this is mostly a wrapper to the different (and better) notations of Luc's code
		// First convert the ints to mpzclass
		// As long as it was only for TIVs, 64 bits should be enough for anybody.
		// but we billgatesed there. Differential compression may indeed be applied to wider tables


		pair<int, int> key (guardBits,alpha);
		if(mpt->DCTIV.count(key)==0) {

			vector<mpz_class> mptiv;
			for (auto i=0; i<(1<<alpha); i++) {
				mptiv.push_back(mpz_class(TIVFunction(i)));
			}
			REPORT(FULL, "Computing new compression for  alpha=" << alpha << "   wOut=" << outputSize + guardBits);
			dcTIV = DifferentialCompression::find_differential_compression(mptiv, alpha, outputSize + guardBits, _target);
			// Trick to save memory: we won't need the complete tables, they take up more than 16Gb for 24 bit functions
			// better free them now, and recompute at the end only the winner
			dcTIV.subsampling.clear();
			dcTIV.diffs.clear();
			mpt->DCTIV[key]=dcTIV;
		}
		else {
			REPORT(DETAILED, "Using cached TIV compression for g=" << guardBits << " and alpha=" << alpha);
			dcTIV = mpt->DCTIV[key];
		}

		totalSize = dcTIV.subsamplingStorageSize() + dcTIV.diffsStorageSize();
		for (int i=0; i<m; i++)		{
			totalSize += sizeTOi[i];
		}
		REPORT(FULL, "New computeTIVCompressionParameters: alpha=" << alpha << "  ssTIVIn=" << dcTIV.subsamplingIndexSize  << "  dcTIV.subsamplingWordSize=" << dcTIV.subsamplingWordSize << "  dcTIV.diffWordSize=" <<  dcTIV.diffWordSize);
		REPORT(FULL, "    total TIV size before=" << ((outputSize+guardBits)<<alpha) << "    compressed=" <<  dcTIV.subsamplingStorageSize() + dcTIV.diffsStorageSize());
		REPORT(FULL, "Exiting computeTIVCompressionParameters");
	}

#endif



	//------------------------------------------------------------------------------------- Public methods

	void Multipartite::buildGuardBitsAndSizes(bool computeGuardBits)
	{

		if(computeGuardBits) {
			guardBits =  (int) ceil(-outputSize - 1
															+ log2(m /(intpow2(-outputSize - 1) - mathError)));
			// With a slack of 1 it works 97% of the time
			guardBits += mpt->guardBitsSlack;
		}

		sizeTIV = (outputSize + guardBits)<<alpha;
		int size = sizeTIV;
		outputSizeTOi = vector<int>(m);
		sizeTOi = vector<int>(m);
		for (int i=0; i<m; i++)		{
			computeTOiSize(i);
			size += sizeTOi[i];
		}
		totalSize = size;
#if 1 // COMPRESSION_IN_EXPLORATION
		if(mpt->compressTIV)
			computeTIVCompressionParameters(); // may change sizeTIV and totalSize
#endif
	}


	void Multipartite::mkTables()
	{
		// The TIV
		tiv.clear();
		for (int j=0; j < 1<<alpha; j++) {
			tiv.push_back(TIVFunction(j));
		}

		// The TOIs
		//toi = vector<Table*>(m);
		toi.clear();
		for(int i = 0; i < m; ++i)
			{
				vector<int64_t> values;
				for (int j=0; j < 1<<(gammai[i]+betai[i]-1); j++) {
					values.push_back(TOiFunction(j, i));
				}
				// If the TOi values are negative, we will store their opposite
				bool allnegative=true;
				bool allpositive=true;
				for (size_t j=0; j < values.size(); j++) {
					if(values[j]>0)
						allnegative=false;
					if(values[j]<0)
						allpositive=false;
				}
				if((!allnegative) && (!allpositive)) {
					const string srcFileName="Multipartite";
					THROWERROR("TO"<< i << " doesn't have a constant sign, please report this as a bug");
				}
				if(allnegative) {
					negativeTOi.push_back(true);
					for (size_t j=0; j < values.size(); j++) {
						values[j] = -values[j];
					}
				}
				else {
					negativeTOi.push_back(false);
				}
				toi.push_back(values);

				//			string name = mpt->getName() + join("_TO",i);
				//toi[i] = new Table(mpt, target, values, name, gammai[i] + betai[i] - 1, outputSizeTOi[i]-1);
			}


#if 1 // Florent's version

		if(mpt->compressTIV) {
			ssTIV.clear();
			diffTIV.clear();
			// TIV compression as per Hsiao with improvements
			int s = alpha-dcTIV.subsamplingIndexSize;
			for(int i = 0; i < (1<<dcTIV.subsamplingIndexSize); i++)	{
				int64_t valLeft  = TIVFunction( i<<s );
				int64_t valRight = TIVFunction( ((i+1)<<s)-1 );
				int64_t refVal;
				// life is simpler if the diff table is always positive
				if(valLeft<=valRight)
					refVal=valLeft;
				else
					refVal=valRight;
				// the improvement: we may shave a few bits from the LSB
				//			int64_t mask = ((1<<outputSize)-1) - ((1<<dcTIV.subsamplingShift())-1);
				refVal = refVal >> dcTIV.subsamplingShift() ;
				ssTIV.push_back(refVal);
				//cerr << " i=" << i << "  SSTIV=" << refVal << "<<" << dcTIV.subsamplingShift() << endl;
				for(int j = 0; j < (1<<s); j++)		{
					int64_t diff = tiv[ (i<<s) + j] - (refVal << dcTIV.subsamplingShift());
					diffTIV.push_back(diff);
					//cerr << "    j=" <<j  << " diffTIV=" << diff << endl;
				}
			}
		}
#else // The previous code is in principle redundant with code in DifferentialCompression
		if(mpt->compressTIV) {
			computeTIVCompressionParameters(); // may change sizeTIV and totalSize
			... TODO
		}
#endif

	}


	//------------------------------------------------------------------------------------- Public classes
	int64_t Multipartite::TOiFunction(int x, int ti)
	{
		int TOi;
		double dTOi;

		double y, slope;
		int Ai, Bi;

		// to lighten the notation and bring them closer to the paper
		int wI = inputSize;
		int wO = outputSize;
		int g = guardBits;

		Ai = x >> (betai[ti]-1);
		Bi = x - (Ai << (betai[ti]-1));
		slope = si(ti,Ai); // mathematical slope

		y = slope * intpow2(-wI + pi[ti]) * (Bi+0.5);
		dTOi = y * intpow2(wO+g) * intpow2(f->lsbIn - f->lsbOut) * intpow2(inputSize - outputSize);
		if(dTOi>0)
			TOi = (int)floor(dTOi);
		else
			TOi = (int)ceil(dTOi);
		return int64_t(TOi);
	}




	int64_t Multipartite::TIVFunction(int x)
	{
		int TIVval;
		double dTIVval;

		double y, yl, yr;
		double offsetX(0);
		double offsetMatula;

		// to lighten the notation and bring them closer to the paper
		int wO = outputSize;
		int g = guardBits;


		for (unsigned int i = 0; i < pi.size(); i++) {
			offsetX+= (1<<pi[i]) * ((1<<betai[i]) -1);
		}

		offsetX = offsetX / ((double)inputRange);

		if (m % 2 == 1) // odd
			offsetMatula = 0.5*(m-1);
		else //even
			offsetMatula = 0.5 * m;

		offsetMatula += intpow2(g-1); //for the final rounding

		double xVal = (f->signedIn ? -1 : 0) + (f->signedIn ? 2 : 1) * x * intpow2(-alpha);
		// we compute the function at the left and at the right of
		// the interval
		yl = f->eval(xVal) * intpow2(f->lsbIn - f->lsbOut) * intpow2(inputSize - outputSize);
		yr = f->eval(xVal+offsetX) * intpow2(f->lsbIn - f->lsbOut) * intpow2(inputSize - outputSize);

		// and we take the mean of these values
		y =  0.5 * (yl + yr);
		dTIVval = y * intpow2(g + wO);

		if(m % 2 == 1)
			TIVval = (int) round(dTIVval + offsetMatula);
		else
			TIVval = (int) floor(dTIVval + offsetMatula);

		return int64_t(TIVval);
	}


	string 	Multipartite::descriptionString(){
		ostringstream s;
		s << "m=" << m << " alpha=" << alpha;
		if(mpt->compressTIV)
			s << " dcTIV.subsamplingIndexSize=" << dcTIV.subsamplingIndexSize << " dcTIV.subsamplingShift()=" << dcTIV.subsamplingShift() << " ";
		for (size_t i =0; i< gammai.size(); i++) {
			s << "  gamma" << i << "=" << gammai[i] << " beta"<<i<<"=" << betai[i];
		}
		s << "  g=" << guardBits << endl;
		if(mpt->compressTIV){
			s << "  dcTIV.subsamplingStorageSize()=" << dcTIV.subsamplingStorageSize() << " dcTIV.diffsStorageSize()=" << dcTIV.diffsStorageSize() ;
		}
		s << "  sizeTIV=" << sizeTIV << " totalSize = " << totalSize ;
		return s.str();
	}

	string 	Multipartite::descriptionStringLaTeX(){
		ostringstream s;
#if 0
		s << "    \\alpha=" << alpha;
		if(mpt->compressTIV)
			s << ", dcTIV.subsamplingIndexSize=" << dcTIV.subsamplingIndexSize << ", dcTIV.subsamplingShift()=" << dcTIV.subsamplingShift() << "   ";
		for (size_t i =0; i< gammai.size(); i++) {
			s << "   \\gamma_" << i << "=" << gammai[i] << " \\beta_"<<i<<"=" << betai[i];
		}
		s << endl;
#endif
		s << "         totalSize = " << totalSize << "   =   " ;

		if(mpt->compressTIV){
			s << "    " << dcTIV.subsamplingWordSize << ".2^" << dcTIV.subsamplingIndexSize << " + " << dcTIV.diffWordSize << ".2^" << alpha;
		}
		else
			s << "    " << outputSize+guardBits << ".2^" << alpha;

		s << "     ";
		for(int t=m-1; t>=0; t-- ) {
			s << " + " << outputSizeTOi[t] << ".2^" << gammai[t] + betai[t] -1;
		}
		return s.str();
	}



	string 	Multipartite::fullTableDump(){
		ostringstream s;
		if(!mpt->compressTIV) { //uncompressed TIV
			for(size_t i=0; i<tiv.size(); i++ ) {
				s << "TIV[" << i << "] \t= " << tiv[i] <<endl;
			}
		}
		else {// compressed TIV
			for(size_t i=0; i<ssTIV.size(); i++ ) {
				s << "ssTIV[" << i << "] \t= " << ssTIV[i] <<endl;
			}
			s << endl;
			for(size_t i=0; i<diffTIV.size(); i++ ) {
				s << "diffTIV[" << i << "] \t= " << diffTIV[i] <<endl;
			}
		}
		for(size_t t=0; t<toi.size(); t++ ) {
			s << endl;
			for(size_t i=0; i<toi[t].size(); i++ ) {
				s << "TO" << t << "[" << i << "] \t= " << toi[t][i] <<endl;
			}
		}

		return s.str();
	}


#define ETDEBUG 0
	bool Multipartite::exhaustiveTest(){
		double maxError=0;
		double rulp=1;
		int lsbIn=mpt->f->lsbIn;
		int lsbOut=mpt->f->lsbOut;
		if(lsbOut<0)
				rulp = 1.0 / ((double) (1<<(-lsbOut)));
		if(lsbOut>0)
			rulp =  (double) (1<<lsbOut);
		for (int x=0; x<(1<<inputSize); x++) {
			int64_t result;
			if(!mpt->compressTIV) {
				int a = x>>(inputSize-alpha);
				int64_t yTIV = tiv[a];
				result = yTIV;
#if ETDEBUG
				cerr 	<< "  x=" << x<< " tiv=" << result;
#endif
			}
			else { //compressed table
				int aa = x>>(inputSize-dcTIV.subsamplingIndexSize);
				int64_t ySSTIV = ssTIV[aa];

				int adiff = x>>(inputSize-alpha);
				int64_t yDiffTIV = diffTIV[adiff];
				result = (ySSTIV << dcTIV.subsamplingShift()) + yDiffTIV;
#if ETDEBUG
				cerr 	<< "  x=" << x<< " yssTIV=" << ySSTIV << " yDiffTIV=" << yDiffTIV ;
#endif
			}
			for(int i=0; i<m; i++) {
				int aTOi = (x>>pi[i]) & ((1<<betai[i])-1);
				int sign = 1-(aTOi >> (betai[i]-1) );
#if ETDEBUG
				cerr << "  aTO" << i << "I=" << aTOi << "  s=" << sign << "  ";
#endif
				aTOi = (aTOi & ((1<<(betai[i]-1))-1));
				if(sign==1)
					aTOi =  ((1<<(betai[i]-1))-1)  - aTOi;
				aTOi +=   ((x>>(inputSize-gammai[i])) << (betai[i]-1));
				int64_t yTOi = toi[i][aTOi];
#if ETDEBUG
				cerr << " aTO" << i << "F=" << aTOi << " yTOi="  << yTOi;
#endif
				if(negativeTOi[i])
					sign=1-sign;
				if(sign==1)
					yTOi = ~yTOi;
				result += yTOi;
			}
			//final rounding
			result = result >> guardBits;
			double fresult = ((double) result) * rulp;
			double ref = f->eval(   ((double)x) / ((double)(1<<(-lsbIn))) );
			double error = abs(fresult-ref);
			maxError = max(maxError, error);
#if ETDEBUG
			cerr //<< ((double)x) / ((double)(1<<(-lsbIn)))
			  << "  sum=" << result<< " fresult=" << fresult << "ref=" <<ref << "   e=" << error << " u=" <<rulp << (error > rulp ? " *******  Error here":"") <<  endl;
#endif
		}
		return (maxError < rulp);
	}

}

