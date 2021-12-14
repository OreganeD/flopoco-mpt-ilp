#include "IntComparator.hpp"


using namespace std;
namespace flopoco{

	// some Kintex7 experiments
	// using method=0
	//  w=64 flags=7 Wrapper: 86 LUT , 8 Carry4, Data Path Delay:         1.371ns 
	//  w=64 flags=1 Wrapper: 32 LUT , 8 Carry4, Data Path Delay:         1.354ns 
	//  w=64 flags=2 Wrapper: 22 LUT , 8 Carry4, Data Path Delay:         1.375ns
	//  w=64 flags=3 Wrapper: 54 LUT , 8 Carry4, Data Path Delay:         1.361ns
	// All this is very consistent. eq can pack 3 bits/LUT, lt and gt pack 2 bits/LUT
	// Using  method=1, where gt is computed out of lt and eq asymmetric: lower area, larger delay
	// w=64 flags=7 Wrapper: 55 LUT , 8 Carry4, Data Path Delay:         2.003ns
	

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
		if(method==-1) { // the user asked us to choose
			if(flags==7) {
				method=1; // asymmetric
			}
			else {
				method=0;
			}
		}

		addInput ("X", w);
		addInput ("Y", w);
		if(flags&1) addOutput("XltY");
		if(flags&2) addOutput("XeqY");
		if(flags&4) addOutput("XgtY");


		if(method==1 && flags!=7){
			REPORT(0, "method=1 only makes sense for flags=7, reverting to method=0");
			method=0;
		}

