#include "IntMultiplierLUT.hpp"
#include "Table.hpp"

namespace flopoco
{

IntMultiplierLUT::IntMultiplierLUT(Operator *parentOp, Target* target, int wX, int wY, bool isSignedX, bool isSignedY, bool flipXY) : Operator (parentOp, target), wX(wX), wY(wY), isSignedX(isSignedX), isSignedY(isSignedY)
{
	if(isSignedX || isSignedY)
		THROWERROR("signed input currently not supported by IntMultiplierLUT, sorry");

	if(flipXY)
	{
		//swapp x and y:
		int tmp = wX;
		wX = wY;
		wY = tmp;
	}

	int wR = wX + wY; //!!! check for signed case!

	ostringstream name;
	name << "IntMultiplierLUT_" << wX << (isSignedX==1 ? "_signed" : "") << "x" << wY  << (isSignedY==1 ? "_signed" : "");

	setNameWithFreqAndUID(name.str());

	addInput("X", wX);
	addInput("Y", wY);
	addInput("O", wR);

	vector<mpz_class> val;
	for (int yx=0; yx < 1<<(wX+wY); yx++)
	{
		val.push_back(function(yx));
	}
	Operator *op = new Table(this, target, val, "MultTable", wX+wY, wR);
	op->setShared();
	UserInterface::addToGlobalOpList(op);

	vhdl << declare(0.0,"Xtable",wX+wY) << " <= Y & X;" << endl;

	inPortMap(op, "X", "Xtable");
	outPortMap(op, "Y", "O");
	vhdl << instance(op, "TableMult");
}

mpz_class IntMultiplierLUT::function(int yx)
{
	mpz_class r;
	int y = yx>>wX;
	int x = yx -(y<<wX);
	int wF=wX+wY;

	if(isSignedX){
		if ( x >= (1 << (wX-1)))
			x -= (1 << wX);
	}
	if(isSignedY){
		if ( y >= (1 << (wY-1)))
			y -= (1 << wY);
	}
	//if(!negate && isSignedX && isSignedY) cerr << "  y=" << y << "  x=" << x;
	r = x * y;
	//if(!negate && isSignedX && isSignedY) cerr << "  r=" << r;
	// if(negate)       r=-r;
	//if(negate && isSignedX && isSignedY) cerr << "  -r=" << r;
	if ( r < 0)
		r += mpz_class(1) << wF;
	//if(!negate && isSignedX && isSignedY) cerr << "  r2C=" << r;

	if(wX+wY<wF){ // wOut is that of Table
		// round to nearest, but not to nearest even
		int tr=wF-wX-wY; // number of truncated bits
		// adding the round bit at half-ulp position
		r += (mpz_class(1) << (tr-1));
		r = r >> tr;
	}
	//if(!negate && isSignedX && isSignedY) cerr << "  rfinal=" << r << endl;

	return r;

}

OperatorPtr IntMultiplierLUT::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
{
	int wX,wY;

	UserInterface::parseStrictlyPositiveInt(args, "wX", &wX);
	UserInterface::parseStrictlyPositiveInt(args, "wY", &wY);

	return new IntMultiplierLUT(parentOp,target,wX,wY);
}

void IntMultiplierLUT::registerFactory(){
	UserInterface::add("IntMultiplierLUT", // name
					   "Implements a LUT multiplier by simply tabulating all results in the LUT, should only be used for very small word sizes",
					   "BasicInteger", // categories
					   "",
					   "wX(int): size of input X;wY(int): size of input Y",
					   "",
					   IntMultiplierLUT::parseArguments,
					   IntMultiplierLUT::unitTest
	) ;
}

void IntMultiplierLUT::emulate(TestCase* tc)
{
	mpz_class svX = tc->getInputValue("X");
	mpz_class svY = tc->getInputValue("Y");
	mpz_class svR = svX * svY;
	tc->addExpectedOutput("O", svR);
}

TestList IntMultiplierLUT::unitTest(int index)
{
	// the static list of mandatory tests
	TestList testStateList;
	vector<pair<string,string>> paramList;

	//test square multiplications:
	for(int w=1; w <= 6; w++)
	{
		paramList.push_back(make_pair("wX", to_string(w)));
		paramList.push_back(make_pair("wY", to_string(w)));
		testStateList.push_back(paramList);
		paramList.clear();
	}

	return testStateList;
}


}
