/*
  Function Table for FloPoCo

  Authors: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,

  All rights reserved.

  */

#include "../utils.hpp"
#include "FixFunctionByTable.hpp"
#include <Tables/DiffCompressedTable.hpp>

using namespace std;

namespace flopoco{

	FixFunctionByTable::FixFunctionByTable(OperatorPtr parentOp_, Target* target_, string func_, bool signedIn_, int lsbIn_, int lsbOut_):
		Table(parentOp_, target_)
	{
		srcFileName="FixFunctionByTable";
		ostringstream name;
		name<<"FixFunctionByTable";
		setNameWithFreqAndUID(name.str());
		setCopyrightString("Florent de Dinechin (2010-2018)");

		f = new FixFunction(func_, signedIn_, lsbIn_, lsbOut_);
		addHeaderComment("Evaluator for " +  f-> getDescription() + "\n");
		wIn = f->wIn;
		wOut = f->wOut;
		if(wIn>30) {
			THROWERROR("lsbIn limited to -30 (a table with 1O^9 entries should be enough for anybody). Do you really want me to write a source file of "
								 << wOut * (mpz_class(1) << wIn) << " bytes?");
		}
		vector<mpz_class> v;
		for(int i=0; i<(1<<wIn); i++) {
			mpz_class rn, devnull;
			f->eval(mpz_class(i), rn, devnull, true);
			v.push_back(rn);
			//cerr <<   f-> getDescription()<< " : f("<< i << ") = " << rn <<endl;
		};

		Table::init(v, join("f", getNewUId()), wIn, wOut);
		if(target_->tableCompression()) {
			DiffCompressedTable::newUniqueInstance(this,	"X", "Y", v , "compressedTable", wIn, wOut);
		}
		else {
			Table::generateVHDL();
		}
	}


	
	FixFunctionByTable::~FixFunctionByTable() {
		delete f;
	}



	void FixFunctionByTable::emulate(TestCase* tc){
		f->emulate(tc, true /* correct rounding */);
	}

	OperatorPtr FixFunctionByTable::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
	{
		bool signedIn;
		int lsbIn, lsbOut;
		string f;
		UserInterface::parseString(args, "f", &f);
		UserInterface::parseBoolean(args, "signedIn", &signedIn);
		UserInterface::parseInt(args, "lsbIn", &lsbIn);
		UserInterface::parseInt(args, "lsbOut", &lsbOut);
		return new FixFunctionByTable(parentOp, target, f, signedIn, lsbIn, lsbOut);
	}

	void FixFunctionByTable::registerFactory()
	{
		UserInterface::add("FixFunctionByTable", // name
											 "Evaluator of function f on [0,1) or [-1,1), depending on signedIn, using a table.",
											 "FunctionApproximation",
											 "",
											 "f(string): function to be evaluated between double-quotes, for instance \"exp(x*x)\";\
signedIn(bool): if true the function input range is [-1,1), if false it is [0,1);\
lsbIn(int): weight of input LSB, for instance -8 for an 8-bit input;\
lsbOut(int): weight of output LSB;",
											 "This operator uses a table to store function values.",
											 FixFunctionByTable::parseArguments
											 ) ;
	}
}
