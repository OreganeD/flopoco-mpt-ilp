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
    class RowAdder : public Compressor //: public VariableColumnCompressor
    {
    public:
        /** constructor **/
        RowAdder(Operator *parentOp, Target *target, vector<int> _heights, vector<int> _outHeights);
        RowAdder(Operator *parentOp, Target *target, int wIn, int type=2);

        /** destructor**/
        ~RowAdder();

        static OperatorPtr parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args );
        static void registerFactory();
        static void calc_widths(int wIn, int type, vector<int> &heights, vector<int> &outHeights);

    protected:
        int wIn;
    };

    class BasicRowAdder : public BasicCompressor
    {
    public:
        BasicRowAdder(Operator* parentOp_, Target * target, int wIn, int type=2);
        virtual Compressor* getCompressor(unsigned int middleLength);
    protected:
        int wIn;
        int type;
        static vector<int> calc_heights(int wIn, int type);
    private:
        using CompressorType = BasicCompressor::CompressorType;
    };
}
