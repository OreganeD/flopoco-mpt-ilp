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

		addComment("Let us trust the synthesis tools on this reduction");
		vhdl<< tab << declare(getTarget()->adderDelay(wE+wF), "ExpFracXeqExpFracY")  << " <= '1' when ExpFracX = ExpFracY else '0';"<<endl;

#if 0 // removed after experiments with intcomparator (was 157sl, 2.14ns for DP)
		vhdl << tab << declare("addltOp1",wE+wF+1) << " <= '0'  & ExpFracX;"<<endl;
		vhdl << tab << declare("addltOp2",wE+wF+1) << " <= '1'  & not ExpFracY;"<<endl;
		newInstance("IntAdder", "addlt", join("wIn=", (wE+wF+1)), "X=>addltOp1,Y=>addltOp2", "R=>addltR", "Cin=>'1'");
		addComment("result of X-Y negative iff X<Y");
		vhdl<< tab << declare("ExpFracXltExpFracY")  << " <= addltR"<<of(wE+wF)<<";"<<endl;

		vhdl << tab << declare("addgtOp1",wE+wF+1) << " <= '0'  & ExpFracY;"<<endl;
		vhdl << tab << declare("addgtOp2",wE+wF+1) << " <= '1'  & not ExpFracX;"<<endl;
		newInstance("IntAdder", "addgt", join("wIn=", (wE+wF+1)), "X=>addgtOp1,Y=>addgtOp2", "R=>addgtR", "Cin=>'1'");
		addComment("result of Y-X negative iff X>Y ");
		vhdl<< tab << declare("ExpFracXgtExpFracY")  << " <= addgtR"<<of(wE+wF)<<";"<<endl;
#else// added after experiments with intcomparator  (was 66sl, 2.31ns for DP)
		addComment("Let us also trust the synthesis tools on this one");
		vhdl<< tab << declare(getTarget()->adderDelay(wE+wF), "ExpFracXltExpFracY")  << " <= '1' when ExpFracX < ExpFracY else '0';"<<endl;
		// since there is more logic behind, it is for free to compute gt out of lt and eq
		// if I copypaste the gt line, it adds 32 slices for DP
		vhdl<< tab << declare("ExpFracXgtExpFracY")  << " <= not(ExpFracXltExpFracY or ExpFracXeqExpFracY);"<<endl;
#endif

		addComment("-- and now the logic");
		vhdl<< tab << declare("sameSign")  << " <= not (signX xor signY) ;" << endl;

		vhdl<< tab << declare("XeqYNum")  << " <= " << endl;
		vhdl << tab << tab << "   (isZeroX and isZeroY) -- explicitely stated by IEEE 754" << endl;
		vhdl << tab << tab << "or (isInfX and isInfY and sameSign)  -- bizarre but also explicitely stated by IEEE 754" << endl;
		vhdl << tab << tab << "or (isNormalX and isNormalY and sameSign and ExpFracXeqExpFracY)" << endl;
		vhdl << tab << ";" << endl;

		vhdl<< tab << declare("XltYNum")  << " <=     -- case enumeration on Y" << endl; 
		vhdl << tab << tab << "   ( (not (isInfX and positiveX)) and (isInfY  and positiveY)) " << endl;
		vhdl << tab << tab << "or ((negativeX or isZeroX) and (isNormalY and positiveY)) " << endl;
		vhdl << tab << tab << "or ((negativeX and not isZeroX) and isZeroY) " << endl;
		vhdl << tab << tab << "or (isNormalX and isNormalY and positiveX and positiveY and ExpFracXltExpFracY)" << endl;
		vhdl << tab << tab << "or (isNormalX and isNormalY and negativeX and negativeY and ExpFracXgtExpFracY)" << endl;
		vhdl << tab << tab << "or ((isInfX and negativeX) and (not (isInfY and negativeY))) " << endl;
		vhdl << tab << ";" << endl;

		vhdl<< tab << declare("XgtYNum")  << " <=     -- case enumeration on X" << endl; 
		vhdl << tab << tab << "   ( (not (isInfY and positiveY)) and (isInfX  and positiveX)) " << endl;
		vhdl << tab << tab << "or ((negativeY or isZeroY) and (isNormalX and positiveX)) " << endl;
		vhdl << tab << tab << "or ((negativeY and not isZeroY) and isZeroX) " << endl;
		vhdl << tab << tab << "or (isNormalX and isNormalY and positiveY and positiveX and ExpFracXgtExpFracY)" << endl;
		vhdl << tab << tab << "or (isNormalX and isNormalY and negativeY and negativeX and ExpFracXltExpFracY)" << endl;
		vhdl << tab << tab << "or ((isInfY and negativeY) and (not (isInfX and negativeX))) " << endl;
		vhdl << tab << ";" << endl;

		// We leave all the logicDelay here, it should be enough 
		vhdl << tab << declare(getTarget()->logicDelay(), "unorderedR") << " <=  isNaNX or isNaNY;" << endl; 
		
		vhdl<< tab << declare(getTarget()->logicDelay(), "XltYR")  << " <= XltYNum and not unorderedR;" << endl;
		vhdl<< tab << declare(getTarget()->logicDelay(), "XeqYR")  << " <= XeqYNum and not unorderedR;" << endl;
		vhdl<< tab << declare(getTarget()->logicDelay(), "XgtYR")  << " <= XgtYNum and not unorderedR;" << endl;
		vhdl<< tab << declare(getTarget()->logicDelay(), "XleYR")  << " <= (XeqYNum or XltYNum)  and not unorderedR;" << endl;
		vhdl<< tab << declare(getTarget()->logicDelay(), "XgeYR")  << " <= (XeqYNum or XgtYNum)  and not unorderedR;" << endl;
		//		vhdl<< tab << declare(getTarget()->logicDelay(), "XneYR")  << " <= (not XeqYNum)  and not unorderedR;" << endl;

		// Transferring to output		
		vhdl << tab << "unordered <= unorderedR;"<<endl;
		vhdl << tab << "XltY <= XltYR;"<<endl;
		vhdl << tab << "XeqY <= XeqYR;"<<endl;
		vhdl << tab << "XgtY <= XgtYR;"<<endl;
		vhdl << tab << "XleY <= XleYR;"<<endl;
		vhdl << tab << "XgeY <= XgeYR;"<<endl;
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
		tc->addExpectedOutput("XltY", XltY);
		tc->addExpectedOutput("XeqY", XeqY);
		tc->addExpectedOutput("XgtY", XgtY);
		tc->addExpectedOutput("XleY", XleY);
		tc->addExpectedOutput("XgeY", XgeY);
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
			"Outputs 4 mutually exclusive signals: unordered (when one input is NaN), XltY (less than, strictly), XeqY (equal), XsgtY (greater than, strictly). Also two derived signals XleY (less or equal) and XgeY (greater or equal). unordered is set iff at least one of the inputs is NaN. The other ones behave as expected on two non-NaN values, with the IEEE 754 conventions: +0 = -0; +infinity = +infinity; -infinity = -infinity; ",
			FPComparator::parseArguments,
			FPComparator::unitTest
			) ;
	}


}
