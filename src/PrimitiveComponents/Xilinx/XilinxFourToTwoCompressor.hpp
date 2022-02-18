#pragma once

#include "Operator.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include "gmp.h"
#include "mpfr.h"
#include <vector>
#include <gmpxx.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.hpp"
#include "BitHeap/Compressor.hpp"


namespace flopoco
{
class XilinxFourToTwoCompressor : public Compressor //: public VariableColumnCompressor
{
public:
    /** constructor **/
    XilinxFourToTwoCompressor(Operator* parentOp, Target* target, int width, bool useLastColumn=true);

    /** destructor**/
    ~XilinxFourToTwoCompressor();

    virtual void setWidth(int width);

    static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args );
    static void registerFactory();

private:
    bool useLastColumn;
protected:
    int width;
};

class BasicXilinxFourToTwoCompressor : public BasicCompressor
{
public:
    BasicXilinxFourToTwoCompressor(Operator* parentOp_, Target * target, int wIn);
    virtual Compressor* getCompressor(unsigned int middleLength);
    static void calc_widths(int wIn, vector<int> &heights, vector<int> &outHeights);
protected:
    int wIn;
    static vector<int> calc_heights(int wIn);
};

}
