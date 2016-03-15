/*
An integer adder for FloPoCo

It may be pipelined to arbitrary frequency.
Also useful to derive the carry-propagate delays for the subclasses of Target

Authors:  Bogdan Pasca, Florent de Dinechin

This file is part of the FloPoCo project
developed by the Arenaire team at Ecole Normale Superieure de Lyon

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2008-2010.
  All rights reserved.
*/

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "IntAdder.hpp"

// #include "IntAdderClassical.hpp"
//#include "IntAdderAlternative.hpp"
//#include "IntAdderShortLatency.hpp"

using namespace std;
namespace flopoco {

	IntAdder::IntAdder ( Target* target, int wIn_, int optimizeType, bool srl, int implementation):
		Operator ( target), wIn ( wIn_ )
	{
		srcFileName="IntAdder";
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2016)" );
		ostringstream name;
		name << "IntAdder_" << wIn;
		setNameWithFreqAndUID(name.str());
													
		// Set up the IO signals
		addInput  ("X"  , wIn, true);
		addInput  ("Y"  , wIn, true);
		addInput  ("Cin");
		addOutput ("R"  , wIn, 1 , true);

		vhdl << tab << " R <= X + Y + Cin;" << endl;
		getSignalByName("R")->setCriticalPathContribution(getTarget()->adderDelay(wIn+1));

	}



	/*************************************************************************/
	IntAdder::~IntAdder() {
	}

	/*************************************************************************/
	void IntAdder::emulate ( TestCase* tc ) {
		// get the inputs from the TestCase
		mpz_class svX = tc->getInputValue ( "X" );
		mpz_class svY = tc->getInputValue ( "Y" );
		mpz_class svC = tc->getInputValue ( "Cin" );

		// compute the multiple-precision output
		mpz_class svR = svX + svY + svC;
		// Don't allow overflow: the output is modulo 2^wIn
		svR = svR & ((mpz_class(1)<<wIn)-1);

		// complete the TestCase with this expected output
		tc->addExpectedOutput ( "R", svR );
	}


	OperatorPtr IntAdder::parseArguments(Target *target, vector<string> &args) {
		int wIn;
		int arch;
		int optObjective;
		bool srl;
		UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn, false);
		UserInterface::parseInt(args, "arch", &arch, false);
		UserInterface::parseInt(args, "optObjective", &optObjective, false);
		UserInterface::parseBoolean(args, "SRL", &srl, false);
		return new IntAdder(target, wIn,optObjective,srl,arch);
	}

	void IntAdder::registerFactory(){
		UserInterface::add("IntAdder", // name
											 "Integer adder. In modern VHDL, integer addition is expressed by a + and one usually needn't define an entity for it. However, this operator will be pipelined if the addition is too large to be performed at the target frequency.",
											 "BasicInteger", // category
											 "",
											 "wIn(int): input size in bits;\
							          arch(int)=-1: -1 for automatic, 0 for classical, 1 for alternative, 2 for short latency; \
							          optObjective(int)=2: 0 to optimize for logic, 1 to optimize for register, 2 to optimize for slice/ALM count; \
							          SRL(bool)=true: optimize for shift registers",
											 "",
											 IntAdder::parseArguments
											 );
		
	}


}