		if (method==0 || method ==1) {
			// determine if we have to split the input to reach the target frequency
			
			double targetPeriod = 1.0/getTarget()->frequency() - getTarget()->ffDelay();
			// What is the maximum lexicographic time of our inputs?
			int maxCycle;
			double maxCP;
			getIOMaxLexicographicTime(maxCycle, maxCP);
			double totalPeriod;
			if(flags==2){ // equality test only
				totalPeriod = maxCP + getTarget()->eqComparatorDelay(w);
			}
			else { // at least one lt or gt
				totalPeriod = maxCP + getTarget()->ltComparatorDelay(w);
			}
			
			REPORT(DETAILED, "maxCycle=" << maxCycle <<  "  maxCP=" << maxCP <<  "  totalPeriod=" << totalPeriod <<  "  targetPeriod=" << targetPeriod );
#if 0
			
			int chunks=0; // optimistic case
			vector<int> chunkSizes;		
			int coveredSize=0;	
			while(coveredSize<w) {
				chunks++;
				int chunkSize=1;
				while (maxCP + (flags==2?getTarget()->eqComparatorDelay(chunkSize):getTarget()->ltComparatorDelay(chunkSize)) < targetPeriod)
					chunkSize++;
				chunkSize--;
				if(coveredSize+chunkSize>w) {
					chunkSize = w-coveredSize;
				}
				coveredSize += chunkSize;
				chunkSizes.push_back(chunkSize);
				maxCP=0; // after the first chunk
			}
			REPORT(DETAILED, "Found " << chunks << " chunks");
			for (int i=0; i<chunks; i++) {
				REPORT(DETAILED, "   chunk " <<i << " : " << chunkSizes[i] << " bit");
			}
#else
			int chunkSize=1;
			while (maxCP + (flags==2?getTarget()->eqComparatorDelay(chunkSize):getTarget()->ltComparatorDelay(chunkSize)) < targetPeriod)
				chunkSize++;
			chunkSize--;
			REPORT(DETAILED, "The first level must be split in chunks of " << chunkSize << " bits");

			int coveredSize=0;	
			vector<int> chunkSizes;		
			while(coveredSize<w) {
				coveredSize += chunkSize;
				if(coveredSize>w) {
					chunkSizes.push_back(w-coveredSize+chunkSize);
				}
				else {
					chunkSizes.push_back(chunkSize);
				}
			}
			
#endif
			// logic is: if both > and < are required then compute 
			if(chunkSizes.size() == 1)		{
				if(flags&1) vhdl << tab << declare(getTarget()->ltComparatorDelay(w), "XltYi") << " <= '1' when X<Y else '0';"<<endl;
				if(flags&2) vhdl << tab << declare(getTarget()->eqComparatorDelay(w), "XeqYi") << " <= '1' when X=Y else '0';"<<endl;
				if(flags&4) {
					if((method==0) || (0==flags&1)) { // a third comparator 
						vhdl << tab << declare(getTarget()->ltComparatorDelay(w), "XgtYi") << " <= '1' when X>Y else '0';"<<endl;
					}
					else{ // compute gt out of lt and eq
						vhdl << tab << declare(getTarget()->logicDelay(), "XgtYi") << " <= not (XeqYi or XltYi);"<<endl;
					}
				}
			}
			else {
				addComment("For this frequency we need to split the comparison into two levels");
				int l=0;
				for(int i=0; i<chunkSizes.size(); i++) {
					string chunkX = "X" + range(l+chunkSizes[i]-1, l);
					string chunkY = "Y" + range(l+chunkSizes[i]-1, l);
					l += chunkSizes[i];
					// we will need the = of chunks in any case
					vhdl << tab << declare(getTarget()->eqComparatorDelay(chunkSizes[i]), join("XeqYi", i)) <<
							" <= '1' when " << chunkX << "=" << chunkY << " else '0';"<<endl;

					if(flags&1) {
						vhdl << tab << declare(getTarget()->ltComparatorDelay(chunkSizes[i]), join("XltYi",i)) <<
							" <= '1' when " << chunkX << "<" << chunkY << " else '0';"<<endl;
						// Now build two vectors XX and YY such that comparing XX and YY will be equivalent to comparing X and Y
						//     01 when lt       00 when eq       10 when not (lt or eq)
						vhdl << tab << declare(join("XX", i)) << " <= not (" << join("XltYi",i) << " or " << join("XeqYi",i) << ");"<<endl;
						vhdl << tab << declare(join("YY", i)) << " <= " << join("XltYi",i) << ";"<<endl;
					}
					if(flags&4 && not (flags&1)) { // cases when we need to build the  XgtYi of chunks
						vhdl << tab << declare(getTarget()->ltComparatorDelay(chunkSizes[i]), join("XgtYi", i))<< 
							" <= '1' when " << chunkX << ">" << chunkY << " else '0';" << endl;
						// Now build two vectors XX and YY such that comparing XX and YY will be equivalent to comparing X and Y
						//     01 when not (gt or eq)       00 when eq       10 when gt 
						vhdl << tab << declare(join("XX", i)) << " <= " << join("XgtYi",i) << ";"<<endl;
						vhdl << tab << declare(join("YY", i)) << " <= not (" << join("XgtYi",i) << " or " << join("XeqYi",i) << ");"<<endl;
					}
				}

				addComment("XXX and YYX are two synthetic numbers such that comparing XX and YY is be equivalent to comparing X and Y");
				vhdl << tab << declare("XXX", chunkSizes.size()) << " <= " ;
				for(int i=chunkSizes.size()-1; i>=0; i--) {
					vhdl << join("XX",i) << (i==0?"":" & ");
				}
				vhdl <<";"<<endl;
				vhdl << tab << declare("YYY", chunkSizes.size()) << " <= " ;
				for(int i=chunkSizes.size()-1; i>=0; i--) {
					vhdl << join("YY",i) << (i==0?"":" & ");
				}
				vhdl <<";"<<endl;
				
				if(flags&1)
					vhdl << tab << declare(getTarget()->ltComparatorDelay(chunkSizes.size()), "XltYi") << " <= '1' when XXX<YYY else '0';"<<endl;
				if(flags&2)
					vhdl << tab << declare(getTarget()->eqComparatorDelay(chunkSizes.size()), "XeqYi") << " <= '1' when XXX=YYY else '0';"<<endl;
				if(flags&4) {
					if((method==0) || (0==flags&1)) { // a third comparator 
						vhdl << tab << declare("XgtYi") << " <= '1' when XXX>YYY else '0';"<<endl;
					}
					else{ // compute gt out of lt and eq
						vhdl << tab << declare(getTarget()->logicDelay(), "XgtYi") << " <= not (XeqYi or XltYi);"<<endl;
					}
				}

			}
		}		
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
		UserInterface::parseInt(args, "method", &method);
		return new IntComparator(parentOp, target, w, flags, method);
	}


	
	TestList IntComparator::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			for(int w=4; w<400; w+=100) { // 5 is an exhaustive test. The others test the decomposition in chunks
				for(int flags=1; flags<8; flags++) { // 5 is an exhaustive test. The others test the decomposition in chunks
					paramList.push_back(make_pair("w",to_string(w)));
					paramList.push_back(make_pair("flags",to_string(flags)));
					testStateList.push_back(paramList);
					paramList.clear();
				}
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
			method(int)=-1: method to be used, for experimental purpose (-1: automatic, 0: symmetric plain VHDL, 1: asymmetric plain VHDL where gt is computed out of lt and eq);",
			"Outputs up to 3 mutually exclusive signals:  XltY (less than, strictly), XeqY (equal), XsgtY (greater than, strictly)",
			IntComparator::parseArguments,
			IntComparator::unitTest
			) ;
	}


}
