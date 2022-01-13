#include "FPComparator.hpp"


using namespace std;
namespace flopoco{


  FPComparator::FPComparator(OperatorPtr parentOp, Target* target, int wE, int wF, int flags, int method) :
		Operator(parentOp, target), wE(wE), wF(wF), flags(flags), method(method) {

		srcFileName="FPComparator";

		ostringstream name;
		name<<"FPComparator_";
		name <<wE<<"_"<<wF;
		setNameWithFreqAndUID(name.str());

		setCopyrightString("Florent de Dinechin (2021)");

		addFPInput ("X", wE, wF);
		addFPInput ("Y", wE, wF);
		addOutput("unordered");
		if(flags&1)	 addOutput("XltY");
		if(flags&2)	 addOutput("XeqY");
		if(flags&4)	 addOutput("XgtY");
		if(flags&8)	 addOutput("XleY");
		if(flags&16) addOutput("XgeY");
		//		addOutput("XneY"); This one doesn't exist in IEEE, it was a mistake to add it

		vhdl << tab << declare("excX",2) << " <= X"<<range(wE+wF+2, wE+wF+1) <<";"<<endl;
		vhdl << tab << declare("excY",2) << " <= Y"<<range(wE+wF+2, wE+wF+1) <<";"<<endl;
		vhdl << tab << declare("signX") << " <= X"<<of(wE+wF) <<";"<<endl;
		vhdl << tab << declare("signY") << " <= Y"<<of(wE+wF) <<";"<<endl;
		vhdl << tab << declare("ExpFracX",wE+wF) << " <= X"<<range(wE+wF-1, 0)<<";"<<endl;
		vhdl << tab << declare("ExpFracY",wE+wF) << " <= Y"<<range(wE+wF-1, 0)<<";"<<endl;
		
		addComment("Comparing (as integers) excX & ExpFracX with excY & ExpFracY would almost work ");
		addComment(" since indeed inf>normal>0	");
		addComment("However we wouldn't capture infinity equality in cases when the infinities have different ExpFracs (who knows)...	 ");
		addComment("Besides, expliciting the isXXX bits will help factoring code with a comparator for IEEE format (some day)");
		vhdl << tab << declare("isZeroX") << " <= '1' when excX=\"00\" else '0' ;"<<endl;
		vhdl << tab << declare("isZeroY") << " <= '1' when excY=\"00\" else '0' ;"<<endl;
		vhdl << tab << declare("isNormalX") << " <= '1' when excX=\"01\" else '0' ;"<<endl;
		vhdl << tab << declare("isNormalY") << " <= '1' when excY=\"01\" else '0' ;"<<endl;
		vhdl << tab << declare("isInfX") << " <= '1' when excX=\"10\" else '0' ;"<<endl;
		vhdl << tab << declare("isInfY") << " <= '1' when excY=\"10\" else '0' ;"<<endl;
		vhdl << tab << declare("isNaNX") << " <= '1' when excX=\"11\" else '0' ;"<<endl;
		vhdl << tab << declare("isNaNY") << " <= '1' when excY=\"11\" else '0' ;"<<endl;
		addComment("Just for readability of the formulae below");
		vhdl << tab << declare("negativeX") << " <= signX ;"<<endl;
		vhdl << tab << declare("positiveX") << " <= not signX ;"<<endl;
		vhdl << tab << declare("negativeY") << " <= signY ;"<<endl;
		vhdl << tab << declare("positiveY") << " <= not signY ;"<<endl;
		
		addComment("expfrac comparisons ");

		if(getTarget()->plainVHDL()) {
			if(flags&2) {
				addComment("Let us trust the synthesis tools on this reduction");
				vhdl<< tab << declare(getTarget()->eqComparatorDelay(wE+wF), "ExpFracXeqExpFracY")  << " <= '1' when ExpFracX = ExpFracY else '0';"<<endl;
			}
			if(flags!=2) {
				addComment("Let us also trust the synthesis tools on this one");
				vhdl<< tab << declare(getTarget()->ltComparatorDelay(wE+wF), "ExpFracXltExpFracY")  << " <= '1' when ExpFracX < ExpFracY else '0';"<<endl;
				// since there is more logic behind, it is for free to compute gt out of lt and eq
				// if I copypaste the gt line, it adds 32 slices for DP
				vhdl<< tab << declare("ExpFracXgtExpFracY")  << " <= not(ExpFracXltExpFracY or ExpFracXeqExpFracY);"<<endl;
			}
		}
		else { // instantiate a self-pipelining integer comparator
			int intComparatorFlags=0;
			if(flags&(2+8+16)) // eq or le or ge
				intComparatorFlags += 2; // eq or le or ge
			if(flags&(1+4+8+16)) intComparatorFlags += 5; 
			string intComparatorOutputs = "";
			if(flags&(2+8+16))
				intComparatorOutputs += "XeqY=>ExpFracXeqExpFracY";
			if(flags&(1+4+8+16)) { // lt or gt or le or ge
				if (intComparatorOutputs != "" )  intComparatorOutputs +=",";
				intComparatorOutputs +=  "XltY=>ExpFracXltExpFracY,XgtY=>ExpFracXgtExpFracY";
			}
			newInstance("IntComparator",
									"ExpFracCmp", "w="+to_string(wE + wF) + " flags="+to_string(intComparatorFlags),
									"X=>ExpFracX,Y=>ExpFracY",
									intComparatorOutputs);
		}
		addComment("-- and now the logic");
		vhdl<< tab << declare("sameSign")  << " <= not (signX xor signY) ;" << endl;
		if(flags&(2+8+16)){ // eq
			vhdl<< tab << declare(getTarget()->logicDelay(), "XeqYNum")  << " <= " << endl;
			vhdl << tab << tab << "   (isZeroX and isZeroY) -- explicitely stated by IEEE 754" << endl;
			vhdl << tab << tab << "or (isInfX and isInfY and sameSign)  -- bizarre but also explicitely stated by IEEE 754" << endl;
			vhdl << tab << tab << "or (isNormalX and isNormalY and sameSign and ExpFracXeqExpFracY)";
			vhdl << tab << ";" << endl;
		}
		if(flags&(1+8)){ // lt or le
			vhdl<< tab << declare(getTarget()->logicDelay(), "XltYNum")  << " <=     -- case enumeration on Y" << endl; 
			vhdl << tab << tab << "   ( (not (isInfX and positiveX)) and (isInfY  and positiveY)) " << endl;
			vhdl << tab << tab << "or ((negativeX or isZeroX) and (isNormalY and positiveY)) " << endl;
			vhdl << tab << tab << "or ((negativeX and not isZeroX) and isZeroY) " << endl;
			vhdl << tab << tab << "or (isNormalX and isNormalY and positiveX and positiveY and ExpFracXltExpFracY)" << endl;
			vhdl << tab << tab << "or (isNormalX and isNormalY and negativeX and negativeY and ExpFracXgtExpFracY)" << endl;
			vhdl << tab << tab << "or ((isInfX and negativeX) and (not (isInfY and negativeY))) ";
			vhdl << tab << ";" << endl;
		}
		if(flags&(4+16)){ // gt or ge
			vhdl<< tab << declare(getTarget()->logicDelay(), "XgtYNum")  << " <=     -- case enumeration on X" << endl; 
			vhdl << tab << tab << "   ( (not (isInfY and positiveY)) and (isInfX  and positiveX)) " << endl;
			vhdl << tab << tab << "or ((negativeY or isZeroY) and (isNormalX and positiveX)) " << endl;
			vhdl << tab << tab << "or ((negativeY and not isZeroY) and isZeroX) " << endl;
			vhdl << tab << tab << "or (isNormalX and isNormalY and positiveY and positiveX and ExpFracXgtExpFracY)" << endl;
			vhdl << tab << tab << "or (isNormalX and isNormalY and negativeY and negativeX and ExpFracXltExpFracY)" << endl;
			vhdl << tab << tab << "or ((isInfY and negativeY) and (not (isInfX and negativeX))) ";
			vhdl << tab << ";" << endl;
		}
		vhdl << tab << declare("unorderedR") << " <=  isNaNX or isNaNY;" << endl; 
		
		if(flags&1)
			vhdl<< tab << declare("XltYR")	 << " <= XltYNum and not unorderedR;" << endl;
		if(flags&2)
			vhdl<< tab << declare("XeqYR")	 << " <= XeqYNum and not unorderedR;" << endl;
		if(flags&4)
			vhdl<< tab << declare("XgtYR")	 << " <= XgtYNum and not unorderedR;" << endl;
		if(flags&8)
			vhdl<< tab << declare("XleYR")	 << " <= (XeqYNum or XltYNum)	 and not unorderedR;" << endl;
		if(flags&16)
			vhdl<< tab << declare("XgeYR")	 << " <= (XeqYNum or XgtYNum)	 and not unorderedR;" << endl;
		//		vhdl<< tab << declare(getTarget()->logicDelay(), "XneYR")  << " <= (not XeqYNum)  and not unorderedR;" << endl;

		// Transferring to output		
		vhdl << tab << "unordered <= unorderedR;"<<endl;
		if(flags&1)  vhdl << tab << "XltY <= XltYR;"<<endl;
		if(flags&2)  vhdl << tab << "XeqY <= XeqYR;"<<endl;
		if(flags&4)  vhdl << tab << "XgtY <= XgtYR;"<<endl;
		if(flags&8)  vhdl << tab << "XleY <= XleYR;"<<endl;
		if(flags&16) vhdl << tab << "XgeY <= XgeYR;"<<endl;
		//		vhdl << tab << "XneY <= XneYR;"<<endl;
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
		//		bool XneY = (cmp!=0) and not unordered;
		tc->addExpectedOutput("unordered", unordered);
		if(flags&1)  tc->addExpectedOutput("XltY", XltY);
		if(flags&2)  tc->addExpectedOutput("XeqY", XeqY);
		if(flags&4)  tc->addExpectedOutput("XgtY", XgtY);
		if(flags&8)  tc->addExpectedOutput("XleY", XleY);
		if(flags&16) tc->addExpectedOutput("XgeY", XgeY);
		//		tc->addExpectedOutput("XneY", XneY);
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
		tc->addFPInput("X", 0.0);
		tc->addFPInput("Y", 0.0);
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
		int wE, wF,flags, method;
		UserInterface::parseStrictlyPositiveInt(args, "wE", &wE); 
		UserInterface::parseStrictlyPositiveInt(args, "wF", &wF);
		UserInterface::parseStrictlyPositiveInt(args, "flags", &flags);
		UserInterface::parseInt(args, "method", &method);
		return new FPComparator(parentOp, target, wE, wF, flags, method);
	}


	
	TestList FPComparator::unitTest(int index)
	{
		// the static list of mandatory tests
		TestList testStateList;
		vector<pair<string,string>> paramList;
		
		if(index==-1) 
		{ // The unit tests

			for(int flags=1;flags<32; flags++) {
				paramList.push_back(make_pair("flags",to_string(flags)));
				paramList.push_back(make_pair("wE",to_string(5)));
				paramList.push_back(make_pair("wF",to_string(10)));
				testStateList.push_back(paramList);
				paramList.clear();
				paramList.push_back(make_pair("flags",to_string(flags)));
				paramList.push_back(make_pair("wE",to_string(8)));
				paramList.push_back(make_pair("wF",to_string(23)));
				testStateList.push_back(paramList);
				paramList.clear();
				paramList.push_back(make_pair("flags",to_string(flags)));
				paramList.push_back(make_pair("wE",to_string(11)));
				paramList.push_back(make_pair("wF",to_string(52)));
				testStateList.push_back(paramList);
				paramList.clear();
				paramList.push_back(make_pair("flags",to_string(flags)));
				paramList.push_back(make_pair("wE",to_string(16)));
				paramList.push_back(make_pair("wF",to_string(111)));
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
			wF(int): mantissa size in bits;\
			flags(int)=31:  generate XltY output if bit 0 set, XeqY if bit 1, XgtY if bit 2, XleY if bit 3, XgeY if bit 4;\
			method(int)=-1: method to be used in the IntComparator, see IntComparator;",
			"Outputs up to 4 mutually exclusive signals: unordered (when one input is NaN), XltY (less than, strictly), XeqY (equal), XsgtY (greater than, strictly). Also two derived signals XleY (less or equal) and XgeY (greater or equal). unordered is set iff at least one of the inputs is NaN. The other ones behave as expected on two non-NaN values, with the IEEE 754 conventions: +0 = -0; +infinity = +infinity; -infinity = -infinity. The flags argument controls which signal is generated.; ",
			FPComparator::parseArguments,
			FPComparator::unitTest
			) ;
	}


}
