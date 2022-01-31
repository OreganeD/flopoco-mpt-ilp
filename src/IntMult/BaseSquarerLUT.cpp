#include "BaseSquarerLUT.hpp"
#include "Tables/Table.hpp"

namespace flopoco
{

    BaseSquarerLUTOp::BaseSquarerLUTOp(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, int wIn) : Operator(parentOp,target), wIn(wIn), isSigned(isSignedX || isSignedY)
    {
        int wInMax = (isSigned)?(-1)*(1<<(wIn-1)):(1<<wIn)-1;
        int wR = ceil(log2(wInMax*wInMax+1));

        ostringstream name;
        name << "BaseSquarerLUT_" << wIn << "x" << wIn  << (isSigned==1 ? "_signed" : "");

        setNameWithFreqAndUID(name.str());

        addInput("X", wIn);
        addOutput("R", wR);

        vector<mpz_class> val;
        REPORT(DEBUG, "Filling table for a LUT squarer of size " << wIn << "x" << wIn << " (out put size is " << wR << ")")
        for (int yx=0; yx < 1<<(wIn); yx++)
        {
            val.push_back(function(yx));
        }
        Operator *op = new Table(this, target, val, "SquareTable", wIn, wR);
        op->setShared();
        UserInterface::addToGlobalOpList(op);

        vhdl << declare(0.0,"Xtable",wIn) << " <= X;" << endl;

        inPortMap("X", "Xtable");
        outPortMap("Y", "Y1");

        vhdl << tab << "R <= Y1;" << endl;
        vhdl << instance(op, "TableSquarer");
    }

    mpz_class BaseSquarerLUTOp::function(int yx)
    {
        mpz_class r;
        if(isSigned && yx >= (1 << (wIn-1))){
            yx -= (1 << wIn);
        }
        r = yx * yx;
        REPORT(DEBUG, "Value for x=" << yx << ", x=" << yx << " : " << r.get_str(2))
        return r;
    }

    OperatorPtr BaseSquarerLUT::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args)
    {
        int wIn;
        bool singedness;

        UserInterface::parseStrictlyPositiveInt(args, "wIn", &wIn);
        UserInterface::parseBoolean(args, "isSigned", &singedness);

        return new BaseSquarerLUTOp(parentOp,target, singedness, singedness, wIn);
    }

    void BaseSquarerLUT::registerFactory(){
        UserInterface::add("IntSquarerLUT", // name
                           "Implements a LUT squarer by simply tabulating all results in the LUT, should only be used for very small word sizes",
                           "BasicInteger", // categories
                           "",
                           "wIn(int): size of the input XxX; isSigned(bool): signedness of the input",
                           "",
                           BaseSquarerLUT::parseArguments,
                           BaseSquarerLUTOp::unitTest
                           ) ;
    }

    double BaseSquarerLUT::getLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY, bool signedIO) {
        int wInMax = (signedIO)?(-1)*(1<<(wIn-1)):(1<<wIn)-1;
        int wR = ceil(log2(wInMax*wInMax+1));
        return (double)((wR<6)?ceil(wR/2.0):wR) + wR*getBitHeapCompressionCostperBit();
    }

    int BaseSquarerLUT::ownLUTCost(int x_anchor, int y_anchor, int wMultX, int wMultY, bool signedIO) {
        int wInMax = (signedIO)?(-1)*(1<<(wIn-1)):(1<<wIn)-1;
        int wR = ceil(log2(wInMax*wInMax+1));
        return (wR<6)?ceil(wR/2.0):wR;
    }

    unsigned BaseSquarerLUT::getArea(void) {
        return wIn*wIn;
    }

    bool BaseSquarerLUT::shapeValid(int x, int y)
    {
        if(0 <= x && x < wIn && 0 <= y && y < wIn && y <= x) return true;
        return false;
    }

    bool BaseSquarerLUT::shapeValid(Parametrization const& param, unsigned x, unsigned y) const
    {
        if(0 < param.getTileXWordSize() && x < param.getTileXWordSize() && 0 < param.getTileYWordSize() && y < param.getTileYWordSize() && y <= x) return true;
        return false;
    }

    Operator *BaseSquarerLUT::generateOperator(Operator *parentOp, Target *target,
                                               const BaseMultiplierCategory::Parametrization &params) const {
        return new BaseSquarerLUTOp(
                parentOp,
                target,
                params.isSignedMultX(),
                params.isSignedMultY(),
                params.getMultXWordSize()
                );
    }

    int BaseSquarerLUT::getRelativeResultMSBWeight(const BaseMultiplierCategory::Parametrization &param) const {
        int wInMax = (param.isSignedMultX() || param.isSignedMultY())?(-1)*(1<<(wIn-1)):(1<<wIn)-1;
        int wR = ceil(log2(wInMax*wInMax+1));
        return wR-1;
    }

    void BaseSquarerLUTOp::emulate(TestCase* tc)
    {
        mpz_class svX = tc->getInputValue("X");
        if(isSigned && svX >= (1 << (wIn-1))){
            svX -= (1 << wIn);
        }
        mpz_class svR = svX * svX;
        tc->addExpectedOutput("R", svR);
    }

    TestList BaseSquarerLUTOp::unitTest(int)
    {
        // the static list of mandatory tests
        TestList testStateList;
        vector<pair<string,string>> paramList;

        //test square multiplications:
        for(int w=1; w <= 6; w++)
        {
            for(int sign=0; sign < 2; sign++){
                paramList.push_back(make_pair("wIn", to_string(w)));
                paramList.push_back(make_pair("isSigned", to_string((bool)sign)));
                testStateList.push_back(paramList);
                paramList.clear();
            }
        }

        return testStateList;
    }
}
