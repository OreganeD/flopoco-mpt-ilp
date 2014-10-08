/*
  Polynomial Function Evaluator for FloPoCo
	This version uses piecewise polynomial approximation for a trade-off between tables and multiplier hardware.
	
  Authors: Florent de Dinechin (rewrite from scratch of original code by Mioara Joldes and Bogdan Pasca, see the attic directory)

  This file is part of the FloPoCo project
	launched by the Arénaire/AriC team of Ecole Normale Superieure de Lyon
  currently developed by the Socrate team at INSA de Lyon
 
  Initial software.
  Copyright © ENS-Lyon, INSA-Lyon, INRIA, CNRS, UCBL,  
  2008-2014.
  All rights reserved.

  */

#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "../utils.hpp"


#include "FixFunctionByPiecewisePoly.hpp"
#include "FixHornerEvaluator.hpp"

#ifdef HAVE_SOLLYA

using namespace std;

namespace flopoco{

#define DEBUGVHDL 0

	FixFunctionByPiecewisePoly::CoeffTable::CoeffTable(Target* target, int wIn, int wOut, PiecewisePolyApprox* polyApprox_) :
	Table(target, wIn, wOut), polyApprox(polyApprox_)
	{
		srcFileName="FixFunctionByPiecewisePoly::CoeffTable";
		ostringstream name; 
		name <<"CoeffTable_" << wIn << "_" << wOut << "_" << getNewUId();
		setName(name.str());
		// outDelayMap["Y"] = target->RAMDelay(); // is this useful?
	}


	mpz_class FixFunctionByPiecewisePoly::CoeffTable::function(int x){
		mpz_class z=0;
		int currentShift=0;
		for(int i=0; i<=polyApprox->degree; i++) {
			z += (polyApprox-> getCoeff(x, i)) << currentShift; // coeff of degree i from poly number x
			currentShift +=  polyApprox->MSB[i] - polyApprox->LSB +1;
		}
		return z;
	}





	FixFunctionByPiecewisePoly::FixFunctionByPiecewisePoly(Target* target, string func, int lsbIn, int msbOut, int lsbOut, int degree_, bool finalRounding_, bool plainStupidVHDL, map<string, double> inputDelays):
		Operator(target, inputDelays), degree(degree_), finalRounding(finalRounding_){

		if(finalRounding==false){
			THROWERROR("FinalRounding=false not implemented yet" );
		}

		f=new FixFunction(func, lsbIn, msbOut, lsbOut); // this will provide emulate etc.
		
		srcFileName="FixFunctionByPiecewisePoly";
		
		ostringstream name;
		name<<"FixFunctionByPiecewisePoly_"<<getNewUId(); 
		setName(name.str()); 

		setCopyrightString("Florent de Dinechin (2014)");
		addHeaderComment("-- Evaluator for " +  f-> getDescription() + "\n"); 
		int wX=-lsbIn;
		addInput("X", wX);
		int outputSize = msbOut-lsbOut+1; // TODO finalRounding would impact this line
		addOutput("Y" ,outputSize , 2);
		useNumericStd();

		// Build the polynomial approximation
		double targetAcc= pow(2, lsbOut-1);
		polyApprox = new PiecewisePolyApprox(func, targetAcc, degree);
		int alpha =  polyApprox-> alpha; // coeff table input size 
		// Store it in a table
		int polyTableOutputSize=0;
		for (int i=0; i<=degree; i++) {
			polyTableOutputSize += polyApprox->MSB[i] - polyApprox->LSB +1;
		} 
		REPORT(DETAILED, "Poly table input size  = " << alpha);
		REPORT(DETAILED, "Poly table output size = " << polyTableOutputSize);

		FixFunctionByPiecewisePoly::CoeffTable* coeffTable = new CoeffTable(target, alpha, polyTableOutputSize, polyApprox) ;
		addSubComponent(coeffTable);

		vhdl << tab << declare("A", alpha)  << " <= X" << range(wX-1, wX-alpha) << ";" << endl;
		vhdl << tab << declare("Z", wX-alpha)  << " <= X" << range(wX-alpha-1, 0) << ";" << endl;

		inPortMap(coeffTable, "X", "A");
		outPortMap(coeffTable, "Y", "Coeffs");
		vhdl << instance(coeffTable, "coeffTable") << endl;

		int currentShift=0;
		for(int i=0; i<=polyApprox->degree; i++) {
			vhdl << tab << declare(join("A",i), polyApprox->MSB[i] - polyApprox->LSB +1)  << " <= Coeffs" << range(currentShift + (polyApprox->MSB[i] - polyApprox->LSB), currentShift) << ";" << endl;
			currentShift +=  polyApprox->MSB[i] - polyApprox->LSB +1;
		}

		FixHornerEvaluator* horner = new FixHornerEvaluator(target, lsbIn+alpha, msbOut, lsbOut, degree, polyApprox->MSB, polyApprox->LSB, true, true, true);		
		addSubComponent(horner);

		inPortMap(horner, "X", "Z");
		outPortMap(horner, "R", "Ys");
		for(int i=0; i<=polyApprox->degree; i++) {
			inPortMap(horner,  join("A",i),  join("A",i));
		}
		vhdl << instance(horner, "horner") << endl;
		
		vhdl << tab << "Y <= " << "std_logic_vector(Ys);" << endl;
	}



	FixFunctionByPiecewisePoly::~FixFunctionByPiecewisePoly() {
		free(f);
	}
	


	void FixFunctionByPiecewisePoly::emulate(TestCase* tc){
		f->emulate(tc);
	}

	void FixFunctionByPiecewisePoly::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		tc = new TestCase(this); 
		tc->addInput("X", 0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", (mpz_class(1) << f->wIn) -1);
		emulate(tc);
		tcl->add(tc);

	}

}
	
#endif //HAVE_SOLLYA	
