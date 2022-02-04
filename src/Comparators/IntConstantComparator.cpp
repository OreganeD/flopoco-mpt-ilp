#include "IntConstantComparator.hpp"

// some Kintex7 experiments using vivado 2020.2
// flopoco IntConstantComparator w=64 c=17979737894628297144

// Using plainVHDL, method=1, (gt is computed out of lt and eq)
//  w=64 flags=7: 53 LUT , 8 Carry4, Data Path Delay:	2.119ns (same as IntComparator)
//  w=64 flags=1: 31 LUT , 8 Carry4, Data Path Delay:	1.753ns (same as IntComparator)
// With a 64-bit comparator for a 32-bit constant we get same number of LUTs
// BUT
// w=32 flags=1: 6 LUT , no Carry4, Data Path Delay:	2.58ns
// It seems the logic optimizer is used in this case.

// Using two-level where the first level is a 5-bit comparison and the second level is a <:
//  w=64 flags=1: 24 LUT, 2 Carry4 (on the second level), Data Path Delay:  2.365ns    
// More or less conform to predictions: 64/5 -> 13 chunks -> 26 LUT + fast carry

// Using method=2 (binary tree, not exploiting fast carry) :  29 LUTs, 2.568 ns

using namespace std;
namespace flopoco{


	IntConstantComparator::IntConstantComparator(OperatorPtr parentOp, Target* target, int w, mpz_class c, int flags, int method) :
		Operator(parentOp, target), w(w), flags(flags), mpC(c), method(method) {

		srcFileName="IntConstantComparator";

		ostringstream name;
		name<<"IntConstantComparator_";
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

		if(method==1 && flags!=7){
			REPORT(0, "method=1 only makes sense for flags=7, reverting to method=0");
			method=0;
		}
		if(method==2 && flags!=7){
			REPORT(0, "Somebody has been lazy, method=2 only implemented for flags=7: this is what you shall get.");
			flags=7;
		}
		addInput ("X", w);
		if(flags&1) addOutput("XltC");
		if(flags&2) addOutput("XeqC");
		if(flags&4) addOutput("XgtC");

		vhdl << tab << declare("C",w) << " <= \""<<unsignedBinary(mpC, w)<<"\";"<<endl;

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
			
			// The following is copypasted from IntComparator, except the part defining chunkSize
			int chunkSize;
			if(getTarget()->plainVHDL()) {
				 chunkSize=w;
			}
			else {
				chunkSize=getTarget()->lutInputs();
			}
			
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
			
			if(chunkSizes.size() == 1)		{
				if(flags&1) vhdl << tab << declare(getTarget()->ltComparatorDelay(w), "XltCi") << " <= '1' when X<C else '0';"<<endl;
				if(flags&2) vhdl << tab << declare(getTarget()->eqComparatorDelay(w), "XeqCi") << " <= '1' when X=C else '0';"<<endl;
				if(flags&4) {
					if((method==0) || not(flags&1)) { // a third comparator 
						vhdl << tab << declare(getTarget()->ltComparatorDelay(w), "XgtCi") << " <= '1' when X>C else '0';"<<endl;
					}
					else{ // compute gt out of lt and eq
						vhdl << tab << declare(getTarget()->logicDelay(), "XgtCi") << " <= not (XeqCi or XltCi);"<<endl;
					}
				}
			}
			else {
				int l=0;
				for(int i=0; i<chunkSizes.size(); i++) {
					string chunkX = "X" + range(l+chunkSizes[i]-1, l);
					string chunkC = "C" + range(l+chunkSizes[i]-1, l);
					l += chunkSizes[i];
					// we will need the = of chunks in any case
					vhdl << tab << declare(getTarget()->eqComparatorDelay(chunkSizes[i]), join("XeqCi", i)) <<
							" <= '1' when " << chunkX << "=" << chunkC << " else '0';"<<endl;

					if(flags&1) {
						vhdl << tab << declare(getTarget()->ltComparatorDelay(chunkSizes[i]), join("XltCi",i)) <<
							" <= '1' when " << chunkX << "<" << chunkC << " else '0';"<<endl;
						// Now build two vectors XX and CC such that comparing XX and CC will be equivalent to comparing X and C
						//     01 when lt       00 when eq       10 when not (lt or eq)
						vhdl << tab << declare(join("XX", i)) << " <= not (" << join("XltCi",i) << " or " << join("XeqCi",i) << ");"<<endl;
						vhdl << tab << declare(join("CC", i)) << " <= " << join("XltCi",i) << ";"<<endl;
					}
					if(flags&4 && not (flags&1)) { // cases when we need to build the  XgtCi of chunks
						vhdl << tab << declare(getTarget()->ltComparatorDelay(chunkSizes[i]), join("XgtCi", i))<< 
							" <= '1' when " << chunkX << ">" << chunkC << " else '0';" << endl;
						// Now build two vectors XX and CC such that comparing XX and CC will be equivalent to comparing X and C
						//     01 when not (gt or eq)       00 when eq       10 when gt 
						vhdl << tab << declare(join("XX", i)) << " <= " << join("XgtCi",i) << ";"<<endl;
						vhdl << tab << declare(join("CC", i)) << " <= not (" << join("XgtCi",i) << " or " << join("XeqCi",i) << ");"<<endl;
					}
					if(flags==2) { // this case needs specific treatment
						// Now build two vectors XX and CC such that comparing XX and CC will be equivalent to comparing X and C
						//     00 when eq       10 when noteq
						// generated code could be more elegant in this case but the optimizer should do the right thing
						vhdl << tab << declare(join("XX", i)) << " <= not " << join("XeqCi",i) << ";"<<endl;
						vhdl << tab << declare(join("CC", i)) << " <= '0';"<<endl;
					}
				}

				addComment("XXX and CCX are two synthetic numbers such that comparing XX and CC is be equivalent to comparing X and C");
				vhdl << tab << declare("XXX", chunkSizes.size()) << " <= " ;
				for(int i=chunkSizes.size()-1; i>=0; i--) {
					vhdl << join("XX",i) << (i==0?"":" & ");
				}
				vhdl <<";"<<endl;
				vhdl << tab << declare("CCC", chunkSizes.size()) << " <= " ;
				for(int i=chunkSizes.size()-1; i>=0; i--) {
					vhdl << join("CC",i) << (i==0?"":" & ");
				}
				vhdl <<";"<<endl;
				
				if(flags&1)
					vhdl << tab << declare(getTarget()->ltComparatorDelay(chunkSizes.size()), "XltCi") << " <= '1' when XXX<CCC else '0';"<<endl;
				if(flags&2)
					vhdl << tab << declare(getTarget()->eqComparatorDelay(chunkSizes.size()), "XeqCi") << " <= '1' when XXX=CCC else '0';"<<endl;
				if(flags&4) {
					if((method==0) || not(flags&1)) { // a third comparator 
						vhdl << tab << declare("XgtCi") << " <= '1' when XXX>CCC else '0';"<<endl;
					}
					else{ // compute gt out of lt and eq
						vhdl << tab << declare(getTarget()->logicDelay(), "XgtCi") << " <= not (XeqCi or XltCi);"<<endl;
					}
				}

			}
		}		
		if(method==2) {
			// plain binary tree, just to check that The Book is correct
			// Useful for ASIC some day, probably not for FPGA

			// initialize the leaves
			for(int i=0; i<w; i++) {
				vhdl << tab << declare("C_" + to_string(i) + "_" + to_string(i), 2)
						 << " <= " << "X" << of(i) << " & "  <<  "C" << of(i) <<  ";"<<endl;
			}
			REPORT(DETAILED, "padding to the next power of two");
			int j=1 << intlog2(w); // next power of two
			for(int i=w; i<j; i++) {
				vhdl << tab << declare("C_" + to_string(i) + "_" + to_string(i), 2)
						 << " <= \"00\" ;"<<endl;
			}
			w=j;
			
			int level=0;
			int stride=1; // invariant stride=2^level
			string Cd; // it needs to exit the loop
			while (stride<w) { // need to add one more level
				REPORT(DETAILED, "level=" << level);
				level+=1;
				stride *= 2;
				for(int i=0; i<w; i+=stride) {
					Cd = "C_" + to_string(stride-1+i) + "_" + to_string(i);
					string Cl = "C_" + to_string(stride-1+i)+"_"+to_string(stride/2+i);
					string Cr = "C_" + to_string(stride/2-1 + i)+"_"+to_string(i);
					vhdl << tab << declare(getTarget()->logicDelay(), Cd,2) << " <= " << endl;
					vhdl << tab << tab << "      \"01\"  when " << Cl << "=\"01\" or (("<< Cl << "=\"00\" or "<< Cl << "=\"11\") and (" << Cr << "=\"01\" ))" << endl;
					vhdl << tab << tab << "else  \"10\"  when " << Cl << "=\"10\" or (("<< Cl << "=\"00\" or "<< Cl << "=\"11\") and (" << Cr << "=\"10\" ))" << endl;
					vhdl << tab << tab << "else  \"00\" ;" << endl; 
				}
			}
			vhdl << tab << declare("XltCi") << " <= '1' when " << Cd << "=\"01\" else '0' ;" << endl;
			vhdl << tab << declare("XgtCi") << " <= '1' when " << Cd << "=\"10\" else '0' ;" << endl;
			vhdl << tab << declare("XeqCi") << " <= '1' when " << Cd << "=\"00\" or " << Cd << "=\"11\"  else '0' ;" << endl;
		}
		// Copying intermediate signals to output
    if(flags&1) 		vhdl << tab << "XltC <= XltCi;"<<endl;
    if(flags&2) 		vhdl << tab << "XeqC <= XeqCi;"<<endl;
    if(flags&4) 		vhdl << tab << "XgtC <= XgtCi;"<<endl;
	}
	

  IntConstantComparator::~IntConstantComparator(){};

	void IntConstantComparator::emulate(TestCase *tc)
	{
		mpz_class svX = tc->getInputValue("X");

		bool XltC = (svX<mpC);
		bool XeqC = (svX==mpC);
		bool XgtC = (svX>mpC);
		if(flags&1) tc->addExpectedOutput("XltC", XltC);
		if(flags&2) tc->addExpectedOutput("XeqC", XeqC);
		if(flags&4) tc->addExpectedOutput("XgtC", XgtC);
	}


	
	void IntConstantComparator::buildStandardTestCases(TestCaseList* tcl){
		/* No regression tests yet */
	}

	
	OperatorPtr IntConstantComparator::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
		int w, flags, method;
		string c;
		UserInterface::parseStrictlyPositiveInt(args, "w", &w);
		UserInterface::parseString(args, "c", &c);
		mpz_class mpc(c); // TODO catch exceptions here?

		UserInterface::parseStrictlyPositiveInt(args, "flags", &flags);
		UserInterface::parseInt(args, "method", &method);
		return new IntConstantComparator(parentOp, target, w, mpc, flags, method);
	}


	
	TestList IntConstantComparator::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		FloPoCoRandomState::init(10, false);

		if(index==-1) 
		{ // The unit tests

			for(int w=4; w<1000; w+=1+(w<10?0:2)+(w<30?0:17)+(w<100?0:300)) {
				for(int flags=1; flags<(w<100?8:3); flags++) { 
					for(int method=0; method<3; method++) { 
						mpz_class c = getLargeRandom(w);
						ostringstream s;
						s << c;
						paramList.push_back(make_pair("w",to_string(w)));
						paramList.push_back(make_pair("c", s.str()));
						paramList.push_back(make_pair("flags",to_string(flags)));
						paramList.push_back(make_pair("method",to_string(method)));
						testStateList.push_back(paramList);
						paramList.clear();
					}
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

	
	void IntConstantComparator::registerFactory(){
		UserInterface::add("IntConstantComparator", // name
			"An integer comparator.",
			"BasicInteger",
			"", //seeAlso
			"w(int): size in bits of integers to be compared;\
			c(int): constant;\
			flags(int)=7: if bit 0 set output  X<C, if bit 1 set output X=C, if bit 2 set output  X>C;\
			method(int)=-1: method to be used, for experimental purpose (-1: automatic, 0: symmetric, 1: asymmetric where gt is computed out of lt and eq, 2: binary tree) (plainVHDL option also supported);",
			"Outputs up to 3 mutually exclusive signals:  XltC (less than, strictly), XeqC (equal), XgtC (greater than, strictly)",
			IntConstantComparator::parseArguments,
			IntConstantComparator::unitTest
			) ;
	}


}
