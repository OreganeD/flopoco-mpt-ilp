#include "FPComparator.hpp"


using namespace std;
namespace flopoco{


  FPComparator::FPComparator(OperatorPtr parentOp, Target* target, int wE, int wF) :
		Operator(parentOp, target), wE(wE), wF(wF) {

		srcFileName="FPComparator";

		ostringstream name;
		name<<"FPComparator_";
		name <<wE<<"_"<<wF;
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Florent de Dinechin (2021)");

		addFPInput ("X", wE, wF);
		addFPInput ("Y", wE, wF);
		addOutput("unordered");
		addOutput("XltY");
		addOutput("XeqY");
		addOutput("XgtY");
		addOutput("XleY");
		addOutput("XgeY");

		REPORT(0, "This operator is work in progress");
		
	}
	

  FPComparator::~FPComparator(){};

	void FPComparator::emulate(TestCase *tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		/* get the various fields */
		FPNumber fpx(wE, wF, svX);
		FPNumber fpy(wE, wF, svY);
		mpz_class exnx=fpx.getExceptionSignalValue();
		mpz_class exny=fpy.getExceptionSignalValue();
		bool unordered = (exnx==3 || exny==3);

		// the following is not efficient, but quite safer than replicating here the logic of the VHDL code... 
		fpx = svX;
		fpy = svY;
		mpfr_t x, y, r;
		mpfr_init2(x, 1+wF);
		mpfr_init2(y, 1+wF);
		fpx.getMPFR(x);
		fpy.getMPFR(y);
		int cmp = mpfr_cmp(x,y);
		bool XltY = (cmp<0) and not unordered;
		bool XeqY = (cmp==0) and not unordered;
		bool XgtY = (cmp>0) and not unordered;
		bool XgeY = (cmp>=0) and not unordered;
		bool XleY = (cmp<=0) and not unordered;
		tc->addExpectedOutput("unordered", unordered);
		tc->addExpectedOutput("XltY", XltY);
		tc->addExpectedOutput("XeqY", XeqY);
		tc->addExpectedOutput("XgtY", XgtY);
		tc->addExpectedOutput("XleY", XleY);
		tc->addExpectedOutput("XgeY", XgeY);
		// clean up
		mpfr_clears(x, y, NULL);
	}


	
	void FPComparator::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		// Regression tests
		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::NaN);
		tc->addFPInput("Y", FPNumber::NaN);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::NaN);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", 2.0);
		emulate(tc);
		tcl->add(tc);
			
		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", -1.0);
		emulate(tc);
		tcl->add(tc);
			
		tc = new TestCase(this);
		tc->addFPInput("X", -1.0);
		tc->addFPInput("Y", -2.0);
		emulate(tc);
		tcl->add(tc);
			
		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::minusDirtyZero);
		tc->addFPInput("Y", FPNumber::minusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::minusDirtyZero);
		tc->addFPInput("Y", FPNumber::plusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::minusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::plusInfty);
		tc->addFPInput("Y", FPNumber::minusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::minusInfty);
		tc->addFPInput("Y", FPNumber::minusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::plusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addFPInput("X", FPNumber::plusInfty);
		tc->addFPInput("Y", FPNumber::plusInfty);
		emulate(tc);
		tcl->add(tc);

	}

	
	OperatorPtr FPComparator::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int wE, wF;
		UserInterface::parseStrictlyPositiveInt(args, "wE", &wE); 
		UserInterface::parseStrictlyPositiveInt(args, "wF", &wF);

		return new FPComparator(parentOp, target, wE, wF);
	}

	TestList FPComparator::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			for(int wF=5; wF<53; wF+=1) {
				int wE = 6+(wF/10);
				while(wE>wF)
					wE -= 2;
				
				paramList.push_back(make_pair("wF",to_string(wF)));
				paramList.push_back(make_pair("wE",to_string(wE)));
				testStateList.push_back(paramList);
				paramList.clear();
			}
		}
		else     
		{
			// finite number of random test computed out of index
			// TODO
		}	
		
		return testStateList;
	}

	void FPComparator::registerFactory(){
		UserInterface::add("FPComparator", // name
			"An IEEE-like floating-point comparator.",
			"BasicFloatingPoint",
			"", //seeAlso
			"wE(int): exponent size in bits; \
			wF(int): mantissa size in bits;",
			"Outputs 4 mutually exclusive signals: unordered (when one input is NaN), XltY (less than, strictly), XeqY (equal), XsgtY (greater than, strictly). Also two derived signals XleY (less or equal)  and XgeY (greater or equal). unordered is set iff at least one of the inputs is NaN. The other ones behave as expected on two non-NaN values, with the IEEE 754 conventions: +0=-0;  infinities of the same sign compare equal. ",
			FPComparator::parseArguments,
			FPComparator::unitTest
			) ;
	}


}
