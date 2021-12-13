#include "IntComparator.hpp"


using namespace std;
namespace flopoco{


  IntComparator::IntComparator(OperatorPtr parentOp, Target* target, int w, int flags, int method) :
		Operator(parentOp, target), w(w), flags(flags), method(method) {

		srcFileName="IntComparator";

		ostringstream name;
		name<<"IntComparator_";
		name << w << "_" << (flags&1) << ((flags>>1)&1) << ((flags>>2)&1);
		setNameWithFreqAndUID(name.str());
		setCopyrightString("Florent de Dinechin (2021)");

		if( (flags<=0) || (flags>7)) {
			THROWERROR("incorrect value of flags: " << flags)
		}
		addInput ("X", w);
		addInput ("Y", w);
		if(flags&1) addOutput("XltY");
		if(flags&2) addOutput("XeqY");
		if(flags&4) addOutput("XgtY");


		if(method==1) { // Plain VHDL, asymmetric: lower area, larger delay
			// w=64 flags=7 Wrapper: 55 LUT , 8 Carry4, Data Path Delay:         2.003ns
			if (flags!=7){
				REPORT(0, "method=1 only makes sense for flags=7, reverting to method=0");
				method=0;
			}
			else{
				vhdl << tab << declare("XltYi") << " <= '1' when X<Y else '0';"<<endl;
				vhdl << tab << declare("XeqYi") << " <= '1' when X=Y else '0';"<<endl;
				vhdl << tab << declare("XgtYi") << " <= not (XeqYi or XltYi);"<<endl;
			}
		} // end method=1


		if(method==0) { // Plain VHDL 
			// w=64 flags=7 Wrapper: 86 LUT , 8 Carry4, Data Path Delay:         1.371ns 
			// w=64 flags=1 Wrapper: 32 LUT , 8 Carry4, Data Path Delay:         1.354ns 
			// w=64 flags=2 Wrapper: 22 LUT , 8 Carry4, Data Path Delay:         1.375ns
			// w=64 flags=3 Wrapper: 54 LUT , 8 Carry4, Data Path Delay:         1.361ns
			// All this is very consistent. eq can pack 3 bits/LUT, lt and gt pack 2 bits/LUT
    if(flags&1) vhdl << tab << declare("XltYi") << " <= '1' when X<Y else '0';"<<endl;
		if(flags&2) vhdl << tab << declare("XeqYi") << " <= '1' when X=Y else '0';"<<endl;
		if(flags&4) vhdl << tab << declare("XgtYi") << " <= '1' when X>Y else '0';"<<endl;
		} // end method=0

#if 0
		if(method==2) { // Here should come a mix of ternary and binary tree
			// This is a Dadda-like algorithm. We aim at the depth of a ternary tree but do as much of it as  
			int level=0;
			bool done=false;
			while(not done) {
				
			}
		}
#endif
		// Copying intermediate signals to output
    if(flags&1) 		vhdl << tab << "XltY <= XltYi;"<<endl;
    if(flags&2) 		vhdl << tab << "XeqY <= XeqYi;"<<endl;
    if(flags&4) 		vhdl << tab << "XgtY <= XgtYi;"<<endl;
	}
	

  IntComparator::~IntComparator(){};

	void IntComparator::emulate(TestCase *tc)
	{
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		bool XltY = (svX<svY);
		bool XeqY = (svX==svY);
		bool XgtY = (svX>svY);
		if(flags&1) tc->addExpectedOutput("XltY", XltY);
		if(flags&2) tc->addExpectedOutput("XeqY", XeqY);
		if(flags&4) tc->addExpectedOutput("XgtY", XgtY);
	}


	
	void IntComparator::buildStandardTestCases(TestCaseList* tcl){
		/* No regression tests yet */
	}

	
	OperatorPtr IntComparator::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int w, flags, method;
		UserInterface::parseStrictlyPositiveInt(args, "w", &w); 
		UserInterface::parseStrictlyPositiveInt(args, "flags", &flags);
		UserInterface::parsePositiveInt(args, "method", &method);
		return new IntComparator(parentOp, target, w, flags, method);
	}


	
	TestList IntComparator::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			for(int w=5; w<53; w+=3) {
				paramList.push_back(make_pair("w",to_string(w)));
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

	
	void IntComparator::registerFactory(){
		UserInterface::add("IntComparator", // name
			"An integer comparator.",
			"BasicInteger",
			"", //seeAlso
			"w(int): size in bits of integers to be compared;\
			flags(int)=7: if bit 0 set output  X<Y, if bit 1 set output X=Y, if bit 2 set output  X>Y;\
			method(int)=0: method to be used, for experimental purpose;",
			"Outputs up to 3 mutually exclusive signals:  XltY (less than, strictly), XeqY (equal), XsgtY (greater than, strictly)",
			IntComparator::parseArguments,
			IntComparator::unitTest
			) ;
	}


}
